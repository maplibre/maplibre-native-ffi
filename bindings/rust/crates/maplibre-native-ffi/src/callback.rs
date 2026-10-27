//! The host side of native callbacks: the registration state a trampoline
//! reads, callback stack scope, and failure containment.
//!
//! Each generated trampoline reads its registration with [`state`] and runs
//! host code through [`invoke`] or [`invoke_status`], which name the C
//! callback type that a contained error is reported against. Native releases a
//! registration through [`release`] once it can no longer call it.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};

use crate::finalize::CallbackScope;
use crate::report::{Report, report};
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
pub(crate) fn require<C: std::ops::Deref>(callback: &Option<C>) -> Result<&C::Target> {
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

/// Runs host code for the `callback` trampoline, which returns a status.
///
/// `body` fails when the trampoline cannot convert its arguments, and
/// otherwise returns host code's result. Host code that succeeds answers
/// `MLN_STATUS_OK`, and one that fails with an [`Error::Status`] other than
/// `Status::Ok` answers that status on purpose. Any other failure, a panic, or
/// a conversion failure is a bug: it reaches the reporter and answers
/// `failure`.
pub(crate) fn invoke_status(
    callback: &'static str,
    failure: sys::mln_status,
    body: impl FnOnce() -> Result<Result<()>>,
) -> sys::mln_status {
    match run(body) {
        Ok(Ok(Ok(()))) => sys::MLN_STATUS_OK,
        Ok(Ok(Err(Error::Status { status, .. }))) if status != crate::Status::Ok => status.as_raw(),
        Ok(Ok(Err(error)) | Err(error)) => {
            report_error(callback, error);
            failure
        }
        Err(payload) => {
            report_panic(callback, payload);
            failure
        }
    }
}

/// Takes a queued delivery's owned handles for the `callback` that adopts
/// them, and reports whether it may run. A failure reaches the reporter, and
/// the batch keeps the handles, which it releases.
pub(crate) fn taken(callback: &'static str, take: &dyn Fn() -> Result<()>) -> bool {
    match take() {
        Ok(()) => true,
        Err(error) => {
            report_error(callback, error);
            false
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Status;

    // A status that host code fails with on purpose reaches native; any other
    // failure answers the callback's failure. The suite checks which reach
    // the reporter, which is process-global.
    #[test]
    fn a_status_callback_answers_a_deliberate_status_and_fails_otherwise() {
        let failure = sys::MLN_STATUS_NATIVE_ERROR;
        let answer = |body: Box<dyn FnOnce() -> Result<Result<()>>>| {
            invoke_status("probe_callback", failure, body)
        };
        assert_eq!(answer(Box::new(|| Ok(Ok(())))), sys::MLN_STATUS_OK);
        let refused = Error::new(Status::Busy, "refused");
        assert_eq!(answer(Box::new(|| Ok(Err(refused)))), sys::MLN_STATUS_BUSY);
        for error in [
            Error::new(Status::Ok, "no status"),
            Error::abi_version_mismatch(1, 2),
        ] {
            assert_eq!(answer(Box::new(|| Ok(Err(error)))), failure);
        }
        let unconverted = Error::invalid_argument("unconverted");
        assert_eq!(answer(Box::new(|| Err(unconverted))), failure);
        assert_eq!(
            answer(Box::new(|| std::panic::resume_unwind(Box::new("host bug")))),
            failure
        );
    }
}
