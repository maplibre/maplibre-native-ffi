//! The state machine of a handle that a callback decision protocol issues.
//!
//! A decision callback receives a provisional handle and answers whether it
//! keeps it. A kept handle is the host's to complete and release; a handle
//! passed through stays native's. The generated binding supplies each
//! protocol's decision values and native functions through [`DecisionHandleFns`].

use crate::{Error, ErrorKind, Result};
use maplibre_native_ffi_sys::{self as sys, NativeHandle};
use std::ffi::c_void;
use std::sync::{Arc, Mutex};

/// The decision reported when the host gave none, which native treats as a
/// failed callback.
const UNKNOWN_DECISION: u32 = u32::MAX;

/// A notification that native calls with its registration context.
pub type ContextCallback = Option<unsafe extern "C" fn(*mut c_void)>;

/// Registers a one-shot cancellation notification on a decision handle and
/// reports whether the handle was already cancelled.
pub type CancelRegistrationFn<H> = unsafe extern "C" fn(
    H,
    ContextCallback,
    *mut c_void,
    ContextCallback,
    *mut bool,
    *mut sys::mln_diagnostic,
) -> sys::mln_status;

/// One decision protocol's values and native functions.
pub struct DecisionHandleFns<H: 'static> {
    owner: &'static str,
    accept: u32,
    pass_through: u32,
    release: unsafe extern "C" fn(H),
    cancel_registration: CancelRegistrationFn<H>,
    reentry: &'static [&'static str],
}

impl<H> std::fmt::Debug for DecisionHandleFns<H> {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        formatter
            .debug_struct("DecisionHandleFns")
            .field("owner", &self.owner)
            .field("accept", &self.accept)
            .field("pass_through", &self.pass_through)
            .finish_non_exhaustive()
    }
}

impl<H> Clone for DecisionHandleFns<H> {
    fn clone(&self) -> Self {
        *self
    }
}

impl<H> Copy for DecisionHandleFns<H> {}

impl<H> DecisionHandleFns<H> {
    /// Creates the function table of one decision protocol.
    ///
    /// `owner` names the public handle type in errors. `accept` keeps a
    /// handle and `pass_through` returns it to native. `reentry` lists the
    /// operations a cancellation notification may call on its handle.
    ///
    /// # Safety
    ///
    /// The functions must implement the C API's decision handle contract:
    /// `release` releases a kept handle exactly once and waits for a
    /// cancellation notification running on another thread, and
    /// `cancel_registration` takes ownership of an accepted registration and
    /// releases it once it can no longer run.
    pub const unsafe fn new(
        owner: &'static str,
        accept: u32,
        pass_through: u32,
        release: unsafe extern "C" fn(H),
        cancel_registration: CancelRegistrationFn<H>,
        reentry: &'static [&'static str],
    ) -> Self {
        Self {
            owner,
            accept,
            pass_through,
            release,
            cancel_registration,
            reentry,
        }
    }
}

#[derive(Debug)]
struct DecisionHandleInner {
    handle: u64,
    decision_finalized: bool,
    provider_owned: bool,
    release_accounted_for: bool,
    closed: bool,
    completed: bool,
    completing: bool,
}

/// Shared state behind a decision handle.
///
/// The `inner` lock is never held while host code runs or while native release
/// runs: native release waits for a cancel callback running on another thread,
/// and that callback may call back into this same state.
#[derive(Debug)]
pub struct DecisionHandleState<H: NativeHandle> {
    inner: Mutex<DecisionHandleInner>,
    fns: DecisionHandleFns<H>,
}

impl<H: NativeHandle> DecisionHandleState<H> {
    /// Takes ownership of a native decision handle state machine.
    ///
    /// # Safety
    ///
    /// `handle` must be a live native decision handle borrowed from a
    /// decision callback, and `fns` must match that handle.
    pub unsafe fn new(handle: H, fns: DecisionHandleFns<H>) -> Result<Arc<Self>> {
        if handle.to_raw() == 0 {
            return Err(Error::invalid_argument(format!(
                "{} must not be the null handle",
                fns.owner
            )));
        }
        Ok(Arc::new(Self {
            inner: Mutex::new(DecisionHandleInner {
                handle: handle.to_raw(),
                decision_finalized: false,
                provider_owned: false,
                release_accounted_for: false,
                closed: false,
                completed: false,
                completing: false,
            }),
            fns,
        }))
    }

    fn native_handle(inner: &DecisionHandleInner) -> H {
        H::from_raw(inner.handle)
    }

