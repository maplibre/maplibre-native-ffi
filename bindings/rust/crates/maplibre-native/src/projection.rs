use std::fmt;

use maplibre_native_core as maplibre_core;
use maplibre_native_core::ptr::const_ptr_or_null;
use maplibre_native_core::values::{empty_lat_lng, empty_screen_point, lat_lngs_to_native};
use maplibre_native_sys as sys;

use crate::camera::CameraOptionsNativeExt;
use crate::geometry::GeometryNativeExt;
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::values::NativeValue;
use crate::{
    CameraOptions, EdgeInsets, Error, Geometry, HandleOperationError, LatLng, MapHandle, Result,
    ScreenPoint,
};

#[derive(Debug)]
pub(crate) struct MapProjectionState {
    handle: ThreadAffineNativeHandle<sys::mln_map_projection>,
}

impl MapProjectionState {
    fn new(ptr: std::ptr::NonNull<sys::mln_map_projection>) -> Self {
        // SAFETY: ptr came from successful mln_map_projection_create and is
        // paired with the matching projection destroy function.
        let handle = unsafe {
            ThreadAffineNativeHandle::from_raw(
                ptr,
                sys::mln_map_projection_destroy,
                "mln_map_projection",
            )
        };
        Self { handle }
    }

    fn as_ptr(&self) -> Result<*mut sys::mln_map_projection> {
        let ptr = self.handle.as_ptr();
        if ptr.is_null() {
            Err(closed_handle_error("MapProjectionHandle"))
        } else {
            Ok(ptr)
        }
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        self.handle.close()
    }
}

/// Standalone projection snapshot created from a map transform.
///
/// The projection does not retain the source map after creation. It remains
/// thread-affine and must be used and closed on its owner thread.
pub struct MapProjectionHandle {
    inner: MapProjectionState,
}

impl fmt::Debug for MapProjectionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapProjectionHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapProjectionHandle {
    pub(crate) fn new(map: &MapHandle) -> Result<Self> {
        let map_ptr = map.inner.as_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_map_projection>::new();
        // SAFETY: map_ptr is a live map handle. out is a valid null-initialized
        // out-pointer owned by this call.
        maplibre_core::check(unsafe { sys::mln_map_projection_create(map_ptr, out.as_mut_ptr()) })?;
        let ptr = out_handle(out, "mln_map_projection")?;
        Ok(Self {
            inner: MapProjectionState::new(ptr),
        })
    }

    /// Explicitly destroys the projection snapshot.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }

    /// Reads the projection helper's current camera snapshot.
    pub fn camera(&self) -> Result<CameraOptions> {
        let projection = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_camera_options_default() };
        // SAFETY: projection is live and raw has a valid size field for C to fill.
        maplibre_core::check(unsafe { sys::mln_map_projection_get_camera(projection, &mut raw) })?;
        Ok(CameraOptions::from_native(raw))
    }

    /// Applies camera fields to this projection helper.
    pub fn set_camera(&self, camera: &CameraOptions) -> Result<()> {
        let projection = self.inner.as_ptr()?;
        let raw = camera.to_native();
        // SAFETY: projection is live and raw is a materialized descriptor valid
        // for the duration of this call.
        maplibre_core::check(unsafe { sys::mln_map_projection_set_camera(projection, &raw) })
    }

    /// Updates the projection camera so coordinates are visible within padding.
    pub fn set_visible_coordinates(
        &self,
        coordinates: &[LatLng],
        padding: EdgeInsets,
    ) -> Result<()> {
        let projection = self.inner.as_ptr()?;
        if coordinates.is_empty() {
            return Err(Error::invalid_argument(
                "set_visible_coordinates requires at least one coordinate",
            ));
        }
        let raw_coordinates = lat_lngs_to_native(coordinates);
        // SAFETY: projection is live. coordinates points to coordinate_count
        // non-empty entries. padding is passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                projection,
                const_ptr_or_null(&raw_coordinates),
                raw_coordinates.len(),
                padding.to_native(),
            )
        })
    }

    /// Updates the projection camera so geometry coordinates are visible.
    pub fn set_visible_geometry(&self, geometry: &Geometry, padding: EdgeInsets) -> Result<()> {
        let projection = self.inner.as_ptr()?;
        let native_geometry = geometry.try_to_native()?;
        // SAFETY: projection is live, native_geometry owns backing storage for
        // the duration of this call, and padding is passed by value.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_geometry(
                projection,
                native_geometry.as_ptr(),
                padding.to_native(),
            )
        })
    }

    /// Converts a geographic world coordinate to a screen point.
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let projection = self.inner.as_ptr()?;
        let mut raw_point = empty_screen_point();
        // SAFETY: projection is live, coordinate is passed by value, and
        // raw_point is writable output storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_pixel_for_lat_lng(
                projection,
                coordinate.to_native(),
                &mut raw_point,
            )
        })?;
        Ok(ScreenPoint::from_native(raw_point))
    }

    /// Converts a screen point to a geographic world coordinate.
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        let projection = self.inner.as_ptr()?;
        let mut raw_coordinate = empty_lat_lng();
        // SAFETY: projection is live, point is passed by value, and
        // raw_coordinate is writable output storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_lat_lng_for_pixel(
                projection,
                point.to_native(),
                &mut raw_coordinate,
            )
        })?;
        Ok(LatLng::from_native(raw_coordinate))
    }
}

#[cfg(test)]
mod tests {
    use static_assertions::assert_not_impl_any;

    use super::*;
    use crate::{ErrorKind, MapOptions, RuntimeHandle};

    assert_not_impl_any!(MapProjectionHandle: Send, Sync);

    #[test]
    fn projection_create_round_trip_close_and_stays_live_after_map_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime
            .create_map_with_options(&MapOptions::new(512, 512, 1.0))
            .unwrap();
        let center = LatLng::new(37.7749, -122.4194);
        map.jump_to(&CameraOptions::new().with_center(center).with_zoom(5.0))
            .unwrap();

        let projection = map.create_projection().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();

        let point = projection.pixel_for_lat_lng(center).unwrap();
        let round_tripped = projection.lat_lng_for_pixel(point).unwrap();
        assert!((round_tripped.latitude - center.latitude).abs() < 1e-7);
        assert!((round_tripped.longitude - center.longitude).abs() < 1e-7);

        projection.close().unwrap();
    }

    #[test]
    fn projection_drops_without_explicit_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();

        {
            let _projection = map.create_projection().unwrap();
        }

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn projection_camera_and_visible_region_helpers_call_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();
        let projection = map.create_projection().unwrap();

        projection
            .set_camera(
                &CameraOptions::new()
                    .with_center(LatLng::new(0.0, 0.0))
                    .with_zoom(2.0),
            )
            .unwrap();
        let camera = projection.camera().unwrap();
        assert_eq!(camera.center, Some(LatLng::new(0.0, 0.0)));
        assert_eq!(camera.zoom, Some(2.0));

        let padding = EdgeInsets::new(0.0, 0.0, 0.0, 0.0);
        projection
            .set_visible_coordinates(&[LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)], padding)
            .unwrap();
        let error = projection
            .set_visible_coordinates(&[], padding)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("at least one coordinate"));
        projection
            .set_visible_geometry(
                &Geometry::LineString(vec![LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)]),
                padding,
            )
            .unwrap();

        projection.close().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }
}
