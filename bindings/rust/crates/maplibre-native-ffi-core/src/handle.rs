use std::cell::Cell;
use std::marker::PhantomData;
use std::num::NonZeroU64;
use std::sync::Mutex;

use maplibre_native_ffi_sys as sys;

use crate::error::Result;

pub type StatusDestroyFn<T> = unsafe extern "C" fn(T) -> sys::mln_status;
pub type InfallibleDestroyFn<T> = unsafe extern "C" fn(T);

/// A C handle type: a transparent newtype over the 64-bit id the C API issues.
pub trait NativeHandle: Copy {
    fn to_raw(self) -> u64;
    fn from_raw(raw: u64) -> Self;
}

macro_rules! native_handle {
    ($($handle:ty),* $(,)?) => {
        $(
            impl NativeHandle for $handle {
                fn to_raw(self) -> u64 {
                    self.0
                }

                fn from_raw(raw: u64) -> Self {
                    Self(raw)
                }
            }
        )*
    };
}

native_handle!(
    sys::mln_buffer,
    sys::mln_runtime,
    sys::mln_map,
    sys::mln_map_projection,
    sys::mln_render_session,
    sys::mln_operation,
    sys::mln_notification_source,
    sys::mln_event_batch,
    sys::mln_ready_batch,
    sys::mln_render_frame_batch,
    sys::mln_acquired_frame,
    sys::mln_resource_request_handle,
    sys::mln_offline_region_snapshot,
    sys::mln_offline_region_list,
    sys::mln_style_id_list,
    sys::mln_style_string_list,
);

/// A native handle a destructor attempted to destroy and could not. The handle
/// stays live.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct NativeHandleLeak {
    /// The native type name, such as `mln_map`.
    pub type_name: &'static str,
    /// The handle id that was not destroyed.
    pub id: u64,
}

type LeakReporter = Box<dyn Fn(NativeHandleLeak) + Send + Sync>;

static LEAK_REPORTER: Mutex<Option<LeakReporter>> = Mutex::new(None);

/// Installs the process-wide reporter for handles a destructor could not
/// destroy, replacing any previous one, and returns whether one was installed.
/// Explicit `close` reports the same failure through the normal error path.
pub fn set_leak_reporter(reporter: Option<LeakReporter>) -> bool {
    let mut slot = LEAK_REPORTER
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let replaced = slot.is_some();
    *slot = reporter;
    replaced
}

/// Reports a handle a destructor could not destroy. Never panics: it is called
/// from `Drop`, where unwinding would abort.
pub fn report_leak(leak: NativeHandleLeak) {
    let slot = LEAK_REPORTER
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if let Some(reporter) = slot.as_ref() {
        // The reporter is arbitrary caller code running from `Drop`; unwinding
        // through a destructor during another unwind aborts the process.
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| reporter(leak)));
    }
}

/// Tracks whether a native handle is still live and centralizes close-once
/// behavior. Public handle policy such as `!Send`, parent retention, and
/// owner-thread checks lives above this.
#[derive(Debug)]
pub struct NativeHandleState<T> {
    id: Cell<Option<NonZeroU64>>,
    type_name: &'static str,
    _typed_handle: PhantomData<fn() -> T>,
}

impl<T: NativeHandle> NativeHandleState<T> {
    /// Takes ownership of a native handle.
    ///
    /// # Safety
    ///
    /// `handle` must be a live handle of the matching native type owned by the
    /// caller. The caller must later close the state with the matching C API
    /// destroy function or intentionally report the handle as leaked.
    pub unsafe fn from_handle(handle: T, type_name: &'static str) -> Result<Self> {
        let Some(id) = NonZeroU64::new(handle.to_raw()) else {
            return Err(crate::ptr::null_handle_error(type_name));
        };
        Ok(Self {
            id: Cell::new(Some(id)),
            type_name,
            _typed_handle: PhantomData,
        })
    }

    /// The live handle, or the null handle once closed.
    pub fn handle(&self) -> T {
        T::from_raw(self.id.get().map_or(0, NonZeroU64::get))
    }

    pub fn live_handle(&self) -> Option<T> {
        self.id.get().map(|id| T::from_raw(id.get()))
    }

    pub fn id(&self) -> Option<u64> {
        self.id.get().map(NonZeroU64::get)
    }

    pub fn mark_closed(&self) {
        self.id.set(None);
    }

    pub fn restore_handle_for_retry(&self, handle: T) {
        self.id.set(NonZeroU64::new(handle.to_raw()));
    }

    pub fn is_closed(&self) -> bool {
        self.id.get().is_none()
    }

