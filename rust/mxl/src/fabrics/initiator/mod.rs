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

/// Trait to extend behavior of Initiators.
pub trait InitiatorExt {
    ///  Configure the initiator.
    fn setup(&self, config: &Config) -> Result<()>;

    /// Add a target to the initiator. This will allow the initiator to send data to the target in subsequent calls to
    /// mxlFabricsInitiatorTransferGrain(). This function is always non-blocking. If additional connection setup is required
    /// by the underlying implementation, it will only happen during a call to make_progress*().
    fn add_target(&self, target: &TargetInfo) -> Result<()>;

    /// Remove a target from the initiator. This function is always non-blocking. If any additional communication for a graceful shutdown is
    /// required it will happend during a call to make_progress*(). It is guaranteed that no new grain transfer operations will
    /// be queued for this target during calls to transfer() after the target was removed, but it is only guaranteed that
    /// the connection shutdown has completed after make_progress*() no longer returns Error::NotReady.
    fn remove_target(&self, target: &TargetInfo) -> Result<()>;

    /// This function must be called regularly for the initiator to make progress on queued transfer operations, connection establishment
    /// operations and connection shutdown operations.
    fn make_progress_non_blocking(&self) -> Result<()>;

    /// This function must be called regularly for the initiator to make progress on queued transfer operations, connection establishment
    /// operations and connection shutdown operations.
    fn make_progress(&self, timeout: Duration) -> Result<()>;
}

/// Implement these shared methods for any Initiator.
impl<I: Initiator> InitiatorExt for I {
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

/// This is an unspecialized initiator. See `into_*_initiator` methods to convert to a
/// specialization. This is created via a [FabricsInstance](crate::fabrics::FabricsInstance).
pub struct UnspecInitiator {
    ctx: Rc<FabricsInstanceContext>,
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

    /// Convert to a Grain Initiator. A Grain Initiator allows to transfer "grains" only.
    pub fn into_grain_initiator(self) -> grain::GrainInitiator {
        grain::GrainInitiator::new(self)
    }
}

impl Initiator for UnspecInitiator {
    fn ctx(&self) -> &Rc<FabricsInstanceContext> {
        &self.ctx
    }

    fn inner(&self) -> mxl_sys::fabrics::FabricsInitiator {
        self.inner
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

#[doc(hidden)]
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
