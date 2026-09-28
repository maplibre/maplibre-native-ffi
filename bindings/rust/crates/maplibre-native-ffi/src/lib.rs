//! Safe Rust binding for the MapLibre Native C API.
//!
//! Runtime and map control handles are any-thread; graphics sessions retain
//! their backend thread affinity. This crate also owns parent retention, Rust
//! errors, callback closures, and render-resource lifetimes. Shared C ABI
//! adaptation lives in `maplibre-native-ffi-core`.
//!
//! Native registrations retain their callback closures until they are cleared
//! or their owner retires. A callback that needs its own owner should capture a
//! `std::sync::Weak` reference. Capturing a strong `Arc` of that owner creates a
//! reference cycle that requires an explicit clear or release.

#![deny(unsafe_op_in_unsafe_fn)]
#![cfg_attr(test, allow(clippy::field_reassign_with_default))]

mod completion;
mod events;
mod geojson;
#[path = "global/generated.rs"]
#[allow(clippy::all)]
mod global;
mod handle;
mod logging;
pub use global::*;
mod map;
#[allow(clippy::all, dead_code, unused_imports)]
mod owned_generated;
pub use owned_generated::*;
mod projection;
mod render;
mod runtime;
#[cfg(test)]
mod test_support;
#[cfg(test)]
mod tests;

pub use completion::{CommandCompletion, NativeFuture};
pub use events::MapId;
pub use geojson::GeoJsonSourceDataHandle;
pub use map::MapHandle;
pub use maplibre_core::generated::*;
pub use maplibre_core::handle::{NativeHandleLeak, set_leak_reporter};
pub use maplibre_core::{Error, ErrorKind, Result};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;
pub use projection::MapProjectionHandle;
pub use render::{AcquiredFrameHandle, RenderSessionHandle};
pub use runtime::RuntimeHandle;