    pub fn type_name(&self) -> &'static str {
        self.type_name
    }

    /// Closes the handle with a status-returning destroy function.
    ///
    /// The state remains live when the destroy function reports an error, which
    /// lets callers retry an owner-thread close later.
    ///
    /// # Safety
    ///
    /// `destroy` must be the matching C API destroy function for this handle
    /// type. It must not take ownership when it returns a non-OK status.
    pub unsafe fn close_status(&self, destroy: StatusDestroyFn<T>) -> Result<()> {
        let Some(handle) = self.live_handle() else {
            return Ok(());
        };

        // SAFETY: The caller promises destroy matches this live handle's type.
        crate::check(unsafe { destroy(handle) })?;
        self.id.set(None);
        Ok(())
    }

    /// Closes the handle with an infallible destroy function.
    ///
    /// # Safety
    ///
    /// `destroy` must be the matching C API destroy function for this handle
    /// type and must release the handle exactly once.
    pub unsafe fn close_infallible(&self, destroy: InfallibleDestroyFn<T>) {
        let Some(handle) = self.live_handle() else {
            return;
        };

        self.id.set(None);
        // SAFETY: The caller promises destroy matches this live handle's type.
        unsafe { destroy(handle) };
    }

    /// Marks the handle as intentionally leaked and returns its id for
    /// diagnostics. This does not call the destroy function, and later close
    /// calls become no-ops. Use it only where the caller must avoid destroying
    /// thread-affine native state, such as a finalizer on an arbitrary thread.
    pub fn leak_for_report(&self) -> Option<u64> {
        let id = self.id.get()?;
        self.id.set(None);
        Some(id.get())
    }
}

#[derive(Debug)]
struct NativeGuardState<T: NativeHandle> {
    state: NativeHandleState<T>,
    destroy: InfallibleDestroyFn<T>,
}

impl<T: NativeHandle> NativeGuardState<T> {
    /// Takes ownership of a native handle pointer.
    ///
    /// # Safety
    ///
    /// `ptr` must be a non-null owned live handle of the matching native type.
    /// `destroy` must be the C API function that releases exactly that handle
    /// type and accepts null as a no-op.
    unsafe fn from_handle(
        handle: T,
        destroy: InfallibleDestroyFn<T>,
        type_name: &'static str,
    ) -> Result<Self> {
        // SAFETY: The caller promises handle is an owned live handle of the
        // matching native type; this constructor pairs it with the matching
        // infallible destroy function.
        let state = unsafe { NativeHandleState::from_handle(handle, type_name) }?;
        Ok(Self { state, destroy })
    }

    fn handle(&self) -> T {
        self.state.handle()
    }

    fn close(self) {
        drop(self);
    }
}

impl<T: NativeHandle> Drop for NativeGuardState<T> {
    fn drop(&mut self) {
        // SAFETY: NativeGuardState binds the owned handle to the matching
        // infallible destroy function at construction.
        unsafe { self.state.close_infallible(self.destroy) };
    }
}

macro_rules! native_guard {
    ($guard:ident, $native:ty, $destroy:path, $type_name:literal, $constructor:ident) => {
        #[derive(Debug)]
        pub struct $guard {
            inner: NativeGuardState<$native>,
        }

        impl $guard {
            pub fn handle(&self) -> $native {
                self.inner.handle()
            }

            pub fn close(self) {
                self.inner.close();
            }
        }

        /// Takes ownership of a native handle.
        ///
        /// # Safety
        ///
        /// `handle` must be a live handle owned by the caller.
        pub unsafe fn $constructor(handle: $native) -> Result<$guard> {
            // SAFETY: The caller promises handle is an owned live handle of the
            // matching native type; this constructor pairs it with the matching
            // destroy function.
            let inner = unsafe { NativeGuardState::from_handle(handle, $destroy, $type_name) }?;
            Ok($guard { inner })
        }
    };
}

native_guard!(
    BufferGuard,
    sys::mln_buffer,
    sys::mln_buffer_destroy,
    "mln_buffer",
    buffer
);
native_guard!(
    OfflineRegionSnapshotGuard,
    sys::mln_offline_region_snapshot,
    sys::mln_offline_region_snapshot_destroy,
    "mln_offline_region_snapshot",
    offline_region_snapshot
);
native_guard!(
    OfflineRegionListGuard,
    sys::mln_offline_region_list,
    sys::mln_offline_region_list_destroy,
    "mln_offline_region_list",
    offline_region_list
);
native_guard!(
    StyleIdListGuard,
    sys::mln_style_id_list,
    sys::mln_style_id_list_destroy,
    "mln_style_id_list",
    style_id_list
);
native_guard!(
    StyleStringListGuard,
    sys::mln_style_string_list,
    sys::mln_style_string_list_destroy,
    "mln_style_string_list",
    style_string_list
);

#[cfg(test)]
mod tests {
    use std::sync::Mutex;
    use std::sync::atomic::{AtomicI32, AtomicUsize, Ordering};

    use super::*;

    /// A synthetic handle type for close-once tests. Nothing here crosses into
    /// the C API: the destroy functions below only count calls.
    #[repr(transparent)]
    #[derive(Debug, Clone, Copy)]
    struct TestHandle(u64);

