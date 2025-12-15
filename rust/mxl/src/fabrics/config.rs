use std::ffi::CString;

use crate::Error;

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
