use std::cell::RefCell;
use std::collections::HashMap;
use std::fmt;
use std::rc::Rc;

use maplibre_native_core as maplibre_core;
use maplibre_native_core::ptr::{const_ptr_or_null, mut_ptr_or_null, option_ptr};
use maplibre_native_core::values::{
    empty_lat_lng, empty_lat_lng_bounds as empty_bounds, empty_screen_point, lat_lngs_to_native,
    screen_points_to_native,
};
use maplibre_native_sys as sys;

use crate::camera::{
    AnimationOptionsNativeExt, BoundOptionsNativeExt, CameraFitOptionsNativeExt,
    CameraOptionsNativeExt, FreeCameraOptionsNativeExt, ProjectionModeNativeExt,
};
#[cfg(test)]
use crate::custom_geometry::CanonicalTileId;
use crate::custom_geometry::CustomGeometrySourceState;
use crate::events::MapId;
use crate::geometry::GeometryNativeExt;
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::options::{MapOptionsNativeExt, MapTileOptionsNativeExt, MapViewportOptionsNativeExt};
use crate::render::{
    MetalBorrowedTextureDescriptor, MetalOwnedTextureDescriptor, MetalSurfaceDescriptor,
    OpenGLBorrowedTextureDescriptor, OpenGLOwnedTextureDescriptor, OpenGLSurfaceDescriptor,
    RenderSessionHandle, VulkanBorrowedTextureDescriptor, VulkanOwnedTextureDescriptor,
    VulkanSurfaceDescriptor,
};
use crate::runtime::{RuntimeHandle, RuntimeState};
use crate::values::NativeValue;
use crate::{
    AnimationOptions, BoundOptions, CameraFitOptions, CameraOptions, Error, ErrorKind,
    FreeCameraOptions, Geometry, HandleOperationError, LatLng, LatLngBounds, MapDebugOptions,
    MapOptions, MapProjectionHandle, MapTileOptions, MapViewportOptions, ProjectionMode, Result,
    ScreenPoint,
};
#[cfg(test)]
use crate::{GeoJson, JsonValue, PremultipliedRgba8Image};

mod style;
pub use style::{
    LocationIndicatorImageKind, RasterDemEncoding, SourceInfo, SourceType, StyleImage,
    StyleImageInfo, StyleImageOptions, TileScheme, TileSourceOptions, VectorTileEncoding,
};

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ThreadAffineNativeHandle<sys::mln_map>,
    runtime: RefCell<Option<Rc<RuntimeState>>>,
    id: MapId,
    custom_geometry_sources: RefCell<HashMap<String, Box<CustomGeometrySourceState>>>,
}

impl MapState {
    fn new(ptr: std::ptr::NonNull<sys::mln_map>, runtime: Rc<RuntimeState>, id: MapId) -> Self {
        // SAFETY: ptr came from successful mln_map_create and is paired with
        // the matching map destroy function.
        let handle =
            unsafe { ThreadAffineNativeHandle::from_raw(ptr, sys::mln_map_destroy, "mln_map") };
        Self {
            handle,
            runtime: RefCell::new(Some(runtime)),
            id,
            custom_geometry_sources: RefCell::new(HashMap::new()),
        }
    }

    pub(crate) fn as_ptr(&self) -> Result<*mut sys::mln_map> {
        let ptr = self.handle.as_ptr();
        if ptr.is_null() {
            Err(closed_handle_error("MapHandle"))
        } else {
            Ok(ptr)
        }
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        let ptr = self.handle.as_ptr();
        self.handle.close()?;
        if let Some(runtime) = self.runtime.borrow_mut().take() {
            runtime.unregister_map(ptr);
        }
        self.clear_custom_geometry_sources();
        Ok(())
    }

    pub(crate) fn clear_custom_geometry_sources(&self) {
        self.custom_geometry_sources.borrow_mut().clear();
    }

