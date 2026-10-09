use crate::{Error, ErrorKind, Result};
use maplibre_native_ffi_sys as sys;
use std::sync::{Arc, Mutex};

pub const UNKNOWN_PROVIDER_DECISION: u32 = u32::MAX;

/// The request operations that a provider or cancel callback may call on the
/// request it was given.
pub const REQUEST_OPERATIONS: &[&str] = &[
    "mln_resource_request_complete",
    "mln_resource_request_cancelled",
    "mln_resource_request_set_cancel_callback",
    "mln_resource_request_release",
];

pub type CompleteRequestFn = unsafe extern "C" fn(
    sys::mln_resource_request_handle,
    *const sys::mln_resource_response,
    *mut sys::mln_diagnostic,
) -> sys::mln_status;
pub type ReleaseRequestFn = unsafe extern "C" fn(sys::mln_resource_request_handle);

#[derive(Clone, Copy, Debug)]
pub struct ResourceRequestHandleFns {
    #[cfg(test)]
    complete: CompleteRequestFn,
    release: ReleaseRequestFn,
}

impl ResourceRequestHandleFns {
    /// Creates a function table for a native resource request handle.
    ///
    /// # Safety
    ///
    /// The functions must implement the same ownership contract as the C API:
    /// `complete` operates on the matching handle type, and `release` releases
    /// a provider-owned handle exactly once. Release waits for cancellation on
    /// another thread; self-release can return before the current cancellation
    /// callback finishes.
    pub const unsafe fn new(_complete: CompleteRequestFn, release: ReleaseRequestFn) -> Self {
        Self {
            #[cfg(test)]
            complete: _complete,
            release,
        }
    }
}

#[derive(Debug)]
struct ResourceRequestHandleInner {
    handle: u64,
    decision_finalized: bool,
    provider_owned: bool,
    release_accounted_for: bool,
    closed: bool,
    completed: bool,
    completing: bool,
}

/// Shared state behind a resource request handle.
///
/// The `inner` lock is never held while host code runs or while native release
/// runs: native release waits for a cancel callback running on another thread,
/// and that callback may call back into this same state.
#[derive(Debug)]
pub struct ResourceRequestHandleState {
    inner: Mutex<ResourceRequestHandleInner>,
    fns: ResourceRequestHandleFns,
}

