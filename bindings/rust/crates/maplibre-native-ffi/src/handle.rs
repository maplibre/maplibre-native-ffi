use maplibre_native_ffi_core::{self as maplibre_core, handle::NativeHandle};

use crate::{Error, Result};

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
