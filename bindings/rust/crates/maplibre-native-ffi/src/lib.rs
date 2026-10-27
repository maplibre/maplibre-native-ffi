//! Safe Rust binding for the MapLibre Native C API.
//!
//! Each handle type is the one owner of its native handle: it can't be
//! cloned, a consuming call such as `release` takes it and hands it back when
//! native refuses, and `Drop` disposes it. Native keeps a parent alive while
//! its children need it, so a child owner holds no reference to its parent's.
//! Handles are any-thread; native rejects a graphics call from the wrong
//! thread with [`Status::WrongThread`].
//!
//! Values that native lends until a call ends borrow what lends them. An
//! acquired frame's backend textures borrow its [`AcquiredFrameView`], which
//! keeps them valid until it drops, and a callback's response borrows the
//! callback. Inputs borrow the caller's text and bytes for the call.
//!
//! Native registrations retain their callback closures until they are cleared
//! or their owner retires. A callback that shares an owner with the host,
//! through an `Arc`, should capture a `std::sync::Weak` reference to it.
//! Capturing a strong `Arc` creates a reference cycle that requires an
//! explicit clear or release.
//!
//! Each owner's `queued(&queue)` view makes the same calls in queue mode on a
//! [`DeliveryQueue`]: native copies each completion and callback into the
//! queue, and the host runs them where it drains it. A completion's handler
//! receives a [`QueuedResult`] that borrows its batch, such as a texture
//! readback whose pixels it reads without a copy, and a registration's
//! callbacks run from the queue until it retires. [`Dispatcher`] drains on a
//! thread of its own, which adds one thread hop to a synchronous round trip;
//! callback mode and a [`LocalDispatcher`] drained by the waiting thread don't.
//!
//! The binding contains callback errors and panics and returns a failure value
//! to native. Failures that no caller can receive go to the reporter that
//! [`set_reporter`] installs, as [`Report::CallbackError`], or to standard error
//! when none is installed. A completion conversion panic resolves its future
//! with an error. A panic also reaches the standard panic hook as usual.
//!
//! The `metal`, `opengl`, `vulkan`, and `webgpu` features pick the one render
//! backend the native library is built with, so they don't combine. Only the
//! final binary enables one; a library that depends on this crate leaves them
//! off.
//!
//! In the browser, [`RuntimeHandle::create`] returns [`Status::WrongThread`] on the
//! main thread, and [`NativeFuture::wait`] blocks, so a host runs the binding
//! on a worker, such as the one Emscripten's `-sPROXY_TO_PTHREAD` provides.

#![deny(unsafe_op_in_unsafe_fn)]

mod abi;
mod binding_support;
mod call;
mod callback;
mod completion;
mod convert;
mod delivery;
mod error;
mod finalize;
#[allow(clippy::all, dead_code, unused_imports)]
mod generated;
mod handle;
mod ptr;
mod report;
mod status;
mod string;

pub use call::Lent;
pub use completion::{CommandCompletion, NativeFuture, Resolver, pending};
pub use delivery::{
    BatchBytes, Callback, Command, CompletionHandler, Copied, CopiedList, CopiedOptional, Delivery,
    DeliveryBatch, DeliveryKind, DeliveryQueue, Dispatcher, Local, LocalDispatcher, QueueKind,
    QueuedResult, QueuedValue, Readback, Shared, TakenDelivery, TextureReadbackView, Unit,
};
pub use error::{Error, Result};
pub use generated::*;
pub use handle::NativeHandleLeak;
/// The raw C declarations, which some calls take or return.
pub use maplibre_native_ffi_sys as sys;
pub use report::{Report, Reporter, set_reporter};
