use std::rc::Rc;

use crate::error::{Error, Result};
use crate::fabrics::initiator::{Initiator, UnspecInitiator};
use crate::fabrics::instance::FabricsInstanceContext;

/// This is a grain-based initiator, used to transfer grains to targets.
/// To get all the available methods, import the trait `InitiatorShared`.
pub struct GrainInitiator {
    #[doc(hidden)]
    inner: UnspecInitiator,
}

impl Initiator for GrainInitiator {
    fn ctx(&self) -> &Rc<FabricsInstanceContext> {
        &self.inner.ctx
    }

    fn inner(&self) -> mxl_sys::fabrics::FabricsInitiator {
        self.inner.inner
    }
}

impl GrainInitiator {
    #[doc(hidden)]
    pub(crate) fn new(inner: UnspecInitiator) -> Self {
        Self { inner }
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
