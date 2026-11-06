// Generated from C headers by tools/bindgen. Do not edit.
use crate::call::{Call, Lent};
use crate::callback;
use crate::completion::{self, CommandCompletion, NativeFuture};
use crate::convert::{
    self, FromNative, InputArena, ToNative, from_native, native_enum, native_flags, to_native,
};
use crate::handle::native_owner;
use crate::{Error, ReleaseError, Result};
use maplibre_native_ffi_sys as sys;

mod values;
pub use values::*;
mod acquired_frame;
pub use acquired_frame::*;
mod frame_pacer;
pub use frame_pacer::*;
mod map;
pub use map::*;
mod map_projection;
pub use map_projection::*;
mod offscreen_device;
pub use offscreen_device::*;
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
mod vulkan_device;
pub use vulkan_device::*;
mod global;
pub use global::*;

#[cfg(test)]
static_assertions::assert_impl_all!(AcquiredFrameHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(AcquiredFrameHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(FramePacerHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(FramePacerHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(MapHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(MapHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(MapProjectionHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(MapProjectionHandle: Clone);
#[cfg(test)]
static_assertions::assert_impl_all!(OffscreenDeviceHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(OffscreenDeviceHandle: Clone);
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
#[cfg(test)]
static_assertions::assert_impl_all!(VulkanDeviceHandle: Send, Sync);
#[cfg(test)]
static_assertions::assert_not_impl_any!(VulkanDeviceHandle: Clone);
