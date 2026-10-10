//! The host side of native callbacks: the registration state a trampoline
//! reads, the reentry contract host code runs under, and failure containment.
//!
//! Each generated trampoline reads its registration with [`state`] and runs
//! host code through [`invoke`] or [`invoke_status`], which name the C
//! callback type that a contained error is reported against. Native releases a
//! registration through [`release`] once it can no longer call it.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};

use maplibre_native_ffi_core::callback::PolicyScope;
use maplibre_native_ffi_core::error::status_for_error;
use maplibre_native_ffi_core::report::{Report, report};
use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};

/// The native operations that host code may call while a callback runs, and
/// the owner they must name, or `None` for a callback that allows any call.
pub(crate) type Reentry = Option<(&'static [&'static str], u64)>;

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

/// Enters the reentry contract for host code that runs outside [`invoke`].
pub(crate) fn enter(reentry: Reentry) -> Option<PolicyScope> {
    reentry.map(|(operations, owner)| PolicyScope::enter(operations, owner))
}

fn run<R>(reentry: Reentry, body: impl FnOnce() -> Result<R>) -> std::thread::Result<Result<R>> {
    catch_unwind(AssertUnwindSafe(|| {
        let _policy = enter(reentry);
        body()
    }))
}

/// Reports an error that the `callback` trampoline contained. The reporter
/// admits no native call, since it runs on the native callback's stack.
fn report_error(callback: &'static str, error: Error) {
    let _policy = PolicyScope::enter(&[], 0);
    report(Report::CallbackError { callback, error });
}

/// Runs host code for the `callback` trampoline, returning `fallback` when it
/// fails or panics. An error is reported; a panic has already reached the
/// panic hook.
pub(crate) fn invoke<R>(
    callback: &'static str,
    reentry: Reentry,
    fallback: R,
    body: impl FnOnce() -> Result<R>,
) -> R {
    match run(reentry, body) {
        Ok(Ok(value)) => value,
        Ok(Err(error)) => {
            report_error(callback, error);
            fallback
        }
        Err(_) => fallback,
    }
}

/// Runs host code for the `callback` trampoline, which returns a status: an
/// error's status when host code fails, and `failure` when it panics. Native
/// treats any failure status as the callback's fallback, so an error is
/// reported too.
pub(crate) fn invoke_status(
    callback: &'static str,
    reentry: Reentry,
    failure: sys::mln_status,
    body: impl FnOnce() -> Result<()>,
) -> sys::mln_status {
    match run(reentry, body) {
        Ok(Ok(())) => sys::MLN_STATUS_OK,
        Ok(Err(error)) => {
            let status = status_for_error(&error);
            report_error(callback, error);
            status
        }
        Err(_) => failure,
    }
}

/// Drops a registration's state of type `S` once native can no longer call
/// it. The drop admits no native call, so a handle that it disposes finalizes
/// off the native callback stack.
///
/// # Safety
///
/// `user_data` must be a boxed `S` that native releases exactly once.
pub(crate) unsafe extern "C" fn release<S>(user_data: *mut c_void) {
    let _policy = PolicyScope::enter(&[], 0);
    let _ = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: native releases the registration exactly once.
        drop(unsafe { Box::from_raw(user_data.cast::<S>()) })
    }));
}
