use std::fmt;
use std::sync::{Arc, Mutex};

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::ptr::const_ptr_or_null;
use maplibre_native_ffi_core::values::lat_lngs_to_native;
use maplibre_native_ffi_sys as sys;

use crate::camera::CameraOptionsNativeExt;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error, out_handle};
use crate::runtime::{RuntimeState, wait_raw_operation_completed};
use crate::values::NativeValue;
use crate::{
    CameraOptions, EdgeInsets, Error, HandleOperationError, LatLng, MapHandle, Result, ScreenPoint,
};

#[derive(Debug)]
pub(crate) struct MapProjectionState {
    handle: ConcurrentNativeHandle<sys::mln_map_projection>,
    runtime: Mutex<Option<Arc<RuntimeState>>>,
}

impl MapProjectionState {
    fn new(native: sys::mln_map_projection, runtime: Arc<RuntimeState>) -> Result<Self> {
        // SAFETY: native came from the typed creation take and projection
        // control state supports calls from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_map_projection") }?;
        Ok(Self {
            handle,
            runtime: Mutex::new(Some(runtime)),
        })
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
        let projection = self.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_projection_close_start(projection, &mut operation)
        })?;
        let result = wait_raw_operation_completed(operation);
        // SAFETY: this call owns the operation observer.
        unsafe { sys::mln_operation_release(operation) };
        result?;
        self.handle.mark_closed();
        self.runtime
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
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
    pub(crate) fn new(map: &MapHandle) -> Result<Self> {
        let map_ptr = map.inner.native()?;
        let runtime = map
            .inner
            .runtime
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .as_ref()
            .cloned()
            .ok_or_else(|| closed_handle_error("MapHandle"))?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_projection_create_start(map_ptr, &mut operation)
        })?;
        let result = (|| {
            wait_raw_operation_completed(operation)?;
            let mut out = maplibre_core::ptr::OutHandle::<sys::mln_map_projection>::new();
            maplibre_core::check(unsafe {
                sys::mln_map_projection_create_take_result(operation, out.as_mut_ptr())
            })?;
            Ok(Self {
                inner: Arc::new(MapProjectionState::new(
                    out_handle(out, "mln_map_projection")?,
                    runtime,
                )?),
            })
        })();
        // SAFETY: this call owns the operation observer.
        unsafe { sys::mln_operation_release(operation) };
        result
    }

    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }

    fn run_query<T>(
        &self,
        start: impl FnOnce(sys::mln_map_projection, *mut sys::mln_operation) -> i32,
        take: impl FnOnce(sys::mln_operation) -> Result<T>,
    ) -> Result<T> {
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(start(self.inner.native()?, &mut operation))?;
        let result = (|| {
            wait_raw_operation_completed(operation)?;
            take(operation)
        })();
        // SAFETY: this call owns the operation observer.
        unsafe { sys::mln_operation_release(operation) };
        result
    }

    pub fn camera(&self) -> Result<CameraOptions> {
        self.run_query(
            |projection, out| unsafe { sys::mln_map_projection_get_camera_start(projection, out) },
            |operation| {
                let mut raw = unsafe { sys::mln_camera_options_default() };
                maplibre_core::check(unsafe {
                    sys::mln_map_projection_get_camera_take_result(operation, &mut raw)
                })?;
                Ok(CameraOptions::from_native(raw))
            },
        )
    }

    pub fn set_camera(&self, camera: &CameraOptions) -> Result<u64> {
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_camera(
                self.inner.native()?,
                &camera.to_native(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    pub fn set_visible_coordinates(
        &self,
        coordinates: &[LatLng],
        padding: EdgeInsets,
    ) -> Result<u64> {
        if coordinates.is_empty() {
            return Err(Error::invalid_argument(
                "set_visible_coordinates requires at least one coordinate",
            ));
        }
        let coordinates = lat_lngs_to_native(coordinates);
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                self.inner.native()?,
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                padding.to_native(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    pub fn set_visible_geometry(&self, geometry: &[u8], padding: EdgeInsets) -> Result<u64> {
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_geometry(
                self.inner.native()?,
                maplibre_core::string::buffer_view(geometry),
                padding.to_native(),
                &mut command_id,
            )
        })?;
        Ok(command_id)
    }

    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        self.run_query(
            |projection, out| unsafe {
                sys::mln_map_projection_pixel_for_lat_lng_start(
                    projection,
                    coordinate.to_native(),
                    out,
                )
            },
            |operation| {
                let mut raw = sys::mln_screen_point { x: 0.0, y: 0.0 };
                maplibre_core::check(unsafe {
                    sys::mln_map_projection_pixel_for_lat_lng_take_result(operation, &mut raw)
                })?;
                Ok(ScreenPoint::from_native(raw))
            },
        )
    }

    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        self.run_query(
            |projection, out| unsafe {
                sys::mln_map_projection_lat_lng_for_pixel_start(projection, point.to_native(), out)
            },
            |operation| {
                let mut raw = sys::mln_lat_lng {
                    latitude: 0.0,
                    longitude: 0.0,
                };
                maplibre_core::check(unsafe {
                    sys::mln_map_projection_lat_lng_for_pixel_take_result(operation, &mut raw)
                })?;
                Ok(LatLng::from_native(raw))
            },
        )
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
    fn projection_create_round_trip_and_close_before_dependencies() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::new(512, 512, 1.0)).unwrap();
        let center = LatLng::new(37.7749, -122.4194);
        let mut camera_options = CameraOptions::default();
        camera_options.center = Some(center);
        camera_options.zoom = Some(5.0);
        let mut update = crate::CameraUpdate::default();
        update.camera = camera_options.clone();
        map.update_camera(&update).unwrap();

        let projection = map.create_projection().unwrap();

        let point = projection.pixel_for_lat_lng(center).unwrap();
        let round_tripped = projection.lat_lng_for_pixel(point).unwrap();
        assert!((round_tripped.latitude - center.latitude).abs() < 1e-7);
        assert!((round_tripped.longitude - center.longitude).abs() < 1e-7);
        projection.close().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Rust regression: dropping a projection without explicit close must not
    // attempt unsafe cleanup from an uncontrolled destructor path.
    fn projection_drops_without_explicit_close() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

        {
            let _projection = map.create_projection().unwrap();
        }

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-103.
    fn projection_camera_and_visible_region_helpers_call_c_api() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        let projection = map.create_projection().unwrap();

        let mut camera_options = CameraOptions::default();
        camera_options.center = Some(LatLng::new(0.0, 0.0));
        camera_options.zoom = Some(2.0);
        projection.set_camera(&camera_options).unwrap();
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
                br#"{"type":"LineString","coordinates":[[0.0,0.0],[1.0,1.0]]}"#,
                padding,
            )
            .unwrap();

        projection.close().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }
}
