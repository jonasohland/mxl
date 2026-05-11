use std::time::Duration;

use crate::{
    Error,
    error::Result,
    fabrics::{TargetInfo, target::Target},
};

pub struct SampleReadResult {
    pub head_index: u64,
    pub count: usize,
}

pub struct SampleTarget {
    inner: Target,
}

impl SampleTarget {
    pub(crate) fn new(target: Target) -> Self {
        Self { inner: target }
    }

    pub fn setup(&self, config: &super::Config) -> Result<TargetInfo> {
        self.inner.setup(config)
    }

    ///Blocking accessor for a new grain.
    pub fn read(&self, timeout: Duration) -> Result<SampleReadResult> {
        let mut head_index = 0;
        let mut count = 0;
        Error::from_status(unsafe {
            self.inner.ctx.api().fabrics_target_read_samples(
                self.inner.inner,
                timeout.as_millis() as u16,
                &mut head_index,
                &mut count,
            )
        })?;
        Ok(SampleReadResult { head_index, count })
    }
    /// Non-blocking accessor for a new grain.
    pub fn read_non_blocking(&self) -> Result<SampleReadResult> {
        let mut head_index = 0;
        let mut count = 0;
        Error::from_status(unsafe {
            self.inner
                .ctx
                .api()
                .fabrics_target_read_samples_non_blocking(
                    self.inner.inner,
                    &mut head_index,
                    &mut count,
                )
        })?;
        Ok(SampleReadResult { head_index, count })
    }
}
