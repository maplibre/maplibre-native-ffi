//! The host side of native callbacks: the registration state a trampoline
//! reads, callback stack scope, and failure containment.
//!
//! Each generated trampoline reads its registration with [`state`] and runs
//! host code through [`invoke`] or [`invoke_status`], which name the C
//! callback type that a contained error is reported against. Native releases a
//! registration through [`release`] once it can no longer call it.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::{Arc, Mutex};

use crate::convert::InputArena;
use crate::finalize::CallbackScope;
use crate::handle::lock;
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

/// A registration that native code supplies: its callbacks `C`, the context
/// they take, and the release that frees it. Copies share it, and one
/// submission takes it, so the context is released once: by native after it
/// accepts the registration, by the submission's arena when it doesn't, or
/// here when no submission takes it before the last copy drops.
#[derive(Clone)]
pub(crate) struct NativeRegistration<C>(Arc<Mutex<Option<Supplied<C>>>>);

struct Supplied<C> {
    callbacks: C,
    user_data: *mut c_void,
    release: Option<unsafe extern "C" fn(*mut c_void)>,
}

// SAFETY: the host that supplies a registration vouches that its callbacks and
// release take its context on any thread.
unsafe impl<C: Send> Send for Supplied<C> {}

impl<C> Drop for Supplied<C> {
    fn drop(&mut self) {
        if let Some(release) = self.release.take() {
            // SAFETY: no submission took the context, so it is released once.
            unsafe { release(self.user_data) };
        }
    }
}

impl<C: Copy> NativeRegistration<C> {
    /// # Safety
    ///
    /// The callbacks and the release must follow the registration's C contract
    /// with `user_data` on any thread, and `release` must free it exactly once
    /// without unwinding.
    pub(crate) unsafe fn new(
        callbacks: C,
        user_data: *mut c_void,
        release: Option<unsafe extern "C" fn(*mut c_void)>,
    ) -> Self {
        Self(Arc::new(Mutex::new(Some(Supplied {
            callbacks,
            user_data,
            release,
        }))))
    }

    /// Moves the registration into `arena`, which releases the context unless
    /// native accepts it. A registration that a submission already took fails
    /// with `Status::InvalidState`.
    pub(crate) fn take(
        &self,
        arena: &mut InputArena,
    ) -> Result<(C, *mut c_void, Option<unsafe extern "C" fn(*mut c_void)>)> {
        let mut supplied = lock(&self.0).take().ok_or_else(|| {
            Error::new(
                crate::Status::InvalidState,
                "a native registration was already submitted",
            )
        })?;
        let release = supplied.release.take();
        if let Some(release) = release {
            // SAFETY: the host's release frees the context once.
            unsafe { arena.track(supplied.user_data, release) };
        }
        Ok((supplied.callbacks, supplied.user_data, release))
    }
}

#[cfg(test)]
mod tests {
    use std::sync::atomic::{AtomicUsize, Ordering};

    use super::*;
    use crate::Status;
    use crate::convert::to_native;

    /// A native registration's context, which counts its callbacks and its
    /// releases.
    #[derive(Default)]
    struct Context {
        calls: AtomicUsize,
        releases: AtomicUsize,
    }

    impl Context {
        fn pointer(&self) -> *mut c_void {
            std::ptr::from_ref(self).cast_mut().cast()
        }
        fn calls(&self) -> usize {
            self.calls.load(Ordering::SeqCst)
        }
        fn releases(&self) -> usize {
            self.releases.load(Ordering::SeqCst)
        }
        /// A wake whose callback and release count on this context.
        fn wake(&self) -> crate::Wake {
            // SAFETY: the context outlives every call through the wake.
            unsafe { crate::Wake::native(call, self.pointer(), Some(release)) }
        }
    }

    unsafe extern "C" fn call(user_data: *mut c_void) {
        // SAFETY: each registration's context is a live `Context`.
        unsafe { &*user_data.cast::<Context>() }
            .calls
            .fetch_add(1, Ordering::SeqCst);
    }

    unsafe extern "C" fn release(user_data: *mut c_void) {
        // SAFETY: as above.
        unsafe { &*user_data.cast::<Context>() }
            .releases
            .fetch_add(1, Ordering::SeqCst);
    }

