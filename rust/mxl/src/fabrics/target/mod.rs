mod config;
mod grain;
mod samples;

use std::rc::Rc;

use crate::{
    FlowConfigInfo,
    error::{Error, Result},
    fabrics::{instance::FabricsInstanceContext, target_info::TargetInfo},
};
pub use config::Config;
pub use grain::GrainTarget;
pub use samples::SampleTarget;

pub struct Target {
    ctx: Rc<FabricsInstanceContext>,
    inner: mxl_sys::fabrics::FabricsTarget,
}

pub enum TargetRead {
    Grain(GrainTarget),
    Sample(SampleTarget),
}

impl Target {
    pub(crate) fn new(
        ctx: Rc<FabricsInstanceContext>,
        target: mxl_sys::fabrics::FabricsTarget,
    ) -> Self {
        Self { ctx, inner: target }
    }
    pub fn setup(&self, config: &Config) -> Result<TargetInfo> {
        let mut info = mxl_sys::fabrics::FabricsTargetInfo::default();
        Error::from_status(unsafe {
            self.ctx
                .api()
                .fabrics_target_setup(self.inner, &config.try_into()?, &mut info)
        })?;
        Ok(TargetInfo::new(self.ctx.clone(), info))
    }

    pub fn specialize(self, flow_config: &FlowConfigInfo) -> TargetRead {
        if flow_config.is_discrete_flow() {
            TargetRead::Grain(GrainTarget::new(self))
        } else {
            TargetRead::Sample(SampleTarget::new(self))
        }
    }
}

impl Drop for Target {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe {
                self.ctx
                    .api()
                    .fabrics_destroy_target(self.ctx.inner, self.inner);
            }
        }
    }
}

/// Create a new unspecialized target.
#[doc(hidden)]
pub(crate) fn create_target(ctx: &Rc<FabricsInstanceContext>) -> Result<Target> {
    let mut target = mxl_sys::fabrics::FabricsTarget::default();
    unsafe {
        Error::from_status(ctx.api().fabrics_create_target(ctx.inner, &mut target))?;
    }
    if target.is_null() {
        return Err(Error::Other("Failed to create fabrics target.".to_string()));
    }

    Ok(Target::new(ctx.clone(), target))
}