    fn closed_error(&self) -> Error {
        Error::new(
            ErrorKind::InvalidState,
            None,
            format!("{} is closed", self.fns.owner),
        )
    }

    /// Reserves one completion attempt without holding a lock across native code.
    /// An accepted completion leaves the handle's owner live.
    pub fn complete_with(&self, complete: impl FnOnce(H) -> Result<()>) -> Result<()> {
        let handle = {
            let mut inner = self.lock_inner()?;
            if inner.completed || inner.completing {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    format!(
                        "{} completion is already accepted or in progress",
                        self.fns.owner
                    ),
                ));
            }
            if inner.closed {
                return Err(self.closed_error());
            }
            inner.completing = true;
            Self::native_handle(&inner)
        };
        let mut reservation = CompletionReservation {
            state: self,
            accepted: false,
        };
        let result = complete(handle);
        reservation.accepted = result.is_ok();
        drop(reservation);
        result
    }

    fn finish_completion(&self, accepted: bool) {
        let mut inner = self
            .inner
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        inner.completing = false;
        inner.completed |= accepted;
        let handle = Self::native_handle(&inner);
        let release = inner.closed
            && inner.decision_finalized
            && inner.provider_owned
            && Self::take_release_locked(&mut inner);
        drop(inner);
        self.release_now(release, handle);
    }

    /// Returns the issued generation ID, including after this owner closes.
    pub fn issued_handle(&self) -> H {
        let inner = self
            .inner
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        Self::native_handle(&inner)
    }

    /// Copies the live generational handle before a non-consuming native call.
    pub fn native_for_call(&self) -> Result<H> {
        let inner = self.lock_inner()?;
        if inner.closed {
            return Err(self.closed_error());
        }
        Ok(Self::native_handle(&inner))
    }

    /// Registers a callback that runs at most once when MapLibre cancels the
    /// handle, returning whether the handle was already cancelled.
    ///
    /// An accepted registration transfers the callback to the C API, which
    /// releases it once it can no longer run. A rejected registration or an
    /// already cancelled handle drops the callback unrun before returning.
    pub fn register_cancel(&self, callback: Box<dyn FnOnce() + Send + 'static>) -> Result<bool> {
        type Registration = (
            u64,
            &'static [&'static str],
            Option<Box<dyn FnOnce() + Send + 'static>>,
        );
        unsafe extern "C" fn invoke(user_data: *mut c_void) {
            // SAFETY: native passes the registration it owns and invokes it at
            // most once, before its release.
            let (owner, reentry, callback) = unsafe { &mut *user_data.cast::<Registration>() };
            let _policy = crate::callback::PolicyScope::enter(reentry, *owner);
            if let Some(callback) = callback.take() {
                let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(callback));
            }
        }
        unsafe extern "C" fn release(user_data: *mut c_void) {
            let _policy = crate::callback::PolicyScope::enter(&[], 0);
            // SAFETY: native or the rejected registration below releases each
            // registration once.
            let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
                drop(Box::from_raw(user_data.cast::<Registration>()))
            }));
        }
        let handle = self.native_for_call()?;
        let registration: Box<Registration> =
            Box::new((handle.to_raw(), self.fns.reentry, Some(callback)));
        let user_data = Box::into_raw(registration).cast();
        let mut cancelled = false;
        let register = self.fns.cancel_registration;
        let registered = crate::check(|diagnostic| unsafe {
            register(
                handle,
                Some(invoke),
                user_data,
                Some(release),
                &mut cancelled,
                diagnostic,
            )
        });
        if registered.is_err() || cancelled {
            // SAFETY: native took no ownership of a rejected or already
            // cancelled registration.
            unsafe { release(user_data) };
        }
        registered.map(|()| cancelled)
    }

    pub fn close(&self) {
        let Ok(mut inner) = self.inner.lock() else {
            return;
        };
        if inner.closed {
            return;
        }
        inner.closed = true;
        let handle = Self::native_handle(&inner);
        let release = !inner.completing
            && inner.decision_finalized
            && inner.provider_owned
            && Self::take_release_locked(&mut inner);
        drop(inner);
        self.release_now(release, handle);
    }

    /// Records the host's decision: `handled` keeps the handle for the host
    /// to complete, and otherwise the C API serves it. Returns the decision to
    /// report to the C API.
    pub fn finish_decision(&self, handled: bool) -> u32 {
        let Ok(mut inner) = self.inner.lock() else {
            return UNKNOWN_DECISION;
        };
        if inner.decision_finalized {
            return if inner.provider_owned {
                self.fns.accept
            } else {
                self.fns.pass_through
            };
        }
        if inner.closed || inner.completed || inner.completing || handled {
            inner.decision_finalized = true;
            inner.provider_owned = true;
            let handle = Self::native_handle(&inner);
            let release =
                inner.closed && !inner.completing && Self::take_release_locked(&mut inner);
            drop(inner);
            self.release_now(release, handle);
            self.fns.accept
        } else {
            // The C API releases a passed-through handle itself, and its
            // release retires any cancel registration.
            Self::finish_unowned_locked(&mut inner);
            self.fns.pass_through
        }
    }

    /// Records a decision callback that failed before it answered.
    pub fn finish_exception(&self) -> u32 {
        let completed = self
            .inner
            .lock()
            .map(|inner| inner.closed || inner.completed || inner.completing)
            .unwrap_or(false);
        if completed {
            return self.finish_decision(true);
        }
        if let Ok(mut inner) = self.inner.lock() {
            // The C API releases the handle it gets no decision for.
            Self::finish_unowned_locked(&mut inner);
        }
        UNKNOWN_DECISION
    }

    /// Records a decision that leaves the release to the C API.
    fn finish_unowned_locked(inner: &mut DecisionHandleInner) {
        inner.decision_finalized = true;
        inner.release_accounted_for = true;
        inner.closed = true;
    }

    fn take_release_locked(inner: &mut DecisionHandleInner) -> bool {
        if inner.release_accounted_for {
            return false;
        }
        inner.release_accounted_for = true;
        true
    }

    /// Calls native release with no lock held. Release waits for a cancel
    /// callback running on another thread, and that callback may take the lock.
    fn release_now(&self, release: bool, handle: H) {
        if !release {
            return;
        }
        // SAFETY: take_release_locked grants this call exactly once per handle.
        unsafe { (self.fns.release)(handle) };
    }

    fn lock_inner(&self) -> Result<std::sync::MutexGuard<'_, DecisionHandleInner>> {
        self.inner.lock().map_err(|_| {
            Error::new(
                ErrorKind::NativeError,
                None,
                format!("{} lock poisoned", self.fns.owner),
            )
        })
    }
}

