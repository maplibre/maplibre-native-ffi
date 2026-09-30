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

mod completion;
#[allow(clippy::all, dead_code, unused_imports, unused_parens)]
mod generated;
mod handle;

pub use completion::{CommandCompletion, NativeFuture};
pub use generated::*;
pub use maplibre_core::generated::*;
pub use maplibre_core::handle::{NativeHandleLeak, set_leak_reporter};
pub use maplibre_core::{Error, ErrorKind, Result};
use maplibre_native_ffi_core as maplibre_core;
