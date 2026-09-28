// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

impl GeoJsonSourceDataHandle {
    /// Calls `mln_geojson_source_data_destroy` using its header execution and ownership contract.
    pub fn destroy(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.handle.close_with(|native| {
            unsafe { sys::mln_geojson_source_data_destroy(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }
}
