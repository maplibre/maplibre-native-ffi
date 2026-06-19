use std::cell::Cell;
use std::marker::PhantomData;
use std::ptr::NonNull;

use maplibre_native_sys as sys;

use crate::error::Result;
use crate::ptr::non_null_mut;

pub type StatusDestroyFn<T> = unsafe extern "C" fn(*mut T) -> sys::mln_status;
pub type InfallibleDestroyFn<T> = unsafe extern "C" fn(*mut T);

/// Bridge-neutral native pointer ownership state.
///
/// `NativeHandleState` tracks whether a native handle pointer is still live and
/// centralizes close-once behavior. It intentionally does not encode public Rust
/// handle policy such as `!Send`, parent retention, owner-thread checks, or
/// finalizer dispatch.
#[derive(Debug)]
pub struct NativeHandleState<T> {
    address: Cell<Option<usize>>,
    type_name: &'static str,
    _typed_handle: PhantomData<fn() -> T>,
}

impl<T> NativeHandleState<T> {
    /// Takes ownership of a native handle pointer.
    ///
    /// # Safety
    ///
    /// `ptr` must be a non-null owned live handle of the matching native type.
    /// The caller must later close the state with the matching C API destroy
    /// function or intentionally report the pointer as leaked.
    pub unsafe fn from_raw(ptr: NonNull<T>, type_name: &'static str) -> Self {
        Self {
            address: Cell::new(Some(ptr.as_ptr() as usize)),
            type_name,
            _typed_handle: PhantomData,
        }
    }

    /// Takes ownership of a native handle pointer after validating non-nullness.
    ///
    /// # Safety
    ///
    /// `ptr` must be an owned live handle of the matching native type when it is
    /// non-null. The caller must later close the state with the matching C API
    /// destroy function or intentionally report the pointer as leaked.
    pub unsafe fn from_raw_ptr(ptr: *mut T, type_name: &'static str) -> Result<Self> {
        let ptr = non_null_mut(ptr, type_name)?;
        // SAFETY: The caller promises ptr is an owned live handle, and
        // non_null_mut validated that it is non-null.
        Ok(unsafe { Self::from_raw(ptr, type_name) })
    }

    pub fn as_ptr(&self) -> *mut T {
        self.address
            .get()
            .map_or(std::ptr::null_mut(), |address| address as *mut T)
    }

    pub fn as_non_null(&self) -> Option<NonNull<T>> {
        NonNull::new(self.as_ptr())
    }

    pub fn is_closed(&self) -> bool {
        self.address.get().is_none()
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
        let Some(ptr) = self.as_non_null() else {
            return Ok(());
        };

        // SAFETY: The caller promises destroy matches the live handle pointer.
        crate::check(unsafe { destroy(ptr.as_ptr()) })?;
        self.address.set(None);
        Ok(())
    }

    /// Closes the handle with an infallible destroy function.
    ///
    /// # Safety
    ///
    /// `destroy` must be the matching C API destroy function for this handle
    /// type and must release the handle exactly once.
    pub unsafe fn close_infallible(&self, destroy: InfallibleDestroyFn<T>) {
        let Some(ptr) = self.as_non_null() else {
            return;
        };

        self.address.set(None);
        // SAFETY: The caller promises destroy matches the live handle pointer.
        unsafe { destroy(ptr.as_ptr()) };
    }

    /// Marks the handle as intentionally leaked and returns its address for
    /// diagnostics or host-runtime finalizer reporting.
    ///
    /// This does not call the destroy function. Use it only on paths where the
    /// caller deliberately avoids destroying thread-affine native state, such as
    /// a GC finalizer running on an arbitrary host thread. It consumes logical
    /// ownership of the handle state: future close calls become no-ops.
    pub fn leak_for_report(&self) -> Option<usize> {
        let address = self.address.get()?;
        self.address.set(None);
        Some(address)
    }
}

#[derive(Debug)]
struct NativeHandle<T> {
    state: NativeHandleState<T>,
    destroy: InfallibleDestroyFn<T>,
}

impl<T> NativeHandle<T> {
    /// Takes ownership of a native handle pointer.
    ///
    /// # Safety
    ///
    /// `ptr` must be a non-null owned live handle of the matching native type.
    /// `destroy` must be the C API function that releases exactly that handle
    /// type and accepts null as a no-op.
    unsafe fn from_raw(
        ptr: *mut T,
        destroy: InfallibleDestroyFn<T>,
        type_name: &'static str,
    ) -> Result<Self> {
        // SAFETY: The caller promises ptr is a non-null owned handle of the
        // matching native type; this constructor pairs it with the matching
        // infallible destroy function.
        let state = unsafe { NativeHandleState::from_raw_ptr(ptr, type_name) }?;
        Ok(Self { state, destroy })
    }

    fn as_ptr(&self) -> *mut T {
        self.state.as_ptr()
    }

    fn as_non_null(&self) -> NonNull<T> {
        self.state
            .as_non_null()
            .expect("native result handle guard is unexpectedly closed")
    }

    fn close(self) {
        drop(self);
    }
}

impl<T> Drop for NativeHandle<T> {
    fn drop(&mut self) {
        // SAFETY: NativeHandle binds the owned pointer to the matching
        // infallible destroy function at construction.
        unsafe { self.state.close_infallible(self.destroy) };
    }
}

