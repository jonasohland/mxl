use std::rc::Rc;

use crate::{Error, FlowReader, FlowWriter, Result, fabrics::instance::FabricsInstanceContext};

/// A collection of memory regions that can be the target or the source of remote write operations.
/// Can be obtained by using a flow reader or writer, and converting it to a regions collection
/// with mxlFabricsRegionsForFlowReader() or mxlFabricsRegionsForFlowWriter().
pub struct Regions {
    ctx: Rc<FabricsInstanceContext>,
    inner: mxl_sys::fabrics::FabricsRegions,
}

impl From<&Regions> for mxl_sys::fabrics::FabricsRegions {
    fn from(value: &Regions) -> Self {
        value.inner
    }
}

impl Regions {
    fn new(ctx: Rc<FabricsInstanceContext>, inner: mxl_sys::fabrics::FabricsRegions) -> Self {
        Regions { ctx, inner }
    }

    /// Get the backing memory regions of a flow associated with a flow reader.
    /// The regions will be used to register the shared memory of the reader as source of data transfer operations.
    /// Public visibility is set to crate only, because a `FabricsInstanceContext` is required.
    /// See `FabricsInstance`
    pub(crate) fn from_flow_reader(
        ctx: Rc<FabricsInstanceContext>,
        flow_reader: &FlowReader,
    ) -> Result<Self> {
        let mut out_regions = mxl_sys::fabrics::FabricsRegions::default();
        Error::from_status(unsafe {
            ctx.api().fabrics_regions_for_flow_reader(
                std::mem::transmute::<
                    *mut mxl_sys::FlowReader_t,
                    *mut mxl_sys::fabrics::FlowReader_t,
                >(flow_reader.inner()),
                &mut out_regions,
            )
        })?;

        Ok(Self::new(ctx, out_regions))
    }

    /// Get the backing memory regions of a flow associated with a flow writer.
    ///The regions will be used to register the shared memory of the writer as the target of data transfer operations.
    // Public visibility is set to crate only, because a `FabricsInstanceContext` is required.
    /// See `FabricsInstance`
    pub(crate) fn from_flow_writer(
        ctx: Rc<FabricsInstanceContext>,
        flow_writer: &FlowWriter,
    ) -> Result<Self> {
        let mut out_regions = mxl_sys::fabrics::FabricsRegions::default();
        Error::from_status(unsafe {
            ctx.api().fabrics_regions_for_flow_writer(
                std::mem::transmute::<
                    *mut mxl_sys::FlowWriter_t,
                    *mut mxl_sys::fabrics::FlowWriter_t,
                >(flow_writer.inner()),
                &mut out_regions,
            )
        })?;

        Ok(Self::new(ctx, out_regions))
    }
}

impl Drop for Regions {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe { self.ctx.api().fabrics_regions_free(self.inner) };
        }
    }
}
