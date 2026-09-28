// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct GeojsonSourceDataHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_geojson_source_data>,
    id: u64,
}
impl GeojsonSourceDataHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_geojson_source_data> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("GeojsonSourceDataHandle"))
    }
}
impl Drop for GeojsonSourceDataHandleState {
    fn drop(&mut self) {
        self.handle.finalize_with(|raw| {
            unsafe { sys::mln_geojson_source_data_destroy(raw) };
            Ok(())
        });
    }
}
/// Owns one `mln_geojson_source_data` native handle.
pub struct GeojsonSourceDataHandle {
    pub(crate) inner: std::sync::Arc<GeojsonSourceDataHandleState>,
}
impl std::fmt::Debug for GeojsonSourceDataHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("GeojsonSourceDataHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl GeojsonSourceDataHandle {
    pub(crate) fn from_native(raw: sys::mln_geojson_source_data) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle = unsafe {
            crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_geojson_source_data")
        }?;
        Ok(Self {
            inner: std::sync::Arc::new(GeojsonSourceDataHandleState { handle, id: raw.0 }),
        })
    }

    /// Returns the native handle value, which event sources report for this handle.
    pub fn id(&self) -> u64 {
        self.inner.id
    }

    /// Reports whether an explicit release, close, or disposal consumed this handle.
    pub fn is_closed(&self) -> bool {
        self.inner.handle.is_closed()
    }
}

impl GeojsonSourceDataHandle {
    /// Calls `mln_geojson_source_data_destroy` using its header execution and ownership contract.
    pub fn destroy(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            unsafe { sys::mln_geojson_source_data_destroy(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }
}
