mod config;
mod grain;
mod samples;

use crate::{
    FlowConfigInfo,
    error::{Error, Result},
    fabrics::{instance::FabricsInstanceContext, target_info::TargetInfo},
};

pub use config::Config;
pub use grain::GrainInitiator;
pub use samples::SampleInitiator;

use std::{rc::Rc, time::Duration};

pub struct Initiator {
    ctx: Rc<FabricsInstanceContext>,
    inner: mxl_sys::fabrics::FabricsInitiator,
}

pub enum InitiatorTransfer {
    Grain(GrainInitiator),
    Sample(SampleInitiator),
}

impl Initiator {
    pub(crate) fn new(
        ctx: Rc<FabricsInstanceContext>,
        initiator: mxl_sys::fabrics::FabricsInitiator,
    ) -> Self {
        Self {
            ctx,
            inner: initiator,
        }
    }

    pub fn setup(&self, config: &Config) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx
                .api()
                .fabrics_initiator_setup(self.inner, &config.try_into()?)
        })
    }

    pub fn add_target(&self, target: &TargetInfo) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx
                .api()
                .fabrics_initiator_add_target(self.inner, target.inner)
        })
    }

    pub fn remove_target(&self, target: &TargetInfo) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx
                .api()
                .fabrics_initiator_remove_target(self.inner, target.inner)
        })
    }

    pub fn make_progress_non_blocking(&self) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx
                .api()
                .fabrics_initiator_make_progress_non_blocking(self.inner)
        })
    }

    pub fn make_progress(&self, timeout: Duration) -> Result<()> {
        Error::from_status(unsafe {
            self.ctx
                .api()
                .fabrics_initiator_make_progress_blocking(self.inner, timeout.as_millis() as u16)
        })
    }

    pub fn specialize(self, flow_config: &FlowConfigInfo) -> Result<InitiatorTransfer> {
        Ok(if flow_config.is_discrete_flow() {
            InitiatorTransfer::Grain(GrainInitiator::new(self))
        } else {
            InitiatorTransfer::Sample(SampleInitiator::new(self))
        })
    }
}

impl Drop for Initiator {
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
pub(crate) fn create_initiator(ctx: &Rc<FabricsInstanceContext>) -> Result<Initiator> {
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
    Ok(Initiator::new(ctx.clone(), initiator))
}
