use std::fmt;
use std::sync::Arc;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::ptr::const_ptr_or_null;
use maplibre_native_ffi_core::values::lat_lngs_to_native;
use maplibre_native_ffi_sys as sys;

use crate::camera::CameraOptionsNativeExt;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};
use crate::values::NativeValue;
use crate::{
    CameraOptions, EdgeInsets, Error, HandleOperationError, LatLng, MapHandle, NativeFuture,
    Result, ScreenPoint,
};

#[derive(Debug)]
pub(crate) struct MapProjectionState {
    handle: ConcurrentNativeHandle<sys::mln_map_projection>,
}

impl MapProjectionState {
    fn new(native: sys::mln_map_projection) -> Result<Self> {
        // SAFETY: native came from the typed creation take and projection
        // control state supports calls from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_map_projection") }?;
        Ok(Self { handle })
    }

    fn native(&self) -> Result<sys::mln_map_projection> {
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("MapProjectionHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        self.handle.close_with(|projection| {
            // SAFETY: projection is live; the synchronous close waits for
            // calls already running on other threads before it retires the
            // handle.
            maplibre_core::check(unsafe { sys::mln_map_projection_close(projection) })
        })?;
        Ok(())
    }
}

impl Drop for MapProjectionState {
    fn drop(&mut self) {
        if self.close().is_err() {
            self.handle.leak_for_report();
        }
    }
}

/// Any-thread standalone projection snapshot created from a map transform.
///
/// Every call after creation is synchronous, runs on the calling thread, and
/// is internally serialized, so a projection is usable from any thread. A
/// projection copies the map transform once at creation and never observes
/// map changes made after it and remains usable after that map and its runtime
/// close.
pub struct MapProjectionHandle {
    inner: Arc<MapProjectionState>,
}

impl fmt::Debug for MapProjectionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapProjectionHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapProjectionHandle {
    pub(crate) fn new(map: &MapHandle) -> Result<NativeFuture<Self>> {
        let map_ptr = map.inner.native()?;
        crate::completion::submit(
            // SAFETY: the handle is live and every borrowed argument stays valid for this
            // synchronous submission.
            move |completion| unsafe { sys::mln_map_projection_create(map_ptr, completion) },
            |result| {
                let native = crate::completion::copy_value::<sys::mln_map_projection>(result)?;
                Ok(Self {
                    inner: Arc::new(MapProjectionState::new(native)?),
                })
            },
        )
    }

    /// Closes the projection synchronously.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }

    /// Copies the projection camera, observing every earlier setter.
    pub fn camera(&self) -> Result<CameraOptions> {
        // SAFETY: the constructor initializes this ABI version's descriptor.
        let mut raw = unsafe { sys::mln_camera_options_default() };
        // SAFETY: projection is live and raw is size-tagged writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_get_camera(self.inner.native()?, &mut raw)
        })?;
        Ok(CameraOptions::from_native(raw))
    }

    /// Applies a camera update before returning; the map camera is unaffected.
    pub fn set_camera(&self, camera: &CameraOptions) -> Result<()> {
        // SAFETY: projection is live and the native camera struct is borrowed
        // for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_camera(self.inner.native()?, &camera.to_native())
        })
    }

    /// Applies a camera fit for geographic coordinates before returning.
    pub fn set_visible_coordinates(
        &self,
        coordinates: &[LatLng],
        padding: EdgeInsets,
    ) -> Result<()> {
        if coordinates.is_empty() {
            return Err(Error::invalid_argument(
                "set_visible_coordinates requires at least one coordinate",
            ));
        }
        let coordinates = lat_lngs_to_native(coordinates);
        // SAFETY: projection is live and coordinates points to call-scoped
        // native coordinate storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                self.inner.native()?,
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                padding.to_native(),
            )
        })
    }

    /// Applies a camera fit for GeoJSON Geometry bytes before returning.
    pub fn set_visible_geometry(&self, geometry: &[u8], padding: EdgeInsets) -> Result<()> {
        // SAFETY: projection is live and geometry is borrowed for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_geometry(
                self.inner.native()?,
                maplibre_core::string::buffer_view(geometry),
                padding.to_native(),
            )
        })
    }

    /// Converts a geographic coordinate to a screen point synchronously.
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let mut raw = sys::mln_screen_point { x: 0.0, y: 0.0 };
        // SAFETY: projection is live and raw is writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_pixel_for_lat_lng(
                self.inner.native()?,
                coordinate.to_native(),
                &mut raw,
            )
        })?;
        Ok(ScreenPoint::from_native(raw))
    }

    /// Converts a screen point to a geographic coordinate synchronously.
    ///
    /// The longitude is wrapped to the range from -180 to 180 degrees.
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        let mut raw = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        // SAFETY: projection is live and raw is writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_lat_lng_for_pixel(
                self.inner.native()?,
                point.to_native(),
                &mut raw,
            )
        })?;
        Ok(LatLng::from_native(raw))
    }

    /// Converts a screen point to an unwrapped geographic coordinate
    /// synchronously.
    ///
    /// The longitude preserves the visible world copy and may fall outside
    /// -180 to 180.
    pub fn lat_lng_for_pixel_unwrapped(&self, point: ScreenPoint) -> Result<LatLng> {
        let mut raw = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        // SAFETY: projection is live and raw is writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_lat_lng_for_pixel_unwrapped(
                self.inner.native()?,
                point.to_native(),
                &mut raw,
            )
        })?;
        Ok(LatLng::from_native(raw))
    }
}