macro_rules! native_guard {
    ($guard:ident, $native:ty, $destroy:path, $type_name:literal, $constructor:ident) => {
        #[derive(Debug)]
        pub struct $guard {
            inner: NativeHandle<$native>,
        }

        impl $guard {
            pub fn as_ptr(&self) -> *mut $native {
                self.inner.as_ptr()
            }

            pub fn as_non_null(&self) -> NonNull<$native> {
                self.inner.as_non_null()
            }

            pub fn close(self) {
                self.inner.close();
            }
        }

        /// Takes ownership of a native handle.
        ///
        /// # Safety
        ///
        /// `ptr` must be a non-null live handle owned by the caller.
        pub unsafe fn $constructor(ptr: *mut $native) -> Result<$guard> {
            // SAFETY: The caller promises ptr is a non-null owned handle of the
            // matching native type; this constructor pairs it with the matching
            // destroy function.
            let inner = unsafe { NativeHandle::from_raw(ptr, $destroy, $type_name) }?;
            Ok($guard { inner })
        }
    };
}

native_guard!(
    FeatureQueryResultGuard,
    sys::mln_feature_query_result,
    sys::mln_feature_query_result_destroy,
    "mln_feature_query_result",
    feature_query_result
);
native_guard!(
    FeatureExtensionResultGuard,
    sys::mln_feature_extension_result,
    sys::mln_feature_extension_result_destroy,
    "mln_feature_extension_result",
    feature_extension_result
);
native_guard!(
    JsonSnapshotGuard,
    sys::mln_json_snapshot,
    sys::mln_json_snapshot_destroy,
    "mln_json_snapshot",
    json_snapshot
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

#[cfg(test)]
mod tests {
    use std::ptr;
    use std::sync::Mutex;
    use std::sync::atomic::{AtomicI32, AtomicUsize, Ordering};

    use super::*;

    static DESTROY_COUNT: AtomicUsize = AtomicUsize::new(0);
    static STATUS_DESTROY_COUNT: AtomicUsize = AtomicUsize::new(0);
    static DESTROY_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);
    static DESTROY_COUNT_LOCK: Mutex<()> = Mutex::new(());

    unsafe extern "C" fn count_destroy(ptr: *mut u8) {
        if !ptr.is_null() {
            DESTROY_COUNT.fetch_add(1, Ordering::SeqCst);
        }
    }

    unsafe extern "C" fn count_status_destroy(ptr: *mut u8) -> sys::mln_status {
        if !ptr.is_null() {
            STATUS_DESTROY_COUNT.fetch_add(1, Ordering::SeqCst);
        }
        DESTROY_STATUS.load(Ordering::SeqCst)
    }

    fn assert_send<T: Send>() {}

    #[test]
    fn native_handle_state_is_send_for_bridge_storage() {
        assert_send::<NativeHandleState<u8>>();
    }

    #[test]
    fn native_handle_destroys_owned_pointer_on_drop() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        let mut value = 1u8;

        {
            let handle =
                unsafe { NativeHandle::from_raw(&mut value, count_destroy, "test_handle") }
                    .unwrap();
            assert_eq!(handle.as_ptr().cast_const(), ptr::addr_of!(value));
        }

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn native_handle_close_destroys_owned_pointer_once() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        let mut value = 1u8;

        let handle =
            unsafe { NativeHandle::from_raw(&mut value, count_destroy, "test_handle") }.unwrap();
        handle.close();

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    // Spec coverage: BND-066. This injects a post-acquire copy failure at
    // Rust's shared native result-handle guard, which every snapshot/list/result
    // copier uses, instead of repeating the same guard behavior per domain.
    fn native_handle_drop_releases_owned_pointer_after_copy_error() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        let mut value = 1u8;

        let error = {
            let handle =
                unsafe { NativeHandle::from_raw(&mut value, count_destroy, "test_handle") }
                    .unwrap();
            assert_eq!(handle.as_ptr().cast_const(), ptr::addr_of!(value));
            let result: crate::Result<()> = Err(crate::Error::invalid_argument("copy failed"));
            result
        }
        .unwrap_err();

        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn native_handle_rejects_null() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        let error =
            unsafe { NativeHandle::from_raw(ptr::null_mut::<u8>(), count_destroy, "test_handle") }
                .unwrap_err();

        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
    }

    #[test]
    fn native_handle_state_retries_status_destroy_after_failure() {
        let _lock = DESTROY_COUNT_LOCK.lock().unwrap();
        STATUS_DESTROY_COUNT.store(0, Ordering::SeqCst);
        DESTROY_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);
        let mut value = 1u8;
        let ptr = NonNull::from(&mut value);
        // SAFETY: ptr points to live test storage, and the fake destroy only
        // records calls without taking ownership.
        let state = unsafe { NativeHandleState::from_raw(ptr, "test_handle") };

        // SAFETY: count_status_destroy is the matching fake destroy function
        // for this test handle and does not take ownership on failure.
        let error = unsafe { state.close_status(count_status_destroy) }.unwrap_err();
        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidState);
        assert_eq!(state.as_ptr().cast_const(), ptr::addr_of!(value));

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
        let mut value = 1u8;
        let ptr = NonNull::from(&mut value);
        // SAFETY: ptr points to live test storage, and count_destroy only
        // records calls.
        let state = unsafe { NativeHandleState::from_raw(ptr, "test_handle") };

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
        let mut value = 1u8;
        let ptr = NonNull::from(&mut value);
        // SAFETY: ptr points to live test storage. The test intentionally uses
        // leak_for_report instead of a destroy function.
        let state = unsafe { NativeHandleState::from_raw(ptr, "test_handle") };

        assert_eq!(state.leak_for_report(), Some(ptr::addr_of!(value) as usize));
        assert_eq!(state.leak_for_report(), None);
        // SAFETY: close after leak_for_report is a no-op and the same matching
        // fake destroy function is still used.
        unsafe { state.close_infallible(count_destroy) };

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 0);
        assert!(state.is_closed());
    }
}