    impl NativeHandle for TestHandle {
        fn to_raw(self) -> u64 {
            self.0
        }

        fn from_raw(raw: u64) -> Self {
            Self(raw)
        }
    }

    const TEST_HANDLE: TestHandle = TestHandle(0x0d00_0000_0000_002a);

    static DESTROY_COUNT: AtomicUsize = AtomicUsize::new(0);
    static STATUS_DESTROY_COUNT: AtomicUsize = AtomicUsize::new(0);
    static DESTROY_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);
    static DESTROY_COUNT_LOCK: Mutex<()> = Mutex::new(());

    unsafe extern "C" fn count_destroy(handle: TestHandle) {
        if handle.0 != 0 {
            DESTROY_COUNT.fetch_add(1, Ordering::SeqCst);
        }
    }

    unsafe extern "C" fn count_status_destroy(handle: TestHandle) -> sys::mln_status {
        if handle.0 != 0 {
            STATUS_DESTROY_COUNT.fetch_add(1, Ordering::SeqCst);
        }
        DESTROY_STATUS.load(Ordering::SeqCst)
    }

    fn assert_send<T: Send>() {}

    #[test]
    fn native_handle_state_is_send_for_bridge_storage() {
        assert_send::<NativeHandleState<TestHandle>>();
    }

    #[test]
    fn native_handle_destroys_owned_pointer_on_drop() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        {
            let handle =
                unsafe { NativeGuardState::from_handle(TEST_HANDLE, count_destroy, "test_handle") }
                    .unwrap();
            assert_eq!(handle.handle().0, TEST_HANDLE.0);
        }

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn native_handle_close_destroys_owned_pointer_once() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        let handle =
            unsafe { NativeGuardState::from_handle(TEST_HANDLE, count_destroy, "test_handle") }
                .unwrap();
        handle.close();

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    // Spec coverage: BND-066.
    fn native_handle_drop_releases_owned_pointer_after_copy_error() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        let error = {
            let handle =
                unsafe { NativeGuardState::from_handle(TEST_HANDLE, count_destroy, "test_handle") }
                    .unwrap();
            assert_eq!(handle.handle().0, TEST_HANDLE.0);
            let result: crate::Result<()> = Err(crate::Error::invalid_argument("copy failed"));
            result
        }
        .unwrap_err();

        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn native_handle_rejects_the_null_handle() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        let error =
            unsafe { NativeGuardState::from_handle(TestHandle(0), count_destroy, "test_handle") }
                .unwrap_err();

        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
    }

    #[test]
    fn native_handle_state_retries_status_destroy_after_failure() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        STATUS_DESTROY_COUNT.store(0, Ordering::SeqCst);
        DESTROY_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);
        // SAFETY: the fake destroy below only records calls and never
        // dereferences the handle.
        let state = unsafe { NativeHandleState::from_handle(TEST_HANDLE, "test_handle") }.unwrap();

        // SAFETY: count_status_destroy is the matching fake destroy function
        // for this test handle and does not take ownership on failure.
        let error = unsafe { state.close_status(count_status_destroy) }.unwrap_err();
        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidState);
        assert_eq!(state.handle().0, TEST_HANDLE.0);

        DESTROY_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        // SAFETY: count_status_destroy is the matching fake destroy function
        // for this test handle and reports success on this path.
        unsafe { state.close_status(count_status_destroy) }.unwrap();
        // SAFETY: close after success is a no-op and the same matching fake
        // destroy function is still used.
        unsafe { state.close_status(count_status_destroy) }.unwrap();

        assert_eq!(STATUS_DESTROY_COUNT.load(Ordering::SeqCst), 2);
        assert!(state.is_closed());
    }

    #[test]
    fn native_handle_state_closes_infallible_once() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        // SAFETY: count_destroy only records calls and never dereferences the
        // handle.
        let state = unsafe { NativeHandleState::from_handle(TEST_HANDLE, "test_handle") }.unwrap();

        // SAFETY: count_destroy is the matching fake destroy function for this
        // test handle.
        unsafe { state.close_infallible(count_destroy) };
        // SAFETY: close after success is a no-op and the same matching fake
        // destroy function is still used.
        unsafe { state.close_infallible(count_destroy) };

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
        assert!(state.is_closed());
    }

    #[test]
    fn native_handle_state_reports_leak_without_destroying() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        // SAFETY: the test intentionally uses leak_for_report instead of a
        // destroy function, so nothing here reaches native code.
        let state = unsafe { NativeHandleState::from_handle(TEST_HANDLE, "test_handle") }.unwrap();

        assert_eq!(state.leak_for_report(), Some(TEST_HANDLE.0));
        assert_eq!(state.leak_for_report(), None);
        // SAFETY: close after leak_for_report is a no-op and the same matching
        // fake destroy function is still used.
        unsafe { state.close_infallible(count_destroy) };

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 0);
        assert!(state.is_closed());
    }
}
