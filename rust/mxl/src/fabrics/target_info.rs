use std::{ffi::CString, rc::Rc};

use crate::error::{Error, Result};
use mxl_sys::fabrics::FabricsTargetInfo;

use crate::fabrics::instance::FabricsInstanceContext;
pub struct TargetInfo {
    ctx: Rc<FabricsInstanceContext>,
    pub(crate) inner: FabricsTargetInfo,
}

impl TargetInfo {
    pub(crate) fn new(ctx: Rc<FabricsInstanceContext>, inner: FabricsTargetInfo) -> Self {
        Self { ctx, inner }
    }

    fn destroy(mut self) -> Result<()> {
        let mut inner = std::ptr::null_mut();
        std::mem::swap(&mut self.inner, &mut inner);
        Error::from_status(unsafe { self.ctx.api().fabrics_free_target_info(inner) })
    }

    pub(crate) fn from_str(ctx: Rc<FabricsInstanceContext>, s: &str) -> Result<Self> {
        let mut inner = FabricsTargetInfo::default();

        Error::from_status(unsafe {
            ctx.api()
                .fabrics_target_info_from_string(CString::new(s)?.as_ptr(), &mut inner)
        })?;

        Ok(Self::new(ctx, inner))
    }

    pub fn to_string(&self) -> Result<String> {
        let mut size = 0;
        Error::from_status(unsafe {
            self.ctx.api().fabrics_target_info_to_string(
                self.inner,
                std::ptr::null_mut(),
                &mut size,
            )
        })?;

        // fabrics_target_info_to_string already includes space for null terminator. So we must remove it here, because CString includes it.
        let out_string = unsafe { CString::from_vec_unchecked(vec![0; size - 1]) };

        Error::from_status(unsafe {
            self.ctx.api().fabrics_target_info_to_string(
                self.inner,
                out_string.as_ptr() as *mut i8,
                &mut size,
            )
        })?;
        out_string
            .into_string()
            .map_err(|e| Error::Other(e.to_string()))
    }
}

impl Drop for TargetInfo {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe { self.ctx.api().fabrics_free_target_info(self.inner) };
        }
    }
}
