#[allow(clippy::all, unused_parens)]
mod generated;
use crate::handle::ConcurrentNativeHandle;
use std::fmt;

use maplibre_native_ffi_sys as sys;

use crate::Result;

/// Owned handle for prepared GeoJSON source data.
///
/// [`crate::geojson_source_data_create`] parses one complete UTF-8 GeoJSON document and tiles or
/// clusters it into the index a GeoJSON source consumes, which is the
/// expensive part of a data update. It needs no runtime or map and runs on
/// any thread, so a host can prepare data concurrently with map work and
/// install it through
/// [`crate::MapHandle::add_geojson_source_data`] or
/// [`crate::MapHandle::set_geojson_source_data`].
///
/// The options are baked into the prepared data and must match the options of
/// every source the data is installed on. Installing borrows the handle, so
/// one prepared value may be installed on any number of sources; dropping or
/// closing it afterwards never invalidates a source, because sources keep
/// their own reference.
pub struct GeoJsonSourceDataHandle {
    handle: ConcurrentNativeHandle<sys::mln_geojson_source_data>,
}

// SAFETY: The prepared native data is immutable, and the C API documents
// create, read, and destroy as callable from any thread.
unsafe impl Send for GeoJsonSourceDataHandle {}
// SAFETY: Shared reads only pass the immutable handle id across the C
// boundary, and release requires exclusive ownership (`Drop` or `close`).
unsafe impl Sync for GeoJsonSourceDataHandle {}

impl fmt::Debug for GeoJsonSourceDataHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("GeoJsonSourceDataHandle")
            .finish_non_exhaustive()
    }
}

impl GeoJsonSourceDataHandle {
    pub(crate) fn from_native(native: sys::mln_geojson_source_data) -> Result<Self> {
        if native.0 == 0 {
            return Err(crate::Error::invalid_argument("native owner is null"));
        }
        Ok(Self {
            handle: unsafe {
                ConcurrentNativeHandle::from_handle(native, "mln_geojson_source_data")
            }?,
        })
    }

    pub(crate) fn native(&self) -> Result<sys::mln_geojson_source_data> {
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("GeoJsonSourceDataHandle"))
    }
}

impl Drop for GeoJsonSourceDataHandle {
    fn drop(&mut self) {
        self.handle.finalize_with(|handle| {
            unsafe { sys::mln_geojson_source_data_destroy(handle) };
            Ok(())
        });
    }
}
