pub use crate::generated::ResourceProviderDecision;
#[cfg(test)]
use crate::generated::ResourceResponse;
use crate::{Error, ErrorKind, Result};
use maplibre_native_ffi_sys as sys;
use std::ffi::c_void;
use std::fmt;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::{Arc, Mutex, Weak};

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
pub type CancelledRequestFn =
    unsafe extern "C" fn(sys::mln_resource_request_handle, *mut bool) -> sys::mln_status;
pub type ReleaseRequestFn = unsafe extern "C" fn(sys::mln_resource_request_handle);
pub type WaitRetiredRequestFn =
    unsafe extern "C" fn(sys::mln_resource_request_handle) -> sys::mln_status;
pub type SetCancelCallbackFn = unsafe extern "C" fn(
    sys::mln_resource_request_handle,
    sys::mln_resource_request_cancel_callback,
    *mut c_void,
    *mut bool,
) -> sys::mln_status;

/// Host callback that runs once when MapLibre cancels a handled request.
pub type CancelCallback = dyn FnOnce() + Send + 'static;

#[derive(Clone, Copy, Debug)]
pub struct ResourceRequestHandleFns {
    #[cfg(test)]
    complete: CompleteRequestFn,
    cancelled: CancelledRequestFn,
    set_cancel_callback: SetCancelCallbackFn,
    release: ReleaseRequestFn,
    wait_retired: WaitRetiredRequestFn,
}

impl ResourceRequestHandleFns {
    pub const NATIVE: Self = crate::generated::RESOURCE_REQUEST_HANDLE_FUNCTIONS;

    /// Creates a function table for a native resource request handle.
    ///
    /// # Safety
    ///
    /// The functions must implement the same ownership contract as the C API:
    /// `complete`, `cancelled`, and `set_cancel_callback` operate on the
    /// matching handle type, `set_cancel_callback` never invokes the callback,
    /// and `release` releases a provider-owned handle exactly once. Release
    /// waits for cancellation on another thread; self-release can return before
    /// the current cancellation callback finishes. `wait_retired` waits
    /// until native has released the request and every cancellation callback
    /// has returned, including after a PassThrough decision.
    pub const unsafe fn new(
        _complete: CompleteRequestFn,
        cancelled: CancelledRequestFn,
        set_cancel_callback: SetCancelCallbackFn,
        release: ReleaseRequestFn,
        wait_retired: WaitRetiredRequestFn,
    ) -> Self {
        Self {
            #[cfg(test)]
            complete: _complete,
            cancelled,
            set_cancel_callback,
            release,
            wait_retired,
        }
    }
}

/// Runs the host cancel callback for a request.
///
/// # Safety
///
/// `user_data` must be the pointer `ResourceRequestHandleState` registered
/// with the C API. Its raw weak reference remains allocated until the native
/// retirement wait confirms that cancellation is quiescent.
unsafe extern "C" fn cancel_callback_trampoline(user_data: *mut c_void) {
    let Some(token) = ptr::NonNull::new(user_data.cast::<ResourceRequestHandleState>()) else {
        return;
    };
    // SAFETY: The token came from Weak::into_raw in set_cancel_callback, and
    // the state reclaims it only after native retirement confirms that no
    // callback is running or can arrive. Handing the weak reference back through
    // into_raw leaves the registration's count untouched.
    let weak = unsafe { Weak::from_raw(token.as_ptr()) };
    let state = weak.upgrade();
    let _ = Weak::into_raw(weak);
    if let Some(state) = state {
        state.run_cancel_callback();
    }
}

/// Runs a host cancel callback with no binding lock held. A panic is contained
/// here: unwinding into C is undefined behavior, and the cancel path has no
/// status to report the failure through.
fn run_cancel_callback_contained(callback: Box<CancelCallback>) {
    let _ = catch_unwind(AssertUnwindSafe(callback));
}

struct ResourceRequestHandleInner {
    handle: u64,
    decision_finalized: bool,
    provider_owned: bool,
    release_accounted_for: bool,
    closed: bool,
    completed: bool,
    completing: bool,
    /// The registered host callback. Taken before it runs, so it runs once.
    cancel_callback: Option<Box<CancelCallback>>,
    /// `Weak<ResourceRequestHandleState>` handed to the C API as `user_data`,
    /// or zero before the first registration. Reclaimed when the state drops.
    cancel_token: usize,
}

