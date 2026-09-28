#[allow(clippy::all, unused_parens)]
mod generated;

use std::fmt;
use std::sync::Arc;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::Result;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};

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
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("MapProjectionHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }
}

impl Drop for MapProjectionState {
    fn drop(&mut self) {
        self.handle.finalize_with(|handle| unsafe {
            maplibre_core::check(sys::mln_map_projection_close(handle))
        });
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
    pub(crate) fn from_native(native: sys::mln_map_projection) -> Result<Self> {
        Ok(Self {
            inner: Arc::new(MapProjectionState::new(native)?),
        })
    }
}

#[cfg(test)]
mod tests {
    use crate::{CameraOptions, EdgeInsets, LatLng};
    use static_assertions::assert_impl_all;

    use super::*;
    use crate::{ErrorKind, MapOptions};

    assert_impl_all!(MapProjectionHandle: Send, Sync);

    #[test]

    fn projection_observes_earlier_camera_commands_and_round_trips_synchronously() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(
            runtime.map_create(&crate::test_support::map_options(512, 512, 1.0)),
        );
        let center = LatLng::new(37.7749, -122.4194);
        let mut camera_options = CameraOptions::default();
        camera_options.center = Some(center);
        camera_options.zoom = Some(5.0);
        let mut update = crate::CameraUpdate::default();
        update.camera = camera_options;
        map.update_camera(&update).unwrap();

        // Creation is ordered after the accepted camera command, so the
        // projection observes it: the committed center is the viewport center.
        let projection = crate::completion::blocking(map.projection_create());
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
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));

        {
            let _projection = crate::completion::blocking(map.projection_create());
        }

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn projection_setters_change_later_conversions_synchronously() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(
            runtime.map_create(&crate::test_support::map_options(512, 512, 1.0)),
        );
        let projection = crate::completion::blocking(map.projection_create());

        let center = LatLng::new(10.0, 20.0);
        let mut camera_options = CameraOptions::default();
        camera_options.center = Some(center);
        camera_options.zoom = Some(2.0);
        projection.set_camera(&camera_options).unwrap();
        let camera = projection.get_camera().unwrap();
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
        let fitted = projection.get_camera().unwrap();
        assert_ne!(fitted.center, Some(center));
        let error = projection
            .set_visible_coordinates(&[], padding)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert!(!error.diagnostic().is_empty());
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

    fn projection_calls_work_from_a_second_thread_and_never_observe_later_map_changes() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(
            runtime.map_create(&crate::test_support::map_options(512, 512, 1.0)),
        );
        let projection = crate::completion::blocking(map.projection_create());
        let creation_camera = projection.get_camera().unwrap();

        // A later map camera command leaves the projection's snapshot alone.
        let mut update = crate::CameraUpdate::default();
        update.camera.center = Some(LatLng::new(45.0, 45.0));
        update.camera.zoom = Some(9.0);
        map.update_camera(&update).unwrap();
        let barrier = runtime.barrier().unwrap();
        assert!(barrier.wait(std::time::Duration::from_secs(5)).unwrap());
        barrier.take().unwrap();
        assert_eq!(projection.get_camera().unwrap(), creation_camera);

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
