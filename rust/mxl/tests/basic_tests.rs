// SPDX-FileCopyrightText: 2025 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

/// Tests of the basic low level synchronous API.
///
/// The tests now require an MXL library of a specific name to be present in the system. This should
/// change in the future. For now, feel free to just edit the path to your library.
use std::{
    sync::Arc,
    time::{Duration, SystemTime},
};

use mxl::{MxlInstance, OwnedGrainData, OwnedSamplesData, config::get_mxl_so_path};
use tracing::info;

static LOG_ONCE: std::sync::Once = std::sync::Once::new();

struct TestDomainGuard {
    dir: std::path::PathBuf,
}

impl TestDomainGuard {
    fn new(test: &str) -> Self {
        let dir = std::path::PathBuf::from(format!(
            "/dev/shm/mxl_rust_unit_tests_domain_{}_{}",
            test,
            uuid::Uuid::new_v4()
        ));
        std::fs::create_dir_all(dir.as_path()).unwrap_or_else(|_| {
            panic!(
                "Failed to create test domain directory \"{}\".",
                dir.display()
            )
        });
        Self { dir }
    }

    fn domain(&self) -> String {
        self.dir.to_string_lossy().to_string()
    }
}

impl Drop for TestDomainGuard {
    fn drop(&mut self) {
        std::fs::remove_dir_all(self.dir.as_path()).unwrap_or_else(|_| {
            panic!(
                "Failed to remove test domain directory \"{}\".",
                self.dir.display()
            )
        });
    }
}

fn setup_test(test: &str) -> (MxlInstance, TestDomainGuard) {
    // Set up the logging to use the RUST_LOG environment variable and if not present, print INFO
    // and higher.
    LOG_ONCE.call_once(|| {
        tracing_subscriber::fmt()
            .with_env_filter(
                tracing_subscriber::EnvFilter::builder()
                    .with_default_directive(tracing::level_filters::LevelFilter::INFO.into())
                    .from_env_lossy(),
            )
            .init();
    });

    let mxl_api = mxl::load_api(get_mxl_so_path()).unwrap();
    let domain_guard = TestDomainGuard::new(test);
    (
        MxlInstance::new(mxl_api, domain_guard.domain().as_str(), "").unwrap(),
        domain_guard,
    )
}

fn read_flow_def<P: AsRef<std::path::Path>>(path: P) -> String {
    let flow_config_file = mxl::config::get_mxl_repo_root().join(path);

    std::fs::read_to_string(flow_config_file.as_path())
        .map_err(|error| {
            mxl::Error::Other(format!(
                "Error while reading flow definition from \"{}\": {}",
                flow_config_file.display(),
                error
            ))
        })
        .unwrap()
}

fn prepare_flow_config_info<P: AsRef<std::path::Path>>(
    mxl_instance: &MxlInstance,
    path: P,
) -> mxl::FlowConfigInfo {
    let flow_def = read_flow_def(path);
    mxl_instance.create_flow(flow_def.as_str(), None).unwrap()
}

#[test]
fn basic_mxl_grain_writing_reading() {
    let (mxl_instance, _domain_guard) = setup_test("grains");
    let flow_config_info = prepare_flow_config_info(&mxl_instance, "lib/tests/data/v210_flow.json");
    let flow_id = flow_config_info.common().id().to_string();
    let flow_writer = mxl_instance.create_flow_writer(flow_id.as_str()).unwrap();
    let grain_writer = flow_writer.to_grain_writer().unwrap();
    let flow_reader = mxl_instance.create_flow_reader(flow_id.as_str()).unwrap();
    let grain_reader = flow_reader.to_grain_reader().unwrap();
    let rate = flow_config_info.common().grain_rate().unwrap();
    let current_index = mxl_instance.get_current_index(&rate);
    let grain_write_access = grain_writer.open_grain(current_index).unwrap();
    let total_slices = grain_write_access.total_slices();
    grain_write_access.commit(total_slices).unwrap();
    let grain_data = grain_reader
        .get_complete_grain(current_index, Duration::from_secs(5))
        .unwrap();
    let grain_data: OwnedGrainData = grain_data.into();
    info!("Grain data len: {:?}", grain_data.payload.len());
    grain_reader.destroy().unwrap();
    grain_writer.destroy().unwrap();
    mxl_instance.destroy_flow(flow_id.as_str()).unwrap();
    mxl_instance.destroy().unwrap();
}

