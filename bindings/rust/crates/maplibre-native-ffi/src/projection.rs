#[allow(clippy::all, unused_parens)]
mod generated;

use std::fmt;
use std::sync::Arc;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::Result;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};

#[derive(Debug)]
pub(crate) struct MapProjectionState {
    handle: ConcurrentNativeHandle<sys::mln_map_projection>,
}

impl MapProjectionState {
    fn new(native: sys::mln_map_projection) -> Result<Self> {
        // SAFETY: native came from the typed creation take and projection
        // control state supports calls from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_map_projection") }?;
        Ok(Self { handle })
    }

    fn native(&self) -> Result<sys::mln_map_projection> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("MapProjectionHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }
}

impl Drop for MapProjectionState {
    fn drop(&mut self) {
        self.handle.finalize_with(|handle| unsafe {
            maplibre_core::check(sys::mln_map_projection_close(handle))
        });
    }
}

/// Any-thread standalone projection snapshot created from a map transform.
///
/// Every call after creation is synchronous, runs on the calling thread, and
/// is internally serialized, so a projection is usable from any thread. A
/// projection copies the map transform once at creation and never observes
/// map changes made after it and remains usable after that map and its runtime
/// close.
pub struct MapProjectionHandle {
    inner: Arc<MapProjectionState>,
}

impl fmt::Debug for MapProjectionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapProjectionHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapProjectionHandle {
    pub(crate) fn from_native(native: sys::mln_map_projection) -> Result<Self> {
        Ok(Self {
            inner: Arc::new(MapProjectionState::new(native)?),
        })
    }
}
