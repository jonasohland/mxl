mod config;
mod grain;

use std::rc::Rc;

use crate::error::{Error, Result};
use crate::fabrics::{instance::FabricsInstanceContext, target_info::TargetInfo};

pub use config::Config;
pub use grain::GrainTarget;

trait Target {
    fn ctx(&self) -> &Rc<FabricsInstanceContext>;
    fn inner(&self) -> mxl_sys::fabrics::FabricsTarget;
}

pub trait TargetShared {
    fn setup(&self, config: &Config) -> Result<TargetInfo>;
}

impl<T: Target> TargetShared for T {
    fn setup(&self, config: &Config) -> Result<TargetInfo> {
        let mut info = mxl_sys::fabrics::FabricsTargetInfo::default();
        Error::from_status(unsafe {
            self.ctx()
                .api()
                .fabrics_target_setup(self.inner(), &config.try_into()?, &mut info)
        })?;
        Ok(TargetInfo::new(self.ctx().clone(), info))
    }
}

/// This is an unspecified target. See `into_*_target` methods to convert into a specific target
/// type.
pub struct UnspecTarget {
    ctx: Rc<FabricsInstanceContext>,
    inner: mxl_sys::fabrics::FabricsTarget,
}

impl UnspecTarget {
    pub(crate) fn new(
        ctx: Rc<FabricsInstanceContext>,
        target: mxl_sys::fabrics::FabricsTarget,
    ) -> Self {
        Self { ctx, inner: target }
    }

    pub(crate) fn destroy(mut self) -> Result<()> {
        let mut inner = std::ptr::null_mut();
        std::mem::swap(&mut self.inner, &mut inner);
        Error::from_status(unsafe { self.ctx.api().fabrics_destroy_target(self.ctx.inner, inner) })
    }

    /// Convert to a Grain Target
    pub fn into_grain_target(self) -> grain::GrainTarget {
        grain::GrainTarget::new(self)
    }
}

impl Drop for UnspecTarget {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe {
                self.ctx
                    .api()
                    .fabrics_destroy_target(self.ctx.inner, self.inner);
            }
        }
    }
}

/// Create a new unspecified target.
pub(crate) fn create_target(ctx: &Rc<FabricsInstanceContext>) -> Result<UnspecTarget> {
    let mut target = mxl_sys::fabrics::FabricsTarget::default();
    unsafe {
        Error::from_status(ctx.api().fabrics_create_target(ctx.inner, &mut target))?;
    }
    if target.is_null() {
        return Err(Error::Other("Failed to create fabrics target.".to_string()));
    }

    Ok(UnspecTarget::new(ctx.clone(), target))
}
