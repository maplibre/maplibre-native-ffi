pub use maplibre_native_core::geojson::{Feature, FeatureIdentifier, GeoJson};

pub(crate) use maplibre_native_core::geojson::{NativeFeature, NativeGeoJson};

use crate::Result;
use crate::sys;

pub(crate) trait FeatureNativeExt {
    fn try_to_native(&self, depth: usize) -> Result<NativeFeature>;

    /// Copies a borrowed native feature descriptor into an owned Rust value.
    ///
    /// # Safety
    ///
    /// `raw` and all nested pointers must be valid for the duration of this call.
    unsafe fn from_native(raw: &sys::mln_feature, depth: usize) -> Result<Feature>;
}

impl FeatureNativeExt for Feature {
    fn try_to_native(&self, depth: usize) -> Result<NativeFeature> {
        maplibre_native_core::geojson::feature_try_to_native(self, depth)
    }

    unsafe fn from_native(raw: &sys::mln_feature, depth: usize) -> Result<Feature> {
        // SAFETY: The caller promises raw and nested pointers are valid for this call.
        unsafe { maplibre_native_core::geojson::feature_from_native(raw, depth) }
    }
}

pub(crate) trait GeoJsonNativeExt {
    fn try_to_native(&self) -> Result<NativeGeoJson>;
}

impl GeoJsonNativeExt for GeoJson {
    fn try_to_native(&self) -> Result<NativeGeoJson> {
        maplibre_native_core::geojson::geojson_try_to_native(self)
    }
}
