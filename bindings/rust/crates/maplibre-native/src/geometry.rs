pub use maplibre_native_core::geometry::Geometry;

pub(crate) use maplibre_native_core::geometry::NativeGeometry;

use crate::Result;

pub(crate) trait GeometryNativeExt {
    fn try_to_native(&self) -> Result<NativeGeometry>;
}

impl GeometryNativeExt for Geometry {
    fn try_to_native(&self) -> Result<NativeGeometry> {
        maplibre_native_core::geometry::geometry_try_to_native(self)
    }
}
