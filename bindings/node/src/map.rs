use std::{
    collections::HashMap,
    ffi::{CString, c_void},
    panic::{AssertUnwindSafe, catch_unwind},
    sync::{Arc, Condvar, Mutex},
};

use maplibre_native_core::{self as core, handle::NativeHandleState};
use maplibre_native_sys as sys;
use napi::bindgen_prelude::{BigInt, Result, Uint8Array};
use napi::threadsafe_function::{ThreadsafeFunction, ThreadsafeFunctionCallMode};
use napi_derive::napi;

use crate::{
    error,
    runtime::NativeRuntimeHandle,
    values::{LatLng, LatLngBounds, ScreenPoint},
};

#[napi(object)]
#[derive(Default)]
pub struct MapOptions {
    pub width: Option<u32>,
    pub height: Option<u32>,
    pub scale_factor: Option<f64>,
    pub map_mode: Option<String>,
}

#[napi(object)]
pub struct CameraOptions {
    pub center: Option<LatLng>,
    pub zoom: Option<f64>,
    pub bearing: Option<f64>,
    pub pitch: Option<f64>,
    pub center_altitude: Option<f64>,
    pub padding: Option<EdgeInsets>,
    pub anchor: Option<ScreenPoint>,
    pub roll: Option<f64>,
    pub field_of_view: Option<f64>,
}

#[napi(object)]
pub struct UnitBezier {
    pub x1: f64,
    pub y1: f64,
    pub x2: f64,
    pub y2: f64,
}

#[napi(object)]
pub struct AnimationOptions {
    pub duration_ms: Option<f64>,
    pub velocity: Option<f64>,
    pub min_zoom: Option<f64>,
    pub easing: Option<UnitBezier>,
}

#[napi(object)]
pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[napi(object)]
pub struct Quaternion {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

#[napi(object)]
pub struct FreeCameraOptions {
    pub position: Option<Vec3>,
    pub orientation: Option<Quaternion>,
}

#[napi(object)]
pub struct EdgeInsets {
    pub top: f64,
    pub left: f64,
    pub bottom: f64,
    pub right: f64,
}

#[napi(object)]
pub struct MapViewportOptions {
    pub north_orientation: Option<String>,
    pub constrain_mode: Option<String>,
    pub viewport_mode: Option<String>,
    pub frustum_offset: Option<EdgeInsets>,
}

#[napi(object)]
pub struct MapTileOptions {
    pub prefetch_zoom_delta: Option<u32>,
    pub lod_min_radius: Option<f64>,
    pub lod_scale: Option<f64>,
    pub lod_pitch_threshold: Option<f64>,
    pub lod_zoom_shift: Option<f64>,
    pub lod_mode: Option<String>,
}

#[napi(object)]
pub struct BoundOptions {
    pub bounds: Option<LatLngBounds>,
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub min_pitch: Option<f64>,
    pub max_pitch: Option<f64>,
}

#[napi(object)]
pub struct ProjectionMode {
    pub axonometric: Option<bool>,
    pub x_skew: Option<f64>,
    pub y_skew: Option<f64>,
}

#[napi(object)]
pub struct StyleSourceInfo {
    pub source_type: String,
    pub raw_type: u32,
    pub id_size: i64,
    pub is_volatile: bool,
    pub has_attribution: bool,
    pub attribution_size: i64,
    pub attribution: Option<String>,
}

#[napi(object)]
pub struct CanonicalTileId {
    pub z: u32,
    pub x: u32,
    pub y: u32,
}

#[napi(object)]
#[derive(Default)]
pub struct CustomGeometrySourceOptions {
    pub min_zoom: Option<f64>,
    pub max_zoom: Option<f64>,
    pub tolerance: Option<f64>,
    pub tile_size: Option<u32>,
    pub buffer: Option<u32>,
    pub clip: Option<bool>,
    pub wrap: Option<bool>,
}

#[napi(object)]
pub struct PremultipliedRgba8ImageInput {
    pub width: u32,
    pub height: u32,
    pub stride: Option<u32>,
    pub pixels: Uint8Array,
}

#[napi(object)]
pub struct StyleImageInput {
    pub width: u32,
    pub height: u32,
    pub stride: Option<u32>,
    pub pixels: Uint8Array,
    pub pixel_ratio: Option<f64>,
    pub sdf: Option<bool>,
}

#[napi(object)]
pub struct StyleImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: i64,
    pub pixel_ratio: f64,
    pub sdf: bool,
}

#[napi(object)]
pub struct StyleImage {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: i64,
    pub pixel_ratio: f64,
    pub sdf: bool,
    pub pixels: Uint8Array,
}

#[napi(js_name = "NativeMapHandle")]
pub struct NativeMapHandle {
    state: NativeHandleState<sys::mln_map>,
    custom_geometry_sources: Mutex<HashMap<String, Box<CustomGeometrySourceState>>>,
}

#[napi(js_name = "createNativeMapHandle")]
pub fn create_native_map_handle(
    runtime: &NativeRuntimeHandle,
    options: Option<MapOptions>,
) -> Result<NativeMapHandle> {
    let options = options.unwrap_or_default().into_core()?;
    let native_options =
        core::options::map_options_to_native(&options).map_err(error::from_core)?;
    let mut map = std::ptr::null_mut();

    core::check(unsafe { sys::mln_map_create(runtime.as_ptr(), &native_options, &mut map) })
        .map_err(error::from_core)?;
    runtime.mark_map_created();
    let state =
        unsafe { NativeHandleState::from_raw_ptr(map, "MapHandle") }.map_err(error::from_core)?;
    Ok(NativeMapHandle {
        state,
        custom_geometry_sources: Mutex::new(HashMap::new()),
    })
}

#[napi]
impl NativeMapHandle {
    pub(crate) fn as_ptr(&self) -> *mut sys::mln_map {
        self.state.as_ptr()
    }

    #[napi]
    pub fn close(&self) -> Result<()> {
        unsafe { self.state.close_status(sys::mln_map_destroy) }.map_err(error::from_core)?;
        self.clear_all_custom_geometry_source_state();
        Ok(())
    }

    #[napi(getter)]
    pub fn closed(&self) -> bool {
        self.state.is_closed()
    }

    #[napi(getter, js_name = "nativeAddress")]
    pub fn native_address(&self) -> BigInt {
        BigInt::from(self.state.as_ptr() as usize as u64)
    }