impl fmt::Debug for ResourceRequestHandleInner {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ResourceRequestHandleInner")
            .field("handle", &self.handle)
            .field("decision_finalized", &self.decision_finalized)
            .field("provider_owned", &self.provider_owned)
            .field("release_accounted_for", &self.release_accounted_for)
            .field("closed", &self.closed)
            .field("completed", &self.completed)
            .field("cancel_registered", &self.cancel_callback.is_some())
            .finish()
    }
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
                cancel_callback: None,
                cancel_token: 0,
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
        let callback = if accepted {
            inner.cancel_callback.take()
        } else {
            None
        };
        let handle = Self::native_handle(&inner);
        let release = inner.closed
            && inner.decision_finalized
            && inner.provider_owned
            && Self::take_release_locked(&mut inner);
        drop(inner);
        self.release_now(release, handle);
        drop(callback);
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

    /// Registers the host callback that runs when MapLibre cancels the request.
    ///
    /// A request accepts one registration. When the C API reports that the
    /// request was already cancelled, the callback runs before this returns.
    pub fn set_cancel_callback(self: &Arc<Self>, callback: Box<CancelCallback>) -> Result<()> {
        if let Some(callback) = self.register_cancel_callback(callback)? {
            run_cancel_callback_contained(callback);
        }
        Ok(())
    }

    /// Returns the callback when native already cancelled and stored no registration.
    pub fn register_cancel_callback(
        self: &Arc<Self>,
        callback: Box<CancelCallback>,
    ) -> Result<Option<Box<CancelCallback>>> {
        let mut inner = self.lock_inner()?;
        if inner.closed {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "resource request is closed",
            ));
        }
        if inner.cancel_callback.is_some() {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "ResourceRequestHandle already has a cancel callback",
            ));
        }
        inner.cancel_callback = Some(callback);
        if inner.cancel_token == 0 {
            inner.cancel_token = Weak::into_raw(Arc::downgrade(self)) as usize;
        }
        let mut cancelled = false;
        // The native setter never blocks or calls back into the host, so the
        // lock stays held across it and a concurrent close waits for this
        // registration like any other in-flight use.
        // SAFETY: handle is live while not closed. user_data is a weak
        // reference this state reclaims only after native retirement, and
        // cancelled points to writable bool storage for this call.
        let status = unsafe {
            (self.fns.set_cancel_callback)(
                Self::native_handle(&inner),
                Some(cancel_callback_trampoline),
                inner.cancel_token as *mut c_void,
                &mut cancelled,
            )
        };
        if let Err(error) = crate::check(status) {
            // Native stored nothing, so the slot goes back to empty.
            let callback = inner.cancel_callback.take();
            drop(inner);
            drop(callback);
            return Err(error);
        }
        // Native stored nothing for a request MapLibre already cancelled, so
        // this call runs the callback itself. Taking it back under the lock
        // keeps a racing close from dropping it unrun.
        let inline = if cancelled {
            inner.cancel_callback.take()
        } else {
            None
        };
        drop(inner);
        Ok(inline)
    }

    pub fn is_cancelled(&self) -> Result<bool> {
        let inner = self.lock_inner()?;
        if inner.closed {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "resource request is closed",
            ));
        }
        let mut cancelled = false;
        // SAFETY: handle is live while not closed/released, and cancelled points
        // to writable bool storage.
        crate::check(unsafe { (self.fns.cancelled)(Self::native_handle(&inner), &mut cancelled) })?;
        Ok(cancelled)
    }

    pub fn close(&self) {
        let Ok(mut inner) = self.inner.lock() else {
            return;
        };
        if inner.closed {
            return;
        }
        inner.closed = true;
        let callback = inner.cancel_callback.take();
        let handle = Self::native_handle(&inner);
        let release = !inner.completing
            && inner.decision_finalized
            && inner.provider_owned
            && Self::take_release_locked(&mut inner);
        drop(inner);
        self.release_now(release, handle);
        drop(callback);
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
            let callback = Self::finish_unowned_locked(&mut inner);
            drop(inner);
            drop(callback);
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
            let callback = Self::finish_unowned_locked(&mut inner);
            drop(inner);
            drop(callback);
        }
        UNKNOWN_PROVIDER_DECISION
    }

    /// Records a decision that leaves the release to the C API, returning the
    /// cancel callback that can no longer run.
    fn finish_unowned_locked(
        inner: &mut ResourceRequestHandleInner,
    ) -> Option<Box<CancelCallback>> {
        inner.decision_finalized = true;
        inner.release_accounted_for = true;
        inner.closed = true;
        inner.cancel_callback.take()
    }

    fn take_release_locked(inner: &mut ResourceRequestHandleInner) -> bool {
        if inner.release_accounted_for {
            return false;
        }
        inner.release_accounted_for = true;
        true
    }

    fn take_cancel_callback(&self) -> Option<Box<CancelCallback>> {
        self.inner
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .cancel_callback
            .take()
    }

    /// Runs the registered callback once, outside the handle lock, so it can
    /// complete or close this same request.
    fn run_cancel_callback(&self) {
        if let Some(callback) = self.take_cancel_callback() {
            run_cancel_callback_contained(callback);
        }
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
        let handle = Self::native_handle(inner);
        let release = inner.provider_owned && Self::take_release_locked(inner);
        let token = inner.cancel_token;
        if !release && token == 0 {
            return;
        }
        let release_native = self.fns.release;
        let wait_retired = self.fns.wait_retired;
        let finalize = move || {
            if release {
                // SAFETY: take_release_locked grants this call exactly once.
                unsafe { release_native(handle) };
            }
            if token != 0 {
                // PassThrough leaves retirement to native after the provider
                // returns. Self-release also permits the current cancel
                // callback to finish after release returns.
                // SAFETY: The issued handle remains valid for retirement waits.
                let status = unsafe { wait_retired(handle) };
                if status == sys::MLN_STATUS_OK {
                    // SAFETY: Native cancellation is now quiescent.
                    drop(unsafe { Weak::from_raw(token as *const Self) });
                }
            }
        };
        if token == 0 {
            crate::callback::finalize(finalize);
        } else {
            // The last strong reference may belong to the native cancellation
            // trampoline; waiting on that same callback would deadlock.
            crate::callback::defer(Box::new(finalize));
        }
    }
}