#[test]
fn basic_mxl_samples_writing_reading() {
    let (mxl_instance, _domain_guard) = setup_test("samples");
    let flow_info = prepare_flow_config_info(&mxl_instance, "lib/tests/data/audio_flow.json");
    let flow_id = flow_info.common().id().to_string();
    let flow_writer = mxl_instance.create_flow_writer(flow_id.as_str()).unwrap();
    let samples_writer = flow_writer.to_samples_writer().unwrap();
    let flow_reader = mxl_instance.create_flow_reader(flow_id.as_str()).unwrap();
    let samples_reader = flow_reader.to_samples_reader().unwrap();
    let rate = flow_info.common().sample_rate().unwrap();
    let current_index = mxl_instance.get_current_index(&rate);
    let samples_write_access = samples_writer.open_samples(current_index, 42).unwrap();
    samples_write_access.commit().unwrap();
    let samples_data = samples_reader
        .get_samples(current_index, 42, Duration::from_secs(5))
        .unwrap();
    let samples_data: OwnedSamplesData = samples_data.into();
    info!(
        "Samples data contains {} channels(s), channel 0 has {} byte(s).",
        samples_data.payload.len(),
        samples_data.payload[0].len()
    );
    samples_reader.destroy().unwrap();
    samples_writer.destroy().unwrap();
    mxl_instance.destroy_flow(flow_id.as_str()).unwrap();
    mxl_instance.destroy().unwrap();
}

#[test]
fn get_flow_def() {
    let (mxl_instance, _domain_guard) = setup_test("flow_def");
    let flow_def = read_flow_def("lib/tests/data/v210_flow.json");
    let flow_info = mxl_instance.create_flow(flow_def.as_str(), None).unwrap();
    let flow_id = flow_info.common().id().to_string();
    let retrieved_flow_def = mxl_instance.get_flow_def(flow_id.as_str()).unwrap();
    assert_eq!(flow_def, retrieved_flow_def);
    mxl_instance.destroy_flow(flow_id.as_str()).unwrap();
    mxl_instance.destroy().unwrap();
}

#[cfg(feature = "mxl-fabrics-ofi")]
mod fabrics {
    use mxl::{InitiatorShared, MxlFabricsApi, TargetInfo, TargetShared};

    use super::*;

    struct GrainTarget {
        flow_info: mxl::FlowConfigInfo,
        _flow_writer: mxl::FlowWriter,
        target: mxl::GrainTarget,
    }
    impl GrainTarget {
        pub fn inner(&self) -> &mxl::GrainTarget {
            &self.target
        }
        pub fn wait_for_completion(&self, initiator: &GrainInitiator, timeout: Duration) -> bool {
            let now = SystemTime::now();
            loop {
                let _ = initiator.inner().make_progress_non_blocking(); //progress must be done
                //manually on the initiator for the transfer to comlete.
                if self.inner().read(Duration::from_millis(250)).is_ok() {
                    // Completed!
                    return true;
                }

                if now.elapsed().unwrap() > timeout {
                    return false;
                }
            }
        }
    }