struct CompletionReservation<'a, H: NativeHandle> {
    state: &'a DecisionHandleState<H>,
    accepted: bool,
}

impl<H: NativeHandle> Drop for CompletionReservation<'_, H> {
    fn drop(&mut self) {
        self.state.finish_completion(self.accepted);
    }
}

impl<H: NativeHandle> Drop for DecisionHandleState<H> {
    fn drop(&mut self) {
        let inner = self
            .inner
            .get_mut()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if !inner.provider_owned || !Self::take_release_locked(inner) {
            return;
        }
        let raw = inner.handle;
        let release = self.fns.release;
        // A drop inside a callback defers the release off that callback's
        // stack, because release waits for a cancel callback on another thread.
        // SAFETY: take_release_locked grants this call exactly once.
        crate::callback::finalize(move || unsafe { release(H::from_raw(raw)) });
    }
}

#[cfg(test)]
mod tests {
    use std::collections::HashMap;
    use std::panic::{AssertUnwindSafe, catch_unwind};
    use std::sync::atomic::{AtomicI32, AtomicU64, AtomicUsize, Ordering};
    use std::sync::mpsc::{Receiver, Sender, channel};
    use std::sync::{Mutex as StdMutex, OnceLock};
    use std::thread::ThreadId;
    use std::time::Duration;

    use super::*;

    /// The longest a test waits for another thread to reach a point it signals.
    const WAIT: Duration = Duration::from_secs(10);

    /// One test's fake native request. The fake functions find it through the
    /// handle id they receive, so tests that run in parallel share no state.
    struct FakeRequest {
        completes: AtomicUsize,
        releases: AtomicUsize,
        complete_status: AtomicI32,
        /// Receives the thread that runs native release.
        release_thread: StdMutex<Option<Sender<ThreadId>>>,
        /// When set, release waits for this signal the way the C API waits
        /// for a cancel callback running on another thread.
        release_waits_for: StdMutex<Option<Receiver<()>>>,
    }