impl ResourceRequestHandleState {
    /// Takes ownership of a native resource request handle state machine.
    ///
    /// # Safety
    ///
    /// `handle` must be a live native resource request handle borrowed from a
    /// provider callback, and `fns` must match that handle.
    pub unsafe fn new(
        handle: sys::mln_resource_request_handle,
        fns: ResourceRequestHandleFns,
    ) -> Result<Arc<Self>> {
        if handle.0 == 0 {
            return Err(Error::invalid_argument(
                "resource request handle must not be the null handle",
            ));
        }
        Ok(Arc::new(Self {
            inner: Mutex::new(ResourceRequestHandleInner {
                handle: handle.0,
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

    fn native_handle(inner: &ResourceRequestHandleInner) -> sys::mln_resource_request_handle {
        sys::mln_resource_request_handle(inner.handle)
    }

    /// Completes the request with an empty response.
    #[cfg(test)]
    fn complete(&self) -> Result<()> {
        // SAFETY: the response is plain data, and all zeroes is a valid value.
        let native: sys::mln_resource_response = unsafe { std::mem::zeroed() };
        self.complete_with(|handle| {
            // SAFETY: complete_with reserves this handle and native owns the input storage.
            crate::check(|diagnostic| unsafe { (self.fns.complete)(handle, &native, diagnostic) })
        })
    }

    /// Reserves one completion attempt without holding a lock across native code.
    /// An accepted response completes the request but leaves its owner live.
    pub fn complete_with(
        &self,
        complete: impl FnOnce(sys::mln_resource_request_handle) -> Result<()>,
    ) -> Result<()> {
        let handle = {
            let mut inner = self.lock_inner()?;
            if inner.completed || inner.completing {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "resource request completion is already accepted or in progress",
                ));
            }
            if inner.closed {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "ResourceRequestHandle is closed",
                ));
            }
            inner.completing = true;
            Self::native_handle(&inner)
        };
        let mut reservation = RequestCompletionReservation {
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
    pub fn issued_handle(&self) -> sys::mln_resource_request_handle {
        let inner = self
            .inner
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        Self::native_handle(&inner)
    }

    /// Copies the live generational handle before a non-consuming native call.
    pub fn native_for_call(&self) -> Result<sys::mln_resource_request_handle> {
        let inner = self.lock_inner()?;
        if inner.closed {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "ResourceRequestHandle is closed",
            ));
        }
        Ok(Self::native_handle(&inner))
    }

    /// Registers a callback that runs at most once when MapLibre cancels the
    /// request, returning whether the request was already cancelled.
    ///
    /// An accepted registration transfers the callback to the C API, which
    /// releases it once it can no longer run. A rejected registration or an
    /// already cancelled request drops the callback unrun before returning.
    pub fn set_cancel_callback(
        &self,
        callback: Box<dyn FnOnce() + Send + 'static>,
    ) -> Result<bool> {
        type Registration = (u64, Option<Box<dyn FnOnce() + Send + 'static>>);
        unsafe extern "C" fn invoke(user_data: *mut std::ffi::c_void) {
            // SAFETY: native passes the registration it owns and invokes it at
            // most once, before its release.
            let (owner, callback) = unsafe { &mut *user_data.cast::<Registration>() };
            let _policy = crate::callback::PolicyScope::enter(REQUEST_OPERATIONS, *owner);
            if let Some(callback) = callback.take() {
                let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(callback));
            }
        }
        unsafe extern "C" fn release(user_data: *mut std::ffi::c_void) {
            let _policy = crate::callback::PolicyScope::enter(&[], 0);
            // SAFETY: native or the rejected registration below releases each
            // registration once.
            let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
                drop(Box::from_raw(user_data.cast::<Registration>()))
            }));
        }
        let handle = self.native_for_call()?;
        let registration: Box<Registration> = Box::new((handle.0, Some(callback)));
        let user_data = Box::into_raw(registration).cast();
        let mut cancelled = false;
        let registered = crate::check(|diagnostic| unsafe {
            sys::mln_resource_request_set_cancel_callback(
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

    /// Records the provider's decision: `handled` keeps the request for the
    /// provider to complete, and otherwise the C API serves it. Returns the
    /// decision to report to the C API.
    pub fn finish_provider_decision(&self, handled: bool) -> u32 {
        let Ok(mut inner) = self.inner.lock() else {
            return UNKNOWN_PROVIDER_DECISION;
        };
        if inner.decision_finalized {
            return if inner.provider_owned {
                sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
            } else {
                sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
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
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        } else {
            // The C API releases a passed-through request itself, and its
            // release retires any cancel registration.
            Self::finish_unowned_locked(&mut inner);
            sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        }
    }

    pub fn finish_provider_exception(&self) -> u32 {
        let completed = self
            .inner
            .lock()
            .map(|inner| inner.closed || inner.completed || inner.completing)
            .unwrap_or(false);
        if completed {
            return self.finish_provider_decision(true);
        }
        if let Ok(mut inner) = self.inner.lock() {
            // The C API releases the request it gets no decision for.
            Self::finish_unowned_locked(&mut inner);
        }
        UNKNOWN_PROVIDER_DECISION
    }

    /// Records a decision that leaves the release to the C API.
    fn finish_unowned_locked(inner: &mut ResourceRequestHandleInner) {
        inner.decision_finalized = true;
        inner.release_accounted_for = true;
        inner.closed = true;
    }

    fn take_release_locked(inner: &mut ResourceRequestHandleInner) -> bool {
        if inner.release_accounted_for {
            return false;
        }
        inner.release_accounted_for = true;
        true
    }

    /// Calls native release with no lock held. Release waits for a cancel
    /// callback running on another thread, and that callback may take the lock.
    fn release_now(&self, release: bool, handle: sys::mln_resource_request_handle) {
        if !release {
            return;
        }
        // SAFETY: take_release_locked grants this call exactly once per handle.
        unsafe { (self.fns.release)(handle) };
    }

    fn lock_inner(&self) -> Result<std::sync::MutexGuard<'_, ResourceRequestHandleInner>> {
        self.inner.lock().map_err(|_| {
            Error::new(
                ErrorKind::NativeError,
                None,
                "ResourceRequestHandle lock poisoned",
            )
        })
    }
}

struct RequestCompletionReservation<'a> {
    state: &'a ResourceRequestHandleState,
    accepted: bool,
}

impl Drop for RequestCompletionReservation<'_> {
    fn drop(&mut self) {
        self.state.finish_completion(self.accepted);
    }
}

impl Drop for ResourceRequestHandleState {
    fn drop(&mut self) {
        let inner = self
            .inner
            .get_mut()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if !inner.provider_owned || !Self::take_release_locked(inner) {
            return;
        }
        let handle = Self::native_handle(inner);
        let release = self.fns.release;
        // A drop inside a callback defers the release off that callback's
        // stack, because release waits for a cancel callback on another thread.
        // SAFETY: take_release_locked grants this call exactly once.
        crate::callback::finalize(move || unsafe { release(handle) });
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
        state: Arc<ResourceRequestHandleState>,
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
                ResourceRequestHandleState::new(
                    sys::mln_resource_request_handle(id),
                    ResourceRequestHandleFns::new(fake_complete, fake_release),
                )
            }
            .unwrap();
            Self { fake, state }
        }

        /// A request the provider kept by answering Handle.
        fn handled() -> Self {
            let fixture = Self::new();
            assert_eq!(
                fixture.state.finish_provider_decision(true),
                sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
            );
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
        let native =
            ResourceRequestHandleState::native_handle(&fixture.state.lock_inner().unwrap());
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
        assert_eq!(
            state.finish_provider_decision(false),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
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
                state.finish_provider_exception()
            } else {
                state.finish_provider_decision(false)
            };
            assert_eq!(decision, sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE);
            assert_eq!(fake.releases.load(Ordering::SeqCst), 1);
            drop(state);
            assert_eq!(fake.releases.load(Ordering::SeqCst), 1);
        }
    }

    #[test]
    // The C API releases a passed-through request itself.
    fn pass_through_leaves_the_release_to_native() {
        let Fixture { fake, state } = Fixture::new();

        assert_eq!(
            state.finish_provider_decision(false),
            sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        );
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

        fixture.state.complete().unwrap();
        let error = fixture.state.complete().unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(fixture.completes(), 1);
    }

    #[test]
    fn request_completion_rejection_is_retryable_and_success_retains_owner() {
        let fixture = Fixture::handled();
        let status = &fixture.fake.complete_status;
        status.store(sys::MLN_STATUS_INVALID_ARGUMENT, Ordering::SeqCst);
        assert_eq!(
            fixture.state.complete().unwrap_err().kind(),
            ErrorKind::InvalidArgument
        );
        assert_eq!(fixture.releases(), 0);
        status.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        fixture.state.complete().unwrap();
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
