use std::{rc::Rc, time::Duration};

use crate::{
    Error, Result,
    fabrics::{
        instance::FabricsInstanceContext,
        target::{Target, UnspecTarget},
    },
};

/// Returned value from calling read* methods.
pub struct GrainReadResult {
    pub grain_index: u16,
    pub slice_index: u16,
}

/// This is a grain-based target used to receive grains from an initiator.
/// To get all the available methods, import the trait `TargetShared`.
pub struct GrainTarget {
    inner: UnspecTarget,
}

impl Target for GrainTarget {
    fn ctx(&self) -> &Rc<FabricsInstanceContext> {
        &self.inner.ctx
    }

    fn inner(&self) -> mxl_sys::fabrics::FabricsTarget {
        self.inner.inner
    }
}

impl GrainTarget {
    pub(crate) fn new(inner: UnspecTarget) -> Self {
        Self { inner }
    }

    /// Non-blocking accessor for a new grain.
    pub fn read(&self, timeout: Duration) -> Result<GrainReadResult> {
        let mut grain_index = 0u16;
        let mut slice_index = 0u16;
        Error::from_status(unsafe {
            self.inner.ctx.api().fabrics_target_read_grain(
                self.inner.inner,
                &mut grain_index,
                &mut slice_index,
                timeout.as_millis() as u16,
            )
        })?;
        Ok(GrainReadResult {
            grain_index,
            slice_index,
        })
    }
    /// Blocking accessor for a new grain.
    pub fn read_non_blocking(&self) -> Result<GrainReadResult> {
        let mut grain_index = 0u16;
        let mut slice_index = 0u16;
        Error::from_status(unsafe {
            self.inner.ctx.api().fabrics_target_read_grain_non_blocking(
                self.inner.inner,
                &mut grain_index,
                &mut slice_index,
            )
        })?;
        Ok(GrainReadResult {
            grain_index,
            slice_index,
        })
    }
}
