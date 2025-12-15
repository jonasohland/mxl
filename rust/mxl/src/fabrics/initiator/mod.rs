mod config;
mod grain;

use crate::{
    error::{Error, Result},
    fabrics::{instance::FabricsInstanceContext, target_info::TargetInfo},
};

pub use config::Config;
pub use grain::GrainInitiator;

use std::{rc::Rc, time::Duration};

/// Defines the basic requirements for behaving as an Initiator.
trait Initiator {
    fn ctx(&self) -> &Rc<FabricsInstanceContext>;
    fn inner(&self) -> mxl_sys::fabrics::FabricsInitiator;
}

/// These methods are generic over all initiator specializations.
pub trait InitiatorShared {
    fn setup(&self, config: &Config) -> Result<()>;
    fn add_target(&self, target: &TargetInfo) -> Result<()>;
    fn remove_target(&self, target: &TargetInfo) -> Result<()>;
    fn make_progress_non_blocking(&self) -> Result<()>;
    fn make_progress(&self, timeout: Duration) -> Result<()>;
}

/// For any Iterator, implement these shared methods.
impl<I: Initiator> InitiatorShared for I {
    fn setup(&self, config: &Config) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx()
                .api()
                .fabrics_initiator_setup(self.inner(), &config.try_into()?)
        })
    }
    fn add_target(&self, target: &TargetInfo) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx()
                .api()
                .fabrics_initiator_add_target(self.inner(), target.inner)
        })
    }
    fn remove_target(&self, target: &TargetInfo) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx()
                .api()
                .fabrics_initiator_remove_target(self.inner(), target.inner)
        })
    }
    fn make_progress_non_blocking(&self) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx()
                .api()
                .fabrics_initiator_make_progress_non_blocking(self.inner())
        })
    }
    fn make_progress(&self, timeout: Duration) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx()
                .api()
                .fabrics_initiator_make_progress_blocking(self.inner(), timeout.as_millis() as u16)
        })
    }
}

/// This is an unspecified initiator. See `into_*_initiator` methods to convert to a specific type.
pub struct UnspecInitiator {
    pub(crate) ctx: Rc<FabricsInstanceContext>,
    inner: mxl_sys::fabrics::FabricsInitiator,
}

impl UnspecInitiator {
    pub(crate) fn new(
        ctx: Rc<FabricsInstanceContext>,
        initiator: mxl_sys::fabrics::FabricsInitiator,
    ) -> Self {
        Self {
            ctx,
            inner: initiator,
        }
    }

    pub(crate) fn destroy(mut self) -> Result<()> {
        let mut inner = std::ptr::null_mut();
        std::mem::swap(&mut self.inner, &mut inner);
        Error::from_status(unsafe {
            self.ctx
                .api()
                .fabrics_destroy_initiator(self.ctx.inner, inner)
        })
    }

    /// Convert to a Grain Initiator
    pub fn into_grain_initiator(self) -> grain::GrainInitiator {
        grain::GrainInitiator::new(self)
    }
}

impl Drop for UnspecInitiator {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe {
                self.ctx
                    .api()
                    .fabrics_destroy_initiator(self.ctx.inner, self.inner);
            }
        }
    }
}

// Create a new unspecified initiator.
pub(crate) fn create_initiator(ctx: &Rc<FabricsInstanceContext>) -> Result<UnspecInitiator> {
    let mut initiator = mxl_sys::fabrics::FabricsInitiator::default();
    unsafe {
        Error::from_status(
            ctx.api()
                .fabrics_create_initiator(ctx.inner, &mut initiator),
        )?;
    }
    if initiator.is_null() {
        return Err(Error::Other(
            "Failed to create fabrics initiator.".to_string(),
        ));
    }
    Ok(UnspecInitiator::new(ctx.clone(), initiator))
}
