#[allow(clippy::all, unused_parens)]
mod generated;

use std::fmt;
use std::sync::Arc;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::events::MapId;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};
use crate::runtime::RuntimeState;
use crate::{NativeFuture, Result};

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ConcurrentNativeHandle<sys::mln_map>,
    _runtime: Arc<RuntimeState>,
    id: MapId,
}

impl MapState {
    fn new(native: sys::mln_map, runtime: Arc<RuntimeState>, id: MapId) -> Result<Self> {
        // SAFETY: native came from a successful typed creation take and map
        // control state supports calls from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_map") }?;
        Ok(Self {
            handle,
            _runtime: runtime,
            id,
        })
    }

    pub(crate) fn native(&self) -> Result<sys::mln_map> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("MapHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }
}

impl Drop for MapState {
    fn drop(&mut self) {
        self.handle
            .finalize_with(|handle| unsafe { maplibre_core::generated::map_dispose(handle) });
    }
}

/// Any-thread map control handle.
///
/// An attached render session holds its own reference to this map's native
/// state, so the map outlives the session even when the host drops its handle
/// first.
pub struct MapHandle {
    pub(crate) inner: Arc<MapState>,
}

impl fmt::Debug for MapHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapHandle {
    pub(crate) fn from_native(native: sys::mln_map, parent: Arc<RuntimeState>) -> Result<Self> {
        Ok(Self {
            inner: Arc::new(MapState::new(native, parent, MapId::new(native.0))?),
        })
    }

    pub fn id(&self) -> MapId {
        self.inner.id
    }

    #[cfg(test)]
    pub(crate) fn close_and_wait(self) {
        let completion = self.release().expect("native close submission failed");
        crate::completion::blocking(Ok(completion));
    }
}

#[cfg(test)]
mod tests;
