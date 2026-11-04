// Ports maplibre-native-ffi src/handles/handle_table.{hpp,cpp}: every object
// the C API hands out is a 64-bit generational handle, so a stale or foreign
// handle is an error with the C++ diagnostic, never a dangling pointer.
//
//   bits 63..56  kind        1..255; 0 never appears in a live handle
//   bits 55..36  index       slot within this kind's table
//   bits 35..0   generation  slot reuse counter, starting at 1
//
// One table for all kinds (C++: one per kind); a freed slot is reused by the
// kind's next object with the next generation.

use crate::delivery::DeliveryQueue;
use crate::diagnostics::Failure;
use crate::map::MapShared;
use crate::projection::ProjectionObject;
use crate::runtime::RuntimeShared;
use crate::session::{FrameBatch, SessionShared};
use crate::types::{MLN_STATUS_INVALID_ARGUMENT, MLN_STATUS_INVALID_STATE};
use std::collections::HashMap;
use std::sync::{Arc, LazyLock, Mutex};

/// C++ `HandleKind`.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[repr(u8)]
pub enum Kind {
    Runtime = 1,
    Map = 2,
    MapProjection = 3,
    RenderSession = 4,
    RenderFrameBatch = 21,
    DeliveryQueue = 23,
    OffscreenDevice = 26,
}

/// C++ `handle_kind_name`, over every kind the C++ table issues.
fn kind_name(kind: u8) -> Option<&'static str> {
    Some(match kind {
        1 => "mln_runtime",
        2 => "mln_map",
        3 => "mln_map_projection",
        4 => "mln_render_session",
        12 => "mln_resource_request",
        13 => "mln_resource_transform_request",
        20 => "mln_acquired_frame",
        21 => "mln_render_frame_batch",
        22 => "mln_geo_json_source_data",
        23 => "mln_delivery_queue",
        25 => "mln_style_expression",
        26 => "mln_offscreen_device",
        _ => return None,
    })
}

const GENERATION_BITS: u32 = 36;
const INDEX_BITS: u32 = 20;
const MAX_GENERATION: u64 = (1 << GENERATION_BITS) - 1;
const MAX_INDEX: u64 = (1 << INDEX_BITS) - 1;

fn encode(kind: Kind, index: u64, generation: u64) -> u64 {
    ((kind as u64) << (INDEX_BITS + GENERATION_BITS)) | (index << GENERATION_BITS) | generation
}

fn kind_of(handle: u64) -> u8 {
    (handle >> (INDEX_BITS + GENERATION_BITS)) as u8
}

fn index_of(handle: u64) -> usize {
    ((handle >> GENERATION_BITS) & MAX_INDEX) as usize
}

fn generation_of(handle: u64) -> u64 {
    handle & MAX_GENERATION
}

#[derive(Clone)]
pub enum Object {
    Runtime(Arc<RuntimeShared>),
    Map(Arc<MapShared>),
    Projection(Arc<ProjectionObject>),
    Session(Arc<SessionShared>),
    Queue(Arc<DeliveryQueue>),
    // Held so the batch's results stay valid until its release.
    #[allow(dead_code)]
    FrameBatch(Arc<FrameBatch>),
    OffscreenDevice,
}

impl Object {
    fn kind(&self) -> Kind {
        match self {
            Object::Runtime(_) => Kind::Runtime,
            Object::Map(_) => Kind::Map,
            Object::Projection(_) => Kind::MapProjection,
            Object::Session(_) => Kind::RenderSession,
            Object::Queue(_) => Kind::DeliveryQueue,
            Object::FrameBatch(_) => Kind::RenderFrameBatch,
            Object::OffscreenDevice => Kind::OffscreenDevice,
        }
    }
}

/// `generation` is the one the slot issues next, or the one it issued last
/// while `object` is set (or reserved). Every lower generation was issued and
/// retired.
struct Slot {
    generation: u64,
    object: Option<Object>,
}

#[derive(Default)]
struct KindTable {
    slots: Vec<Slot>,
    free: Vec<usize>,
}

/// Each kind's table, by its `Kind` byte.
static TABLE: LazyLock<Mutex<HashMap<u8, KindTable>>> = LazyLock::new(|| Mutex::new(HashMap::new()));

/// A handle of `kind` whose object comes later through `insert_at`, for an
/// object that holds its own handle. It names nothing until then.
pub fn reserve(kind: Kind) -> u64 {
    let mut table = TABLE.lock().unwrap();
    let table = table.entry(kind as u8).or_default();
    let index = match table.free.pop() {
        Some(index) => index,
        None => {
            table.slots.push(Slot { generation: 1, object: None });
            table.slots.len() - 1
        }
    };
    encode(kind, index as u64, table.slots[index].generation)
}

