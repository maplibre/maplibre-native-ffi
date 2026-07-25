use maplibre_native_core::{self as core, handle::NativeHandleState};
use maplibre_native_sys as sys;
use napi::bindgen_prelude::Result;
use napi_derive::napi;

use crate::{
    error,
    map::{CameraOptions, EdgeInsets, NativeMapHandle, parse_geometry},
    values::{LatLng, ScreenPoint},
};

#[napi(js_name = "NativeMapProjectionHandle")]
pub struct NativeMapProjectionHandle {
    state: NativeHandleState<sys::mln_map_projection>,
}

#[napi(js_name = "createNativeMapProjectionHandle")]
pub fn create_native_map_projection_handle(
    map: &NativeMapHandle,
) -> Result<NativeMapProjectionHandle> {
    let mut projection = std::ptr::null_mut();
    core::check(unsafe { sys::mln_map_projection_create(map.as_ptr(), &mut projection) })
        .map_err(error::from_core)?;
    let state = unsafe { NativeHandleState::from_raw_ptr(projection, "MapProjectionHandle") }
        .map_err(error::from_core)?;
    Ok(NativeMapProjectionHandle { state })
}

#[napi]
impl NativeMapProjectionHandle {
    #[napi]
    pub fn close(&self) -> Result<()> {
        unsafe { self.state.close_status(sys::mln_map_projection_destroy) }
            .map_err(error::from_core)
    }

    #[napi(getter)]
    pub fn closed(&self) -> bool {
        self.state.is_closed()
    }

    #[napi(js_name = "getCamera")]
    pub fn get_camera(&self) -> Result<CameraOptions> {
        let mut raw = unsafe { sys::mln_camera_options_default() };
        core::check(unsafe { sys::mln_map_projection_get_camera(self.state.as_ptr(), &mut raw) })
            .map_err(error::from_core)?;
        Ok(CameraOptions::from_core(
            core::camera::camera_options_from_native(raw),
        ))
    }

    #[napi(js_name = "setCamera")]
    pub fn set_camera(&self, camera: CameraOptions) -> Result<()> {
        let camera = core::camera::camera_options_to_native(&camera.into_core());
        core::check(unsafe { sys::mln_map_projection_set_camera(self.state.as_ptr(), &camera) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "setVisibleCoordinates")]
    pub fn set_visible_coordinates(
        &self,
        coordinates: Vec<LatLng>,
        padding: EdgeInsets,
    ) -> Result<()> {
        let coordinates = coordinates
            .into_iter()
            .map(LatLng::into_native)
            .collect::<Vec<_>>();
        let padding = core::values::edge_insets_to_native(padding.into_core());
        core::check(unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                self.state.as_ptr(),
                coordinates.as_ptr(),
                coordinates.len(),
                padding,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setVisibleGeometry")]
    pub fn set_visible_geometry(&self, geometry: String, padding: EdgeInsets) -> Result<()> {
        let geometry = parse_geometry(geometry)?;
        let native_geometry =
            core::geometry::geometry_try_to_native(&geometry).map_err(error::from_core)?;
        let padding = core::values::edge_insets_to_native(padding.into_core());
        core::check(unsafe {
            sys::mln_map_projection_set_visible_geometry(
                self.state.as_ptr(),
                native_geometry.as_ref(),
                padding,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "pixelForLatLng")]
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let mut raw_point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        core::check(unsafe {
            sys::mln_map_projection_pixel_for_lat_lng(
                self.state.as_ptr(),
                coordinate.into_native(),
                &mut raw_point,
            )
        })
        .map_err(error::from_core)?;
        Ok(ScreenPoint::from_native(raw_point))
    }

    #[napi(js_name = "latLngForPixel")]
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        let mut raw_coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        core::check(unsafe {
            sys::mln_map_projection_lat_lng_for_pixel(
                self.state.as_ptr(),
                point.into_native(),
                &mut raw_coordinate,
            )
        })
        .map_err(error::from_core)?;
        Ok(LatLng::from_native(raw_coordinate))
    }
}

impl Drop for NativeMapProjectionHandle {
    fn drop(&mut self) {
        if let Some(address) = self.state.leak_for_report() {
            crate::maplibre::report_native_handle_leak(self.state.type_name(), address);
        }
    }
}
