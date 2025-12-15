use std::{
    sync::{Arc, atomic::AtomicBool},
    time::Duration,
};

use clap::Parser;
use mxl::{
    EndpointAddress, Error, FabricsInstance, FlowConfigInfo, GrainInitiator, GrainReader,
    GrainTarget, GrainWriter, InitiatorShared, MxlFabricsApi, MxlInstance, TargetInfo,
    TargetShared,
    config::{get_mxl_fabrics_ofi_so_path, get_mxl_so_path},
};

#[derive(Debug, Parser)]
pub struct Cli {
    #[arg(short, long)]
    pub domain: String,

    #[arg(short, long)]
    pub flow_id: String,

    #[arg(short, long, default_value = "tcp")]
    pub provider: String,

    #[arg(short, long)]
    pub node: String,

    #[arg(short, long)]
    pub service: String,

    #[arg(short = 'i', long)]
    pub as_initiator: bool,

    #[arg(short, long)]
    pub target_info: Option<String>,
}

struct TargetEndpoint<'a> {
    instance: &'a MxlInstance,
    flow_config: FlowConfigInfo,
    flow_writer: GrainWriter,
    target: GrainTarget,
}

impl<'a> TargetEndpoint<'a> {
    pub fn new(
        instance: &'a MxlInstance,
        fabrics_api: &Arc<MxlFabricsApi>,
        cli: &Cli,
    ) -> Result<(Self, TargetInfo), mxl::Error> {
        let flow_config_str =
            std::fs::read_to_string(&cli.flow_id).expect("Failed to read flow file");

        let flow_config = instance.create_flow(&flow_config_str, None)?;

        let writer = instance.create_flow_writer(&flow_config.common().id().to_string())?;

        let fabrics_instance = instance.create_fabrics_instance(fabrics_api)?;

        let regions = fabrics_instance.regions_from_writer(&writer)?;
        let provider = fabrics_instance.provider_from_str(&cli.provider)?;

        let target_config = mxl::TargetConfig::new(
            EndpointAddress {
                node: cli.node.clone(),
                service: cli.service.clone(),
            },
            provider,
            regions,
            false,
        );

        let target = fabrics_instance.create_target()?.into_grain_target();
        let target_info = target.setup(&target_config)?;

        let grain_writer = writer.to_grain_writer()?;
        Ok((
            Self {
                instance,
                flow_config,
                flow_writer: grain_writer,
                target,
            },
            target_info,
        ))
    }

    pub fn run(&self, running: Arc<AtomicBool>) -> Result<(), mxl::Error> {
        loop {
            if !running.load(std::sync::atomic::Ordering::SeqCst) {
                println!("Stopping as requested.");
                break;
            }

            match self.target.read(Duration::from_millis(200)) {
                Ok(data) => {
                    let grain_info = self.flow_writer.grain_info(data.grain_index as u64)?;
                    let grain = self.flow_writer.open_grain(grain_info.index)?;
                    grain.commit(data.slice_index)?;

                    println!(
                        "Commited grain index {}, slice index {}.",
                        grain_info.index, data.slice_index
                    );
                }
                Err(mxl::Error::NotReady) => {
                    continue;
                }
                Err(mxl::Error::Interrupted) => {
                    println!("Interrupted, exiting.");
                    break;
                }
                Err(e) => {
                    println!("Error reading from target: {}.", e);
                    break;
                }
            }
        }

        Ok(())
    }
}

struct InitiatorEndpoint<'a> {
    instance: &'a MxlInstance,
    fabrics_instance: FabricsInstance,
    flow_reader: GrainReader,
    initiator: GrainInitiator,
}

