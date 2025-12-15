use std::rc::Rc;

use crate::{Error, FlowReader, FlowWriter, Result, fabrics::instance::FabricsInstanceContext};

pub struct Regions {
    ctx: Rc<FabricsInstanceContext>,
    inner: mxl_sys::FabricsRegions,
}

impl From<&Regions> for mxl_sys::FabricsRegions {
    fn from(value: &Regions) -> Self {
        value.inner
    }
}

impl Regions {
    fn new(ctx: Rc<FabricsInstanceContext>, inner: mxl_sys::FabricsRegions) -> Self {
        Regions { ctx, inner }
    }

    pub(crate) fn from_flow_reader(
        ctx: Rc<FabricsInstanceContext>,
        flow_reader: &FlowReader,
    ) -> Result<Self> {
        let mut out_regions = mxl_sys::FabricsRegions::default();
        Error::from_status(unsafe {
            ctx.api()
                .fabrics_regions_for_flow_reader(flow_reader.inner(), &mut out_regions)
        })?;

        Ok(Self::new(ctx, out_regions))
    }

    pub(crate) fn from_flow_writer(
        ctx: Rc<FabricsInstanceContext>,
        flow_writer: &FlowWriter,
    ) -> Result<Self> {
        let mut out_regions = mxl_sys::FabricsRegions::default();
        Error::from_status(unsafe {
            ctx.api()
                .fabrics_regions_for_flow_writer(flow_writer.inner(), &mut out_regions)
        })?;

        Ok(Self::new(ctx, out_regions))
    }
}

impl Drop for Regions {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe { self.ctx.api().fabrics_regions_free(self.inner) };
        }

        // Clean up resources associated with Regions if necessary
    }
}
