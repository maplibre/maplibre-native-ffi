pub use maplibre_native_core::geojson::{Feature, FeatureIdentifier, GeoJson};

pub(crate) use maplibre_native_core::geojson::{NativeFeature, NativeGeoJson};

use crate::Result;

pub(crate) trait FeatureNativeExt {
    fn try_to_native(&self, depth: usize) -> Result<NativeFeature>;
}

impl FeatureNativeExt for Feature {
    fn try_to_native(&self, depth: usize) -> Result<NativeFeature> {
        maplibre_native_core::geojson::feature_try_to_native(self, depth)
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