impl<'a> InitiatorEndpoint<'a> {
    pub fn new(
        instance: &'a MxlInstance,
        fabrics_api: &Arc<MxlFabricsApi>,
        cli: &Cli,
    ) -> Result<Self, mxl::Error> {
        let flow_reader = instance.create_flow_reader(&cli.flow_id)?;

        let fabrics_instance = instance.create_fabrics_instance(fabrics_api)?;

        let initiator = fabrics_instance.create_initiator()?.into_grain_initiator();

        let regions = fabrics_instance.regions_from_reader(&flow_reader)?;
        let provider = fabrics_instance.provider_from_str(&cli.provider)?;

        let initiator_config = mxl::InitiatorConfig::new(
            EndpointAddress {
                node: cli.node.clone(),
                service: cli.service.clone(),
            },
            provider,
            regions,
            false,
        );

        initiator.setup(&initiator_config)?;

        let grain_reader = flow_reader.to_grain_reader()?;

        Ok(Self {
            instance,
            fabrics_instance,
            flow_reader: grain_reader,
            initiator,
        })
    }

    pub fn run(&self, target_info_str: &str, running: Arc<AtomicBool>) -> Result<(), mxl::Error> {
        let target_info = self
            .fabrics_instance
            .target_info_from_str(target_info_str)?;
        self.initiator.add_target(&target_info)?;

        // Wait to be connected
        loop {
            if !running.load(std::sync::atomic::Ordering::SeqCst) {
                return Ok(());
            }

            if self
                .initiator
                .make_progress(Duration::from_millis(250))
                .is_ok()
            {
                break;
            }
        }

        let rate = self.flow_reader.get_info()?.config.common().grain_rate()?;

        let mut grain_index = self.instance.get_current_index(&rate);

        loop {
            if !running.load(std::sync::atomic::Ordering::SeqCst) {
                break;
            }

            match self
                .flow_reader
                .get_complete_grain(grain_index, Duration::from_millis(200))
            {
                Ok(_grain) => {
                    match self.initiator.transfer(grain_index, 0, 720) {
                        //TODO:expose the end slice
                        Err(Error::NotReady) => {
                            // Retry the same grain
                            continue;
                        }
                        Err(e) => {
                            return Err(e);
                        }
                        Ok(_) => {}
                    };

                    loop {
                        match self.initiator.make_progress(Duration::from_millis(10)) {
                            Ok(_) => {
                                // we're done exiting the loop
                                break;
                            }
                            Err(Error::Interrupted) => {
                                return Ok(());
                            }
                            Err(Error::NotReady) => {
                                // Retry
                                continue;
                            }
                            Err(e) => {
                                return Err(e);
                            }
                        }
                    }
                    grain_index += 1;
                }
                Err(Error::OutOfRangeTooLate) => {
                    // We are too late, move to the next grain
                    grain_index = self.instance.get_current_index(&rate);
                }
                Err(Error::OutOfRangeTooEarly) => {
                    // We are too early, retry the same grain
                }
                Err(e) => {
                    println!("Error reading from flow: {}.", e);
                }
            };
        }

        println!("Stopping as requested.");

        Ok(())
    }
}

//TODO: Fix broken destroy flow: FlowWriter should be destroyed before destroying the flow
impl<'a> Drop for TargetEndpoint<'a> {
    fn drop(&mut self) {
        self.instance
            .destroy_flow(&self.flow_config.common().id().to_string())
            .inspect_err(|e| println!("Error destroying flow: {}", e))
            .ok();
    }
}

fn main() -> Result<(), mxl::Error> {
    let cli = Cli::parse();

    let running = Arc::new(AtomicBool::new(true));
    let running2 = running.clone();
    ctrlc::set_handler(move || {
        running2.store(false, std::sync::atomic::Ordering::SeqCst);
    })
    .expect("Error setting Ctrl-C handler");

    let api = mxl::load_api(get_mxl_so_path())?;

    let fabrics_api = mxl::load_fabrics_api(get_mxl_fabrics_ofi_so_path())?;

    let instance = mxl::MxlInstance::new(api, "/dev/shm", "")?;

    if cli.as_initiator {
        let initiator = InitiatorEndpoint::new(&instance, &fabrics_api, &cli)?;
        initiator.run(
            &cli.target_info
                .expect("When running as initiator target-info is mandatory"),
            running,
        )?;
    } else {
        let (target, target_info) = TargetEndpoint::new(&instance, &fabrics_api, &cli)?;
        println!("Target Info: {}", target_info.to_string()?);
        target.run(running)?;
    }

    Ok(())
}
