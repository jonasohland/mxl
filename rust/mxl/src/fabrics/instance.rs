use std::rc::Rc;
use std::sync::Arc;

use crate::api::MxlApiHandle;
use crate::error::{Error, Result};
use crate::fabrics::initiator::{UnspecInitiator, create_initiator};
use crate::fabrics::provider::Provider;
use crate::fabrics::region::Regions;
use crate::fabrics::target::{UnspecTarget, create_target};
use crate::fabrics::target_info::TargetInfo;
use crate::instance::InstanceContext;
use crate::{FlowReader, FlowWriter};

pub(crate) fn create_instance(ctx: &Arc<InstanceContext>) -> Result<FabricsInstance> {
    let mut inst = std::ptr::null_mut();
    unsafe {
        Error::from_status(ctx.api.fabrics_create_instance(ctx.instance, &mut inst))?;
    }
    if inst.is_null() {
        return Err(Error::Other(
            "Failed to create fabrics instance.".to_string(),
        ));
    }

    let ctx = Rc::new(FabricsInstanceContext {
        instance_ctx: ctx.clone(),
        inner: inst,
    });

    Ok(FabricsInstance::new(ctx))
}

pub(crate) struct FabricsInstanceContext {
    instance_ctx: Arc<InstanceContext>,
    pub(crate) inner: mxl_sys::FabricsInstance,
}

impl FabricsInstanceContext {
    pub(crate) fn api(&self) -> &MxlApiHandle {
        &self.instance_ctx.api
    }
}

impl Drop for FabricsInstanceContext {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe {
                let _ = self.instance_ctx.api.fabrics_destroy_instance(self.inner);
            }
        }
    }
}

/// This is merely a factory for creating Fabrics related objects such as Targets, Initiators, etc.
/// The fabrics instance and its pointer are held in the FabricsInstanceContext.
pub struct FabricsInstance {
    ctx: Rc<FabricsInstanceContext>,
}
impl FabricsInstance {
    fn new(ctx: Rc<FabricsInstanceContext>) -> Self {
        Self { ctx }
    }

    pub fn create_target(&self) -> Result<UnspecTarget> {
        create_target(&self.ctx)
    }

    pub fn create_initiator(&self) -> Result<UnspecInitiator> {
        create_initiator(&self.ctx)
    }

    pub fn regions_from_reader(&self, flow_reader: &FlowReader) -> Result<Regions> {
        Regions::from_flow_reader(self.ctx.clone(), flow_reader)
    }

    pub fn regions_from_writer(&self, flow_writer: &FlowWriter) -> Result<Regions> {
        Regions::from_flow_writer(self.ctx.clone(), flow_writer)
    }

    pub fn provider_from_str(&self, provider: &str) -> Result<Provider> {
        Provider::from_str(self.ctx.clone(), provider)
    }

    pub fn target_info_from_str(&self, target_info: &str) -> Result<TargetInfo> {
        TargetInfo::from_str(self.ctx.clone(), target_info)
    }
}
