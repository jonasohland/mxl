mod config;
mod grain;

use std::rc::Rc;

use crate::error::{Error, Result};
use crate::fabrics::{instance::FabricsInstanceContext, target_info::TargetInfo};

pub use config::Config;
pub use grain::GrainTarget;

/// Defines the basic requirements for behaving as a Target
trait Target {
    fn ctx(&self) -> &Rc<FabricsInstanceContext>;
    fn inner(&self) -> mxl_sys::fabrics::FabricsTarget;
}

/// Trait to extend behavior of Targets.
pub trait TargetExt {
    fn setup(&self, config: &Config) -> Result<TargetInfo>;
}

impl<T: Target> TargetExt for T {
    /// Configure the target. After the target has been configured, it is ready to receive transfers from an initiator.
    /// If additional connection setup is required by the underlying implementation it might not happen during the call to
    /// setup(), but be deferred until the first call to mxlFabricsTargetTryNewGrain().
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

/// This is an unspecialized target. See `into_*_target` methods to convert into a specific target
/// type. This is created via a [FabricsInstance](crate::fabrics::FabricsInstance).
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

    /// Convert to a Grain Target.
    pub fn into_grain_target(self) -> grain::GrainTarget {
        grain::GrainTarget::new(self)
    }
}

impl Target for UnspecTarget {
    fn ctx(&self) -> &Rc<FabricsInstanceContext> {
        &self.ctx
    }

    fn inner(&self) -> mxl_sys::fabrics::FabricsTarget {
        self.inner
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

/// Create a new unspecialized target.
#[doc(hidden)]
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
