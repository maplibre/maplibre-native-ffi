// Generated from C headers by tools/bindgen. Do not edit.
use crate::call::Call;
use crate::callback;
use crate::completion::{self, CommandCompletion, NativeFuture};
use crate::convert::{
    self, FromNative, InputArena, ToNative, from_native, native_enum, native_flags, to_native,
};
use crate::handle::native_owner;
use crate::{Error, Result};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

mod values;
pub use values::*;
mod acquired_frame;
pub use acquired_frame::*;
mod buffer;
pub use buffer::*;
mod event_batch;
pub use event_batch::*;
mod geojson_source_data;
pub use geojson_source_data::*;
mod map;
pub use map::*;
mod map_projection;
pub use map_projection::*;
mod render_frame_batch;
pub use render_frame_batch::*;
mod render_session;
pub use render_session::*;
mod runtime;
pub use runtime::*;
mod global;
pub use global::*;

#[cfg(test)]
static_assertions::assert_impl_all!(AcquiredFrameHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(BufferHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(EventBatchHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(GeojsonSourceDataHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(MapHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(MapProjectionHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(RenderFrameBatchHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(RenderSessionHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_impl_all!(RuntimeHandle: Send, Sync);