    pub(crate) fn release_detached_custom_geometry_sources(&self) {
        let map = match self.as_ptr() {
            Ok(map) => map,
            Err(_) => return,
        };
        let source_ids = self
            .custom_geometry_sources
            .borrow()
            .keys()
            .cloned()
            .collect::<Vec<_>>();
        let mut detached = Vec::new();
        for source_id in source_ids {
            let source_id_view = maplibre_core::string::string_view(&source_id);
            let mut source_type = 0;
            let mut found = false;
            // SAFETY: map is live, source_id_view is valid for this call, and
            // output pointers refer to writable storage.
            let status = unsafe {
                sys::mln_map_get_style_source_type(
                    map,
                    source_id_view.raw(),
                    &mut source_type,
                    &mut found,
                )
            };
            if status == sys::MLN_STATUS_OK
                && (!found || source_type != sys::MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR)
            {
                detached.push(source_id);
            }
        }
        if !detached.is_empty() {
            let mut sources = self.custom_geometry_sources.borrow_mut();
            for source_id in detached {
                sources.remove(&source_id);
            }
        }
    }
}

impl Drop for MapState {
    fn drop(&mut self) {
        if let Some(runtime) = self.runtime.borrow_mut().take() {
            runtime.unregister_map(self.handle.as_ptr());
        }
    }
}

/// Owner-thread map handle bound to a retained runtime.
pub struct MapHandle {
    pub(crate) inner: Rc<MapState>,
}

impl fmt::Debug for MapHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapHandle {
    /// Creates a map with explicit map options on the runtime owner thread.
    pub fn with_options(runtime: &RuntimeHandle, options: &MapOptions) -> Result<Self> {
        let runtime_ptr = runtime.inner.as_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_map>::new();
        let raw_options = options.to_native();

        // SAFETY: runtime_ptr is a live runtime handle. raw_options is a
        // materialized map descriptor with size filled by the binding. out is a
        // valid null-initialized out-pointer owned by this call.
        maplibre_core::check(unsafe {
            sys::mln_map_create(runtime_ptr, &raw_options, out.as_mut_ptr())
        })?;
        let ptr = out_handle(out, "mln_map")?;
        let id = runtime.inner.register_map(ptr.as_ptr());
        let state = Rc::new(MapState::new(ptr, Rc::clone(&runtime.inner), id));
        runtime
            .inner
            .register_map_state(ptr.as_ptr(), Rc::downgrade(&state));

        Ok(Self { inner: state })
    }

    /// Returns this map's runtime-local event source identity.
    pub fn id(&self) -> MapId {
        self.inner.id
    }

    #[cfg(test)]
    fn custom_geometry_source_count_for_testing(&self) -> usize {
        self.inner.custom_geometry_sources.borrow().len()
    }

