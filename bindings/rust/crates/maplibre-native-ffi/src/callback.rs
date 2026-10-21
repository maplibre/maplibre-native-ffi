//! The host side of native callbacks: the registration state a trampoline
//! reads, callback stack scope, and failure containment.
//!
//! Each generated trampoline reads its registration with [`state`] and runs
//! host code through [`invoke`] or [`invoke_status`], which name the C
//! callback type that a contained error is reported against. Native releases a
//! registration through [`release`] once it can no longer call it.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};

use maplibre_native_ffi_core::callback::CallbackScope;
use maplibre_native_ffi_core::report::{Report, report};
use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};

/// The registration state that native passes a trampoline.
///
/// # Safety
///
/// `user_data` must be the state of type `S` that a registration transferred
/// to native, which native keeps alive through each callback.
pub(crate) unsafe fn state<'a, S>(user_data: *mut c_void) -> &'a S {
    // SAFETY: the caller passes the live registration state.
    unsafe { &*user_data.cast::<S>() }
}

/// The registration's callback, which a trampoline runs only when it is set.
pub(crate) fn require<T: ?Sized>(callback: &Option<std::sync::Arc<T>>) -> Result<&T> {
    callback
        .as_deref()
        .ok_or_else(|| Error::invalid_argument("missing callback"))
}

fn run<R>(body: impl FnOnce() -> Result<R>) -> std::thread::Result<Result<R>> {
    catch_unwind(AssertUnwindSafe(|| {
        let _scope = CallbackScope::enter();
        body()
    }))
}

/// Reports an error that the `callback` trampoline contained.
fn report_error(callback: &'static str, error: Error) {
    let _scope = CallbackScope::enter();
    report(Report::CallbackError { callback, error });
}

/// Reports a panic that the `callback` trampoline caught, leaking its payload.
pub(crate) fn report_panic(callback: &'static str, payload: Box<dyn std::any::Any + Send>) {
    // A panic payload can itself panic when dropped across this C boundary.
    std::mem::forget(payload);
    report_error(
        callback,
        Error::new(crate::Status::NativeError, "callback panicked"),
    );
}

/// Runs host code for the `callback` trampoline, returning `fallback` when it
/// fails or panics. Both failures reach the reporter.
pub(crate) fn invoke<R>(
    callback: &'static str,
    fallback: R,
    body: impl FnOnce() -> Result<R>,
) -> R {
    match run(body) {
        Ok(Ok(value)) => value,
        Ok(Err(error)) => {
            report_error(callback, error);
            fallback
        }
        Err(payload) => {
            report_panic(callback, payload);
            fallback
        }
    }
}

/// Runs host code for the `callback` trampoline, which returns a status: an
/// error's status when host code fails, and `failure` when it panics. Native
/// treats any failure status as the callback's fallback, so an error is
/// reported too.
pub(crate) fn invoke_status(
    callback: &'static str,
    failure: sys::mln_status,
    body: impl FnOnce() -> Result<()>,
) -> sys::mln_status {
    match run(body) {
        Ok(Ok(())) => sys::MLN_STATUS_OK,
        Ok(Err(error)) => {
            let status = error.status().as_raw();
            report_error(callback, error);
            status
        }
        Err(payload) => {
            report_panic(callback, payload);
            failure
        }
    }
}

/// Drops a registration's state of type `S` once native can no longer call
/// it. A handle that it disposes finalizes off the native callback stack.
///
/// # Safety
///
/// `user_data` must be a boxed `S` that native releases exactly once.
pub(crate) unsafe extern "C" fn release<S>(user_data: *mut c_void) {
    let _scope = CallbackScope::enter();
    if let Err(payload) = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: native releases the registration exactly once.
        drop(unsafe { Box::from_raw(user_data.cast::<S>()) })
    })) {
        report_panic("mln_user_data_release", payload);
    }
}
