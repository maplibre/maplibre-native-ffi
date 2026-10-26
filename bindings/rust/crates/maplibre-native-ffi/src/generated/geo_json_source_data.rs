// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_geo_json_source_data` native handle.
    ///
    /// Prepared GeoJSON data that sources retain independently of its handle.
    ///
    /// See `mln_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct GeoJsonSourceDataHandle(mln_geo_json_source_data) dispose |raw| { unsafe { sys::mln_geo_json_source_data_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl GeoJsonSourceDataHandle {
    /// Prepares GeoJSON source data for installation on a map.
    ///
    /// See `mln_geo_json_source_data_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn create(
        data: &str,
        options: Option<&GeoJsonSourceOptions>,
    ) -> Result<GeoJsonSourceDataHandle> {
        let mut call = Call::global();
        maplibre_core::validate_abi_version()?;
        let mut out_data = sys::mln_geo_json_source_data(0);
        let data = call.input(&data)?;
        let options = call.optional_reference(options.as_ref())?;
        call.status(|_, out_diagnostic| unsafe {
            sys::mln_geo_json_source_data_create(data, options, &mut out_data, out_diagnostic)
        })?;
        Ok(GeoJsonSourceDataHandle::adopt(out_data, None)?)
    }

    /// Releases prepared GeoJSON source data.
    ///
    /// See `mln_geo_json_source_data_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.inner
            .close(|data| {
                let mut call = Call::new(data);
                call.run(|data| unsafe { sys::mln_geo_json_source_data_release(data) });
                Ok(())
            })
            .map_err(|error| (self, error))
    }
}
