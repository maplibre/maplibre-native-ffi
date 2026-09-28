#[allow(clippy::all, unused_parens)]
mod generated;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};
use crate::{NativeFuture, Result};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;
use std::fmt;
use std::sync::Arc;

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ConcurrentNativeHandle<sys::mln_runtime>,
}
impl RuntimeState {
    fn new(native: sys::mln_runtime) -> Result<Self> {
        Ok(Self {
            handle: unsafe { ConcurrentNativeHandle::from_handle(native, "mln_runtime") }?,
        })
    }
    pub(crate) fn native(&self) -> Result<sys::mln_runtime> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RuntimeHandle"))
    }
}
impl Drop for RuntimeState {
    fn drop(&mut self) {
        self.handle
            .finalize_with(|handle| unsafe { maplibre_core::generated::runtime_dispose(handle) });
    }
}
/// Any-thread runtime handle backed by a native worker.
pub struct RuntimeHandle {
    pub(crate) inner: Arc<RuntimeState>,
}
impl fmt::Debug for RuntimeHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeHandle")
            .field("closed", &self.inner.handle.is_closed())
            .finish()
    }
}
impl RuntimeHandle {
    pub(crate) fn from_native(native: sys::mln_runtime) -> Result<Self> {
        Ok(Self {
            inner: Arc::new(RuntimeState::new(native)?),
        })
    }
}
