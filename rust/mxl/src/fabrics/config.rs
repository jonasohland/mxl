use std::ffi::CString;

use crate::Error;

/// Address of a logical network endpoint. This is analogous to a hostname and port number in classic ipv4 networking.
/// The actual values for node and service vary between providers, but often an ip address as the node value and a port number as the service
/// value are sufficient.
pub struct EndpointAddress {
    pub node: String,
    pub service: String,
}

impl TryFrom<&EndpointAddress> for mxl_sys::fabrics::FabricsEndpointAddress {
    type Error = Error;

    fn try_from(value: &EndpointAddress) -> Result<Self, Self::Error> {
        Ok(mxl_sys::fabrics::FabricsEndpointAddress {
            node: CString::new(value.node.clone())?.into_raw(),
            service: CString::new(value.service.clone())?.into_raw(),
        })
    }
}
