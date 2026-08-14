use std::fmt;
use std::ptr;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use maplibre_core::style::{NativeGeoJsonSourceOptions, geojson_source_options_to_native};

use crate::{GeoJsonSourceOptions, Result};

/// Owned handle for prepared GeoJSON source data.
///
/// [`Self::new`] parses one complete UTF-8 GeoJSON document and tiles or
/// clusters it into the index a GeoJSON source consumes, which is the
/// expensive part of a data update. It needs no runtime or map and runs on
/// any thread, so a host prepares data on a worker thread and installs it on
/// the map owner thread through
/// [`crate::MapHandle::add_geojson_source_data`] or
/// [`crate::MapHandle::set_geojson_source_data`].
///
/// The options are baked into the prepared data and must match the options of
/// every source the data is installed on. Installing borrows the handle, so
/// one prepared value may be installed on any number of sources; dropping or
/// closing it afterwards never invalidates a source, because sources keep
/// their own reference.
pub struct GeoJsonSourceDataHandle {
    handle: sys::mln_geojson_source_data,
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
    /// Prepares GeoJSON source data. `options` may be `None` for defaults.
    ///
    /// When `options` enable clustering, the data must be a feature collection
    /// whose every feature carries point geometry; anything else is rejected
    /// with an invalid-argument error naming the constraint.
    pub fn new(data: &[u8], options: Option<&GeoJsonSourceOptions>) -> Result<Self> {
        let data = maplibre_core::string::buffer_view(data);
        let options = options.map(geojson_source_options_to_native).transpose()?;
        let options_ptr = options
            .as_ref()
            .map_or(ptr::null(), NativeGeoJsonSourceOptions::as_ptr);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_geojson_source_data>::new();
        // SAFETY: data and the optional native options are valid for this call,
        // and out is a null-initialized out-pointer owned by this call.
        maplibre_core::check(unsafe {
            sys::mln_geojson_source_data_create(data, options_ptr, out.as_mut_ptr())
        })?;
        Ok(Self {
            handle: out.into_live("mln_geojson_source_data")?,
        })
    }

    /// The live native handle, borrowed by map install calls.
    pub(crate) fn native(&self) -> sys::mln_geojson_source_data {
        self.handle
    }

    /// Releases the prepared data. Sources it was installed on keep their own
    /// reference and stay valid. Dropping the handle releases it the same way.
    pub fn close(self) {
        drop(self);
    }
}

impl Drop for GeoJsonSourceDataHandle {
    fn drop(&mut self) {
        // SAFETY: handle is the owned live handle this wrapper was constructed
        // with; ownership makes this the only release, and the C API accepts
        // release from any thread.
        unsafe { sys::mln_geojson_source_data_destroy(self.handle) };
    }
}
