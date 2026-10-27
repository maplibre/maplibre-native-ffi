// Generated from C headers by tools/bindgen. Do not edit.
use crate::call::{Call, Lent};
use crate::callback;
use crate::completion::{self, CommandCompletion, NativeFuture};
use crate::convert::{
    self, FromNative, InputArena, ToNative, from_native, native_enum, native_flags, to_native,
};
use crate::handle::native_owner;
use crate::{Error, Result};
use maplibre_native_ffi_sys as sys;

mod values;
pub use values::*;
mod acquired_frame;
pub use acquired_frame::*;
mod geo_json_source_data;
pub use geo_json_source_data::*;
mod graphics_device;
pub use graphics_device::*;
mod map;
pub use map::*;
mod map_projection;
pub use map_projection::*;
mod render_session;
pub use render_session::*;
mod resource_request;
pub use resource_request::*;
mod resource_transform_request;
pub use resource_transform_request::*;
mod runtime;
pub use runtime::*;
mod style_expression;
pub use style_expression::*;
mod global;
pub use global::*;

#[cfg(test)]
static_assertions::assert_impl_all!(AcquiredFrameHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(AcquiredFrameHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(GeoJsonSourceDataHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(GeoJsonSourceDataHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(GraphicsDeviceHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(GraphicsDeviceHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(MapHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(MapHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(MapProjectionHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(MapProjectionHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(RenderSessionHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(RenderSessionHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(ResourceRequestHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(ResourceRequestHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(ResourceTransformRequestHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(ResourceTransformRequestHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(RuntimeHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(RuntimeHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(StyleExpressionHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(StyleExpressionHandle: Clone);
