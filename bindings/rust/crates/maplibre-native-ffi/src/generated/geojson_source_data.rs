// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_geojson_source_data` native handle.
    pub struct GeojsonSourceDataHandle(mln_geojson_source_data) dispose |raw| { unsafe { sys::mln_geojson_source_data_destroy(raw) }; Ok(()) };
}

impl GeojsonSourceDataHandle {
    /// Calls `mln_geojson_source_data_destroy`.
    pub fn destroy(&self) -> Result<()> {
        self.inner.close(|data| {
            let mut call = Call::new(data, None);
            call.run(|data| unsafe { sys::mln_geojson_source_data_destroy(data) });
            Ok(())
        })
    }
}