pub fn insert(object: Object) -> u64 {
    let handle = reserve(object.kind());
    insert_at(handle, object);
    handle
}

pub fn insert_at(handle: u64, object: Object) {
    let mut table = TABLE.lock().unwrap();
    if let Some(slot) = table.get_mut(&(object.kind() as u8)).and_then(|t| t.slots.get_mut(index_of(handle))) {
        slot.object = Some(object);
    }
}

fn live(table: &HashMap<u8, KindTable>, handle: u64) -> Option<&Slot> {
    let slot = table.get(&kind_of(handle))?.slots.get(index_of(handle))?;
    (slot.object.is_some() && slot.generation == generation_of(handle)).then_some(slot)
}

pub fn get(handle: impl Into<u64>) -> Option<Object> {
    let table = TABLE.lock().unwrap();
    live(&table, handle.into()).and_then(|slot| slot.object.clone())
}

/// Retires `handle` and returns what it named. Every copy of the handle is
/// stale from here on.
pub fn remove(handle: impl Into<u64>) -> Option<Object> {
    let handle = handle.into();
    let mut table = TABLE.lock().unwrap();
    live(&table, handle)?;
    let table = table.get_mut(&kind_of(handle))?;
    let index = index_of(handle);
    let slot = &mut table.slots[index];
    let object = slot.object.take();
    slot.generation += 1;
    // A slot whose generation is exhausted is retired instead of recycled.
    if slot.generation <= MAX_GENERATION {
        table.free.push(index);
    }
    object
}

/// C++ `classify_handle_fault` + `report_handle_fault`: the failure for a
/// `handle` that doesn't name a live object of kind `expected`.
fn fault(expected: Kind, handle: u64) -> Failure {
    let expected_name = kind_name(expected as u8).unwrap_or("handle");
    if handle == 0 {
        return Failure::new(MLN_STATUS_INVALID_ARGUMENT, format!("{expected_name} handle must not be null"));
    }
    let Some(actual_name) = kind_name(kind_of(handle)) else {
        return Failure::new(MLN_STATUS_INVALID_ARGUMENT, format!("{handle:#x} is not a valid {expected_name} handle"));
    };
    if kind_of(handle) != expected as u8 {
        return Failure::new(
            MLN_STATUS_INVALID_ARGUMENT,
            format!("handle {handle:#x} is an {actual_name} handle, not an {expected_name} handle"),
        );
    }
    let issued = {
        let table = TABLE.lock().unwrap();
        let generation = generation_of(handle);
        table
            .get(&(expected as u8))
            .and_then(|t| t.slots.get(index_of(handle)))
            .is_some_and(|slot| generation != 0 && generation < slot.generation)
    };
    if issued {
        Failure::new(
            MLN_STATUS_INVALID_STATE,
            format!("{expected_name} handle {handle:#x} is stale; the object it named has been released"),
        )
    } else {
        Failure::new(
            MLN_STATUS_INVALID_ARGUMENT,
            format!("{expected_name} handle {handle:#x} was never created by this process"),
        )
    }
}

/// The live object of kind `expected` that `handle` names, or the C++ fault.
pub fn resolve(expected: Kind, handle: impl Into<u64>) -> Result<Object, Failure> {
    let handle = handle.into();
    match get(handle) {
        Some(object) if object.kind() == expected => Ok(object),
        _ => Err(fault(expected, handle)),
    }
}

pub fn runtime(handle: impl Into<u64>) -> Result<Arc<RuntimeShared>, Failure> {
    match resolve(Kind::Runtime, handle)? {
        Object::Runtime(runtime) => Ok(runtime),
        _ => unreachable!(),
    }
}

pub fn map(handle: impl Into<u64>) -> Result<Arc<MapShared>, Failure> {
    match resolve(Kind::Map, handle)? {
        Object::Map(map) => Ok(map),
        _ => unreachable!(),
    }
}

pub fn projection(handle: impl Into<u64>) -> Result<Arc<ProjectionObject>, Failure> {
    match resolve(Kind::MapProjection, handle)? {
        Object::Projection(projection) => Ok(projection),
        _ => unreachable!(),
    }
}

pub fn session(handle: impl Into<u64>) -> Result<Arc<SessionShared>, Failure> {
    match resolve(Kind::RenderSession, handle)? {
        Object::Session(session) => Ok(session),
        _ => unreachable!(),
    }
}

pub fn queue(handle: impl Into<u64>) -> Result<Arc<DeliveryQueue>, Failure> {
    match resolve(Kind::DeliveryQueue, handle)? {
        Object::Queue(queue) => Ok(queue),
        _ => unreachable!(),
    }
}
