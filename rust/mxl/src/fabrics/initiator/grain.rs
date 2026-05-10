use std::time::Duration;

use crate::error::{Error, Result};
use crate::fabrics::{Initiator, InitiatorConfig as Config, TargetInfo};

/// This is a grain-based initiator, used to transfer grains to targets.
pub struct GrainInitiator {
    inner: Initiator,
}

impl GrainInitiator {
    pub(crate) fn new(initiator: Initiator) -> Self {
        Self { inner: initiator }
    }

    pub fn setup(&self, config: &Config) -> Result<()> {
        self.inner.setup(config)
    }

    pub fn add_target(&self, target: &TargetInfo) -> Result<()> {
        self.inner.add_target(target)
    }

    pub fn remove_target(&self, target: &TargetInfo) -> Result<()> {
        self.inner.remove_target(target)
    }

    pub fn make_progress_non_blocking(&self) -> Result<()> {
        self.inner.make_progress_non_blocking()
    }

    pub fn make_progress(&self, timeout: Duration) -> Result<()> {
        self.inner.make_progress(timeout)
    }

    /// Enqueue a transfer operation to all added targets. This function is always non-blocking. The transfer operation might be started right
    /// away, but is only guaranteed to have completed after mxlFabricsInitiatorMakeProgress*() no longer returns Error::NotReady.
    pub fn transfer(&self, grain_index: u64, start_slice: u16, end_slice: u16) -> Result<()> {
        Error::from_status(unsafe {
            self.inner.ctx.api().fabrics_initiator_transfer_grain(
                self.inner.inner,
                grain_index,
                start_slice,
                end_slice,
            )
        })
    }
}
