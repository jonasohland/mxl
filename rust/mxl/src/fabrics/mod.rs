mod config;
mod initiator;
mod instance;
mod provider;
mod region;
mod target;
mod target_info;

pub use config::EndpointAddress;
pub use initiator::{Config as InitiatorConfig, GrainInitiator, InitiatorShared};
pub use instance::FabricsInstance;
pub use target::{Config as TargetConfig, GrainTarget, TargetShared};
pub use target_info::TargetInfo;

pub(crate) use instance::create_instance;
