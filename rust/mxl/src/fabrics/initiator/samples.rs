use std::time::Duration;

use crate::error::{Error, Result};
use crate::fabrics::{Initiator, InitiatorConfig as Config, TargetInfo};

/// This is a sample-based initiator, used to transfer samples to targets.
pub struct SampleInitiator {
    inner: Initiator,
}

impl SampleInitiator {
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
    pub fn transfer(&self, head_index: u64, count: usize) -> Result<()> {
        Error::from_status(unsafe {
            self.inner.ctx.api().fabrics_initiator_transfer_samples(
                self.inner.inner,
                head_index,
                count,
            )
        })
    }
}