    #[napi(js_name = "requestRepaint")]
    pub fn request_repaint(&self) -> Result<()> {
        core::check(unsafe { sys::mln_map_request_repaint(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "requestStillImage")]
    pub fn request_still_image(&self) -> Result<()> {
        core::check(unsafe { sys::mln_map_request_still_image(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "isFullyLoaded")]
    pub fn is_fully_loaded(&self) -> Result<bool> {
        let mut loaded = false;
        core::check(unsafe { sys::mln_map_is_fully_loaded(self.state.as_ptr(), &mut loaded) })
            .map_err(error::from_core)?;
        Ok(loaded)
    }

    #[napi(js_name = "dumpDebugLogs")]
    pub fn dump_debug_logs(&self) -> Result<()> {
        core::check(unsafe { sys::mln_map_dump_debug_logs(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(getter, js_name = "renderingStatsViewEnabled")]
    pub fn rendering_stats_view_enabled(&self) -> Result<bool> {
        let mut enabled = false;
        core::check(unsafe {
            sys::mln_map_get_rendering_stats_view_enabled(self.state.as_ptr(), &mut enabled)
        })
        .map_err(error::from_core)?;
        Ok(enabled)
    }

    #[napi(setter, js_name = "renderingStatsViewEnabled")]
    pub fn set_rendering_stats_view_enabled(&self, enabled: bool) -> Result<()> {
        core::check(unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(self.state.as_ptr(), enabled)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "moveBy")]
    pub fn move_by(&self, delta_x: f64, delta_y: f64) -> Result<()> {
        core::check(unsafe { sys::mln_map_move_by(self.state.as_ptr(), delta_x, delta_y) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "scaleBy")]
    pub fn scale_by(&self, scale: f64, anchor: Option<ScreenPoint>) -> Result<()> {
        let anchor = anchor.map(ScreenPoint::into_native);
        let anchor_ptr = anchor.as_ref().map_or(std::ptr::null(), |anchor| {
            anchor as *const sys::mln_screen_point
        });
        core::check(unsafe { sys::mln_map_scale_by(self.state.as_ptr(), scale, anchor_ptr) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "rotateBy")]
    pub fn rotate_by(&self, first: ScreenPoint, second: ScreenPoint) -> Result<()> {
        core::check(unsafe {
            sys::mln_map_rotate_by(
                self.state.as_ptr(),
                first.into_native(),
                second.into_native(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "pitchBy")]
    pub fn pitch_by(&self, pitch: f64) -> Result<()> {
        core::check(unsafe { sys::mln_map_pitch_by(self.state.as_ptr(), pitch) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "moveByAnimated")]
    pub fn move_by_animated(
        &self,
        delta_x: f64,
        delta_y: f64,
        animation: Option<AnimationOptions>,
    ) -> Result<()> {
        let animation = animation
            .map(|animation| core::camera::animation_options_to_native(&animation.into_core()));
        let animation_ptr = animation.as_ref().map_or(std::ptr::null(), |animation| {
            animation as *const sys::mln_animation_options
        });
        core::check(unsafe {
            sys::mln_map_move_by_animated(self.state.as_ptr(), delta_x, delta_y, animation_ptr)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "scaleByAnimated")]
    pub fn scale_by_animated(
        &self,
        scale: f64,
        anchor: Option<ScreenPoint>,
        animation: Option<AnimationOptions>,
    ) -> Result<()> {
        let anchor = anchor.map(ScreenPoint::into_native);
        let anchor_ptr = anchor.as_ref().map_or(std::ptr::null(), |anchor| {
            anchor as *const sys::mln_screen_point
        });
        let animation = animation
            .map(|animation| core::camera::animation_options_to_native(&animation.into_core()));
        let animation_ptr = animation.as_ref().map_or(std::ptr::null(), |animation| {
            animation as *const sys::mln_animation_options
        });
        core::check(unsafe {
            sys::mln_map_scale_by_animated(self.state.as_ptr(), scale, anchor_ptr, animation_ptr)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "rotateByAnimated")]
    pub fn rotate_by_animated(
        &self,
        first: ScreenPoint,
        second: ScreenPoint,
        animation: Option<AnimationOptions>,
    ) -> Result<()> {
        let animation = animation
            .map(|animation| core::camera::animation_options_to_native(&animation.into_core()));
        let animation_ptr = animation.as_ref().map_or(std::ptr::null(), |animation| {
            animation as *const sys::mln_animation_options
        });
        core::check(unsafe {
            sys::mln_map_rotate_by_animated(
                self.state.as_ptr(),
                first.into_native(),
                second.into_native(),
                animation_ptr,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "pitchByAnimated")]
    pub fn pitch_by_animated(&self, pitch: f64, animation: Option<AnimationOptions>) -> Result<()> {
        let animation = animation
            .map(|animation| core::camera::animation_options_to_native(&animation.into_core()));
        let animation_ptr = animation.as_ref().map_or(std::ptr::null(), |animation| {
            animation as *const sys::mln_animation_options
        });
        core::check(unsafe {
            sys::mln_map_pitch_by_animated(self.state.as_ptr(), pitch, animation_ptr)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "cancelTransitions")]
    pub fn cancel_transitions(&self) -> Result<()> {
        core::check(unsafe { sys::mln_map_cancel_transitions(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "getViewportOptions")]
    pub fn get_viewport_options(&self) -> Result<MapViewportOptions> {
        let mut raw = unsafe { sys::mln_map_viewport_options_default() };
        core::check(unsafe { sys::mln_map_get_viewport_options(self.state.as_ptr(), &mut raw) })
            .map_err(error::from_core)?;
        Ok(MapViewportOptions::from_core(
            core::options::map_viewport_options_from_native(raw),
        ))
    }

    #[napi(js_name = "setViewportOptions")]
    pub fn set_viewport_options(&self, options: MapViewportOptions) -> Result<()> {
        let options = core::options::map_viewport_options_to_native(&options.into_core()?);
        core::check(unsafe { sys::mln_map_set_viewport_options(self.state.as_ptr(), &options) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "getTileOptions")]
    pub fn get_tile_options(&self) -> Result<MapTileOptions> {
        let mut raw = unsafe { sys::mln_map_tile_options_default() };
        core::check(unsafe { sys::mln_map_get_tile_options(self.state.as_ptr(), &mut raw) })
            .map_err(error::from_core)?;
        Ok(MapTileOptions::from_core(
            core::options::map_tile_options_from_native(raw),
        ))
    }

    #[napi(js_name = "setTileOptions")]
    pub fn set_tile_options(&self, options: MapTileOptions) -> Result<()> {
        let options = core::options::map_tile_options_to_native(&options.into_core()?);
        core::check(unsafe { sys::mln_map_set_tile_options(self.state.as_ptr(), &options) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "getBounds")]
    pub fn get_bounds(&self) -> Result<BoundOptions> {
        let mut raw = unsafe { sys::mln_bound_options_default() };
        core::check(unsafe { sys::mln_map_get_bounds(self.state.as_ptr(), &mut raw) })
            .map_err(error::from_core)?;
        Ok(BoundOptions::from_core(
            core::camera::bound_options_from_native(raw),
        ))
    }

    #[napi(js_name = "setBounds")]
    pub fn set_bounds(&self, options: BoundOptions) -> Result<()> {
        let options = core::camera::bound_options_to_native(&options.into_core());
        core::check(unsafe { sys::mln_map_set_bounds(self.state.as_ptr(), &options) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "getFreeCameraOptions")]
    pub fn get_free_camera_options(&self) -> Result<FreeCameraOptions> {
        let mut raw = unsafe { sys::mln_free_camera_options_default() };
        core::check(unsafe { sys::mln_map_get_free_camera_options(self.state.as_ptr(), &mut raw) })
            .map_err(error::from_core)?;
        Ok(FreeCameraOptions::from_core(
            core::camera::free_camera_options_from_native(raw),
        ))
    }

    #[napi(js_name = "setFreeCameraOptions")]
    pub fn set_free_camera_options(&self, options: FreeCameraOptions) -> Result<()> {
        let options = core::camera::free_camera_options_to_native(&options.into_core());
        core::check(unsafe { sys::mln_map_set_free_camera_options(self.state.as_ptr(), &options) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "getProjectionMode")]
    pub fn get_projection_mode(&self) -> Result<ProjectionMode> {
        let mut raw = unsafe { sys::mln_projection_mode_default() };
        core::check(unsafe { sys::mln_map_get_projection_mode(self.state.as_ptr(), &mut raw) })
            .map_err(error::from_core)?;
        Ok(ProjectionMode::from_core(
            core::camera::projection_mode_from_native(raw),
        ))
    }

    #[napi(js_name = "setProjectionMode")]
    pub fn set_projection_mode(&self, mode: ProjectionMode) -> Result<()> {
        let mode = core::camera::projection_mode_to_native(&mode.into_core());
        core::check(unsafe { sys::mln_map_set_projection_mode(self.state.as_ptr(), &mode) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "getCamera")]
    pub fn get_camera(&self) -> Result<CameraOptions> {
        let mut raw = unsafe { sys::mln_camera_options_default() };
        core::check(unsafe { sys::mln_map_get_camera(self.state.as_ptr(), &mut raw) })
            .map_err(error::from_core)?;
        Ok(CameraOptions::from_core(
            core::camera::camera_options_from_native(raw),
        ))
    }

    #[napi(js_name = "jumpTo")]
    pub fn jump_to(&self, camera: CameraOptions) -> Result<()> {
        let camera = core::camera::camera_options_to_native(&camera.into_core());
        core::check(unsafe { sys::mln_map_jump_to(self.state.as_ptr(), &camera) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "easeTo")]
    pub fn ease_to(
        &self,
        camera: CameraOptions,
        animation: Option<AnimationOptions>,
    ) -> Result<()> {
        let camera = core::camera::camera_options_to_native(&camera.into_core());
        let animation = animation
            .map(|animation| core::camera::animation_options_to_native(&animation.into_core()));
        let animation_ptr = animation.as_ref().map_or(std::ptr::null(), |animation| {
            animation as *const sys::mln_animation_options
        });
        core::check(unsafe { sys::mln_map_ease_to(self.state.as_ptr(), &camera, animation_ptr) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "flyTo")]
    pub fn fly_to(&self, camera: CameraOptions, animation: Option<AnimationOptions>) -> Result<()> {
        let camera = core::camera::camera_options_to_native(&camera.into_core());
        let animation = animation
            .map(|animation| core::camera::animation_options_to_native(&animation.into_core()));
        let animation_ptr = animation.as_ref().map_or(std::ptr::null(), |animation| {
            animation as *const sys::mln_animation_options
        });
        core::check(unsafe { sys::mln_map_fly_to(self.state.as_ptr(), &camera, animation_ptr) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "cameraForLatLngBounds")]
    pub fn camera_for_lat_lng_bounds(&self, bounds: LatLngBounds) -> Result<CameraOptions> {
        let bounds = core::values::lat_lng_bounds_to_native(bounds.into_core());
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        core::check(unsafe {
            sys::mln_map_camera_for_lat_lng_bounds(
                self.state.as_ptr(),
                bounds,
                std::ptr::null(),
                &mut raw_camera,
            )
        })
        .map_err(error::from_core)?;
        Ok(CameraOptions::from_core(
            core::camera::camera_options_from_native(raw_camera),
        ))
    }

    #[napi(js_name = "cameraForLatLngs")]
    pub fn camera_for_lat_lngs(&self, coordinates: Vec<LatLng>) -> Result<CameraOptions> {
        let coordinates = lat_lngs_to_native(coordinates);
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        core::check(unsafe {
            sys::mln_map_camera_for_lat_lngs(
                self.state.as_ptr(),
                coordinates.as_ptr(),
                coordinates.len(),
                std::ptr::null(),
                &mut raw_camera,
            )
        })
        .map_err(error::from_core)?;
        Ok(CameraOptions::from_core(
            core::camera::camera_options_from_native(raw_camera),
        ))
    }

    #[napi(js_name = "cameraForGeometry")]
    pub fn camera_for_geometry(&self, geometry: String) -> Result<CameraOptions> {
        let geometry = parse_geometry(geometry)?;
        let native_geometry =
            core::geometry::geometry_try_to_native(&geometry).map_err(error::from_core)?;
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        core::check(unsafe {
            sys::mln_map_camera_for_geometry(
                self.state.as_ptr(),
                native_geometry.as_ref(),
                std::ptr::null(),
                &mut raw_camera,
            )
        })
        .map_err(error::from_core)?;
        Ok(CameraOptions::from_core(
            core::camera::camera_options_from_native(raw_camera),
        ))
    }

    #[napi(js_name = "latLngBoundsForCamera")]
    pub fn lat_lng_bounds_for_camera(&self, camera: CameraOptions) -> Result<LatLngBounds> {
        let camera = core::camera::camera_options_to_native(&camera.into_core());
        let mut raw_bounds = sys::mln_lat_lng_bounds {
            southwest: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
            northeast: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
        };
        core::check(unsafe {
            sys::mln_map_lat_lng_bounds_for_camera(self.state.as_ptr(), &camera, &mut raw_bounds)
        })
        .map_err(error::from_core)?;
        Ok(LatLngBounds::from_core(
            core::values::lat_lng_bounds_from_native(raw_bounds),
        ))
    }

    #[napi(js_name = "latLngBoundsForCameraUnwrapped")]
    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: CameraOptions,
    ) -> Result<LatLngBounds> {
        let camera = core::camera::camera_options_to_native(&camera.into_core());
        let mut raw_bounds = sys::mln_lat_lng_bounds {
            southwest: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
            northeast: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
        };
        core::check(unsafe {
            sys::mln_map_lat_lng_bounds_for_camera_unwrapped(
                self.state.as_ptr(),
                &camera,
                &mut raw_bounds,
            )
        })
        .map_err(error::from_core)?;
        Ok(LatLngBounds::from_core(
            core::values::lat_lng_bounds_from_native(raw_bounds),
        ))
    }

    #[napi(js_name = "pixelForLatLng")]
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let mut raw_point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        core::check(unsafe {
            sys::mln_map_pixel_for_lat_lng(
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
            sys::mln_map_lat_lng_for_pixel(
                self.state.as_ptr(),
                point.into_native(),
                &mut raw_coordinate,
            )
        })
        .map_err(error::from_core)?;
        Ok(LatLng::from_native(raw_coordinate))
    }

    #[napi(js_name = "pixelsForLatLngs")]
    pub fn pixels_for_lat_lngs(&self, coordinates: Vec<LatLng>) -> Result<Vec<ScreenPoint>> {
        let coordinates = lat_lngs_to_native(coordinates);
        let mut points = vec![sys::mln_screen_point { x: 0.0, y: 0.0 }; coordinates.len()];
        core::check(unsafe {
            sys::mln_map_pixels_for_lat_lngs(
                self.state.as_ptr(),
                coordinates.as_ptr(),
                coordinates.len(),
                points.as_mut_ptr(),
            )
        })
        .map_err(error::from_core)?;
        Ok(points.into_iter().map(ScreenPoint::from_native).collect())
    }

    #[napi(js_name = "latLngsForPixels")]
    pub fn lat_lngs_for_pixels(&self, points: Vec<ScreenPoint>) -> Result<Vec<LatLng>> {
        let points = screen_points_to_native(points);
        let mut coordinates = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            };
            points.len()
        ];
        core::check(unsafe {
            sys::mln_map_lat_lngs_for_pixels(
                self.state.as_ptr(),
                points.as_ptr(),
                points.len(),
                coordinates.as_mut_ptr(),
            )
        })
        .map_err(error::from_core)?;
        Ok(coordinates.into_iter().map(LatLng::from_native).collect())
    }

    #[napi(js_name = "getDebugOptionsRaw")]
    pub fn get_debug_options_raw(&self) -> Result<u32> {
        let mut options = 0;
        core::check(unsafe { sys::mln_map_get_debug_options(self.state.as_ptr(), &mut options) })
            .map_err(error::from_core)?;
        Ok(options)
    }

    #[napi(js_name = "setDebugOptionsRaw")]
    pub fn set_debug_options_raw(&self, options: u32) -> Result<()> {
        core::check(unsafe { sys::mln_map_set_debug_options(self.state.as_ptr(), options) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "addStyleSourceJson")]
    pub fn add_style_source_json(&self, source_id: String, source_json: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let source_json = parse_json_value(source_json)?;
        let native_source_json =
            core::json::json_value_try_to_native(&source_json).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_map_add_style_source_json(
                self.state.as_ptr(),
                source_id.raw(),
                native_source_json.as_ptr(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "styleSourceExists")]
    pub fn style_source_exists(&self, source_id: String) -> Result<bool> {
        let source_id = core::string::string_view(&source_id);
        let mut exists = false;
        core::check(unsafe {
            sys::mln_map_style_source_exists(self.state.as_ptr(), source_id.raw(), &mut exists)
        })
        .map_err(error::from_core)?;
        Ok(exists)
    }

    #[napi(js_name = "removeStyleSource")]
    pub fn remove_style_source(&self, source_id: String) -> Result<bool> {
        let source_id_string = source_id.clone();
        let source_id = core::string::string_view(&source_id);
        let mut removed = false;
        core::check(unsafe {
            sys::mln_map_remove_style_source(self.state.as_ptr(), source_id.raw(), &mut removed)
        })
        .map_err(error::from_core)?;
        if removed {
            self.custom_geometry_sources
                .lock()
                .expect("custom geometry source mutex poisoned")
                .remove(&source_id_string);
        }
        Ok(removed)
    }

    #[napi(js_name = "listStyleSourceIds")]
    pub fn list_style_source_ids(&self) -> Result<Vec<String>> {
        let mut list = std::ptr::null_mut();
        core::check(unsafe { sys::mln_map_list_style_source_ids(self.state.as_ptr(), &mut list) })
            .map_err(error::from_core)?;
        copy_style_id_list(list).map_err(error::from_core)
    }

    #[napi(js_name = "getStyleSourceType")]
    pub fn get_style_source_type(&self, source_id: String) -> Result<Option<String>> {
        let source_id = core::string::string_view(&source_id);
        let mut raw_type = 0;
        let mut found = false;
        core::check(unsafe {
            sys::mln_map_get_style_source_type(
                self.state.as_ptr(),
                source_id.raw(),
                &mut raw_type,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        Ok(found.then(|| style_source_type_name(raw_type).to_owned()))
    }

    #[napi(js_name = "getStyleSourceInfo")]
    pub fn get_style_source_info(&self, source_id: String) -> Result<Option<StyleSourceInfo>> {
        let source_id_view = core::string::string_view(&source_id);
        let mut raw_info = sys::mln_style_source_info {
            size: std::mem::size_of::<sys::mln_style_source_info>() as u32,
            type_: 0,
            id_size: 0,
            is_volatile: false,
            has_attribution: false,
            attribution_size: 0,
        };
        let mut found = false;
        core::check(unsafe {
            sys::mln_map_get_style_source_info(
                self.state.as_ptr(),
                source_id_view.raw(),
                &mut raw_info,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        if !found {
            return Ok(None);
        }
        let attribution = copy_style_source_attribution(
            self.state.as_ptr(),
            source_id_view.raw(),
            raw_info.attribution_size,
        )
        .map_err(error::from_core)?;
        Ok(Some(StyleSourceInfo {
            source_type: style_source_type_name(raw_info.type_).to_owned(),
            raw_type: raw_info.type_,
            id_size: raw_info.id_size as i64,
            is_volatile: raw_info.is_volatile,
            has_attribution: raw_info.has_attribution,
            attribution_size: raw_info.attribution_size as i64,
            attribution,
        }))
    }

    #[napi(js_name = "addGeoJsonSourceUrl")]
    pub fn add_geo_json_source_url(&self, source_id: String, url: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let url = core::string::string_view(&url);
        core::check(unsafe {
            sys::mln_map_add_geojson_source_url(self.state.as_ptr(), source_id.raw(), url.raw())
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addGeoJsonSourceData")]
    pub fn add_geo_json_source_data(&self, source_id: String, data: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let data = parse_geojson(data)?;
        let native_data = core::geojson::geojson_try_to_native(&data).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_map_add_geojson_source_data(
                self.state.as_ptr(),
                source_id.raw(),
                native_data.as_ptr(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setGeoJsonSourceUrl")]
    pub fn set_geo_json_source_url(&self, source_id: String, url: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let url = core::string::string_view(&url);
        core::check(unsafe {
            sys::mln_map_set_geojson_source_url(self.state.as_ptr(), source_id.raw(), url.raw())
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setGeoJsonSourceData")]
    pub fn set_geo_json_source_data(&self, source_id: String, data: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let data = parse_geojson(data)?;
        let native_data = core::geojson::geojson_try_to_native(&data).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_map_set_geojson_source_data(
                self.state.as_ptr(),
                source_id.raw(),
                native_data.as_ptr(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addVectorSourceUrl")]
    pub fn add_vector_source_url(&self, source_id: String, url: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let url = core::string::string_view(&url);
        core::check(unsafe {
            sys::mln_map_add_vector_source_url(
                self.state.as_ptr(),
                source_id.raw(),
                url.raw(),
                std::ptr::null(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addRasterSourceUrl")]
    pub fn add_raster_source_url(&self, source_id: String, url: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let url = core::string::string_view(&url);
        core::check(unsafe {
            sys::mln_map_add_raster_source_url(
                self.state.as_ptr(),
                source_id.raw(),
                url.raw(),
                std::ptr::null(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addRasterDemSourceUrl")]
    pub fn add_raster_dem_source_url(&self, source_id: String, url: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let url = core::string::string_view(&url);
        core::check(unsafe {
            sys::mln_map_add_raster_dem_source_url(
                self.state.as_ptr(),
                source_id.raw(),
                url.raw(),
                std::ptr::null(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addVectorSourceTiles")]
    pub fn add_vector_source_tiles(&self, source_id: String, tiles: Vec<String>) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let tiles = StringViews::new(tiles);
        core::check(unsafe {
            sys::mln_map_add_vector_source_tiles(
                self.state.as_ptr(),
                source_id.raw(),
                tiles.as_ptr(),
                tiles.len(),
                std::ptr::null(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addRasterSourceTiles")]
    pub fn add_raster_source_tiles(&self, source_id: String, tiles: Vec<String>) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let tiles = StringViews::new(tiles);
        core::check(unsafe {
            sys::mln_map_add_raster_source_tiles(
                self.state.as_ptr(),
                source_id.raw(),
                tiles.as_ptr(),
                tiles.len(),
                std::ptr::null(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addRasterDemSourceTiles")]
    pub fn add_raster_dem_source_tiles(&self, source_id: String, tiles: Vec<String>) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let tiles = StringViews::new(tiles);
        core::check(unsafe {
            sys::mln_map_add_raster_dem_source_tiles(
                self.state.as_ptr(),
                source_id.raw(),
                tiles.as_ptr(),
                tiles.len(),
                std::ptr::null(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addCustomGeometrySource")]
    pub fn add_custom_geometry_source(
        &self,
        source_id: String,
        options: Option<CustomGeometrySourceOptions>,
        fetch_tile: Option<ThreadsafeFunction<CanonicalTileId>>,
        cancel_tile: Option<ThreadsafeFunction<CanonicalTileId>>,
    ) -> Result<()> {
        let source_id_view = core::string::string_view(&source_id);
        let mut state = CustomGeometrySourceState::new(fetch_tile, cancel_tile).map(Box::new);
        let options = custom_geometry_source_options_to_native(
            options.unwrap_or_default(),
            state.as_deref_mut(),
        );
        core::check(unsafe {
            sys::mln_map_add_custom_geometry_source(
                self.state.as_ptr(),
                source_id_view.raw(),
                &options,
            )
        })
        .map_err(error::from_core)?;
        if let Some(state) = state {
            self.custom_geometry_sources
                .lock()
                .expect("custom geometry source mutex poisoned")
                .insert(source_id, state);
        }
        Ok(())
    }

    #[napi(js_name = "setCustomGeometrySourceTileData")]
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: String,
        tile_id: CanonicalTileId,
        data: String,
    ) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let data = parse_geojson(data)?;
        let native_data = core::geojson::geojson_try_to_native(&data).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                self.state.as_ptr(),
                source_id.raw(),
                tile_id.into_native(),
                native_data.as_ptr(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "invalidateCustomGeometrySourceTile")]
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: String,
        tile_id: CanonicalTileId,
    ) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        core::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                self.state.as_ptr(),
                source_id.raw(),
                tile_id.into_native(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "invalidateCustomGeometrySourceRegion")]
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: String,
        bounds: LatLngBounds,
    ) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let bounds = core::values::lat_lng_bounds_to_native(bounds.into_core());
        core::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                self.state.as_ptr(),
                source_id.raw(),
                bounds,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setStyleImage")]
    pub fn set_style_image(&self, image_id: String, image: StyleImageInput) -> Result<()> {
        let image_id = core::string::string_view(&image_id);
        let stride = image.stride.unwrap_or(image.width.saturating_mul(4));
        let mut raw_image = unsafe { sys::mln_premultiplied_rgba8_image_default() };
        raw_image.width = image.width;
        raw_image.height = image.height;
        raw_image.stride = stride;
        raw_image.pixels = image.pixels.as_ptr();
        raw_image.byte_length = image.pixels.len();
        let mut options = unsafe { sys::mln_style_image_options_default() };
        if let Some(pixel_ratio) = image.pixel_ratio {
            options.fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
            options.pixel_ratio = pixel_ratio as f32;
        }
        if let Some(sdf) = image.sdf {
            options.fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
            options.sdf = sdf;
        }
        core::check(unsafe {
            sys::mln_map_set_style_image(self.state.as_ptr(), image_id.raw(), &raw_image, &options)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "styleImageExists")]
    pub fn style_image_exists(&self, image_id: String) -> Result<bool> {
        let image_id = core::string::string_view(&image_id);
        let mut exists = false;
        core::check(unsafe {
            sys::mln_map_style_image_exists(self.state.as_ptr(), image_id.raw(), &mut exists)
        })
        .map_err(error::from_core)?;
        Ok(exists)
    }

    #[napi(js_name = "removeStyleImage")]
    pub fn remove_style_image(&self, image_id: String) -> Result<bool> {
        let image_id = core::string::string_view(&image_id);
        let mut removed = false;
        core::check(unsafe {
            sys::mln_map_remove_style_image(self.state.as_ptr(), image_id.raw(), &mut removed)
        })
        .map_err(error::from_core)?;
        Ok(removed)
    }

    #[napi(js_name = "getStyleImageInfo")]
    pub fn get_style_image_info(&self, image_id: String) -> Result<Option<StyleImageInfo>> {
        let image_id = core::string::string_view(&image_id);
        let mut raw_info = unsafe { sys::mln_style_image_info_default() };
        let mut found = false;
        core::check(unsafe {
            sys::mln_map_get_style_image_info(
                self.state.as_ptr(),
                image_id.raw(),
                &mut raw_info,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        Ok(found.then(|| style_image_info_from_native(raw_info)))
    }

    #[napi(js_name = "copyStyleImagePremultipliedRgba8")]
    pub fn copy_style_image_premultiplied_rgba8(
        &self,
        image_id: String,
    ) -> Result<Option<StyleImage>> {
        let image_id = core::string::string_view(&image_id);
        let mut raw_info = unsafe { sys::mln_style_image_info_default() };
        let mut info_found = false;
        core::check(unsafe {
            sys::mln_map_get_style_image_info(
                self.state.as_ptr(),
                image_id.raw(),
                &mut raw_info,
                &mut info_found,
            )
        })
        .map_err(error::from_core)?;
        if !info_found {
            return Ok(None);
        }
        let info = style_image_info_from_native(raw_info);
        let mut pixels = vec![0; info.byte_length as usize];
        let mut byte_length = 0;
        let mut found = false;
        core::check(unsafe {
            sys::mln_map_copy_style_image_premultiplied_rgba8(
                self.state.as_ptr(),
                image_id.raw(),
                pixels.as_mut_ptr(),
                pixels.len(),
                &mut byte_length,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        if !found {
            return Ok(None);
        }
        pixels.truncate(byte_length);
        Ok(Some(StyleImage {
            width: info.width,
            height: info.height,
            stride: info.stride,
            byte_length: byte_length as i64,
            pixel_ratio: info.pixel_ratio,
            sdf: info.sdf,
            pixels: Uint8Array::from(pixels),
        }))
    }

    #[napi(js_name = "addImageSourceUrl")]
    pub fn add_image_source_url(
        &self,
        source_id: String,
        coordinates: Vec<LatLng>,
        url: String,
    ) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let url = core::string::string_view(&url);
        core::check(unsafe {
            sys::mln_map_add_image_source_url(
                self.state.as_ptr(),
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
                url.raw(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addImageSourceImage")]
    pub fn add_image_source_image(
        &self,
        source_id: String,
        coordinates: Vec<LatLng>,
        image: PremultipliedRgba8ImageInput,
    ) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        let image = premultiplied_rgba8_image_from_input(&image);
        core::check(unsafe {
            sys::mln_map_add_image_source_image(
                self.state.as_ptr(),
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
                &image,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setImageSourceUrl")]
    pub fn set_image_source_url(&self, source_id: String, url: String) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let url = core::string::string_view(&url);
        core::check(unsafe {
            sys::mln_map_set_image_source_url(self.state.as_ptr(), source_id.raw(), url.raw())
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setImageSourceImage")]
    pub fn set_image_source_image(
        &self,
        source_id: String,
        image: PremultipliedRgba8ImageInput,
    ) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let image = premultiplied_rgba8_image_from_input(&image);
        core::check(unsafe {
            sys::mln_map_set_image_source_image(self.state.as_ptr(), source_id.raw(), &image)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setImageSourceCoordinates")]
    pub fn set_image_source_coordinates(
        &self,
        source_id: String,
        coordinates: Vec<LatLng>,
    ) -> Result<()> {
        let source_id = core::string::string_view(&source_id);
        let coordinates = lat_lngs_to_native(coordinates);
        core::check(unsafe {
            sys::mln_map_set_image_source_coordinates(
                self.state.as_ptr(),
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "getImageSourceCoordinates")]
    pub fn get_image_source_coordinates(&self, source_id: String) -> Result<Option<Vec<LatLng>>> {
        let source_id = core::string::string_view(&source_id);
        let mut coordinates = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            };
            4
        ];
        let mut coordinate_count = 0;
        let mut found = false;
        core::check(unsafe {
            sys::mln_map_get_image_source_coordinates(
                self.state.as_ptr(),
                source_id.raw(),
                coordinates.as_mut_ptr(),
                coordinates.len(),
                &mut coordinate_count,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        if !found {
            return Ok(None);
        }
        coordinates.truncate(coordinate_count);
        Ok(Some(
            coordinates.into_iter().map(LatLng::from_native).collect(),
        ))
    }

    #[napi(js_name = "addHillshadeLayer")]
    pub fn add_hillshade_layer(
        &self,
        layer_id: String,
        source_id: String,
        before_layer_id: Option<String>,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        let source_id = core::string::string_view(&source_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = core::string::string_view(&before_layer_id);
        core::check(unsafe {
            sys::mln_map_add_hillshade_layer(
                self.state.as_ptr(),
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addColorReliefLayer")]
    pub fn add_color_relief_layer(
        &self,
        layer_id: String,
        source_id: String,
        before_layer_id: Option<String>,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        let source_id = core::string::string_view(&source_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = core::string::string_view(&before_layer_id);
        core::check(unsafe {
            sys::mln_map_add_color_relief_layer(
                self.state.as_ptr(),
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addLocationIndicatorLayer")]
    pub fn add_location_indicator_layer(
        &self,
        layer_id: String,
        before_layer_id: Option<String>,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = core::string::string_view(&before_layer_id);
        core::check(unsafe {
            sys::mln_map_add_location_indicator_layer(
                self.state.as_ptr(),
                layer_id.raw(),
                before_layer_id.raw(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setLocationIndicatorLocation")]
    pub fn set_location_indicator_location(
        &self,
        layer_id: String,
        coordinate: LatLng,
        altitude: f64,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        core::check(unsafe {
            sys::mln_map_set_location_indicator_location(
                self.state.as_ptr(),
                layer_id.raw(),
                coordinate.into_native(),
                altitude,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setLocationIndicatorBearing")]
    pub fn set_location_indicator_bearing(&self, layer_id: String, bearing: f64) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        core::check(unsafe {
            sys::mln_map_set_location_indicator_bearing(
                self.state.as_ptr(),
                layer_id.raw(),
                bearing,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setLocationIndicatorAccuracyRadius")]
    pub fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: String,
        radius: f64,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        core::check(unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(
                self.state.as_ptr(),
                layer_id.raw(),
                radius,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setLocationIndicatorImageName")]
    pub fn set_location_indicator_image_name(
        &self,
        layer_id: String,
        image_kind: String,
        image_id: String,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        let image_id = core::string::string_view(&image_id);
        core::check(unsafe {
            sys::mln_map_set_location_indicator_image_name(
                self.state.as_ptr(),
                layer_id.raw(),
                location_indicator_image_kind_from_string(&image_kind)?,
                image_id.raw(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "addStyleLayerJson")]
    pub fn add_style_layer_json(
        &self,
        layer_json: String,
        before_layer_id: Option<String>,
    ) -> Result<()> {
        let layer_json = parse_json_value(layer_json)?;
        let native_layer_json =
            core::json::json_value_try_to_native(&layer_json).map_err(error::from_core)?;
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = core::string::string_view(&before_layer_id);
        core::check(unsafe {
            sys::mln_map_add_style_layer_json(
                self.state.as_ptr(),
                native_layer_json.as_ptr(),
                before_layer_id.raw(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "styleLayerExists")]
    pub fn style_layer_exists(&self, layer_id: String) -> Result<bool> {
        let layer_id = core::string::string_view(&layer_id);
        let mut exists = false;
        core::check(unsafe {
            sys::mln_map_style_layer_exists(self.state.as_ptr(), layer_id.raw(), &mut exists)
        })
        .map_err(error::from_core)?;
        Ok(exists)
    }

    #[napi(js_name = "removeStyleLayer")]
    pub fn remove_style_layer(&self, layer_id: String) -> Result<bool> {
        let layer_id = core::string::string_view(&layer_id);
        let mut removed = false;
        core::check(unsafe {
            sys::mln_map_remove_style_layer(self.state.as_ptr(), layer_id.raw(), &mut removed)
        })
        .map_err(error::from_core)?;
        Ok(removed)
    }

    #[napi(js_name = "listStyleLayerIds")]
    pub fn list_style_layer_ids(&self) -> Result<Vec<String>> {
        let mut list = std::ptr::null_mut();
        core::check(unsafe { sys::mln_map_list_style_layer_ids(self.state.as_ptr(), &mut list) })
            .map_err(error::from_core)?;
        copy_style_id_list(list).map_err(error::from_core)
    }

    #[napi(js_name = "getStyleLayerType")]
    pub fn get_style_layer_type(&self, layer_id: String) -> Result<Option<String>> {
        let layer_id = core::string::string_view(&layer_id);
        let mut raw_type = sys::mln_string_view {
            data: std::ptr::null(),
            size: 0,
        };
        let mut found = false;
        core::check(unsafe {
            sys::mln_map_get_style_layer_type(
                self.state.as_ptr(),
                layer_id.raw(),
                &mut raw_type,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        if found {
            Ok(Some(
                unsafe { core::string::copy_string_view(raw_type) }.map_err(error::from_core)?,
            ))
        } else {
            Ok(None)
        }
    }

    #[napi(js_name = "getStyleLayerJson")]
    pub fn get_style_layer_json(&self, layer_id: String) -> Result<Option<String>> {
        let layer_id = core::string::string_view(&layer_id);
        let mut snapshot = std::ptr::null_mut();
        let mut found = false;
        core::check(unsafe {
            sys::mln_map_get_style_layer_json(
                self.state.as_ptr(),
                layer_id.raw(),
                &mut snapshot,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        if !found {
            return Ok(None);
        }
        json_snapshot_to_string(snapshot)
    }

    #[napi(js_name = "moveStyleLayer")]
    pub fn move_style_layer(
        &self,
        layer_id: String,
        before_layer_id: Option<String>,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = core::string::string_view(&before_layer_id);
        core::check(unsafe {
            sys::mln_map_move_style_layer(
                self.state.as_ptr(),
                layer_id.raw(),
                before_layer_id.raw(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setLayerPropertyJson")]
    pub fn set_layer_property_json(
        &self,
        layer_id: String,
        property_name: String,
        value_json: String,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        let property_name = core::string::string_view(&property_name);
        let value = parse_json_value(value_json)?;
        let native_value =
            core::json::json_value_try_to_native(&value).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_map_set_layer_property(
                self.state.as_ptr(),
                layer_id.raw(),
                property_name.raw(),
                native_value.as_ptr(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "getLayerPropertyJson")]
    pub fn get_layer_property_json(
        &self,
        layer_id: String,
        property_name: String,
    ) -> Result<Option<String>> {
        let layer_id = core::string::string_view(&layer_id);
        let property_name = core::string::string_view(&property_name);
        let mut snapshot = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_map_get_layer_property(
                self.state.as_ptr(),
                layer_id.raw(),
                property_name.raw(),
                &mut snapshot,
            )
        })
        .map_err(error::from_core)?;
        json_snapshot_to_string(snapshot)
    }

    #[napi(js_name = "setLayerFilterJson")]
    pub fn set_layer_filter_json(
        &self,
        layer_id: String,
        filter_json: Option<String>,
    ) -> Result<()> {
        let layer_id = core::string::string_view(&layer_id);
        let filter = filter_json.map(parse_json_value).transpose()?;
        let native_filter = filter
            .as_ref()
            .map(core::json::json_value_try_to_native)
            .transpose()
            .map_err(error::from_core)?;
        let filter_ptr = native_filter
            .as_ref()
            .map_or(std::ptr::null(), |filter| filter.as_ptr());
        core::check(unsafe {
            sys::mln_map_set_layer_filter(self.state.as_ptr(), layer_id.raw(), filter_ptr)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "getLayerFilterJson")]
    pub fn get_layer_filter_json(&self, layer_id: String) -> Result<Option<String>> {
        let layer_id = core::string::string_view(&layer_id);
        let mut snapshot = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_map_get_layer_filter(self.state.as_ptr(), layer_id.raw(), &mut snapshot)
        })
        .map_err(error::from_core)?;
        json_snapshot_to_string(snapshot)
    }

    #[napi(js_name = "setStyleLightJson")]
    pub fn set_style_light_json(&self, light_json: String) -> Result<()> {
        let light = parse_json_value(light_json)?;
        let native_light =
            core::json::json_value_try_to_native(&light).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_map_set_style_light_json(self.state.as_ptr(), native_light.as_ptr())
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "setStyleLightPropertyJson")]
    pub fn set_style_light_property_json(
        &self,
        property_name: String,
        value_json: String,
    ) -> Result<()> {
        let property_name = core::string::string_view(&property_name);
        let value = parse_json_value(value_json)?;
        let native_value =
            core::json::json_value_try_to_native(&value).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_map_set_style_light_property(
                self.state.as_ptr(),
                property_name.raw(),
                native_value.as_ptr(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "getStyleLightPropertyJson")]
    pub fn get_style_light_property_json(&self, property_name: String) -> Result<Option<String>> {
        let property_name = core::string::string_view(&property_name);
        let mut snapshot = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_map_get_style_light_property(
                self.state.as_ptr(),
                property_name.raw(),
                &mut snapshot,
            )
        })
        .map_err(error::from_core)?;
        json_snapshot_to_string(snapshot)
    }

    #[napi(js_name = "setStyleJson")]
    pub fn set_style_json(&self, json: String) -> Result<()> {
        let json = c_string(json, "style JSON")?;
        core::check(unsafe { sys::mln_map_set_style_json(self.state.as_ptr(), json.as_ptr()) })
            .map_err(error::from_core)?;
        self.clear_all_custom_geometry_source_state();
        Ok(())
    }

    #[napi(js_name = "setStyleUrl")]
    pub fn set_style_url(&self, url: String) -> Result<()> {
        let url = c_string(url, "style URL")?;
        core::check(unsafe { sys::mln_map_set_style_url(self.state.as_ptr(), url.as_ptr()) })
            .map_err(error::from_core)?;
        Ok(())
    }

    #[napi(js_name = "releaseDetachedCustomGeometrySources")]
    pub fn release_detached_custom_geometry_sources(&self) {
        let source_ids = match self.custom_geometry_sources.lock() {
            Ok(sources) => sources.keys().cloned().collect::<Vec<_>>(),
            Err(_) => return,
        };
        let mut detached_sources = Vec::new();
        for source_id in source_ids {
            let source_id_view = core::string::string_view(&source_id);
            let mut source_type = 0;
            let mut found = false;
            let status = unsafe {
                sys::mln_map_get_style_source_type(
                    self.state.as_ptr(),
                    source_id_view.raw(),
                    &mut source_type,
                    &mut found,
                )
            };
            if status == sys::MLN_STATUS_OK
                && (!found || source_type != sys::MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR)
            {
                detached_sources.push(source_id);
            }
        }
        if detached_sources.is_empty() {
            return;
        }
        if let Ok(mut sources) = self.custom_geometry_sources.lock() {
            for source_id in detached_sources {
                sources.remove(&source_id);
            }
        }
    }

    #[napi(js_name = "customGeometrySourceCountForTesting")]
    pub fn custom_geometry_source_count_for_testing(&self) -> u32 {
        self.custom_geometry_sources
            .lock()
            .expect("custom geometry source mutex poisoned")
            .len() as u32
    }
}

impl Drop for NativeMapHandle {
    fn drop(&mut self) {
        if let Some(address) = self.state.leak_for_report() {
            crate::maplibre::report_native_handle_leak(self.state.type_name(), address);
            if let Ok(mut sources) = self.custom_geometry_sources.lock() {
                for (_, source) in sources.drain() {
                    Box::leak(source);
                }
            }
        }
    }
}

impl EdgeInsets {
    pub(crate) fn into_core(self) -> core::EdgeInsets {
        core::EdgeInsets::new(self.top, self.left, self.bottom, self.right)
    }

    fn from_core(value: core::EdgeInsets) -> Self {
        Self {
            top: value.top,
            left: value.left,
            bottom: value.bottom,
            right: value.right,
        }
    }
}

impl MapViewportOptions {
    fn into_core(self) -> Result<core::MapViewportOptions> {
        let mut options = core::MapViewportOptions::default();
        if let Some(value) = self.north_orientation {
            options.north_orientation = Some(north_orientation_from_string(&value)?);
        }
        if let Some(value) = self.constrain_mode {
            options.constrain_mode = Some(constrain_mode_from_string(&value)?);
        }
        if let Some(value) = self.viewport_mode {
            options.viewport_mode = Some(viewport_mode_from_string(&value)?);
        }
        if let Some(value) = self.frustum_offset {
            options.frustum_offset = Some(value.into_core());
        }
        Ok(options)
    }

    fn from_core(options: core::MapViewportOptions) -> Self {
        Self {
            north_orientation: options.north_orientation.map(north_orientation_name),
            constrain_mode: options.constrain_mode.map(constrain_mode_name),
            viewport_mode: options.viewport_mode.map(viewport_mode_name),
            frustum_offset: options.frustum_offset.map(EdgeInsets::from_core),
        }
    }
}

impl MapTileOptions {
    fn into_core(self) -> Result<core::MapTileOptions> {
        let mut options = core::MapTileOptions::default();
        if let Some(value) = self.prefetch_zoom_delta {
            options.prefetch_zoom_delta = Some(value);
        }
        if let Some(value) = self.lod_min_radius {
            options.lod_min_radius = Some(value);
        }
        if let Some(value) = self.lod_scale {
            options.lod_scale = Some(value);
        }
        if let Some(value) = self.lod_pitch_threshold {
            options.lod_pitch_threshold = Some(value);
        }
        if let Some(value) = self.lod_zoom_shift {
            options.lod_zoom_shift = Some(value);
        }
        if let Some(value) = self.lod_mode {
            options.lod_mode = Some(tile_lod_mode_from_string(&value)?);
        }
        Ok(options)
    }

    fn from_core(options: core::MapTileOptions) -> Self {
        Self {
            prefetch_zoom_delta: options.prefetch_zoom_delta,
            lod_min_radius: options.lod_min_radius,
            lod_scale: options.lod_scale,
            lod_pitch_threshold: options.lod_pitch_threshold,
            lod_zoom_shift: options.lod_zoom_shift,
            lod_mode: options.lod_mode.map(tile_lod_mode_name),
        }
    }
}

impl BoundOptions {
    fn into_core(self) -> core::BoundOptions {
        let mut options = core::BoundOptions::default();
        if let Some(value) = self.bounds {
            options.bounds = Some(value.into_core());
        }
        if let Some(value) = self.min_zoom {
            options.min_zoom = Some(value);
        }
        if let Some(value) = self.max_zoom {
            options.max_zoom = Some(value);
        }
        if let Some(value) = self.min_pitch {
            options.min_pitch = Some(value);
        }
        if let Some(value) = self.max_pitch {
            options.max_pitch = Some(value);
        }
        options
    }

    fn from_core(options: core::BoundOptions) -> Self {
        Self {
            bounds: options.bounds.map(LatLngBounds::from_core),
            min_zoom: options.min_zoom,
            max_zoom: options.max_zoom,
            min_pitch: options.min_pitch,
            max_pitch: options.max_pitch,
        }
    }
}

impl ProjectionMode {
    fn into_core(self) -> core::ProjectionMode {
        let mut mode = core::ProjectionMode::default();
        if let Some(value) = self.axonometric {
            mode.axonometric = Some(value);
        }
        if let Some(value) = self.x_skew {
            mode.x_skew = Some(value);
        }
        if let Some(value) = self.y_skew {
            mode.y_skew = Some(value);
        }
        mode
    }

    fn from_core(mode: core::ProjectionMode) -> Self {
        Self {
            axonometric: mode.axonometric,
            x_skew: mode.x_skew,
            y_skew: mode.y_skew,
        }
    }
}

impl CameraOptions {
    pub(crate) fn into_core(self) -> core::CameraOptions {
        let mut camera = core::CameraOptions::default();
        if let Some(center) = self.center {
            camera.center = Some(center.into_core());
        }
        if let Some(zoom) = self.zoom {
            camera.zoom = Some(zoom);
        }
        if let Some(bearing) = self.bearing {
            camera.bearing = Some(bearing);
        }
        if let Some(pitch) = self.pitch {
            camera.pitch = Some(pitch);
        }
        if let Some(center_altitude) = self.center_altitude {
            camera.center_altitude = Some(center_altitude);
        }
        if let Some(padding) = self.padding {
            camera.padding = Some(padding.into_core());
        }
        if let Some(anchor) = self.anchor {
            camera.anchor = Some(anchor.into_core());
        }
        if let Some(roll) = self.roll {
            camera.roll = Some(roll);
        }
        if let Some(field_of_view) = self.field_of_view {
            camera.field_of_view = Some(field_of_view);
        }
        camera
    }

    pub(crate) fn from_core(camera: core::CameraOptions) -> Self {
        Self {
            center: camera.center.map(LatLng::from_core),
            zoom: camera.zoom,
            bearing: camera.bearing,
            pitch: camera.pitch,
            center_altitude: camera.center_altitude,
            padding: camera.padding.map(EdgeInsets::from_core),
            anchor: camera.anchor.map(ScreenPoint::from_core),
            roll: camera.roll,
            field_of_view: camera.field_of_view,
        }
    }
}

impl UnitBezier {
    fn into_core(self) -> core::values::UnitBezier {
        core::values::UnitBezier::new(self.x1, self.y1, self.x2, self.y2)
    }
}

impl AnimationOptions {
    fn into_core(self) -> core::AnimationOptions {
        let mut animation = core::AnimationOptions::default();
        if let Some(duration_ms) = self.duration_ms {
            animation.duration_ms = Some(duration_ms);
        }
        if let Some(velocity) = self.velocity {
            animation.velocity = Some(velocity);
        }
        if let Some(min_zoom) = self.min_zoom {
            animation.min_zoom = Some(min_zoom);
        }
        if let Some(easing) = self.easing {
            animation.easing = Some(easing.into_core());
        }
        animation
    }
}

impl Vec3 {
    fn into_core(self) -> core::values::Vec3 {
        core::values::Vec3::new(self.x, self.y, self.z)
    }

    fn from_core(value: core::values::Vec3) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
        }
    }
}

impl Quaternion {
    fn into_core(self) -> core::values::Quaternion {
        core::values::Quaternion::new(self.x, self.y, self.z, self.w)
    }

    fn from_core(value: core::values::Quaternion) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
            w: value.w,
        }
    }
}

impl CanonicalTileId {
    fn into_native(self) -> sys::mln_canonical_tile_id {
        sys::mln_canonical_tile_id {
            z: self.z,
            x: self.x,
            y: self.y,
        }
    }

    fn from_native(raw: sys::mln_canonical_tile_id) -> Self {
        Self {
            z: raw.z,
            x: raw.x,
            y: raw.y,
        }
    }
}

impl FreeCameraOptions {
    fn into_core(self) -> core::FreeCameraOptions {
        let mut options = core::FreeCameraOptions::default();
        if let Some(position) = self.position {
            options.position = Some(position.into_core());
        }
        if let Some(orientation) = self.orientation {
            options.orientation = Some(orientation.into_core());
        }
        options
    }

    fn from_core(options: core::FreeCameraOptions) -> Self {
        Self {
            position: options.position.map(Vec3::from_core),
            orientation: options.orientation.map(Quaternion::from_core),
        }
    }
}

impl MapOptions {
    fn into_core(self) -> Result<core::MapOptions> {
        let defaults = core::MapOptions::default();
        let mut options = core::MapOptions::new(
            self.width.unwrap_or(defaults.width),
            self.height.unwrap_or(defaults.height),
            self.scale_factor.unwrap_or(defaults.scale_factor),
        );
        if let Some(map_mode) = self.map_mode {
            options.mode = map_mode_from_string(&map_mode)?;
        }
        Ok(options)
    }
}

fn north_orientation_from_string(value: &str) -> Result<core::NorthOrientation> {
    match value {
        "up" => Ok(core::NorthOrientation::Up),
        "right" => Ok(core::NorthOrientation::Right),
        "down" => Ok(core::NorthOrientation::Down),
        "left" => Ok(core::NorthOrientation::Left),
        other => Err(error::invalid_argument(format!(
            "northOrientation must be 'up', 'right', 'down', or 'left', got '{other}'"
        ))),
    }
}

fn north_orientation_name(value: core::NorthOrientation) -> String {
    match value {
        core::NorthOrientation::Up => "up",
        core::NorthOrientation::Right => "right",
        core::NorthOrientation::Down => "down",
        core::NorthOrientation::Left => "left",
        core::NorthOrientation::Unknown(_) => "unknown",
        _ => "unknown",
    }
    .to_owned()
}

fn constrain_mode_from_string(value: &str) -> Result<core::ConstrainMode> {
    match value {
        "none" => Ok(core::ConstrainMode::None),
        "heightOnly" => Ok(core::ConstrainMode::HeightOnly),
        "widthAndHeight" => Ok(core::ConstrainMode::WidthAndHeight),
        "screen" => Ok(core::ConstrainMode::Screen),
        other => Err(error::invalid_argument(format!(
            "constrainMode must be 'none', 'heightOnly', 'widthAndHeight', or 'screen', got '{other}'"
        ))),
    }
}

fn constrain_mode_name(value: core::ConstrainMode) -> String {
    match value {
        core::ConstrainMode::None => "none",
        core::ConstrainMode::HeightOnly => "heightOnly",
        core::ConstrainMode::WidthAndHeight => "widthAndHeight",
        core::ConstrainMode::Screen => "screen",
        core::ConstrainMode::Unknown(_) => "unknown",
        _ => "unknown",
    }
    .to_owned()
}

fn viewport_mode_from_string(value: &str) -> Result<core::ViewportMode> {
    match value {
        "default" => Ok(core::ViewportMode::Default),
        "flippedY" => Ok(core::ViewportMode::FlippedY),
        other => Err(error::invalid_argument(format!(
            "viewportMode must be 'default' or 'flippedY', got '{other}'"
        ))),
    }
}

fn viewport_mode_name(value: core::ViewportMode) -> String {
    match value {
        core::ViewportMode::Default => "default",
        core::ViewportMode::FlippedY => "flippedY",
        core::ViewportMode::Unknown(_) => "unknown",
        _ => "unknown",
    }
    .to_owned()
}

fn tile_lod_mode_from_string(value: &str) -> Result<core::TileLodMode> {
    match value {
        "default" => Ok(core::TileLodMode::Default),
        "distance" => Ok(core::TileLodMode::Distance),
        other => Err(error::invalid_argument(format!(
            "lodMode must be 'default' or 'distance', got '{other}'"
        ))),
    }
}

fn tile_lod_mode_name(value: core::TileLodMode) -> String {
    match value {
        core::TileLodMode::Default => "default",
        core::TileLodMode::Distance => "distance",
        core::TileLodMode::Unknown(_) => "unknown",
        _ => "unknown",
    }
    .to_owned()
}

fn map_mode_from_string(map_mode: &str) -> Result<core::MapMode> {
    match map_mode {
        "continuous" => Ok(core::MapMode::Continuous),
        "static" => Ok(core::MapMode::Static),
        "tile" => Ok(core::MapMode::Tile),
        other => Err(error::invalid_argument(format!(
            "mapMode must be 'continuous', 'static', or 'tile', got '{other}'"
        ))),
    }
}

#[napi(js_name = "nativeMapDebugOptionMaskBit")]
pub fn native_map_debug_option_mask_bit(option: String) -> Result<u32> {
    match option.as_str() {
        "tileBorders" => Ok(sys::MLN_MAP_DEBUG_TILE_BORDERS),
        "parseStatus" => Ok(sys::MLN_MAP_DEBUG_PARSE_STATUS),
        "timestamps" => Ok(sys::MLN_MAP_DEBUG_TIMESTAMPS),
        "collision" => Ok(sys::MLN_MAP_DEBUG_COLLISION),
        "overdraw" => Ok(sys::MLN_MAP_DEBUG_OVERDRAW),
        "stencilClip" => Ok(sys::MLN_MAP_DEBUG_STENCIL_CLIP),
        "depthBuffer" => Ok(sys::MLN_MAP_DEBUG_DEPTH_BUFFER),
        other => Err(error::invalid_argument(format!(
            "debug option must be a known MapDebugOption, got '{other}'"
        ))),
    }
}

struct StringViews {
    _strings: Vec<String>,
    views: Vec<sys::mln_string_view>,
}

impl StringViews {
    fn new(strings: Vec<String>) -> Self {
        let views = strings
            .iter()
            .map(|value| core::string::string_view(value).raw())
            .collect();
        Self {
            _strings: strings,
            views,
        }
    }

    fn as_ptr(&self) -> *const sys::mln_string_view {
        self.views.as_ptr()
    }

    fn len(&self) -> usize {
        self.views.len()
    }
}

fn location_indicator_image_kind_from_string(kind: &str) -> Result<u32> {
    match kind {
        "top" => Ok(sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP),
        "bearing" => Ok(sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING),
        "shadow" => Ok(sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW),
        other => Err(error::invalid_argument(format!(
            "location indicator image kind must be 'top', 'bearing', or 'shadow', got '{other}'"
        ))),
    }
}

struct CustomGeometrySourceState {
    fetch_tile: Arc<ThreadsafeFunction<CanonicalTileId>>,
    cancel_tile: Option<Arc<ThreadsafeFunction<CanonicalTileId>>>,
    lifecycle: Mutex<CustomGeometrySourceLifecycle>,
    idle: Condvar,
}

struct CustomGeometrySourceLifecycle {
    closing: bool,
    active: usize,
}

struct CustomGeometryUpcallGuard<'a> {
    state: &'a CustomGeometrySourceState,
}

impl NativeMapHandle {
    fn clear_all_custom_geometry_source_state(&self) {
        if let Ok(mut sources) = self.custom_geometry_sources.lock() {
            sources.clear();
        }
    }
}

impl CustomGeometrySourceState {
    fn new(
        fetch_tile: Option<ThreadsafeFunction<CanonicalTileId>>,
        cancel_tile: Option<ThreadsafeFunction<CanonicalTileId>>,
    ) -> Option<Self> {
        fetch_tile.map(|fetch_tile| Self {
            fetch_tile: Arc::new(fetch_tile),
            cancel_tile: cancel_tile.map(Arc::new),
            lifecycle: Mutex::new(CustomGeometrySourceLifecycle {
                closing: false,
                active: 0,
            }),
            idle: Condvar::new(),
        })
    }

    fn enter_callback(&self) -> Option<CustomGeometryUpcallGuard<'_>> {
        let mut lifecycle = self
            .lifecycle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if lifecycle.closing {
            return None;
        }
        lifecycle.active += 1;
        Some(CustomGeometryUpcallGuard { state: self })
    }

    fn close(&self) {
        let mut lifecycle = self
            .lifecycle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        lifecycle.closing = true;
        while lifecycle.active != 0 {
            lifecycle = self
                .idle
                .wait(lifecycle)
                .unwrap_or_else(|poisoned| poisoned.into_inner());
        }
    }
}

impl Drop for CustomGeometrySourceState {
    fn drop(&mut self) {
        self.close();
    }
}

impl Drop for CustomGeometryUpcallGuard<'_> {
    fn drop(&mut self) {
        let mut lifecycle = self
            .state
            .lifecycle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        lifecycle.active = lifecycle.active.saturating_sub(1);
        if lifecycle.active == 0 {
            self.state.idle.notify_all();
        }
    }
}

extern "C" fn custom_geometry_source_noop_tile_callback(
    _user_data: *mut c_void,
    _tile_id: sys::mln_canonical_tile_id,
) {
}

extern "C" fn custom_geometry_source_fetch_tile_callback(
    user_data: *mut c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    let _ = catch_unwind(AssertUnwindSafe(|| {
        if user_data.is_null() {
            return;
        }
        let state = unsafe { &*(user_data as *const CustomGeometrySourceState) };
        let Some(_guard) = state.enter_callback() else {
            return;
        };
        state.fetch_tile.call(
            Ok(CanonicalTileId::from_native(tile_id)),
            ThreadsafeFunctionCallMode::NonBlocking,
        );
    }));
}

extern "C" fn custom_geometry_source_cancel_tile_callback(
    user_data: *mut c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    let _ = catch_unwind(AssertUnwindSafe(|| {
        if user_data.is_null() {
            return;
        }
        let state = unsafe { &*(user_data as *const CustomGeometrySourceState) };
        let Some(_guard) = state.enter_callback() else {
            return;
        };
        if let Some(cancel_tile) = &state.cancel_tile {
            cancel_tile.call(
                Ok(CanonicalTileId::from_native(tile_id)),
                ThreadsafeFunctionCallMode::NonBlocking,
            );
        }
    }));
}

fn custom_geometry_source_options_to_native(
    options: CustomGeometrySourceOptions,
    state: Option<&mut CustomGeometrySourceState>,
) -> sys::mln_custom_geometry_source_options {
    let mut raw = unsafe { sys::mln_custom_geometry_source_options_default() };
    if let Some(state) = state {
        raw.fetch_tile = Some(custom_geometry_source_fetch_tile_callback);
        raw.cancel_tile = if state.cancel_tile.is_some() {
            Some(custom_geometry_source_cancel_tile_callback)
        } else {
            None
        };
        raw.user_data = state as *mut CustomGeometrySourceState as *mut c_void;
    } else {
        raw.fetch_tile = Some(custom_geometry_source_noop_tile_callback);
        raw.cancel_tile = Some(custom_geometry_source_noop_tile_callback);
    }
    if let Some(value) = options.min_zoom {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = value;
    }
    if let Some(value) = options.max_zoom {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = value;
    }
    if let Some(value) = options.tolerance {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
        raw.tolerance = value;
    }
    if let Some(value) = options.tile_size {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
        raw.tile_size = value;
    }
    if let Some(value) = options.buffer {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
        raw.buffer = value;
    }
    if let Some(value) = options.clip {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
        raw.clip = value;
    }
    if let Some(value) = options.wrap {
        raw.fields |= sys::MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
        raw.wrap = value;
    }
    raw
}

fn premultiplied_rgba8_image_from_input(
    image: &PremultipliedRgba8ImageInput,
) -> sys::mln_premultiplied_rgba8_image {
    let mut raw = unsafe { sys::mln_premultiplied_rgba8_image_default() };
    raw.width = image.width;
    raw.height = image.height;
    raw.stride = image.stride.unwrap_or(image.width.saturating_mul(4));
    raw.pixels = image.pixels.as_ptr();
    raw.byte_length = image.pixels.len();
    raw
}

fn style_image_info_from_native(raw: sys::mln_style_image_info) -> StyleImageInfo {
    StyleImageInfo {
        width: raw.width,
        height: raw.height,
        stride: raw.stride,
        byte_length: raw.byte_length as i64,
        pixel_ratio: raw.pixel_ratio as f64,
        sdf: raw.sdf,
    }
}

fn lat_lngs_to_native(coordinates: Vec<LatLng>) -> Vec<sys::mln_lat_lng> {
    coordinates.into_iter().map(LatLng::into_native).collect()
}

fn screen_points_to_native(points: Vec<ScreenPoint>) -> Vec<sys::mln_screen_point> {
    points.into_iter().map(ScreenPoint::into_native).collect()
}

fn json_snapshot_to_string(snapshot: *mut sys::mln_json_snapshot) -> Result<Option<String>> {
    let value = unsafe { core::json::copy_json_snapshot(std::ptr::NonNull::new(snapshot)) }
        .map_err(error::from_core)?;
    value.map(json_value_to_string).transpose()
}

fn copy_style_source_attribution(
    map: *mut sys::mln_map,
    source_id: sys::mln_string_view,
    attribution_size: usize,
) -> core::Result<Option<String>> {
    if attribution_size == 0 {
        return Ok(None);
    }
    let mut buffer = vec![0; attribution_size];
    let mut copied_size = 0;
    let mut found = false;
    core::check(unsafe {
        sys::mln_map_copy_style_source_attribution(
            map,
            source_id,
            buffer.as_mut_ptr().cast(),
            buffer.len(),
            &mut copied_size,
            &mut found,
        )
    })?;
    if !found || copied_size == 0 {
        return Ok(None);
    }
    buffer.truncate(copied_size);
    String::from_utf8(buffer).map(Some).map_err(|error| {
        core::Error::invalid_argument(format!("style source attribution was not UTF-8: {error}"))
    })
}

fn copy_style_id_list(list: *mut sys::mln_style_id_list) -> core::Result<Vec<String>> {
    struct StyleIdListGuard(*mut sys::mln_style_id_list);

    impl Drop for StyleIdListGuard {
        fn drop(&mut self) {
            unsafe { sys::mln_style_id_list_destroy(self.0) };
        }
    }

    let list = StyleIdListGuard(list);
    let mut count = 0;
    core::check(unsafe { sys::mln_style_id_list_count(list.0, &mut count) })?;
    let mut ids = Vec::with_capacity(count);
    for index in 0..count {
        let mut raw_id = sys::mln_string_view {
            data: std::ptr::null(),
            size: 0,
        };
        core::check(unsafe { sys::mln_style_id_list_get(list.0, index, &mut raw_id) })?;
        ids.push(unsafe { core::string::copy_string_view(raw_id) }?);
    }
    Ok(ids)
}

fn style_source_type_name(raw_type: u32) -> &'static str {
    match raw_type {
        sys::MLN_STYLE_SOURCE_TYPE_VECTOR => "vector",
        sys::MLN_STYLE_SOURCE_TYPE_RASTER => "raster",
        sys::MLN_STYLE_SOURCE_TYPE_RASTER_DEM => "raster-dem",
        sys::MLN_STYLE_SOURCE_TYPE_GEOJSON => "geojson",
        sys::MLN_STYLE_SOURCE_TYPE_IMAGE => "image",
        sys::MLN_STYLE_SOURCE_TYPE_VIDEO => "video",
        sys::MLN_STYLE_SOURCE_TYPE_ANNOTATIONS => "annotations",
        sys::MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR => "custom-vector",
        _ => "unknown",
    }
}

pub(crate) fn parse_json_value(value: String) -> Result<core::JsonValue> {
    let value: serde_json::Value = serde_json::from_str(&value).map_err(|parse_error| {
        error::invalid_argument(format!("JSON input is invalid: {parse_error}"))
    })?;
    json_value_from_serde(value)
}

pub(crate) fn parse_geometry(value: String) -> Result<core::Geometry> {
    let value: serde_json::Value = serde_json::from_str(&value).map_err(|parse_error| {
        error::invalid_argument(format!("GeoJSON geometry input is invalid: {parse_error}"))
    })?;
    geometry_from_serde(&value)
}

pub(crate) fn parse_geojson(value: String) -> Result<core::GeoJson> {
    let value: serde_json::Value = serde_json::from_str(&value).map_err(|parse_error| {
        error::invalid_argument(format!("GeoJSON input is invalid: {parse_error}"))
    })?;
    geojson_from_serde(&value)
}

fn geojson_from_serde(value: &serde_json::Value) -> Result<core::GeoJson> {
    match object_type(value)? {
        "Feature" => Ok(core::GeoJson::Feature(feature_from_serde(value)?)),
        "FeatureCollection" => {
            let features = value
                .get("features")
                .and_then(serde_json::Value::as_array)
                .ok_or_else(|| {
                    error::invalid_argument("GeoJSON FeatureCollection.features must be an array")
                })?
                .iter()
                .map(feature_from_serde)
                .collect::<Result<Vec<_>>>()?;
            Ok(core::GeoJson::FeatureCollection(features))
        }
        _ => Ok(core::GeoJson::Geometry(geometry_from_serde(value)?)),
    }
}

fn feature_from_serde(value: &serde_json::Value) -> Result<core::Feature> {
    if object_type(value)? != "Feature" {
        return Err(error::invalid_argument(
            "GeoJSON feature type must be 'Feature'",
        ));
    }
    let geometry = match value.get("geometry") {
        Some(serde_json::Value::Null) | None => core::Geometry::Empty,
        Some(geometry) => geometry_from_serde(geometry)?,
    };
    let properties = match value.get("properties") {
        Some(serde_json::Value::Object(properties)) => properties
            .iter()
            .map(|(key, value)| {
                Ok(core::JsonMember::new(
                    key.clone(),
                    json_value_from_serde(value.clone())?,
                ))
            })
            .collect::<Result<Vec<_>>>()?,
        Some(serde_json::Value::Null) | None => Vec::new(),
        _ => {
            return Err(error::invalid_argument(
                "GeoJSON feature properties must be an object or null",
            ));
        }
    };
    let mut feature = core::Feature::new(geometry, properties);
    if let Some(identifier) = value.get("id") {
        feature.identifier = feature_identifier_from_serde(identifier)?;
    }
    Ok(feature)
}

fn feature_identifier_from_serde(value: &serde_json::Value) -> Result<core::FeatureIdentifier> {
    match value {
        serde_json::Value::Null => Ok(core::FeatureIdentifier::Null),
        serde_json::Value::String(value) => Ok(core::FeatureIdentifier::String(value.clone())),
        serde_json::Value::Number(value) => {
            if let Some(value) = value.as_u64() {
                Ok(core::FeatureIdentifier::UInt(value))
            } else if let Some(value) = value.as_i64() {
                Ok(core::FeatureIdentifier::Int(value))
            } else if let Some(value) = value.as_f64() {
                Ok(core::FeatureIdentifier::Double(value))
            } else {
                Err(error::invalid_argument(
                    "GeoJSON feature id number is not representable",
                ))
            }
        }
        _ => Err(error::invalid_argument(
            "GeoJSON feature id must be a string, number, or null",
        )),
    }
}

fn geometry_from_serde(value: &serde_json::Value) -> Result<core::Geometry> {
    match object_type(value)? {
        "Point" => Ok(core::Geometry::Point(position_from_serde(
            required_member(value, "coordinates")?,
        )?)),
        "LineString" => Ok(core::Geometry::LineString(positions_from_serde(
            required_member(value, "coordinates")?,
        )?)),
        "Polygon" => Ok(core::Geometry::Polygon(linear_rings_from_serde(
            required_member(value, "coordinates")?,
        )?)),
        "MultiPoint" => Ok(core::Geometry::MultiPoint(positions_from_serde(
            required_member(value, "coordinates")?,
        )?)),
        "MultiLineString" => Ok(core::Geometry::MultiLineString(lines_from_serde(
            required_member(value, "coordinates")?,
        )?)),
        "MultiPolygon" => Ok(core::Geometry::MultiPolygon(polygons_from_serde(
            required_member(value, "coordinates")?,
        )?)),
        "GeometryCollection" => {
            let geometries = value
                .get("geometries")
                .and_then(serde_json::Value::as_array)
                .ok_or_else(|| {
                    error::invalid_argument(
                        "GeoJSON GeometryCollection.geometries must be an array",
                    )
                })?
                .iter()
                .map(geometry_from_serde)
                .collect::<Result<Vec<_>>>()?;
            Ok(core::Geometry::GeometryCollection(geometries))
        }
        other => Err(error::invalid_argument(format!(
            "unsupported GeoJSON geometry type '{other}'"
        ))),
    }
}

fn object_type(value: &serde_json::Value) -> Result<&str> {
    value
        .get("type")
        .and_then(serde_json::Value::as_str)
        .ok_or_else(|| error::invalid_argument("GeoJSON object must have a string type"))
}

fn required_member<'a>(value: &'a serde_json::Value, name: &str) -> Result<&'a serde_json::Value> {
    value
        .get(name)
        .ok_or_else(|| error::invalid_argument(format!("GeoJSON object missing {name}")))
}

fn position_from_serde(value: &serde_json::Value) -> Result<core::LatLng> {
    let values = value
        .as_array()
        .ok_or_else(|| error::invalid_argument("GeoJSON position must be an array"))?;
    if values.len() < 2 {
        return Err(error::invalid_argument(
            "GeoJSON position must contain longitude and latitude",
        ));
    }
    let longitude = values[0]
        .as_f64()
        .ok_or_else(|| error::invalid_argument("GeoJSON longitude must be a number"))?;
    let latitude = values[1]
        .as_f64()
        .ok_or_else(|| error::invalid_argument("GeoJSON latitude must be a number"))?;
    Ok(core::LatLng::new(latitude, longitude))
}

fn positions_from_serde(value: &serde_json::Value) -> Result<Vec<core::LatLng>> {
    value
        .as_array()
        .ok_or_else(|| error::invalid_argument("GeoJSON coordinate list must be an array"))?
        .iter()
        .map(position_from_serde)
        .collect()
}

fn linear_rings_from_serde(value: &serde_json::Value) -> Result<Vec<Vec<core::LatLng>>> {
    value
        .as_array()
        .ok_or_else(|| error::invalid_argument("GeoJSON polygon coordinates must be an array"))?
        .iter()
        .map(positions_from_serde)
        .collect()
}

fn lines_from_serde(value: &serde_json::Value) -> Result<Vec<Vec<core::LatLng>>> {
    linear_rings_from_serde(value)
}

fn polygons_from_serde(value: &serde_json::Value) -> Result<Vec<Vec<Vec<core::LatLng>>>> {
    value
        .as_array()
        .ok_or_else(|| {
            error::invalid_argument("GeoJSON multi-polygon coordinates must be an array")
        })?
        .iter()
        .map(linear_rings_from_serde)
        .collect()
}

pub(crate) fn json_value_to_string(value: core::JsonValue) -> Result<String> {
    serde_json::to_string(&json_value_to_serde(value)).map_err(|serialize_error| {
        error::invalid_argument(format!(
            "JSON value could not be serialized: {serialize_error}"
        ))
    })
}

pub(crate) fn json_value_to_serde(value: core::JsonValue) -> serde_json::Value {
    match value {
        core::JsonValue::Null => serde_json::Value::Null,
        core::JsonValue::Bool(value) => serde_json::Value::Bool(value),
        core::JsonValue::UInt(value) => serde_json::Value::Number(value.into()),
        core::JsonValue::Int(value) => serde_json::Value::Number(value.into()),
        core::JsonValue::Double(value) => serde_json::Number::from_f64(value)
            .map(serde_json::Value::Number)
            .unwrap_or(serde_json::Value::Null),
        core::JsonValue::String(value) => serde_json::Value::String(value),
        core::JsonValue::Array(values) => {
            serde_json::Value::Array(values.into_iter().map(json_value_to_serde).collect())
        }
        core::JsonValue::Object(members) => serde_json::Value::Object(
            members
                .into_iter()
                .map(|member| (member.key, json_value_to_serde(member.value)))
                .collect(),
        ),
        _ => serde_json::Value::Null,
    }
}

fn json_value_from_serde(value: serde_json::Value) -> Result<core::JsonValue> {
    match value {
        serde_json::Value::Null => Ok(core::JsonValue::Null),
        serde_json::Value::Bool(value) => Ok(core::JsonValue::Bool(value)),
        serde_json::Value::Number(value) => {
            if let Some(value) = value.as_u64() {
                Ok(core::JsonValue::UInt(value))
            } else if let Some(value) = value.as_i64() {
                Ok(core::JsonValue::Int(value))
            } else if let Some(value) = value.as_f64() {
                Ok(core::JsonValue::Double(value))
            } else {
                Err(error::invalid_argument(
                    "JSON number could not be represented",
                ))
            }
        }
        serde_json::Value::String(value) => Ok(core::JsonValue::String(value)),
        serde_json::Value::Array(values) => values
            .into_iter()
            .map(json_value_from_serde)
            .collect::<Result<Vec<_>>>()
            .map(core::JsonValue::Array),
        serde_json::Value::Object(members) => members
            .into_iter()
            .map(|(key, value)| Ok(core::JsonMember::new(key, json_value_from_serde(value)?)))
            .collect::<Result<Vec<_>>>()
            .map(core::JsonValue::Object),
    }
}

fn c_string(value: String, field_name: &str) -> Result<CString> {
    CString::new(value)
        .map_err(|_| error::invalid_argument(format!("{field_name} contains an embedded NUL byte")))
}
