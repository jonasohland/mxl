use crate::{
    Error,
    fabrics::{config::EndpointAddress, provider::Provider, region::Regions},
};

pub struct Config {
    endpoint_addr: EndpointAddress,
    provider: Provider,
    regions: Regions,
    device_support: bool,
}

impl TryFrom<&Config> for mxl_sys::FabricsTargetConfig {
    type Error = Error;

    fn try_from(value: &Config) -> Result<Self, Self::Error> {
        Ok(Self {
            endpointAddress: (&value.endpoint_addr).try_into()?,
            provider: (&value.provider).into(),
            regions: (&value.regions).into(),
            deviceSupport: value.device_support,
        })
    }
}