    #[derive(Default)]
    struct TargetBuilder<'a> {
        flow_file: std::path::PathBuf,
        provider: &'a str,
        node: Option<&'a str>,
        service: Option<&'a str>,
    }
    impl<'a> TargetBuilder<'a> {
        pub fn new(flow_file: std::path::PathBuf) -> Self {
            Self {
                flow_file,
                provider: "tcp",
                ..Default::default()
            }
        }
        pub fn provider(mut self, provider: &'a str) -> Self {
            self.provider = provider;
            self
        }
        pub fn node(mut self, node: &'a str) -> Self {
            self.node = Some(node);
            self
        }
        pub fn service(mut self, service: &'a str) -> Self {
            self.service = Some(service);
            self
        }

        pub fn build_grain(
            self,
            mxl_instance: &MxlInstance,
            fabrics_api: &Arc<MxlFabricsApi>,
        ) -> Result<(GrainTarget, TargetInfo), mxl::Error> {
            let flow_def = read_flow_def(self.flow_file.as_path());
            let flow_info = mxl_instance.create_flow(flow_def.as_str(), None)?;
            let flow_id = flow_info.common().id().to_string();
            let flow_writer = mxl_instance.create_flow_writer(flow_id.as_str())?;
            let fabrics_instance = mxl_instance.create_fabrics_instance(fabrics_api)?;
            let provider = fabrics_instance.provider_from_str(self.provider)?;
            let target_regions = fabrics_instance.regions_from_writer(&flow_writer).unwrap();
            let target_config = mxl::TargetConfig::new(
                mxl::EndpointAddress {
                    node: self.node,
                    service: self.service,
                },
                provider.clone(),
                target_regions,
                false,
            );
            let target = fabrics_instance
                .create_target()
                .unwrap()
                .into_grain_target();
            let target_info = target.setup(&target_config)?;

            Ok((
                GrainTarget {
                    flow_info,
                    _flow_writer: flow_writer,
                    target,
                },
                target_info,
            ))
        }
    }

    struct GrainInitiator {
        _flow_reader: mxl::FlowReader,
        initiator: mxl::GrainInitiator,
    }
    impl GrainInitiator {
        pub fn add_target(&self, target_info: &TargetInfo) -> Result<(), mxl::Error> {
            self.initiator.add_target(target_info)
        }
        pub fn inner(&self) -> &mxl::GrainInitiator {
            &self.initiator
        }
        fn post_transfer(
            &self,
            grain_index: u64,
            start_slice: u16,
            end_slice: u16,
            timeout: Duration,
        ) -> bool {
            let now = SystemTime::now();
            loop {
                let _ = self.inner().make_progress_non_blocking();
                if self
                    .inner()
                    .transfer(grain_index, start_slice, end_slice)
                    .is_ok()
                {
                    return true;
                }

                let elapsed = now.elapsed().unwrap();
                if elapsed > timeout {
                    return false;
                }

                let remain = timeout - elapsed;
                std::thread::sleep(std::cmp::min(remain, Duration::from_millis(100)));
            }
        }
    }

    #[derive(Default)]
    struct InitiatorBuilder<'a> {
        flow_id: &'a str,
        provider: &'a str,
        node: Option<&'a str>,
        service: Option<&'a str>,
    }
    impl<'a> InitiatorBuilder<'a> {
        pub fn new(flow_id: &'a str) -> Self {
            Self {
                flow_id,
                provider: "tcp",
                ..Default::default()
            }
        }
        pub fn provider(mut self, provider: &'a str) -> Self {
            self.provider = provider;
            self
        }
        pub fn node(mut self, node: &'a str) -> Self {
            self.node = Some(node);
            self
        }
        pub fn service(mut self, service: &'a str) -> Self {
            self.service = Some(service);
            self
        }
        pub fn build_grain(
            self,
            mxl_instance: &MxlInstance,
            fabrics_api: &Arc<MxlFabricsApi>,
        ) -> Result<GrainInitiator, mxl::Error> {
            let fabrics_instance = mxl_instance.create_fabrics_instance(fabrics_api)?;
            let provider = fabrics_instance.provider_from_str(self.provider)?;
            let flow_reader = mxl_instance.create_flow_reader(self.flow_id)?;
            let initiator_regions = fabrics_instance.regions_from_reader(&flow_reader)?;

            let initiator_config = mxl::InitiatorConfig::new(
                mxl::EndpointAddress {
                    node: self.node,
                    service: self.service,
                },
                provider,
                initiator_regions,
                false,
            );
            let initiator = fabrics_instance
                .create_initiator()
                .unwrap()
                .into_grain_initiator();
            initiator.setup(&initiator_config)?;

            Ok(GrainInitiator {
                _flow_reader: flow_reader,
                initiator,
            })
        }
    }
    fn wait_for_connection(
        target: &GrainTarget,
        initiator: &GrainInitiator,
        timeout: Duration,
    ) -> bool {
        let now = SystemTime::now();
        loop {
            let _ = target.inner().read_non_blocking();
            if initiator
                .inner()
                .make_progress(Duration::from_millis(250))
                .is_ok()
            {
                // Connected!
                return true;
            }

            if now.elapsed().unwrap() > timeout {
                return false;
            }
        }
    }

    #[cfg(feature = "mxl-fabrics-ofi")]
    #[test]
    fn test_fabrics_connection() {
        use mxl::config::get_mxl_fabrics_ofi_so_path;

        let (mxl_instance, _domain_guard) = setup_test("fabrics_basic");

        let fabrics_api = mxl::load_fabrics_api(get_mxl_fabrics_ofi_so_path()).unwrap();

        let (target, target_info) = TargetBuilder::new("lib/tests/data/v210_flow.json".into())
            .provider("tcp")
            .node("127.0.0.1")
            .service("0")
            .build_grain(&mxl_instance, &fabrics_api)
            .unwrap();
        let initiator = InitiatorBuilder::new(target.flow_info.common().id().to_string().as_str())
            .provider("tcp")
            .node("127.0.0.1")
            .service("0")
            .build_grain(&mxl_instance, &fabrics_api)
            .unwrap();
        initiator.add_target(&target_info).unwrap();

        assert!(wait_for_connection(
            &target,
            &initiator,
            Duration::from_secs(5),
        ));
    }

    #[cfg(feature = "mxl-fabrics-ofi")]
    #[test]
    fn test_fabrics_grain_transfer() {
        use mxl::config::get_mxl_fabrics_ofi_so_path;

        let (mxl_instance, _domain_guard) = setup_test("fabrics_basic");

        let fabrics_api = mxl::load_fabrics_api(get_mxl_fabrics_ofi_so_path()).unwrap();

        let (target, target_info) = TargetBuilder::new("lib/tests/data/v210_flow.json".into())
            .provider("tcp")
            .node("127.0.0.1")
            .service("0")
            .build_grain(&mxl_instance, &fabrics_api)
            .unwrap();
        let initiator = InitiatorBuilder::new(target.flow_info.common().id().to_string().as_str())
            .provider("tcp")
            .node("127.0.0.1")
            .service("0")
            .build_grain(&mxl_instance, &fabrics_api)
            .unwrap();
        initiator.add_target(&target_info).unwrap();

        assert!(wait_for_connection(
            &target,
            &initiator,
            Duration::from_secs(5),
        ));

        assert!(initiator.post_transfer(0, 0, 1080, Duration::from_secs(5)));
        assert!(target.wait_for_completion(&initiator, Duration::from_secs(5)));
    }
}
