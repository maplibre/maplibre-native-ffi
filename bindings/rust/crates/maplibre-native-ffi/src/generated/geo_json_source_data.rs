// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// GeoJSON data, parsed and ready to add to a GeoJSON source.
    ///
    /// See `mln_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct GeoJsonSourceDataHandle(mln_geo_json_source_data) dispose |raw| { unsafe { sys::mln_geo_json_source_data_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl GeoJsonSourceDataHandle {
    /// Prepares GeoJSON data for GeoJSON sources.
    ///
    /// `data` is a GeoJSON feature collection, feature, or geometry. Preparing
    /// it can be slow, but adding the result to a source is quick, and several
    /// sources can share it.
    ///
    /// See `mln_geo_json_source_data_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when `data` isn't valid GeoJSON, or when
    ///   `options` turns on clustering and `data` isn't a feature collection of
    ///   points.
    pub fn create(
        data: &str,
        options: Option<&GeoJsonSourceOptions<'_>>,
    ) -> Result<GeoJsonSourceDataHandle> {
        let mut call = Call::global();
        crate::abi::validate_abi_version()?;
        let mut out_data = sys::mln_geo_json_source_data(0);
        let data = call.input(&data)?;
        let options = call.optional_reference(options.as_ref())?;
        call.status(|_, out_diagnostic| unsafe {
            sys::mln_geo_json_source_data_create(data, options, &mut out_data, out_diagnostic)
        })?;
        Ok(GeoJsonSourceDataHandle::adopt(out_data)?)
    }

    /// Releases prepared GeoJSON data.
    ///
    /// Sources that use the data keep it.
    ///
    /// See `mln_geo_json_source_data_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|data| {
            let mut call = Call::new(data);
            call.run(|data| unsafe { sys::mln_geo_json_source_data_release(data) });
            Ok(())
        })
    }
}
