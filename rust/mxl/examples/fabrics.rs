use clap::Parser;
use mxl::{EndpointAddress, TargetShared, config::get_mxl_so_path};

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
}

fn main() -> Result<(), mxl::Error> {
    let cli = Cli::parse();

    let api = mxl::load_api(get_mxl_so_path())?;

    let instance = mxl::MxlInstance::new(api, "/dev/shm", "")?;
    let writer = instance.create_flow_writer(&cli.flow_id)?;

    let fabrics_instance = instance.create_fabrics_instance()?;

    let regions = fabrics_instance.regions_from_writer(&writer)?;
    let provider = fabrics_instance.provider_from_str(&cli.provider)?;

    let target_config = mxl::TargetConfig::new(
        EndpointAddress {
            node: cli.node,
            service: cli.service,
        },
        provider,
        regions,
        false,
    );

    let target = fabrics_instance.create_target()?.into_grain_target();
    let target_info = target.setup(&target_config)?;

    let s = target_info.to_string()?;
    println!("Target Info: {}", s);

    Ok(())
}