    #[test]
    fn a_native_wake_reaches_native_unchanged_and_native_releases_an_accepted_one() {
        let context = Context::default();
        let host = context.wake();
        let mut arena = InputArena::default();
        let raw: sys::mln_wake = to_native(&host, &mut arena).unwrap();
        assert_eq!(raw.user_data, context.pointer());
        arena.accept_registrations();
        drop((arena, host));
        assert_eq!(context.releases(), 0, "native owns an accepted context");
        // SAFETY: native calls the accepted wake, then releases it once.
        unsafe {
            raw.callback.unwrap()(raw.user_data);
            raw.release_user_data.unwrap()(raw.user_data);
        }
        assert_eq!((context.calls(), context.releases()), (1, 1));
    }

    #[test]
    fn a_native_queue_lock_reaches_native_unchanged_and_a_refused_one_is_released() {
        let context = Context::default();
        // SAFETY: the context outlives every call through the lock.
        let host =
            unsafe { crate::QueueLock::native(call, call, context.pointer(), Some(release)) };
        let mut arena = InputArena::default();
        let raw: sys::mln_queue_lock = to_native(&host, &mut arena).unwrap();
        assert_eq!(raw.user_data, context.pointer());
        // SAFETY: the context is live, and these are the host's callbacks.
        unsafe {
            raw.lock.unwrap()(raw.user_data);
            raw.unlock.unwrap()(raw.user_data);
        }
        assert_eq!(context.calls(), 2);
        // Native refused it, so the arena releases the context.
        drop(arena);
        assert_eq!(context.releases(), 1);
        drop(host);
        assert_eq!(context.releases(), 1);
    }

    #[test]
    fn a_native_wake_releases_its_context_once_when_refused_unused_or_reused() {
        // Refused: the submission's arena drops without native accepting it.
        let context = Context::default();
        let host = context.wake();
        let _: sys::mln_wake = to_native(&host, &mut InputArena::default()).unwrap();
        assert_eq!(context.releases(), 1);
        drop(host);
        assert_eq!(context.releases(), 1);

        // Unused: no submission takes it before its last copy drops.
        let context = Context::default();
        let host = context.wake();
        let copy = host.clone();
        drop(host);
        assert_eq!(context.releases(), 0);
        drop(copy);
        assert_eq!(context.releases(), 1);

        // Reused: a copy submitted after the first submission took the context
        // fails before native sees it.
        let context = Context::default();
        let host = context.wake();
        let copy = host.clone();
        let mut first = InputArena::default();
        let raw: sys::mln_wake = to_native(&host, &mut first).unwrap();
        first.accept_registrations();
        let error = to_native::<sys::mln_wake, _>(&copy, &mut InputArena::default()).unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
        drop((host, copy, first));
        assert_eq!(
            context.releases(),
            0,
            "native owns the first submission's context"
        );
        // SAFETY: native releases the accepted context once.
        unsafe { raw.release_user_data.unwrap()(raw.user_data) };
        assert_eq!(context.releases(), 1);

        // A call whose later input fails to convert drops its arena, which
        // releases the wake that converted before it.
        let context = Context::default();
        let host = context.wake();
        let mut arena = InputArena::default();
        let _: sys::mln_wake = to_native(&host, &mut arena).unwrap();
        let _ = arena.c_string("nul\0inside").unwrap_err();
        drop(arena);
        assert_eq!(context.releases(), 1);
    }

    #[test]
    fn a_native_wake_without_a_release_releases_nothing_and_takes_no_host_callback() {
        let context = Context::default();
        // SAFETY: the context outlives every call through the wake.
        let host = unsafe { crate::Wake::native(call, context.pointer(), None) };
        let raw: sys::mln_wake = to_native(&host, &mut InputArena::default()).unwrap();
        assert!(raw.release_user_data.is_none());
        drop(host);
        // SAFETY: as above.
        unsafe { raw.callback.unwrap()(raw.user_data) };
        assert_eq!((context.calls(), context.releases()), (1, 0));

        let context = Context::default();
        let mixed = context.wake().with_callback(|| {});
        let error = to_native::<sys::mln_wake, _>(&mixed, &mut InputArena::default()).unwrap_err();
        assert_eq!(error.status(), Status::InvalidArgument);
        drop(mixed);
        assert_eq!(context.releases(), 1);
    }

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