#[cfg(test)]
mod tests {
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
    fn pass_through_cancel_token_waits_for_native_retirement() {
        static RETIREMENT: StdMutex<
            Option<(std::sync::mpsc::Sender<()>, std::sync::mpsc::Receiver<()>)>,
        > = StdMutex::new(None);
        unsafe extern "C" fn wait_retired(
            _handle: sys::mln_resource_request_handle,
        ) -> sys::mln_status {
            let (entered, proceed) = RETIREMENT.lock().unwrap().take().unwrap();
            entered.send(()).unwrap();
            proceed.recv_timeout(Duration::from_secs(5)).unwrap();
            sys::MLN_STATUS_OK
        }
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let (entered_sender, entered) = std::sync::mpsc::channel();
        let (proceed, proceed_receiver) = std::sync::mpsc::channel();
        *RETIREMENT.lock().unwrap() = Some((entered_sender, proceed_receiver));
        let mut state = fake_state();
        Arc::get_mut(&mut state).unwrap().fns.wait_retired = wait_retired;
        state
            .set_cancel_callback(Box::new(|| {
                panic!("unowned request callback must be disarmed")
            }))
            .unwrap();
        state.finish_provider_decision(ResourceProviderDecision::PassThrough);
        drop(state);
        entered
            .recv_timeout(Duration::from_secs(5))
            .expect("token reclamation must wait for native retirement");
        // Native may have captured the token before the provider returned.
        // The weak allocation remains valid even though its owner is gone.
        fire_registered_cancel();
        *REGISTERED_CANCEL.lock().unwrap() = None;
        proceed.send(()).unwrap();
        let (finished, finish) = std::sync::mpsc::channel();
        crate::callback::defer(Box::new(move || finished.send(()).unwrap()));
        finish.recv_timeout(Duration::from_secs(5)).unwrap();
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
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
    static CANCELLED_SLEEP_MS: AtomicUsize = AtomicUsize::new(0);
    static CANCELLED_STARTED: AtomicBool = AtomicBool::new(false);
    static CANCELLED_FINISHED: AtomicBool = AtomicBool::new(false);
    static SET_CANCEL_COUNT: AtomicUsize = AtomicUsize::new(0);
    static ALREADY_CANCELLED: AtomicBool = AtomicBool::new(false);
    /// The registration the fake C API stored, as (callback, user_data).
    static REGISTERED_CANCEL: StdMutex<Option<(sys::mln_resource_request_cancel_callback, usize)>> =
        StdMutex::new(None);
    /// When set, the fake release waits for this flag like the C API waits for
    /// a cancel callback running on another thread.
    static RELEASE_WAITS_FOR_CALLBACK: AtomicBool = AtomicBool::new(false);
    static CALLBACK_FINISHED: AtomicBool = AtomicBool::new(false);

    unsafe extern "C" fn fake_complete(
        _handle: sys::mln_resource_request_handle,
        _response: *const sys::mln_resource_response,
    ) -> sys::mln_status {
        COMPLETE_COUNT.fetch_add(1, Ordering::SeqCst);
        COMPLETE_STATUS.load(Ordering::SeqCst)
    }

    unsafe extern "C" fn fake_cancelled(
        _handle: sys::mln_resource_request_handle,
        out_cancelled: *mut bool,
    ) -> sys::mln_status {
        if out_cancelled.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let sleep_ms = CANCELLED_SLEEP_MS.load(Ordering::SeqCst);
        if sleep_ms != 0 {
            CANCELLED_STARTED.store(true, Ordering::SeqCst);
            std::thread::sleep(Duration::from_millis(sleep_ms as u64));
            CANCELLED_FINISHED.store(true, Ordering::SeqCst);
        }
        // SAFETY: out_cancelled is non-null and points to caller-owned output storage.
        unsafe { *out_cancelled = ALREADY_CANCELLED.load(Ordering::SeqCst) };
        sys::MLN_STATUS_OK
    }

    unsafe extern "C" fn fake_release(_handle: sys::mln_resource_request_handle) {
        if RELEASE_WAITS_FOR_CALLBACK.load(Ordering::SeqCst) {
            let deadline = Instant::now() + Duration::from_secs(5);
            while !CALLBACK_FINISHED.load(Ordering::SeqCst) && Instant::now() < deadline {
                std::thread::yield_now();
            }
        }
        *REGISTERED_CANCEL.lock().unwrap() = None;
        RELEASE_COUNT.fetch_add(1, Ordering::SeqCst);
    }

    /// Stands in for the C API's registration: it stores the callback for a
    /// live request and reports an already cancelled one without storing it.
    unsafe extern "C" fn fake_set_cancel_callback(
        _handle: sys::mln_resource_request_handle,
        callback: sys::mln_resource_request_cancel_callback,
        user_data: *mut c_void,
        out_cancelled: *mut bool,
    ) -> sys::mln_status {
        SET_CANCEL_COUNT.fetch_add(1, Ordering::SeqCst);
        if callback.is_none() || out_cancelled.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let cancelled = ALREADY_CANCELLED.load(Ordering::SeqCst);
        // SAFETY: out_cancelled is non-null and points to caller-owned output storage.
        unsafe { *out_cancelled = cancelled };
        if !cancelled {
            *REGISTERED_CANCEL.lock().unwrap() = Some((callback, user_data as usize));
        }
        sys::MLN_STATUS_OK
    }

    /// Invokes the stored registration the way the C API does when MapLibre
    /// discards the request.
    fn fire_registered_cancel() {
        let (callback, user_data) = REGISTERED_CANCEL
            .lock()
            .unwrap()
            .expect("a registered cancel callback");
        // SAFETY: The fake registration stored this pair from the state under
        // test, which is still alive and unreleased.
        unsafe { callback.unwrap()(user_data as *mut c_void) };
    }

    unsafe extern "C" fn fake_wait_retired(
        _handle: sys::mln_resource_request_handle,
    ) -> sys::mln_status {
        sys::MLN_STATUS_OK
    }

    fn fake_fns() -> ResourceRequestHandleFns {
        // SAFETY: These fake functions implement the native handle contract for tests.
        unsafe {
            ResourceRequestHandleFns::new(
                fake_complete,
                fake_cancelled,
                fake_set_cancel_callback,
                fake_release,
                fake_wait_retired,
            )
        }
    }

    fn fake_state() -> Arc<ResourceRequestHandleState> {
        COMPLETE_COUNT.store(0, Ordering::SeqCst);
        RELEASE_COUNT.store(0, Ordering::SeqCst);
        COMPLETE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        CANCELLED_SLEEP_MS.store(0, Ordering::SeqCst);
        CANCELLED_STARTED.store(false, Ordering::SeqCst);
        CANCELLED_FINISHED.store(false, Ordering::SeqCst);
        SET_CANCEL_COUNT.store(0, Ordering::SeqCst);
        ALREADY_CANCELLED.store(false, Ordering::SeqCst);
        *REGISTERED_CANCEL.lock().unwrap() = None;
        RELEASE_WAITS_FOR_CALLBACK.store(false, Ordering::SeqCst);
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
    fn request_release_waits_for_in_flight_cancellation_check() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        CANCELLED_SLEEP_MS.store(50, Ordering::SeqCst);
        let thread_state = Arc::clone(&state);
        let thread = std::thread::spawn(move || {
            thread_state.is_cancelled().unwrap();
        });
        let started_deadline = Instant::now() + Duration::from_secs(5);
        while !CANCELLED_STARTED.load(Ordering::SeqCst) {
            assert!(
                Instant::now() < started_deadline,
                "timed out waiting for cancellation check to start"
            );
            std::thread::yield_now();
        }

        state.close();

        assert!(CANCELLED_FINISHED.load(Ordering::SeqCst));
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        thread.join().unwrap();
    }

    #[test]

    // registration instead of storing the callback, so the binding runs the
    // callback itself, with no lock held, before registration returns.
    fn cancel_registration_on_a_cancelled_request_runs_the_callback_before_returning() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        ALREADY_CANCELLED.store(true, Ordering::SeqCst);
        COMPLETE_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);
        let callback_state = Arc::clone(&state);
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);

        state
            .set_cancel_callback(Box::new(move || {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                assert_eq!(
                    callback_state
                        .complete(&ResourceResponse {
                            status: crate::ResourceResponseStatus::NoContent,
                            ..Default::default()
                        })
                        .unwrap_err()
                        .kind(),
                    ErrorKind::InvalidState
                );
                callback_state.close();
            }))
            .unwrap();

        assert_eq!(calls.load(Ordering::SeqCst), 1);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]

    // second registration reports invalid state, both without reaching C.
    fn cancel_registration_rejects_closed_and_registered_requests() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        state.set_cancel_callback(Box::new(|| {})).unwrap();
        assert_eq!(SET_CANCEL_COUNT.load(Ordering::SeqCst), 1);

        let error = state.set_cancel_callback(Box::new(|| {})).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(SET_CANCEL_COUNT.load(Ordering::SeqCst), 1);

        state.close();
        let error = state.set_cancel_callback(Box::new(|| {})).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(SET_CANCEL_COUNT.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]

    // own thread, and the callback closes the request it belongs to.
    fn cancel_callback_from_another_thread_may_close_its_request() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        let callback_state = Arc::clone(&state);
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        state
            .set_cancel_callback(Box::new(move || {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                callback_state.close();
            }))
            .unwrap();

        std::thread::spawn(fire_registered_cancel).join().unwrap();

        assert_eq!(calls.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        state.close();
        drop(state);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]

    // callback running on another thread, so close must not hold the handle
    // lock across it when that callback calls back into the same handle.
    fn close_holds_no_lock_while_native_release_waits_for_the_callback() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        RELEASE_WAITS_FOR_CALLBACK.store(true, Ordering::SeqCst);
        let callback_started = Arc::new(AtomicBool::new(false));
        let callback_state = Arc::clone(&state);
        let started = Arc::clone(&callback_started);
        let observed_closed = Arc::new(AtomicBool::new(false));
        let callback_observed_closed = Arc::clone(&observed_closed);
        state
            .set_cancel_callback(Box::new(move || {
                started.store(true, Ordering::SeqCst);
                // Let close begin its release before this takes the lock.
                std::thread::sleep(Duration::from_millis(50));
                let closed = callback_state.is_cancelled().is_err();
                callback_observed_closed.store(closed, Ordering::SeqCst);
                CALLBACK_FINISHED.store(true, Ordering::SeqCst);
            }))
            .unwrap();
        let callback_thread = std::thread::spawn(fire_registered_cancel);
        wait_until(&callback_started, "the cancel callback to start");

        state.close();

        assert!(CALLBACK_FINISHED.load(Ordering::SeqCst));
        assert!(observed_closed.load(Ordering::SeqCst));
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        callback_thread.join().unwrap();
    }

    #[test]

    // completion drops the callback's captures.
    fn completion_drops_the_cancel_callback() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        state
            .set_cancel_callback(Box::new(move || {
                let _ = &callback_token;
                panic!("a completed request must not run its cancel callback");
            }))
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        state
            .complete(&ResourceResponse {
                status: crate::ResourceResponseStatus::NoContent,
                ..Default::default()
            })
            .unwrap();

        assert_eq!(Arc::strong_count(&token), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
        state.close();
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]

    // request itself, and that release retires the registration.
    fn cancel_registration_leaves_a_passed_through_release_to_native() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();
        state.set_cancel_callback(Box::new(|| {})).unwrap();

        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::PassThrough),
            sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        );
        drop(state);

        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
    }
}
