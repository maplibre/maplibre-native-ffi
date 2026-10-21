// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_geojson_source_data` native handle.
    ///
    /// Prepared GeoJSON data that sources retain independently of its handle.
    ///
    /// See `mln_geojson_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct GeojsonSourceDataHandle(mln_geojson_source_data) dispose |raw| { unsafe { sys::mln_geojson_source_data_release(raw) }; Ok(()) }, counted false;
}

impl GeojsonSourceDataHandle {
    /// Releases prepared GeoJSON source data.
    ///
    /// See `mln_geojson_source_data_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn release(&self) -> Result<()> {
        self.inner.close(|data| {
            let mut call = Call::new(data, None);
            call.run(|data| unsafe { sys::mln_geojson_source_data_release(data) });
            Ok(())
        })
    }
}
