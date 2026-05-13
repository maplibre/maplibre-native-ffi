use std::marker::PhantomData;
use std::ptr::NonNull;

use maplibre_native_sys as sys;

use crate::error::Result;
use crate::ptr::non_null_mut;

#[derive(Debug)]
struct NativeHandle<T> {
    ptr: NonNull<T>,
    destroy: unsafe extern "C" fn(*mut T),
    _owns_handle: PhantomData<T>,
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
        destroy: unsafe extern "C" fn(*mut T),
        type_name: &'static str,
    ) -> Result<Self> {
        Ok(Self {
            ptr: non_null_mut(ptr, type_name)?,
            destroy,
            _owns_handle: PhantomData,
        })
    }

    fn as_ptr(&self) -> *mut T {
        self.ptr.as_ptr()
    }

    fn as_non_null(&self) -> NonNull<T> {
        self.ptr
    }

    fn close(self) {
        drop(self);
    }
}

impl<T> Drop for NativeHandle<T> {
    fn drop(&mut self) {
        // SAFETY: NativeHandle is constructed only from an owned pointer and the
        // matching C API destroy function. Destroy functions for scoped handles
        // are void, noexcept, and accept null; ptr is non-null here.
        unsafe { (self.destroy)(self.ptr.as_ptr()) }
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
    use std::sync::atomic::{AtomicUsize, Ordering};

    use super::*;

    static DESTROY_COUNT: AtomicUsize = AtomicUsize::new(0);

    unsafe extern "C" fn count_destroy(ptr: *mut u8) {
        if !ptr.is_null() {
            DESTROY_COUNT.fetch_add(1, Ordering::SeqCst);
        }
    }

    #[test]
    fn native_handle_destroys_owned_pointer_on_drop() {
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
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        let mut value = 1u8;

        let handle =
            unsafe { NativeHandle::from_raw(&mut value, count_destroy, "test_handle") }.unwrap();
        handle.close();

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn native_handle_rejects_null() {
        let error =
            unsafe { NativeHandle::from_raw(ptr::null_mut::<u8>(), count_destroy, "test_handle") }
                .unwrap_err();

        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
    }
}
