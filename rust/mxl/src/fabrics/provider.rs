use mxl_sys::FabricsProvider;

use crate::error::{Error, Result};
use std::{ffi::CString, rc::Rc};

use crate::fabrics::instance::FabricsInstanceContext;

pub enum Provider {
    Auto,
    Tcp,
    Verbs,
    Efa,
    Shm,
}

impl From<mxl_sys::FabricsProvider> for Provider {
    fn from(value: mxl_sys::FabricsProvider) -> Self {
        match value {
            mxl_sys::MXL_FABRICS_PROVIDER_AUTO => Provider::Auto,
            mxl_sys::MXL_FABRICS_PROVIDER_TCP => Provider::Tcp,
            mxl_sys::MXL_FABRICS_PROVIDER_VERBS => Provider::Verbs,
            mxl_sys::MXL_FABRICS_PROVIDER_EFA => Provider::Efa,
            mxl_sys::MXL_FABRICS_PROVIDER_SHM => Provider::Shm,
            _ => panic!("Unknown FabricsProvider value"),
        }
    }
}

impl From<&Provider> for mxl_sys::FabricsProvider {
    fn from(value: &Provider) -> Self {
        match value {
            Provider::Auto => mxl_sys::MXL_FABRICS_PROVIDER_AUTO,
            Provider::Tcp => mxl_sys::MXL_FABRICS_PROVIDER_TCP,
            Provider::Verbs => mxl_sys::MXL_FABRICS_PROVIDER_VERBS,
            Provider::Efa => mxl_sys::MXL_FABRICS_PROVIDER_EFA,
            Provider::Shm => mxl_sys::MXL_FABRICS_PROVIDER_SHM,
        }
    }
}

impl Provider {
    fn new(inner: FabricsProvider) -> Self {
        inner.into()
    }

    pub fn from_str(ctx: Rc<FabricsInstanceContext>, s: &str) -> Result<Provider> {
        let mut inner = FabricsProvider::default();

        Error::from_status(unsafe {
            ctx.api()
                .fabrics_provider_from_string(CString::new(s)?.as_ptr(), &mut inner)
        })?;

        Ok(Self::new(inner))
    }

    pub fn to_string(&self, ctx: Rc<FabricsInstanceContext>) -> Result<String> {
        let mut size = 0;

        let prov: mxl_sys::FabricsProvider = self.into();

        Error::from_status(unsafe {
            ctx.api()
                .fabrics_provider_to_string(prov, std::ptr::null_mut(), &mut size)
        })?;

        // fabrics_provider_to_string already includes space for null terminator. So we must remove it here, because CString includes it.
        let out_string = unsafe { CString::from_vec_unchecked(vec![0; size - 1]) };

        Error::from_status(unsafe {
            ctx.api()
                .fabrics_provider_to_string(prov, out_string.as_ptr() as *mut i8, &mut size)
        })?;
        out_string
            .into_string()
            .map_err(|e| Error::Other(e.to_string()))
    }
}
