pub use crate::generated::ResourceProviderDecision;
#[cfg(test)]
use crate::generated::ResourceResponse;
use crate::{Error, ErrorKind, Result};
use maplibre_native_ffi_sys as sys;
use std::sync::{Arc, Mutex};

pub fn status_for_error(error: &Error) -> sys::mln_status {
    if let Some(status) = error.raw_status() {
        return status;
    }
    match error.kind() {
        ErrorKind::InvalidArgument => sys::MLN_STATUS_INVALID_ARGUMENT,
        ErrorKind::InvalidState => sys::MLN_STATUS_INVALID_STATE,
        ErrorKind::WrongThread => sys::MLN_STATUS_WRONG_THREAD,
        ErrorKind::Unsupported => sys::MLN_STATUS_UNSUPPORTED,
        ErrorKind::Cancelled => sys::MLN_STATUS_CANCELLED,
        ErrorKind::Busy => sys::MLN_STATUS_BUSY,
        ErrorKind::TargetLost => sys::MLN_STATUS_TARGET_LOST,
        ErrorKind::NotReady => sys::MLN_STATUS_NOT_READY,
        ErrorKind::NotFound => sys::MLN_STATUS_NOT_FOUND,
        ErrorKind::NativeError | ErrorKind::AbiVersionMismatch | ErrorKind::UnknownStatus => {
            sys::MLN_STATUS_NATIVE_ERROR
        }
    }
}

pub const UNKNOWN_PROVIDER_DECISION: u32 = u32::MAX;

pub type CompleteRequestFn = unsafe extern "C" fn(
    sys::mln_resource_request_handle,
    *const sys::mln_resource_response,
) -> sys::mln_status;
pub type ReleaseRequestFn = unsafe extern "C" fn(sys::mln_resource_request_handle);

#[derive(Clone, Copy, Debug)]
pub struct ResourceRequestHandleFns {
    #[cfg(test)]
    complete: CompleteRequestFn,
    release: ReleaseRequestFn,
}

impl ResourceRequestHandleFns {
    pub const NATIVE: Self = crate::generated::RESOURCE_REQUEST_HANDLE_FUNCTIONS;

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

    #[cfg(test)]
    pub fn complete(&self, response: &ResourceResponse) -> Result<()> {
        let mut arena = crate::input::InputArena::default();
        let native = response.to_native(&mut arena)?;
        self.complete_with(|handle| {
            // SAFETY: complete_with reserves this handle and native owns the input storage.
            crate::check(unsafe { (self.fns.complete)(handle, &native) })
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
                    "resource request is closed",
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
                "resource request is closed",
            ));
        }
        Ok(Self::native_handle(&inner))
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