#[cfg(test)]
mod tests {
    use static_assertions::assert_impl_all;

    use super::*;
    use crate::{ErrorKind, MapOptions, RuntimeHandle};

    assert_impl_all!(MapProjectionHandle: Send, Sync);

    #[test]
    // Spec coverage: BND-043 and BND-103.
    fn projection_observes_earlier_camera_commands_and_round_trips_synchronously() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(MapHandle::with_options(
            &runtime,
            &MapOptions::new(512, 512, 1.0),
        ));
        let center = LatLng::new(37.7749, -122.4194);
        let mut camera_options = CameraOptions::default();
        camera_options.center = Some(center);
        camera_options.zoom = Some(5.0);
        let mut update = crate::CameraUpdate::default();
        update.camera = camera_options.clone();
        map.update_camera(&update).unwrap();

        // Creation is ordered after the accepted camera command, so the
        // projection observes it: the committed center is the viewport center.
        let projection = crate::completion::blocking(map.create_projection());
        let center_point = projection.pixel_for_lat_lng(center).unwrap();
        assert!((center_point.x - 256.0).abs() < 1e-6);
        assert!((center_point.y - 256.0).abs() < 1e-6);

        map.close_and_wait();
        runtime.close_and_wait();
        std::thread::spawn(move || {
            let round_tripped = projection.lat_lng_for_pixel(center_point).unwrap();
            assert!((round_tripped.latitude - center.latitude).abs() < 1e-7);
            assert!((round_tripped.longitude - center.longitude).abs() < 1e-7);
            projection.close().unwrap();
        })
        .join()
        .unwrap();
    }

    #[test]
    // Rust regression: dropping a projection without explicit close must not
    // attempt unsafe cleanup from an uncontrolled destructor path.
    fn projection_drops_without_explicit_close() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

        {
            let _projection = crate::completion::blocking(map.create_projection());
        }

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]
    // Spec coverage: BND-103.
    fn projection_setters_change_later_conversions_synchronously() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(MapHandle::with_options(
            &runtime,
            &MapOptions::new(512, 512, 1.0),
        ));
        let projection = crate::completion::blocking(map.create_projection());

        let center = LatLng::new(10.0, 20.0);
        let mut camera_options = CameraOptions::default();
        camera_options.center = Some(center);
        camera_options.zoom = Some(2.0);
        projection.set_camera(&camera_options).unwrap();
        let camera = projection.camera().unwrap();
        let read_center = camera.center.unwrap();
        assert!((read_center.latitude - center.latitude).abs() < 1e-9);
        assert!((read_center.longitude - center.longitude).abs() < 1e-9);
        assert_eq!(camera.zoom, Some(2.0));
        // The setter completed before returning, so the very next conversion
        // maps the new center to the viewport center.
        let center_point = projection.pixel_for_lat_lng(center).unwrap();
        assert!((center_point.x - 256.0).abs() < 1e-6);
        assert!((center_point.y - 256.0).abs() < 1e-6);

        let padding = EdgeInsets::new(0.0, 0.0, 0.0, 0.0);
        projection
            .set_visible_coordinates(&[LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)], padding)
            .unwrap();
        let fitted = projection.camera().unwrap();
        assert_ne!(fitted.center, Some(center));
        let error = projection
            .set_visible_coordinates(&[], padding)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("at least one coordinate"));
        projection
            .set_visible_geometry(
                br#"{"type":"LineString","coordinates":[[0.0,0.0],[1.0,1.0]]}"#,
                padding,
            )
            .unwrap();

        projection.close().unwrap();
        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]
    // Spec coverage: BND-049 and BND-190.
    fn projection_calls_work_from_a_second_thread_and_never_observe_later_map_changes() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(MapHandle::with_options(
            &runtime,
            &MapOptions::new(512, 512, 1.0),
        ));
        let projection = crate::completion::blocking(map.create_projection());
        let creation_camera = projection.camera().unwrap();

        // A later map camera command leaves the projection's snapshot alone.
        let mut update = crate::CameraUpdate::default();
        update.camera.center = Some(LatLng::new(45.0, 45.0));
        update.camera.zoom = Some(9.0);
        map.update_camera(&update).unwrap();
        let barrier = runtime.barrier().unwrap();
        assert!(barrier.wait(std::time::Duration::from_secs(5)).unwrap());
        barrier.take().unwrap();
        assert_eq!(projection.camera().unwrap(), creation_camera);

        std::thread::scope(|scope| {
            let worker = scope.spawn(|| {
                let point = projection.pixel_for_lat_lng(LatLng::new(0.0, 0.0)).unwrap();
                projection.lat_lng_for_pixel(point).unwrap()
            });
            let round_tripped = worker.join().unwrap();
            assert!(round_tripped.latitude.abs() < 1e-7);
            assert!(round_tripped.longitude.abs() < 1e-7);
        });

        projection.close().unwrap();
        map.close_and_wait();
        runtime.close_and_wait();
    }
}
