use crate::{
    Error,
    fabrics::{config::EndpointAddress, provider::Provider, region::Regions},
};

/// Configuration object required to set up an initiator.
pub struct Config<'a> {
    endpoint_addr: EndpointAddress<'a>,
    provider: Provider,
    regions: Regions,
    device_support: bool,
}

impl<'a> Config<'a> {
    pub fn new(
        endpoint_addr: EndpointAddress<'a>,
        provider: Provider,
        regions: Regions,
        device_support: bool,
    ) -> Self {
        Self {
            endpoint_addr,
            provider,
            regions,
            device_support,
        }
    }
}

impl<'a> TryFrom<&Config<'a>> for mxl_sys::fabrics::FabricsInitiatorConfig {
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
