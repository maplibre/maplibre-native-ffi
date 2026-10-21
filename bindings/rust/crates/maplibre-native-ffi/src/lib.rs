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
//!
//! The binding contains callback errors and panics and returns a failure value
//! to native. Failures that no caller can receive go to the reporter that
//! [`set_reporter`] installs, as [`Report::CallbackError`], or to standard error
//! when none is installed. A completion conversion panic resolves its future
//! with an error. A panic also reaches the standard panic hook as usual.

#![deny(unsafe_op_in_unsafe_fn)]

mod call;
mod callback;
mod completion;
mod convert;
#[allow(clippy::all, dead_code, unused_imports)]
mod generated;
mod handle;

pub use completion::{CommandCompletion, NativeFuture};
pub use generated::*;
pub use maplibre_core::handle::NativeHandleLeak;
pub use maplibre_core::report::{Report, Reporter, set_reporter};
pub use maplibre_core::{Error, Result};
use maplibre_native_ffi_core as maplibre_core;