    fn fakes() -> &'static StdMutex<HashMap<u64, Arc<FakeRequest>>> {
        static FAKES: OnceLock<StdMutex<HashMap<u64, Arc<FakeRequest>>>> = OnceLock::new();
        FAKES.get_or_init(Default::default)
    }

    /// The fixture's protocol values, as a provider decision declares them.
    const ACCEPT: u32 = 1;
    const PASS_THROUGH: u32 = 0;

    fn fake_for(handle: sys::mln_resource_request_handle) -> Arc<FakeRequest> {
        Arc::clone(&fakes().lock().unwrap()[&handle.0])
    }

    unsafe extern "C" fn fake_complete(
        handle: sys::mln_resource_request_handle,
        _response: *const sys::mln_resource_response,
        _diagnostic: *mut sys::mln_diagnostic,
    ) -> sys::mln_status {
        let fake = fake_for(handle);
        fake.completes.fetch_add(1, Ordering::SeqCst);
        fake.complete_status.load(Ordering::SeqCst)
    }

    /// Completes the request with an empty response, as a generated owner does.
    fn complete(state: &DecisionHandleState<sys::mln_resource_request_handle>) -> Result<()> {
        state.complete_with(|handle| {
            // SAFETY: the fake reads nothing through the response.
            crate::check(|diagnostic| unsafe {
                fake_complete(handle, std::ptr::null(), diagnostic)
            })
        })
    }

    unsafe extern "C" fn fake_cancel_registration(
        _handle: sys::mln_resource_request_handle,
        _callback: ContextCallback,
        _user_data: *mut c_void,
        _release: ContextCallback,
        _cancelled: *mut bool,
        _diagnostic: *mut sys::mln_diagnostic,
    ) -> sys::mln_status {
        sys::MLN_STATUS_UNSUPPORTED
    }

    unsafe extern "C" fn fake_release(handle: sys::mln_resource_request_handle) {
        let fake = fake_for(handle);
        if let Some(sender) = fake.release_thread.lock().unwrap().take() {
            sender.send(std::thread::current().id()).unwrap();
        }
        if let Some(receiver) = fake.release_waits_for.lock().unwrap().take() {
            receiver
                .recv_timeout(WAIT)
                .expect("the cancel callback never finished");
        }
        fake.releases.fetch_add(1, Ordering::SeqCst);
    }

    /// A request state machine over a fake native request of its own.
    struct Fixture {
        fake: Arc<FakeRequest>,
        state: Arc<DecisionHandleState<sys::mln_resource_request_handle>>,
    }

    impl Fixture {
        fn new() -> Self {
            // The ids keep their high bits set, as the C API's generational
            // handles do, so a conversion that truncated one would miss its fake.
            static NEXT_ID: AtomicU64 = AtomicU64::new(0x0c00_0000_0000_0001);
            let id = NEXT_ID.fetch_add(1, Ordering::Relaxed);
            let fake = Arc::new(FakeRequest {
                completes: AtomicUsize::new(0),
                releases: AtomicUsize::new(0),
                complete_status: AtomicI32::new(sys::MLN_STATUS_OK),
                release_thread: StdMutex::new(None),
                release_waits_for: StdMutex::new(None),
            });
            fakes().lock().unwrap().insert(id, Arc::clone(&fake));
            // SAFETY: These fake functions implement the native handle
            // contract, and this id reaches only them, never the C API.
            let state = unsafe {
                DecisionHandleState::new(
                    sys::mln_resource_request_handle(id),
                    DecisionHandleFns::new(
                        "ResourceRequestHandle",
                        ACCEPT,
                        PASS_THROUGH,
                        fake_release,
                        fake_cancel_registration,
                        &[],
                    ),
                )
            }
            .unwrap();
            Self { fake, state }
        }

        /// A request the provider kept by answering Handle.
        fn handled() -> Self {
            let fixture = Self::new();
            assert_eq!(fixture.state.finish_decision(true), ACCEPT);
            fixture
        }

        fn completes(&self) -> usize {
            self.fake.completes.load(Ordering::SeqCst)
        }

        fn releases(&self) -> usize {
            self.fake.releases.load(Ordering::SeqCst)
        }
    }

    #[test]
    fn a_request_keeps_all_64_bits_of_its_native_handle() {
        let fixture = Fixture::new();
        let native = DecisionHandleState::<sys::mln_resource_request_handle>::native_handle(
            &fixture.state.lock_inner().unwrap(),
        );
        // The fixture's id sets the high bits, and only the whole id finds
        // this fixture's fake.
        assert_ne!(native.0 >> 32, 0);
        assert!(Arc::ptr_eq(&fake_for(native), &fixture.fake));
    }

    #[test]
    fn last_request_reference_releases_off_the_callback_stack() {
        let Fixture { fake, state } = Fixture::handled();
        let (sender, receiver) = channel();
        *fake.release_thread.lock().unwrap() = Some(sender);
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        drop(state);
        let releasing_thread = receiver
            .recv_timeout(WAIT)
            .expect("native release must progress off the callback stack");
        assert_ne!(releasing_thread, std::thread::current().id());
    }

    #[test]
    fn provider_decision_finalization_is_idempotent_for_owned_handles() {
        let Fixture { fake, state } = Fixture::handled();
        assert_eq!(state.finish_decision(false), ACCEPT);
        drop(state);

        assert_eq!(fake.releases.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn explicit_close_claims_provider_decision_and_releases_once() {
        for exception in [false, true] {
            let Fixture { fake, state } = Fixture::new();
            state.close();
            state.close();
            assert_eq!(fake.releases.load(Ordering::SeqCst), 0);
            let decision = if exception {
                state.finish_exception()
            } else {
                state.finish_decision(false)
            };
            assert_eq!(decision, ACCEPT);
            assert_eq!(fake.releases.load(Ordering::SeqCst), 1);
            drop(state);
            assert_eq!(fake.releases.load(Ordering::SeqCst), 1);
        }
    }

    #[test]
    // The C API releases a passed-through request itself.
    fn pass_through_leaves_the_release_to_native() {
        let Fixture { fake, state } = Fixture::new();

        assert_eq!(state.finish_decision(false), PASS_THROUGH);
        assert_eq!(
            state.native_for_call().unwrap_err().kind(),
            ErrorKind::InvalidState
        );
        drop(state);

        assert_eq!(fake.releases.load(Ordering::SeqCst), 0);
    }

    #[test]
    fn request_handle_rejects_double_successful_completion() {
        let fixture = Fixture::new();

        complete(&fixture.state).unwrap();
        let error = complete(&fixture.state).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(fixture.completes(), 1);
    }

    #[test]
    fn request_completion_rejection_is_retryable_and_success_retains_owner() {
        let fixture = Fixture::handled();
        let status = &fixture.fake.complete_status;
        status.store(sys::MLN_STATUS_INVALID_ARGUMENT, Ordering::SeqCst);
        assert_eq!(
            complete(&fixture.state).unwrap_err().kind(),
            ErrorKind::InvalidArgument
        );
        assert_eq!(fixture.releases(), 0);
        status.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        complete(&fixture.state).unwrap();
        assert!(fixture.state.native_for_call().is_ok());
        assert_eq!(fixture.completes(), 2);
        assert_eq!(fixture.releases(), 0);
        fixture.state.close();
        assert_eq!(fixture.releases(), 1);
    }

    #[test]
    fn completion_reservation_allows_reentry_and_restores_after_panic() {
        let fixture = Fixture::handled();
        let state = &fixture.state;
        let result = catch_unwind(AssertUnwindSafe(|| {
            state.complete_with(|_| {
                assert!(state.native_for_call().is_ok());
                assert_eq!(
                    state.complete_with(|_| Ok(())).unwrap_err().kind(),
                    ErrorKind::InvalidState
                );
                panic!("conversion failure");
            })
        }));
        assert!(result.is_err());
        state
            .complete_with(|_| {
                state.close();
                assert_eq!(fixture.releases(), 0);
                Ok(())
            })
            .unwrap();
        assert_eq!(fixture.releases(), 1);
        state.close();
        assert_eq!(fixture.releases(), 1);
    }

    #[test]
    // Native release waits for a cancel callback running on another thread, so
    // close must not hold the handle lock while that callback uses the handle.
    fn close_holds_no_lock_while_native_release_waits_for_the_callback() {
        let fixture = Fixture::handled();
        let (started_sender, started) = channel();
        let (finished, finished_receiver) = channel();
        *fixture.fake.release_thread.lock().unwrap() = Some(started_sender);
        *fixture.fake.release_waits_for.lock().unwrap() = Some(finished_receiver);
        let callback_state = Arc::clone(&fixture.state);
        let callback_thread = std::thread::spawn(move || {
            started
                .recv_timeout(WAIT)
                .expect("native release never started");
            let closed = callback_state.native_for_call().is_err();
            finished.send(()).unwrap();
            closed
        });

        fixture.state.close();

        assert!(callback_thread.join().unwrap());
        assert_eq!(fixture.releases(), 1);
    }
}