    pub fn finish_provider_decision(&self, decision: ResourceProviderDecision) -> u32 {
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
        if inner.closed
            || inner.completed
            || inner.completing
            || matches!(decision, ResourceProviderDecision::Handle)
        {
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
            return self.finish_provider_decision(ResourceProviderDecision::Handle);
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
    use std::panic::{AssertUnwindSafe, catch_unwind};
    use std::sync::Mutex as StdMutex;
    use std::sync::atomic::{AtomicBool, AtomicI32, AtomicUsize, Ordering};
    use std::time::{Duration, Instant};

    use super::*;
    fn ok_response(bytes: impl Into<Vec<u8>>) -> ResourceResponse {
        ResourceResponse {
            status: crate::ResourceResponseStatus::Ok,
            bytes: bytes.into(),
            ..Default::default()
        }
    }

    #[test]
    fn last_request_reference_releases_off_the_callback_stack() {
        static RELEASED: StdMutex<Option<std::sync::mpsc::Sender<std::thread::ThreadId>>> =
            StdMutex::new(None);
        unsafe extern "C" fn record_release(_handle: sys::mln_resource_request_handle) {
            RELEASED
                .lock()
                .unwrap()
                .take()
                .unwrap()
                .send(std::thread::current().id())
                .unwrap();
        }
        let (sender, receiver) = std::sync::mpsc::channel();
        *RELEASED.lock().unwrap() = Some(sender);
        let mut fns = fake_fns();
        fns.release = record_release;
        // SAFETY: This handle reaches only the local fake function table.
        let state =
            unsafe { ResourceRequestHandleState::new(sys::mln_resource_request_handle(7), fns) }
                .unwrap();
        state.finish_provider_decision(ResourceProviderDecision::Handle);
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        drop(state);
        let releasing_thread = receiver
            .recv_timeout(Duration::from_secs(5))
            .expect("native release must progress off the callback stack");
        assert_ne!(releasing_thread, std::thread::current().id());
    }

    static HANDLE_TEST_LOCK: StdMutex<()> = StdMutex::new(());
    static COMPLETE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static RELEASE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static COMPLETE_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);
    /// When set, the fake release waits for this flag like the C API waits for
    /// a cancel callback running on another thread.
    static RELEASE_WAITS_FOR_CALLBACK: AtomicBool = AtomicBool::new(false);
    static RELEASE_STARTED: AtomicBool = AtomicBool::new(false);
    static CALLBACK_FINISHED: AtomicBool = AtomicBool::new(false);

    unsafe extern "C" fn fake_complete(
        _handle: sys::mln_resource_request_handle,
        _response: *const sys::mln_resource_response,
    ) -> sys::mln_status {
        COMPLETE_COUNT.fetch_add(1, Ordering::SeqCst);
        COMPLETE_STATUS.load(Ordering::SeqCst)
    }

    unsafe extern "C" fn fake_release(_handle: sys::mln_resource_request_handle) {
        RELEASE_STARTED.store(true, Ordering::SeqCst);
        if RELEASE_WAITS_FOR_CALLBACK.load(Ordering::SeqCst) {
            let deadline = Instant::now() + Duration::from_secs(5);
            while !CALLBACK_FINISHED.load(Ordering::SeqCst) && Instant::now() < deadline {
                std::thread::yield_now();
            }
        }
        RELEASE_COUNT.fetch_add(1, Ordering::SeqCst);
    }

    fn fake_fns() -> ResourceRequestHandleFns {
        // SAFETY: These fake functions implement the native handle contract for tests.
        unsafe { ResourceRequestHandleFns::new(fake_complete, fake_release) }
    }

    fn fake_state() -> Arc<ResourceRequestHandleState> {
        COMPLETE_COUNT.store(0, Ordering::SeqCst);
        RELEASE_COUNT.store(0, Ordering::SeqCst);
        COMPLETE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        RELEASE_WAITS_FOR_CALLBACK.store(false, Ordering::SeqCst);
        RELEASE_STARTED.store(false, Ordering::SeqCst);
        CALLBACK_FINISHED.store(false, Ordering::SeqCst);
        // SAFETY: This synthetic handle reaches only the fake functions above,
        // never the C API.
        unsafe {
            ResourceRequestHandleState::new(
                sys::mln_resource_request_handle(0x0c00_0000_0000_0034),
                fake_fns(),
            )
        }
        .unwrap()
    }

    fn handled_fake_state() -> Arc<ResourceRequestHandleState> {
        let state = fake_state();
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        state
    }

    fn wait_until(flag: &AtomicBool, what: &str) {
        let deadline = Instant::now() + Duration::from_secs(5);
        while !flag.load(Ordering::SeqCst) {
            assert!(Instant::now() < deadline, "timed out waiting for {what}");
            std::thread::yield_now();
        }
    }

    #[test]
    fn resource_request_handle_preserves_all_64_bits() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();
        let inner = state.lock_inner().unwrap();
        assert_eq!(
            ResourceRequestHandleState::native_handle(&inner).0,
            0x0c00_0000_0000_0034
        );
    }

    #[test]
    fn provider_decision_finalization_is_idempotent_for_owned_handles() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();

        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::PassThrough),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        drop(state);

        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn explicit_close_claims_provider_decision_and_releases_once() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        for exception in [false, true] {
            let state = fake_state();
            state.close();
            state.close();
            assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
            let decision = if exception {
                state.finish_provider_exception()
            } else {
                state.finish_provider_decision(ResourceProviderDecision::PassThrough)
            };
            assert_eq!(decision, sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE);
            assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
            drop(state);
            assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        }
    }

    #[test]
    // The C API releases a passed-through request itself.
    fn pass_through_leaves_the_release_to_native() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();

        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::PassThrough),
            sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        );
        assert_eq!(
            state.native_for_call().unwrap_err().kind(),
            ErrorKind::InvalidState
        );
        drop(state);

        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
    }

    #[test]
    fn request_handle_rejects_double_successful_completion() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();

        state.complete(&ok_response([1, 2, 3])).unwrap();
        let error = state.complete(&ok_response([4, 5, 6])).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn request_completion_rejection_is_retryable_and_success_retains_owner() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        COMPLETE_STATUS.store(sys::MLN_STATUS_INVALID_ARGUMENT, Ordering::SeqCst);
        assert_eq!(
            state.complete(&ok_response([1])).unwrap_err().kind(),
            ErrorKind::InvalidArgument
        );
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
        COMPLETE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        state.complete(&ok_response([2])).unwrap();
        assert!(state.native_for_call().is_ok());
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 2);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
        state.close();
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn completion_reservation_allows_reentry_and_restores_after_panic() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
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
                assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
                Ok(())
            })
            .unwrap();
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        state.close();
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    // Native release waits for a cancel callback running on another thread, so
    // close must not hold the handle lock while that callback uses the handle.
    fn close_holds_no_lock_while_native_release_waits_for_the_callback() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        RELEASE_WAITS_FOR_CALLBACK.store(true, Ordering::SeqCst);
        let callback_state = Arc::clone(&state);
        let callback_thread = std::thread::spawn(move || {
            wait_until(&RELEASE_STARTED, "native release to start");
            let closed = callback_state.native_for_call().is_err();
            CALLBACK_FINISHED.store(true, Ordering::SeqCst);
            closed
        });

        state.close();

        assert!(CALLBACK_FINISHED.load(Ordering::SeqCst));
        assert!(callback_thread.join().unwrap());
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }
}
