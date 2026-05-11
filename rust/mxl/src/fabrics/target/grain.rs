use std::time::Duration;

use crate::{
    Error, Result,
    fabrics::{TargetInfo, target::Target},
};

/// Returned value from calling read* methods.
pub struct GrainReadResult {
    pub grain_index: u64,
}

/// This is a grain-based target used to receive grains from an initiator.
/// To get all the available methods, import the trait `TargetExt`.
pub struct GrainTarget {
    inner: Target,
}

impl GrainTarget {
    pub(crate) fn new(target: Target) -> Self {
        Self { inner: target }
    }

    pub fn setup(&self, config: &super::Config) -> Result<TargetInfo> {
        self.inner.setup(config)
    }

    ///Blocking accessor for a new grain.
    pub fn read(&self, timeout: Duration) -> Result<GrainReadResult> {
        let mut grain_index = 0;
        Error::from_status(unsafe {
            self.inner.ctx.api().fabrics_target_read_grain(
                self.inner.inner,
                timeout.as_millis() as u16,
                &mut grain_index,
            )
        })?;
        Ok(GrainReadResult { grain_index })
    }
    /// Non-blocking accessor for a new grain.
    pub fn read_non_blocking(&self) -> Result<GrainReadResult> {
        let mut grain_index = 0;
        Error::from_status(unsafe {
            self.inner
                .ctx
                .api()
                .fabrics_target_read_grain_non_blocking(self.inner.inner, &mut grain_index)
        })?;
        Ok(GrainReadResult { grain_index })
    }
}
