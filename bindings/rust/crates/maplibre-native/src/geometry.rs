pub use maplibre_native_core::geometry::Geometry;

pub(crate) use maplibre_native_core::geometry::NativeGeometry;

use crate::Result;
use crate::sys;

pub(crate) trait GeometryNativeExt {
    fn try_to_native(&self) -> Result<NativeGeometry>;

    /// Copies a borrowed native geometry descriptor into an owned Rust value.
    ///
    /// # Safety
    ///
    /// `raw` and all nested pointers must be valid for the duration of this call.
    unsafe fn from_native_with_depth(raw: &sys::mln_geometry, depth: usize) -> Result<Geometry>;
}

impl GeometryNativeExt for Geometry {
    fn try_to_native(&self) -> Result<NativeGeometry> {
        maplibre_native_core::geometry::geometry_try_to_native(self)
    }

    unsafe fn from_native_with_depth(raw: &sys::mln_geometry, depth: usize) -> Result<Geometry> {
        // SAFETY: The caller promises raw and nested pointers are valid for this call.
        unsafe { maplibre_native_core::geometry::geometry_from_native_with_depth(raw, depth) }
    }
}
