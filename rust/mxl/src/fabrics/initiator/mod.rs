mod config;
mod grain;
mod samples;

use crate::{
    FlowConfigInfo,
    error::{Error, Result},
    fabrics::instance::FabricsInstanceContext,
};

pub use config::Config;

use std::{marker::PhantomData, rc::Rc};

/// Wrapper class that holds a reference count to the Fabrics Instance and the actual initiator instance.
struct InitiatorInstance {
    ctx: Rc<FabricsInstanceContext>,
    inner: mxl_sys::fabrics::FabricsInitiator,
}
impl Drop for InitiatorInstance {
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

// Initiator states
pub struct New {}
pub struct Initializing {}
pub struct Specializing {}
pub struct Grain {}
pub struct Samples {}
impl InitiatorState for New {}
impl InitiatorState for Initializing {}
impl InitiatorState for Specializing {}
impl InitiatorState for Grain {}
impl InitiatorState for Samples {}
pub trait InitiatorState {}

pub struct Initiator<S: InitiatorState> {
    instance: InitiatorInstance,
    marker: std::marker::PhantomData<S>,
}
pub enum Either<G, S> {
    Grain(G),
    Samples(S),
}

impl Initiator<New> {
    /// Create a new initiator
    pub(crate) fn new(
        ctx: Rc<FabricsInstanceContext>,
        initiator: mxl_sys::fabrics::FabricsInitiator,
    ) -> Initiator<Initializing> {
        let instance = InitiatorInstance {
            ctx,
            inner: initiator,
        };
        Initiator {
            instance,
            marker: std::marker::PhantomData,
        }
    }
}

impl Initiator<Initializing> {
    ///  Configure the initiator.
    pub fn setup(self, config: &Config) -> Result<Initiator<Specializing>> {
        Error::from_status(unsafe {
            self.instance
                .ctx
                .api()
                .fabrics_initiator_setup(self.instance.inner, &config.try_into()?)
        })?;
        Ok(Initiator {
            instance: self.instance,
            marker: PhantomData,
        })
    }
}

impl Initiator<Specializing> {
    /// Specialize the initator into a concrete grain or samples initator
    pub fn specialize(
        self,
        flow_config: &FlowConfigInfo,
    ) -> Result<Either<Initiator<Grain>, Initiator<Samples>>> {
        Ok(if flow_config.is_discrete_flow() {
            Either::Grain(Initiator {
                instance: self.instance,
                marker: PhantomData,
            })
        } else {
            Either::Samples(Initiator {
                instance: self.instance,
                marker: PhantomData,
            })
        })
    }
}

#[doc(hidden)]
pub(crate) fn create_initiator(
    ctx: &Rc<FabricsInstanceContext>,
) -> Result<Initiator<Initializing>> {
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
