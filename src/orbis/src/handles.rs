// The handles the C API hands out: a 64-bit id with the kind in its top byte
// and a counter below that never repeats, so a stale or foreign handle is an
// error, never a dangling pointer.

use std::collections::HashMap;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, LazyLock, Mutex};

use crate::base::{Failure, Result};
use crate::delivery::DeliveryQueue;
use crate::map::MapHandle;
use crate::runtime::RuntimeHandle;
use crate::session::{FramePacer, SessionHandle};
use crate::types::mln_render_frame_result;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[repr(u8)]
pub enum Kind {
    Runtime = 1,
    Map = 2,
    Session = 4,
    Batch = 21,
    Queue = 23,
    OffscreenDevice = 26,
    Pacer = 27,
}

impl Kind {
    fn name(self) -> &'static str {
        match self {
            Kind::Runtime => "mln_runtime",
            Kind::Map => "mln_map",
            Kind::Session => "mln_render_session",
            Kind::Batch => "mln_render_frame_batch",
            Kind::Queue => "mln_delivery_queue",
            Kind::OffscreenDevice => "mln_offscreen_device",
            Kind::Pacer => "mln_frame_pacer",
        }
    }
}

#[derive(Clone)]
pub enum Object {
    Runtime(Arc<RuntimeHandle>),
    Map(Arc<MapHandle>),
    Session(Arc<SessionHandle>),
    /// Holds the results a drained batch's view points at.
    Batch(#[allow(dead_code)] Arc<Vec<mln_render_frame_result>>),
    Queue(Arc<DeliveryQueue>),
    /// Orbis owns the device of its runtime, so an offscreen device is only a
    /// token that sessions accept.
    OffscreenDevice,
    Pacer(Arc<FramePacer>),
}

impl Object {
    fn kind(&self) -> Kind {
        match self {
            Object::Runtime(_) => Kind::Runtime,
            Object::Map(_) => Kind::Map,
            Object::Session(_) => Kind::Session,
            Object::Batch(_) => Kind::Batch,
            Object::Queue(_) => Kind::Queue,
            Object::OffscreenDevice => Kind::OffscreenDevice,
            Object::Pacer(_) => Kind::Pacer,
        }
    }
}

static TABLE: LazyLock<Mutex<HashMap<u64, Object>>> = LazyLock::new(Default::default);
static NEXT: AtomicU64 = AtomicU64::new(1);

pub fn insert(object: Object) -> u64 {
    let id = ((object.kind() as u64) << 56) | NEXT.fetch_add(1, Ordering::Relaxed);
    TABLE.lock().unwrap().insert(id, object);
    id
}

pub fn get(handle: u64) -> Option<Object> {
    TABLE.lock().unwrap().get(&handle).cloned()
}

pub fn remove(handle: u64) -> Option<Object> {
    TABLE.lock().unwrap().remove(&handle)
}

fn lookup(handle: u64, kind: Kind) -> Result<Object> {
    if handle == 0 {
        return Err(Failure::argument(format!("the {} is null", kind.name())));
    }
    match get(handle) {
        Some(object) if object.kind() == kind => Ok(object),
        Some(_) => Err(Failure::argument(format!(
            "the handle isn't an {}",
            kind.name()
        ))),
        None => Err(Failure::state(format!("the {} was released", kind.name()))),
    }
}

pub fn runtime(handle: u64) -> Result<Arc<RuntimeHandle>> {
    match lookup(handle, Kind::Runtime)? {
        Object::Runtime(r) => Ok(r),
        _ => unreachable!(),
    }
}

pub fn map(handle: u64) -> Result<Arc<MapHandle>> {
    match lookup(handle, Kind::Map)? {
        Object::Map(m) => Ok(m),
        _ => unreachable!(),
    }
}

pub fn session(handle: u64) -> Result<Arc<SessionHandle>> {
    match lookup(handle, Kind::Session)? {
        Object::Session(s) => Ok(s),
        _ => unreachable!(),
    }
}

pub fn queue(handle: u64) -> Result<Arc<DeliveryQueue>> {
    match lookup(handle, Kind::Queue)? {
        Object::Queue(q) => Ok(q),
        _ => unreachable!(),
    }
}

pub fn pacer(handle: u64) -> Result<Arc<FramePacer>> {
    match lookup(handle, Kind::Pacer)? {
        Object::Pacer(p) => Ok(p),
        _ => unreachable!(),
    }
}

pub fn offscreen_device(handle: u64) -> Result<()> {
    lookup(handle, Kind::OffscreenDevice).map(|_| ())
}
