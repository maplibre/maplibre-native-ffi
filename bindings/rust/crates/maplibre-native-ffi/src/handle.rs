use std::marker::PhantomData;
use std::rc::Rc;

use maplibre_native_ffi_core::{
    self as maplibre_core,
    handle::{NativeHandle, NativeHandleState},
};
use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};

#[derive(Debug)]
pub(crate) struct ThreadAffineNativeHandle<T: NativeHandle> {
    state: NativeHandleState<T>,
    destroy: unsafe extern "C" fn(T) -> sys::mln_status,
    _thread_affine: PhantomData<Rc<()>>,
}

impl<T: NativeHandle> ThreadAffineNativeHandle<T> {
    /// Takes ownership of a native thread-affine handle.
    ///
    /// # Safety
    ///
    /// `handle` must be a live handle of the matching native type owned by the
    /// caller. `destroy` must be the C API function that releases exactly that
    /// handle type and returns a status without taking ownership on failure.
    pub(crate) unsafe fn from_handle(
        handle: T,
        destroy: unsafe extern "C" fn(T) -> sys::mln_status,
        type_name: &'static str,
    ) -> Result<Self> {
        Ok(Self {
            // SAFETY: The caller promises handle is an owned live handle of the
            // matching native type.
            state: unsafe { NativeHandleState::from_handle(handle, type_name) }?,
            destroy,
            _thread_affine: PhantomData,
        })
    }

    pub(crate) fn live_handle(&self) -> Option<T> {
        self.state.live_handle()
    }

    pub(crate) fn is_closed(&self) -> bool {
        self.state.is_closed()
    }

    pub(crate) fn close(&self) -> Result<()> {
        // SAFETY: from_handle binds this Rust handle to the matching C API
        // destroy function for its owned native handle.
        unsafe { self.state.close_status(self.destroy) }
    }
}

impl<T: NativeHandle> Drop for ThreadAffineNativeHandle<T> {
    fn drop(&mut self) {
        // Drop cannot return an error and must not panic, so a destroy that
        // reports failure leaves the handle live and goes to the leak channel.
        let id = self.state.id().unwrap_or_default();
        // SAFETY: from_handle binds this Rust handle to the matching C API
        // destroy function for its owned native handle.
        if unsafe { self.state.close_status(self.destroy) }.is_err() {
            maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                type_name: self.state.type_name(),
                id,
            });
        }
    }
}

#[derive(Debug)]
pub(crate) struct ConcurrentNativeHandle<T: NativeHandle> {
    handle: std::sync::Mutex<Option<T>>,
    type_name: &'static str,
}

impl<T: NativeHandle> ConcurrentNativeHandle<T> {
    /// Takes ownership of a native handle whose registry and control state are
    /// safe to inspect from any thread.
    ///
    /// # Safety
    ///
    /// `handle` must be a live owned handle of the matching native type.
    pub(crate) unsafe fn from_handle(handle: T, type_name: &'static str) -> Result<Self> {
        if handle.to_raw() == 0 {
            return Err(Error::invalid_argument(format!(
                "{type_name} handle must not be zero"
            )));
        }
        Ok(Self {
            handle: std::sync::Mutex::new(Some(handle)),
            type_name,
        })
    }

    pub(crate) fn live_handle(&self) -> Option<T> {
        *self
            .handle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }

    pub(crate) fn is_closed(&self) -> bool {
        self.live_handle().is_none()
    }

    pub(crate) fn mark_closed(&self) {
        self.handle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
    }

    pub(crate) fn leak_for_report(&self) {
        let Some(handle) = self
            .handle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take()
        else {
            return;
        };
        maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
            type_name: self.type_name,
            id: handle.to_raw(),
        });
    }
}

pub(crate) fn closed_handle_error(type_name: &'static str) -> Error {
    Error::invalid_argument(format!("{type_name} is closed"))
}

pub(crate) fn out_handle<T: NativeHandle>(
    out: maplibre_core::ptr::OutHandle<T>,
    type_name: &'static str,
) -> Result<T> {
    out.into_live(type_name)
}

#[cfg(test)]
mod tests {
    use std::sync::Mutex;
    use std::sync::atomic::{AtomicI32, AtomicUsize, Ordering};

    use super::*;

    static DESTROY_COUNT: AtomicUsize = AtomicUsize::new(0);
    static DESTROY_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);
    static DESTROY_TEST_LOCK: Mutex<()> = Mutex::new(());

    /// A synthetic map handle for close-once tests. It reaches only
    /// `count_destroy` below, never the C API. The kind byte matches a map so
    /// a value escaping into a diagnostic reads as one.
    const TEST_HANDLE: sys::mln_map = sys::mln_map(0x0200_0000_0000_002a);

    unsafe extern "C" fn count_destroy(handle: sys::mln_map) -> sys::mln_status {
        if handle.0 != 0 {
            DESTROY_COUNT.fetch_add(1, Ordering::SeqCst);
        }
        DESTROY_STATUS.load(Ordering::SeqCst)
    }

    fn test_handle() -> ThreadAffineNativeHandle<sys::mln_map> {
        DESTROY_COUNT.store(0, Ordering::SeqCst);
        DESTROY_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);

        // SAFETY: count_destroy only records calls and never reaches native.
        unsafe { ThreadAffineNativeHandle::from_handle(TEST_HANDLE, count_destroy, "test_handle") }
            .unwrap()
    }

    #[test]
    // Spec coverage: BND-040.
    fn close_is_internally_idempotent_after_success() {
        let _guard = DESTROY_TEST_LOCK.lock().unwrap();
        let handle = test_handle();

        handle.close().unwrap();
        handle.close().unwrap();

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 1);
        assert!(handle.live_handle().is_none());
    }

    #[test]
    // Spec coverage: BND-041.
    fn failed_close_leaves_handle_live_for_later_close() {
        let _guard = DESTROY_TEST_LOCK.lock().unwrap();
        let handle = test_handle();
        DESTROY_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);

        let error = handle.close().unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidState);
        assert_eq!(handle.live_handle().unwrap().0, TEST_HANDLE.0);

        DESTROY_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        handle.close().unwrap();

        assert_eq!(DESTROY_COUNT.load(Ordering::SeqCst), 2);
        assert!(handle.live_handle().is_none());
    }
}
