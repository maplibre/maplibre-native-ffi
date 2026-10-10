// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_geojson_source_data` native handle.
    pub struct GeojsonSourceDataHandle(mln_geojson_source_data) dispose |raw| { unsafe { sys::mln_geojson_source_data_destroy(raw) }; Ok(()) };
}

impl GeojsonSourceDataHandle {
    /// Releases prepared GeoJSON source data.
    ///
    /// See `mln_geojson_source_data_destroy` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn destroy(&self) -> Result<()> {
        self.inner.close(|data| {
            let mut call = Call::new(data, None);
            call.run(|data| unsafe { sys::mln_geojson_source_data_destroy(data) });
            Ok(())
        })
    }
}