    /// Explicitly destroys the map.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live in the shared state so future child
    /// handles can continue to retain and close the map safely.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.inner.is_closed() {
            return Ok(());
        }
        if Rc::strong_count(&self.inner) > 1 {
            return Err(HandleOperationError::new(
                Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "MapHandle cannot close while child handles are live",
                ),
                self,
            ));
        }
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }

    /// Requests a repaint for a continuous map.
    pub fn request_repaint(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is a live map handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_map_request_repaint(map) })
    }

    /// Requests one still image for a static or tile map.
    pub fn request_still_image(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is a live map handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_map_request_still_image(map) })
    }

    /// Applies MapLibre debug overlay mask bits.
    pub fn set_debug_options(&self, options: MapDebugOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates unknown mask bits.
        maplibre_core::check(unsafe { sys::mln_map_set_debug_options(map, options.bits()) })
    }

    /// Reads MapLibre debug overlay mask bits.
    pub fn debug_options(&self) -> Result<MapDebugOptions> {
        let map = self.inner.as_ptr()?;
        let mut raw = 0;
        // SAFETY: map is live and out_options points to writable u32 storage.
        maplibre_core::check(unsafe { sys::mln_map_get_debug_options(map, &mut raw) })?;
        Ok(MapDebugOptions::from_bits_retain(raw))
    }

    /// Enables or disables MapLibre's rendering stats overlay view.
    pub fn set_rendering_stats_view_enabled(&self, enabled: bool) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live and enabled is passed by value.
        maplibre_core::check(unsafe { sys::mln_map_set_rendering_stats_view_enabled(map, enabled) })
    }

    /// Reads whether MapLibre's rendering stats overlay view is enabled.
    pub fn rendering_stats_view_enabled(&self) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let mut enabled = false;
        // SAFETY: map is live and out_enabled points to writable bool storage.
        maplibre_core::check(unsafe {
            sys::mln_map_get_rendering_stats_view_enabled(map, &mut enabled)
        })?;
        Ok(enabled)
    }

    /// Reads whether MapLibre currently considers the map fully loaded.
    pub fn is_fully_loaded(&self) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let mut loaded = false;
        // SAFETY: map is live and out_loaded points to writable bool storage.
        maplibre_core::check(unsafe { sys::mln_map_is_fully_loaded(map, &mut loaded) })?;
        Ok(loaded)
    }

    /// Dumps map debug logs through MapLibre Native logging.
    pub fn dump_debug_logs(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live.
        maplibre_core::check(unsafe { sys::mln_map_dump_debug_logs(map) })
    }

    /// Reads live viewport and render-transform controls.
    pub fn viewport_options(&self) -> Result<MapViewportOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_viewport_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        maplibre_core::check(unsafe { sys::mln_map_get_viewport_options(map, &mut raw) })?;
        Ok(MapViewportOptions::from_native(raw))
    }

    /// Applies selected live viewport and render-transform controls.
    pub fn set_viewport_options(&self, options: &MapViewportOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        maplibre_core::check(unsafe { sys::mln_map_set_viewport_options(map, &raw) })
    }

    /// Reads tile prefetch and LOD tuning controls.
    pub fn tile_options(&self) -> Result<MapTileOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_tile_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        maplibre_core::check(unsafe { sys::mln_map_get_tile_options(map, &mut raw) })?;
        Ok(MapTileOptions::from_native(raw))
    }

    /// Applies selected tile prefetch and LOD tuning controls.
    pub fn set_tile_options(&self, options: &MapTileOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        maplibre_core::check(unsafe { sys::mln_map_set_tile_options(map, &raw) })
    }

    /// Reads the current camera snapshot.
    pub fn camera(&self) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        maplibre_core::check(unsafe { sys::mln_map_get_camera(map, &mut raw) })?;
        Ok(CameraOptions::from_native(raw))
    }

    /// Applies a camera jump command.
    pub fn jump_to(&self, camera: &CameraOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = camera.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        maplibre_core::check(unsafe { sys::mln_map_jump_to(map, &raw) })
    }

    /// Applies a camera ease transition command.
    pub fn ease_to(
        &self,
        camera: &CameraOptions,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and descriptors are valid for this call. A null
        // animation pointer requests native defaults.
        maplibre_core::check(unsafe {
            sys::mln_map_ease_to(map, &raw_camera, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a camera fly transition command.
    pub fn fly_to(
        &self,
        camera: &CameraOptions,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and descriptors are valid for this call. A null
        // animation pointer requests native defaults.
        maplibre_core::check(unsafe {
            sys::mln_map_fly_to(map, &raw_camera, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a screen-space pan command.
    pub fn move_by(&self, delta_x: f64, delta_y: f64) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates numeric values.
        maplibre_core::check(unsafe { sys::mln_map_move_by(map, delta_x, delta_y) })
    }

    /// Applies an animated screen-space pan command.
    pub fn move_by_animated(
        &self,
        delta_x: f64,
        delta_y: f64,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and the optional animation descriptor is valid
        // for this call. The C API validates numeric values.
        maplibre_core::check(unsafe {
            sys::mln_map_move_by_animated(map, delta_x, delta_y, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a screen-space zoom command.
    pub fn scale_by(&self, scale: f64, anchor: Option<ScreenPoint>) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_anchor = anchor.map(ScreenPoint::to_native);
        // SAFETY: map is live and the optional anchor pointer is valid for this
        // call. The C API validates numeric values.
        maplibre_core::check(unsafe {
            sys::mln_map_scale_by(map, scale, option_ptr(raw_anchor.as_ref()))
        })
    }

    /// Applies an animated screen-space zoom command.
    pub fn scale_by_animated(
        &self,
        scale: f64,
        anchor: Option<ScreenPoint>,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_anchor = anchor.map(ScreenPoint::to_native);
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and optional descriptors are valid for this call.
        // The C API validates numeric values.
        maplibre_core::check(unsafe {
            sys::mln_map_scale_by_animated(
                map,
                scale,
                option_ptr(raw_anchor.as_ref()),
                option_ptr(raw_animation.as_ref()),
            )
        })
    }

    /// Applies a screen-space rotate command.
    pub fn rotate_by(&self, first: ScreenPoint, second: ScreenPoint) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. Points are passed by value and validated by C.
        maplibre_core::check(unsafe {
            sys::mln_map_rotate_by(map, first.to_native(), second.to_native())
        })
    }

    /// Applies an animated screen-space rotate command.
    pub fn rotate_by_animated(
        &self,
        first: ScreenPoint,
        second: ScreenPoint,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and optional animation descriptor is valid for
        // this call. Points are passed by value and validated by C.
        maplibre_core::check(unsafe {
            sys::mln_map_rotate_by_animated(
                map,
                first.to_native(),
                second.to_native(),
                option_ptr(raw_animation.as_ref()),
            )
        })
    }

    /// Applies a pitch delta command.
    pub fn pitch_by(&self, pitch: f64) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates numeric values.
        maplibre_core::check(unsafe { sys::mln_map_pitch_by(map, pitch) })
    }

    /// Applies an animated pitch delta command.
    pub fn pitch_by_animated(
        &self,
        pitch: f64,
        animation: Option<&AnimationOptions>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_animation = animation.map(AnimationOptions::to_native);
        // SAFETY: map is live and optional animation descriptor is valid for
        // this call. The C API validates numeric values.
        maplibre_core::check(unsafe {
            sys::mln_map_pitch_by_animated(map, pitch, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Cancels active camera transitions.
    pub fn cancel_transitions(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live.
        maplibre_core::check(unsafe { sys::mln_map_cancel_transitions(map) })
    }

    /// Computes a camera that fits geographic bounds in the current viewport.
    pub fn camera_for_lat_lng_bounds(
        &self,
        bounds: LatLngBounds,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        let raw_fit = fit_options.map(CameraFitOptions::to_native);
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live, bounds is passed by value, optional fit options
        // are valid for this call, and raw_camera is writable.
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lng_bounds(
                map,
                bounds.to_native(),
                option_ptr(raw_fit.as_ref()),
                &mut raw_camera,
            )
        })?;
        Ok(CameraOptions::from_native(raw_camera))
    }

    /// Computes a camera that fits geographic coordinates in the current viewport.
    pub fn camera_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        if coordinates.is_empty() {
            return Err(Error::invalid_argument(
                "camera_for_lat_lngs requires at least one coordinate",
            ));
        }
        let raw_coordinates = lat_lngs_to_native(coordinates);
        let raw_fit = fit_options.map(CameraFitOptions::to_native);
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live, arrays are valid for coordinate_count non-empty
        // entries, optional fit options are valid, and raw_camera is writable.
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lngs(
                map,
                const_ptr_or_null(&raw_coordinates),
                raw_coordinates.len(),
                option_ptr(raw_fit.as_ref()),
                &mut raw_camera,
            )
        })?;
        Ok(CameraOptions::from_native(raw_camera))
    }

    /// Computes a camera that fits a geometry in the current viewport.
    pub fn camera_for_geometry(
        &self,
        geometry: &Geometry,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        let native_geometry = geometry.try_to_native()?;
        let raw_fit = fit_options.map(CameraFitOptions::to_native);
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw_camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live, native_geometry owns backing storage for the
        // duration of this call, optional fit options are valid, and raw_camera
        // is writable.
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_geometry(
                map,
                native_geometry.as_ptr(),
                option_ptr(raw_fit.as_ref()),
                &mut raw_camera,
            )
        })?;
        Ok(CameraOptions::from_native(raw_camera))
    }

    /// Computes wrapped geographic bounds for a camera in the current viewport.
    pub fn lat_lng_bounds_for_camera(&self, camera: &CameraOptions) -> Result<LatLngBounds> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let mut raw_bounds = empty_bounds();
        // SAFETY: map is live, raw_camera is a valid descriptor for this call,
        // and raw_bounds points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lng_bounds_for_camera(map, &raw_camera, &mut raw_bounds)
        })?;
        Ok(LatLngBounds::from_native(raw_bounds))
    }

    /// Computes unwrapped geographic bounds for a camera in the current viewport.
    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: &CameraOptions,
    ) -> Result<LatLngBounds> {
        let map = self.inner.as_ptr()?;
        let raw_camera = camera.to_native();
        let mut raw_bounds = empty_bounds();
        // SAFETY: map is live, raw_camera is a valid descriptor for this call,
        // and raw_bounds points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lng_bounds_for_camera_unwrapped(map, &raw_camera, &mut raw_bounds)
        })?;
        Ok(LatLngBounds::from_native(raw_bounds))
    }

    /// Reads map camera constraint options.
    pub fn bounds(&self) -> Result<BoundOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_bound_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        maplibre_core::check(unsafe { sys::mln_map_get_bounds(map, &mut raw) })?;
        Ok(BoundOptions::from_native(raw))
    }

    /// Applies selected map camera constraint options.
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_bounds(map, &raw) })
    }

    /// Reads the current free camera position and orientation.
    pub fn free_camera_options(&self) -> Result<FreeCameraOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_free_camera_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        maplibre_core::check(unsafe { sys::mln_map_get_free_camera_options(map, &mut raw) })?;
        Ok(FreeCameraOptions::from_native(raw))
    }

    /// Applies selected free camera position and orientation fields.
    pub fn set_free_camera_options(&self, options: &FreeCameraOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_free_camera_options(map, &raw) })
    }

    /// Reads current axonometric rendering options.
    pub fn projection_mode(&self) -> Result<ProjectionMode> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_projection_mode_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        maplibre_core::check(unsafe { sys::mln_map_get_projection_mode(map, &mut raw) })?;
        Ok(ProjectionMode::from_native(raw))
    }

    /// Applies selected axonometric rendering option fields.
    pub fn set_projection_mode(&self, mode: &ProjectionMode) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = mode.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_projection_mode(map, &raw) })
    }

    /// Converts a geographic world coordinate to a screen point for the current map.
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let map = self.inner.as_ptr()?;
        let mut raw_point = empty_screen_point();
        // SAFETY: map is live, coordinate is passed by value, and raw_point is
        // writable storage for the output.
        maplibre_core::check(unsafe {
            sys::mln_map_pixel_for_lat_lng(map, coordinate.to_native(), &mut raw_point)
        })?;
        Ok(ScreenPoint::from_native(raw_point))
    }

    /// Converts a screen point to a geographic world coordinate for the current map.
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        let map = self.inner.as_ptr()?;
        let mut raw_coordinate = empty_lat_lng();
        // SAFETY: map is live, point is passed by value, and raw_coordinate is
        // writable storage for the output.
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lng_for_pixel(map, point.to_native(), &mut raw_coordinate)
        })?;
        Ok(LatLng::from_native(raw_coordinate))
    }

    /// Converts geographic world coordinates to screen points for the current map.
    pub fn pixels_for_lat_lngs(&self, coordinates: &[LatLng]) -> Result<Vec<ScreenPoint>> {
        let map = self.inner.as_ptr()?;
        let raw_coordinates = lat_lngs_to_native(coordinates);
        let mut raw_points = vec![empty_screen_point(); coordinates.len()];
        // SAFETY: map is live. Input and output arrays are valid for len
        // entries, or null when len is 0.
        maplibre_core::check(unsafe {
            sys::mln_map_pixels_for_lat_lngs(
                map,
                const_ptr_or_null(&raw_coordinates),
                raw_coordinates.len(),
                mut_ptr_or_null(&mut raw_points),
            )
        })?;
        Ok(raw_points
            .into_iter()
            .map(ScreenPoint::from_native)
            .collect())
    }

    /// Converts screen points to geographic world coordinates for the current map.
    pub fn lat_lngs_for_pixels(&self, points: &[ScreenPoint]) -> Result<Vec<LatLng>> {
        let map = self.inner.as_ptr()?;
        let raw_points = screen_points_to_native(points);
        let mut raw_coordinates = vec![empty_lat_lng(); points.len()];
        // SAFETY: map is live. Input and output arrays are valid for len
        // entries, or null when len is 0.
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lngs_for_pixels(
                map,
                const_ptr_or_null(&raw_points),
                raw_points.len(),
                mut_ptr_or_null(&mut raw_coordinates),
            )
        })?;
        Ok(raw_coordinates
            .into_iter()
            .map(LatLng::from_native)
            .collect())
    }

    /// Creates a standalone projection snapshot from the current map transform.
    pub fn create_projection(&self) -> Result<MapProjectionHandle> {
        MapProjectionHandle::new(self)
    }

    /// Attaches a Metal native surface render target to this map.
    ///
    /// The layer and optional device pointers are backend-native handles. They
    /// must name valid Metal objects for this session and remain usable on the
    /// owner thread until the session is detached or closed.
    pub fn attach_metal_surface(
        &self,
        descriptor: &MetalSurfaceDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_metal_surface_attach(map, &raw, out) }
        })
    }

    /// Attaches a Vulkan native surface render target to this map.
    ///
    /// Vulkan handles are borrowed. They must remain valid and externally
    /// synchronized until the session is detached or closed.
    pub fn attach_vulkan_surface(
        &self,
        descriptor: &VulkanSurfaceDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_vulkan_surface_attach(map, &raw, out) }
        })
    }

    /// Attaches an OpenGL native surface render target to this map.
    ///
    /// OpenGL context provider and surface handles are borrowed. They must
    /// remain valid and externally synchronized until the session is detached
    /// or closed.
    pub fn attach_opengl_surface(
        &self,
        descriptor: &OpenGLSurfaceDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_opengl_surface_attach(map, &raw, out) }
        })
    }

    /// Attaches a Metal session-owned texture render target to this map.
    ///
    /// The device pointer must name a valid Metal device that remains usable on
    /// the owner thread until the session is detached or closed.
    pub fn attach_metal_owned_texture(
        &self,
        descriptor: &MetalOwnedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_metal_owned_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches a Metal caller-owned texture render target to this map.
    ///
    /// The texture pointer is borrowed. The caller owns the texture, keeps it
    /// valid until detach or close, and synchronizes use outside this session.
    pub fn attach_metal_borrowed_texture(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_metal_borrowed_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches a Vulkan session-owned texture render target to this map.
    ///
    /// Vulkan device and queue handles are borrowed. They must remain valid and
    /// externally synchronized until the session is detached or closed.
    pub fn attach_vulkan_owned_texture(
        &self,
        descriptor: &VulkanOwnedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_vulkan_owned_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches a Vulkan caller-owned texture render target to this map.
    ///
    /// Vulkan handles, image, and image view are borrowed. The caller owns the
    /// image resources, keeps them valid until detach or close, and handles
    /// queue-family ownership and synchronization outside this session.
    pub fn attach_vulkan_borrowed_texture(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_vulkan_borrowed_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches an OpenGL session-owned texture render target to this map.
    ///
    /// The context provider handles are borrowed. They must remain valid until
    /// the session is detached or closed. Host sampling must use a context in
    /// the same share group while the acquired frame remains open.
    pub fn attach_opengl_owned_texture(
        &self,
        descriptor: &OpenGLOwnedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_opengl_owned_texture_attach(map, &raw, out) }
        })
    }

    /// Attaches an OpenGL caller-owned texture render target to this map.
    ///
    /// The context provider handles and texture object are borrowed. The caller
    /// owns the texture, keeps it valid until detach or close, and synchronizes
    /// use outside this session.
    pub fn attach_opengl_borrowed_texture(
        &self,
        descriptor: &OpenGLBorrowedTextureDescriptor,
    ) -> Result<RenderSessionHandle> {
        let raw = descriptor.to_native();
        RenderSessionHandle::attach(self, |map, out| {
            // SAFETY: map is live, raw is a materialized descriptor valid for
            // this call, and out is a null-initialized out-pointer.
            unsafe { sys::mln_opengl_borrowed_texture_attach(map, &raw, out) }
        })
    }
}

#[cfg(test)]
mod tests;
