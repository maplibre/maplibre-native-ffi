use std::cell::Cell;
use std::marker::PhantomData;
use std::ptr::NonNull;
use std::rc::Rc;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::{Error, Result};

#[derive(Debug)]
pub(crate) struct ThreadAffineNativeHandle<T> {
    ptr: Cell<Option<NonNull<T>>>,
    destroy: unsafe extern "C" fn(*mut T) -> sys::mln_status,
    _thread_affine: PhantomData<Rc<()>>,
}

impl<T> ThreadAffineNativeHandle<T> {
    /// Takes ownership of a native thread-affine handle pointer.
    ///
    /// # Safety
    ///
    /// `ptr` must be a non-null owned live handle of the matching native type.
    /// `destroy` must be the C API function that releases exactly that handle
    /// type and returns a status without taking ownership on failure.
    pub(crate) unsafe fn from_raw(
        ptr: NonNull<T>,
        destroy: unsafe extern "C" fn(*mut T) -> sys::mln_status,
        _type_name: &'static str,
    ) -> Self {
        Self {
            ptr: Cell::new(Some(ptr)),
            destroy,
            _thread_affine: PhantomData,
        }
    }

    pub(crate) fn as_ptr(&self) -> *mut T {
        self.ptr.get().map_or(std::ptr::null_mut(), NonNull::as_ptr)
    }

    pub(crate) fn is_closed(&self) -> bool {
        self.ptr.get().is_none()
    }

    pub(crate) fn close(&self) -> Result<()> {
        let Some(ptr) = self.ptr.get() else {
            return Ok(());
        };

        // SAFETY: ptr is a live owned handle while stored in self.ptr, and
        // destroy is the matching C API destroy function for T.
        let status = unsafe { (self.destroy)(ptr.as_ptr()) };
        if status == sys::MLN_STATUS_OK {
            self.ptr.set(None);
            Ok(())
        } else {
            Err(Error::from_status(status))
        }
    }
}

impl<T> Drop for ThreadAffineNativeHandle<T> {
    fn drop(&mut self) {
        let Some(ptr) = self.ptr.get() else {
            return;
        };

        // SAFETY: ptr is a live owned handle while stored in self.ptr, and
        // destroy is the matching C API destroy function for T. Drop cannot
        // report errors, so it ignores non-OK statuses and never panics.
        let status = unsafe { (self.destroy)(ptr.as_ptr()) };
        if status == sys::MLN_STATUS_OK {
            self.ptr.set(None);
        }
    }
}

pub(crate) fn closed_handle_error(type_name: &'static str) -> Error {
    Error::invalid_argument(format!("{type_name} is closed"))
}

pub(crate) fn out_handle<T>(
    out: support::ptr::OutPtr<T>,
    type_name: &'static str,
) -> Result<NonNull<T>> {
    out.into_non_null(type_name)
}

#[cfg(test)]
mod tests {
    use std::ptr;
    use std::sync::Mutex;
    use std::sync::atomic::{AtomicI32, AtomicUsize, Ordering};

    use super::*;

    static DESTROY_COUNT: AtomicUsize = AtomicUsize::new(0);
    static DESTROY_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);
    static DESTROY_TEST_LOCK: Mutex<()> = Mutex::new(());

    unsafe extern "C" fn count_destroy(ptr: *mut u8) -> sys::mln_status {
        if !ptr.is_null() {
            DESTROY_COUNT.fetch_add(1, Ordering::SeqCst);
        }
        DESTROY_STATUS.load(Ordering::SeqCst)
    }

    fn test_handle(value: &mut u8) -> ThreadAffineNativeHandle<u8> {
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        DESTROY_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        let ptr = NonNull::from(value);

        // SAFETY: ptr points to test storage that remains live for the test,
        // and count_destroy only records calls.
        unsafe { ThreadAffineNativeHandle::from_raw(ptr, count_destroy, "test_handle") }
    }

    #[test]
    fn close_is_internally_idempotent_after_success() {
        let _guard = DESTROY_TEST_LOCK.lock().unwrap();
        let mut value = 1u8;
        let handle = test_handle(&mut value);

        handle.close().unwrap();
        handle.close().unwrap();

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
        assert!(handle.as_ptr().is_null());
    }

    #[test]
    fn failed_close_leaves_handle_live_for_later_close() {
        let _guard = DESTROY_TEST_LOCK.lock().unwrap();
        let mut value = 1u8;
        let handle = test_handle(&mut value);
        DESTROY_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);

        let error = handle.close().unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidState);
        assert_eq!(handle.as_ptr().cast_const(), ptr::addr_of!(value));

        DESTROY_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        handle.close().unwrap();

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 2);
        assert!(handle.as_ptr().is_null());
    }
}
