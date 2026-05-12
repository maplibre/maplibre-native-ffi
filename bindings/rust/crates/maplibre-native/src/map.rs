use std::cell::RefCell;
use std::fmt;
use std::ptr;
use std::rc::Rc;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::events::MapId;
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::runtime::{RuntimeHandle, RuntimeState};
use crate::{
    AnimationOptions, BoundOptions, CameraFitOptions, CameraOptions, Error, FreeCameraOptions,
    GeoJson, Geometry, JsonValue, LatLng, LatLngBounds, MapDebugOptions, MapOptions,
    MapProjectionHandle, MapTileOptions, MapViewportOptions, ProjectionMode, Result, ScreenPoint,
};

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ThreadAffineNativeHandle<sys::mln_map>,
    runtime: RefCell<Option<Rc<RuntimeState>>>,
    id: MapId,
}

impl MapState {
    fn new(ptr: std::ptr::NonNull<sys::mln_map>, runtime: Rc<RuntimeState>) -> Self {
        // SAFETY: ptr came from successful mln_map_create and is paired with
        // the matching map destroy function.
        let handle =
            unsafe { ThreadAffineNativeHandle::from_raw(ptr, sys::mln_map_destroy, "mln_map") };
        let id = runtime.register_map(ptr.as_ptr());
        Self {
            handle,
            runtime: RefCell::new(Some(runtime)),
            id,
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
        Ok(())
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
    /// Creates a map with native default map options on the runtime owner thread.
    pub fn new(runtime: &RuntimeHandle) -> Result<Self> {
        Self::with_options(runtime, &MapOptions::default())
    }

    /// Creates a map with explicit map options on the runtime owner thread.
    pub fn with_options(runtime: &RuntimeHandle, options: &MapOptions) -> Result<Self> {
        let runtime_ptr = runtime.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_map>::new();
        let raw_options = options.to_native();

        // SAFETY: runtime_ptr is a live runtime handle. raw_options is a
        // materialized map descriptor with size filled by the binding. out is a
        // valid null-initialized out-pointer owned by this call.
        support::check(unsafe {
            sys::mln_map_create(runtime_ptr, &raw_options, out.as_mut_ptr())
        })?;
        let ptr = out_handle(out, "mln_map")?;

        Ok(Self {
            inner: Rc::new(MapState::new(ptr, Rc::clone(&runtime.inner))),
        })
    }

    /// Returns this map's runtime-local event source identity.
    pub fn id(&self) -> MapId {
        self.inner.id
    }

    /// Explicitly destroys the map.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live in the shared state so future child
    /// handles can continue to retain and close the map safely.
    pub fn close(&self) -> Result<()> {
        self.inner.close()
    }

    /// Requests a repaint for a continuous map.
    pub fn request_repaint(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is a live map handle owned by this wrapper.
        support::check(unsafe { sys::mln_map_request_repaint(map) })
    }

    /// Requests one still image for a static or tile map.
    pub fn request_still_image(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is a live map handle owned by this wrapper.
        support::check(unsafe { sys::mln_map_request_still_image(map) })
    }

    /// Loads a style URL through MapLibre Native style APIs.
    pub fn set_style_url(&self, url: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let url = support::string::c_string(url)?;
        // SAFETY: map is live and url is a NUL-terminated UTF-8 string valid
        // for the duration of this command. The C API copies/consumes it before
        // returning.
        support::check(unsafe { sys::mln_map_set_style_url(map, url.as_ptr()) })
    }

    /// Loads inline style JSON through MapLibre Native style APIs.
    pub fn set_style_json(&self, json: &str) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let json = support::string::c_string(json)?;
        // SAFETY: map is live and json is a NUL-terminated UTF-8 string valid
        // for the duration of this command. The C API copies/consumes it before
        // returning.
        support::check(unsafe { sys::mln_map_set_style_json(map, json.as_ptr()) })
    }

    /// Adds one style source from a style-spec source JSON object.
    pub fn add_style_source_json(&self, source_id: &str, source_json: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let source_json = source_json.try_to_native()?;
        // SAFETY: map is live, source_id is an explicit-length view valid for
        // this call, and source_json owns the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_add_style_source_json(map, source_id.raw(), source_json.as_ptr())
        })
    }

    /// Adds a GeoJSON source with inline data.
    pub fn add_geojson_source_data(&self, source_id: &str, data: &GeoJson) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, and data owns
        // the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_add_geojson_source_data(map, source_id.raw(), data.as_ptr())
        })
    }

    /// Updates one GeoJSON source with inline data.
    pub fn set_geojson_source_data(&self, source_id: &str, data: &GeoJson) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let source_id = support::string::string_view(source_id);
        let data = data.try_to_native()?;
        // SAFETY: map is live, source_id is valid for this call, and data owns
        // the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_set_geojson_source_data(map, source_id.raw(), data.as_ptr())
        })
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    pub fn add_style_layer_json(
        &self,
        layer_json: &JsonValue,
        before_layer_id: Option<&str>,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_json = layer_json.try_to_native()?;
        let before_layer_id = support::string::string_view(before_layer_id.unwrap_or(""));
        // SAFETY: map is live, layer_json owns the descriptor graph, and
        // before_layer_id is an explicit-length view valid for this call.
        support::check(unsafe {
            sys::mln_map_add_style_layer_json(map, layer_json.as_ptr(), before_layer_id.raw())
        })
    }

    /// Copies one style layer as a full style-spec JSON object.
    pub fn style_layer_json(&self, layer_id: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        let mut found = false;
        // SAFETY: map is live, layer_id is valid for this call, out is a
        // null-initialized out-pointer, and found points to writable storage.
        support::check(unsafe {
            sys::mln_map_get_style_layer_json(map, layer_id.raw(), out.as_mut_ptr(), &mut found)
        })?;
        let snapshot = json_snapshot(out.into_option())?;
        if found { Ok(snapshot) } else { Ok(None) }
    }

    /// Sets the style light from a style-spec light JSON object.
    pub fn set_style_light_json(&self, light_json: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let light_json = light_json.try_to_native()?;
        // SAFETY: map is live and light_json owns the descriptor graph for this call.
        support::check(unsafe { sys::mln_map_set_style_light_json(map, light_json.as_ptr()) })
    }

    /// Sets one style light property.
    pub fn set_style_light_property(&self, property_name: &str, value: &JsonValue) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let property_name = support::string::string_view(property_name);
        let value = value.try_to_native()?;
        // SAFETY: map is live, property_name is valid for this call, and value
        // owns the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_set_style_light_property(map, property_name.raw(), value.as_ptr())
        })
    }

    /// Copies one style light property as a style-spec JSON value.
    pub fn style_light_property(&self, property_name: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let property_name = support::string::string_view(property_name);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, property_name is valid for this call, and out is
        // a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_map_get_style_light_property(map, property_name.raw(), out.as_mut_ptr())
        })?;
        json_snapshot(out.into_option())
    }

    /// Sets one layer style property.
    pub fn set_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &JsonValue,
    ) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let property_name = support::string::string_view(property_name);
        let value = value.try_to_native()?;
        // SAFETY: map is live, string views are valid for this call, and value
        // owns the descriptor graph for this call.
        support::check(unsafe {
            sys::mln_map_set_layer_property(
                map,
                layer_id.raw(),
                property_name.raw(),
                value.as_ptr(),
            )
        })
    }

    /// Copies one layer style property as a style-spec JSON value.
    pub fn layer_property(&self, layer_id: &str, property_name: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let property_name = support::string::string_view(property_name);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, string views are valid for this call, and out is
        // a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_map_get_layer_property(
                map,
                layer_id.raw(),
                property_name.raw(),
                out.as_mut_ptr(),
            )
        })?;
        json_snapshot(out.into_option())
    }

    /// Sets or clears one layer filter.
    pub fn set_layer_filter(&self, layer_id: &str, filter: Option<&JsonValue>) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let native_filter = filter.map(JsonValue::try_to_native).transpose()?;
        // SAFETY: map is live, layer_id is valid for this call, and the
        // optional filter descriptor is either null or valid for this call.
        support::check(unsafe {
            sys::mln_map_set_layer_filter(
                map,
                layer_id.raw(),
                native_filter
                    .as_ref()
                    .map_or(ptr::null(), |filter| filter.as_ptr()),
            )
        })
    }

    /// Copies one layer filter as a style-spec JSON value.
    pub fn layer_filter(&self, layer_id: &str) -> Result<Option<JsonValue>> {
        let map = self.inner.as_ptr()?;
        let layer_id = support::string::string_view(layer_id);
        let mut out = support::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: map is live, layer_id is valid for this call, and out is a
        // null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_map_get_layer_filter(map, layer_id.raw(), out.as_mut_ptr())
        })?;
        json_snapshot(out.into_option())
    }

    /// Copies current style source IDs into owned Rust strings.
    pub fn style_source_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        support::check(unsafe { sys::mln_map_list_style_source_ids(map, out.as_mut_ptr()) })?;
        let list = out_handle(out, "mln_style_id_list")?;
        // SAFETY: list came from mln_map_list_style_source_ids and is owned by
        // this function until the guard drops.
        let list = unsafe { support::handle::style_id_list(list.as_ptr()) }?;
        copy_style_id_list(&list)
    }

    /// Copies current style layer IDs into owned Rust strings.
    pub fn style_layer_ids(&self) -> Result<Vec<String>> {
        let map = self.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_style_id_list>::new();
        // SAFETY: map is live and out is a null-initialized out-pointer owned by
        // this call. On success the returned handle is wrapped and destroyed by
        // the copying helper below.
        support::check(unsafe { sys::mln_map_list_style_layer_ids(map, out.as_mut_ptr()) })?;
        let list = out_handle(out, "mln_style_id_list")?;
        // SAFETY: list came from mln_map_list_style_layer_ids and is owned by
        // this function until the guard drops.
        let list = unsafe { support::handle::style_id_list(list.as_ptr()) }?;
        copy_style_id_list(&list)
    }

    /// Applies MapLibre debug overlay mask bits.
    pub fn set_debug_options(&self, options: MapDebugOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates unknown mask bits.
        support::check(unsafe { sys::mln_map_set_debug_options(map, options.bits()) })
    }

    /// Reads MapLibre debug overlay mask bits.
    pub fn debug_options(&self) -> Result<MapDebugOptions> {
        let map = self.inner.as_ptr()?;
        let mut raw = 0;
        // SAFETY: map is live and out_options points to writable u32 storage.
        support::check(unsafe { sys::mln_map_get_debug_options(map, &mut raw) })?;
        Ok(MapDebugOptions::from_bits_retain(raw))
    }

    /// Enables or disables MapLibre's rendering stats overlay view.
    pub fn set_rendering_stats_view_enabled(&self, enabled: bool) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live and enabled is passed by value.
        support::check(unsafe { sys::mln_map_set_rendering_stats_view_enabled(map, enabled) })
    }

    /// Reads whether MapLibre's rendering stats overlay view is enabled.
    pub fn rendering_stats_view_enabled(&self) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let mut enabled = false;
        // SAFETY: map is live and out_enabled points to writable bool storage.
        support::check(unsafe {
            sys::mln_map_get_rendering_stats_view_enabled(map, &mut enabled)
        })?;
        Ok(enabled)
    }

    /// Reads whether MapLibre currently considers the map fully loaded.
    pub fn is_fully_loaded(&self) -> Result<bool> {
        let map = self.inner.as_ptr()?;
        let mut loaded = false;
        // SAFETY: map is live and out_loaded points to writable bool storage.
        support::check(unsafe { sys::mln_map_is_fully_loaded(map, &mut loaded) })?;
        Ok(loaded)
    }

    /// Dumps map debug logs through MapLibre Native logging.
    pub fn dump_debug_logs(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live.
        support::check(unsafe { sys::mln_map_dump_debug_logs(map) })
    }

    /// Reads live viewport and render-transform controls.
    pub fn viewport_options(&self) -> Result<MapViewportOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_viewport_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_viewport_options(map, &mut raw) })?;
        Ok(MapViewportOptions::from_native(raw))
    }

    /// Applies selected live viewport and render-transform controls.
    pub fn set_viewport_options(&self, options: &MapViewportOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        support::check(unsafe { sys::mln_map_set_viewport_options(map, &raw) })
    }

    /// Reads tile prefetch and LOD tuning controls.
    pub fn tile_options(&self) -> Result<MapTileOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_map_tile_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_tile_options(map, &mut raw) })?;
        Ok(MapTileOptions::from_native(raw))
    }

    /// Applies selected tile prefetch and LOD tuning controls.
    pub fn set_tile_options(&self, options: &MapTileOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        support::check(unsafe { sys::mln_map_set_tile_options(map, &raw) })
    }

    /// Reads the current camera snapshot.
    pub fn camera(&self) -> Result<CameraOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_camera_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_camera(map, &mut raw) })?;
        Ok(CameraOptions::from_native(raw))
    }

    /// Applies a camera jump command.
    pub fn jump_to(&self, camera: &CameraOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = camera.to_native();
        // SAFETY: map is live and raw is a materialized descriptor valid for
        // the duration of this call.
        support::check(unsafe { sys::mln_map_jump_to(map, &raw) })
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
        support::check(unsafe {
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
        support::check(unsafe {
            sys::mln_map_fly_to(map, &raw_camera, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a screen-space pan command.
    pub fn move_by(&self, delta_x: f64, delta_y: f64) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live. The C API validates numeric values.
        support::check(unsafe { sys::mln_map_move_by(map, delta_x, delta_y) })
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
        support::check(unsafe {
            sys::mln_map_move_by_animated(map, delta_x, delta_y, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Applies a screen-space zoom command.
    pub fn scale_by(&self, scale: f64, anchor: Option<ScreenPoint>) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw_anchor = anchor.map(ScreenPoint::to_native);
        // SAFETY: map is live and the optional anchor pointer is valid for this
        // call. The C API validates numeric values.
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe { sys::mln_map_pitch_by(map, pitch) })
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
        support::check(unsafe {
            sys::mln_map_pitch_by_animated(map, pitch, option_ptr(raw_animation.as_ref()))
        })
    }

    /// Cancels active camera transitions.
    pub fn cancel_transitions(&self) -> Result<()> {
        let map = self.inner.as_ptr()?;
        // SAFETY: map is live.
        support::check(unsafe { sys::mln_map_cancel_transitions(map) })
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
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe { sys::mln_map_get_bounds(map, &mut raw) })?;
        Ok(BoundOptions::from_native(raw))
    }

    /// Applies selected map camera constraint options.
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        support::check(unsafe { sys::mln_map_set_bounds(map, &raw) })
    }

    /// Reads the current free camera position and orientation.
    pub fn free_camera_options(&self) -> Result<FreeCameraOptions> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_free_camera_options_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_free_camera_options(map, &mut raw) })?;
        Ok(FreeCameraOptions::from_native(raw))
    }

    /// Applies selected free camera position and orientation fields.
    pub fn set_free_camera_options(&self, options: &FreeCameraOptions) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = options.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        support::check(unsafe { sys::mln_map_set_free_camera_options(map, &raw) })
    }

    /// Reads current axonometric rendering options.
    pub fn projection_mode(&self) -> Result<ProjectionMode> {
        let map = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_projection_mode_default() };
        // SAFETY: map is live and raw has a valid size field for C to fill.
        support::check(unsafe { sys::mln_map_get_projection_mode(map, &mut raw) })?;
        Ok(ProjectionMode::from_native(raw))
    }

    /// Applies selected axonometric rendering option fields.
    pub fn set_projection_mode(&self, mode: &ProjectionMode) -> Result<()> {
        let map = self.inner.as_ptr()?;
        let raw = mode.to_native();
        // SAFETY: map is live and raw is a valid descriptor for this call.
        support::check(unsafe { sys::mln_map_set_projection_mode(map, &raw) })
    }

    /// Converts a geographic world coordinate to a screen point for the current map.
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let map = self.inner.as_ptr()?;
        let mut raw_point = empty_screen_point();
        // SAFETY: map is live, coordinate is passed by value, and raw_point is
        // writable storage for the output.
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe {
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
        support::check(unsafe {
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
}

pub(crate) fn empty_lat_lng() -> sys::mln_lat_lng {
    sys::mln_lat_lng {
        latitude: 0.0,
        longitude: 0.0,
    }
}

pub(crate) fn empty_screen_point() -> sys::mln_screen_point {
    sys::mln_screen_point { x: 0.0, y: 0.0 }
}

pub(crate) fn empty_bounds() -> sys::mln_lat_lng_bounds {
    sys::mln_lat_lng_bounds {
        southwest: empty_lat_lng(),
        northeast: empty_lat_lng(),
    }
}

pub(crate) fn option_ptr<T>(value: Option<&T>) -> *const T {
    value.map_or(ptr::null(), |value| value as *const T)
}

pub(crate) fn const_ptr_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    }
}

pub(crate) fn mut_ptr_or_null<T>(values: &mut [T]) -> *mut T {
    if values.is_empty() {
        ptr::null_mut()
    } else {
        values.as_mut_ptr()
    }
}

pub(crate) fn lat_lngs_to_native(coordinates: &[LatLng]) -> Vec<sys::mln_lat_lng> {
    coordinates.iter().copied().map(LatLng::to_native).collect()
}

pub(crate) fn screen_points_to_native(points: &[ScreenPoint]) -> Vec<sys::mln_screen_point> {
    points.iter().copied().map(ScreenPoint::to_native).collect()
}

fn json_snapshot(
    snapshot: Option<std::ptr::NonNull<sys::mln_json_snapshot>>,
) -> Result<Option<JsonValue>> {
    let Some(snapshot) = snapshot else {
        return Ok(None);
    };
    // SAFETY: snapshot is an owned JSON snapshot returned by the C API and is
    // destroyed by the guard after copying.
    let snapshot = unsafe { support::handle::json_snapshot(snapshot.as_ptr()) }?;
    let mut value = ptr::null();
    // SAFETY: snapshot is live and value points to writable storage. The
    // borrowed JSON value is copied before the guard drops.
    support::check(unsafe { sys::mln_json_snapshot_get(snapshot.as_ptr(), &mut value) })?;
    if value.is_null() {
        return Ok(None);
    }
    // SAFETY: value is borrowed from the live snapshot guard and copied before
    // the guard drops at the end of this function.
    unsafe { JsonValue::from_native(&*value) }.map(Some)
}

fn copy_style_id_list(list: &support::handle::StyleIdListGuard) -> Result<Vec<String>> {
    let mut count = 0;
    // SAFETY: list is a live style ID list guard and count points to writable storage.
    support::check(unsafe { sys::mln_style_id_list_count(list.as_ptr(), &mut count) })?;

    let mut ids = Vec::with_capacity(count);
    for index in 0..count {
        let mut view = sys::mln_string_view {
            data: ptr::null(),
            size: 0,
        };
        // SAFETY: list is live, index is less than count, and view points to
        // writable storage. The borrowed view is copied before the next loop
        // iteration and before the guard drops.
        support::check(unsafe { sys::mln_style_id_list_get(list.as_ptr(), index, &mut view) })?;
        // SAFETY: The C API returns a view into list-owned storage that remains
        // valid until the list guard drops at the end of this function.
        ids.push(unsafe { support::string::copy_string_view(view) }?);
    }
    Ok(ids)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        ConstrainMode, EdgeInsets, ErrorKind, Feature, FeatureIdentifier, JsonMember, MapMode,
        NorthOrientation, TileLodMode, ViewportMode,
    };

    const VALID_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;
    const STYLE_WITH_IDS_JSON: &str = r#"{"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"background","type":"background"},{"id":"geo-fill","type":"fill","source":"geo"}]}"#;

    fn object_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
        let JsonValue::Object(members) = value else {
            return None;
        };
        members
            .iter()
            .find(|member| member.key == key)
            .map(|member| &member.value)
    }

    #[test]
    fn map_create_and_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_create_with_options_and_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let options = MapOptions::new(320, 240, 2.0).with_mode(MapMode::Static);
        let map = MapHandle::with_options(&runtime, &options).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_close_is_idempotent() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.close().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_retains_runtime_after_runtime_handle_is_dropped() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        drop(runtime);

        map.close().unwrap();
    }

    #[test]
    fn style_setters_accept_valid_input_and_reject_embedded_nul() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.set_style_json(VALID_STYLE_JSON).unwrap();
        let _ = map.style_source_ids().unwrap();
        let _ = map.style_layer_ids().unwrap();

        map.set_style_json(STYLE_WITH_IDS_JSON).unwrap();
        let source_ids = map.style_source_ids().unwrap();
        let layer_ids = map.style_layer_ids().unwrap();
        assert!(source_ids.iter().any(|id| id == "geo"));
        assert!(layer_ids.iter().any(|id| id == "background"));
        assert!(layer_ids.iter().any(|id| id == "geo-fill"));

        map.set_style_url("https://example.com/style.json").unwrap();

        let error = map
            .set_style_url("https://example.com/\0style.json")
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("embedded NUL"));

        let error = map.set_style_json("{").unwrap_err();
        assert!(matches!(
            error.kind(),
            ErrorKind::InvalidArgument | ErrorKind::NativeError
        ));
        assert!(error.raw_status().is_some());
        assert!(!error.diagnostic().trim().is_empty());

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn style_json_and_geojson_descriptors_call_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        map.set_style_json(VALID_STYLE_JSON).unwrap();

        let source = JsonValue::Object(vec![
            JsonMember::new("type", JsonValue::String("geojson".to_owned())),
            JsonMember::new(
                "data",
                JsonValue::Object(vec![
                    JsonMember::new("type", JsonValue::String("FeatureCollection".to_owned())),
                    JsonMember::new("features", JsonValue::Array(Vec::new())),
                ]),
            ),
        ]);
        map.add_style_source_json("owned-json-source", &source)
            .unwrap();

        let geojson = GeoJson::Feature(
            Feature::new(
                Geometry::Point(LatLng::new(1.0, 2.0)),
                vec![JsonMember::new("name", JsonValue::String("one".to_owned()))],
            )
            .with_identifier(FeatureIdentifier::String("feature-1".to_owned())),
        );
        map.add_geojson_source_data("owned-geojson-source", &geojson)
            .unwrap();
        map.set_geojson_source_data(
            "owned-geojson-source",
            &GeoJson::FeatureCollection(Vec::new()),
        )
        .unwrap();

        let layer = JsonValue::Object(vec![
            JsonMember::new("id", JsonValue::String("owned-background".to_owned())),
            JsonMember::new("type", JsonValue::String("background".to_owned())),
            JsonMember::new(
                "paint",
                JsonValue::Object(vec![JsonMember::new(
                    "background-opacity",
                    JsonValue::Double(0.5),
                )]),
            ),
        ]);
        map.add_style_layer_json(&layer, None).unwrap();
        let copied_layer = map
            .style_layer_json("owned-background")
            .unwrap()
            .expect("added layer should have a JSON snapshot");
        assert_eq!(
            object_member(&copied_layer, "id"),
            Some(&JsonValue::String("owned-background".to_owned()))
        );
        assert_eq!(
            object_member(&copied_layer, "type"),
            Some(&JsonValue::String("background".to_owned()))
        );
        let paint = object_member(&copied_layer, "paint").expect("layer paint should be copied");
        assert_eq!(
            object_member(paint, "background-opacity"),
            Some(&JsonValue::Double(0.5))
        );
        map.set_layer_property(
            "owned-background",
            "background-opacity",
            &JsonValue::Double(0.75),
        )
        .unwrap();
        assert_eq!(
            map.layer_property("owned-background", "background-opacity")
                .unwrap(),
            Some(JsonValue::Double(0.75))
        );

        let error = map
            .set_layer_filter("owned-background", Some(&JsonValue::Double(f64::NAN)))
            .err()
            .unwrap();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn camera_jump_and_coordinate_conversions_round_trip() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = RuntimeHandle::create_map_with_options(
            &runtime,
            &MapOptions::new(512, 512, 1.0).with_mode(MapMode::Continuous),
        )
        .unwrap();
        let center = LatLng::new(45.0, -122.0);

        map.jump_to(&CameraOptions::new().with_center(center).with_zoom(4.0))
            .unwrap();
        let camera = map.camera().unwrap();
        assert_eq!(camera.center, Some(center));
        assert_eq!(camera.zoom, Some(4.0));

        let point = map.pixel_for_lat_lng(center).unwrap();
        let round_tripped = map.lat_lng_for_pixel(point).unwrap();
        assert!((round_tripped.latitude - center.latitude).abs() < 1e-7);
        assert!((round_tripped.longitude - center.longitude).abs() < 1e-7);

        let points = map.pixels_for_lat_lngs(&[center]).unwrap();
        let coordinates = map.lat_lngs_for_pixels(&points).unwrap();
        assert_eq!(points.len(), 1);
        assert!((coordinates[0].latitude - center.latitude).abs() < 1e-7);
        assert!((coordinates[0].longitude - center.longitude).abs() < 1e-7);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn camera_commands_and_queries_use_real_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();
        let camera = CameraOptions::new()
            .with_center(LatLng::new(0.0, 0.0))
            .with_zoom(1.0);
        let animation = AnimationOptions::new().with_duration_ms(0.0);

        map.ease_to(&camera, Some(&animation)).unwrap();
        map.fly_to(&camera, Some(&animation)).unwrap();
        map.move_by(0.0, 0.0).unwrap();
        map.move_by_animated(0.0, 0.0, Some(&animation)).unwrap();
        map.scale_by(1.0, Some(ScreenPoint::new(128.0, 128.0)))
            .unwrap();
        map.scale_by_animated(1.0, None, Some(&animation)).unwrap();
        map.rotate_by(ScreenPoint::new(0.0, 0.0), ScreenPoint::new(0.0, 0.0))
            .unwrap();
        map.rotate_by_animated(
            ScreenPoint::new(0.0, 0.0),
            ScreenPoint::new(0.0, 0.0),
            Some(&animation),
        )
        .unwrap();
        map.pitch_by(0.0).unwrap();
        map.pitch_by_animated(0.0, Some(&animation)).unwrap();
        map.cancel_transitions().unwrap();

        let bounds = LatLngBounds::new(LatLng::new(-1.0, -1.0), LatLng::new(1.0, 1.0));
        let fit = CameraFitOptions::new().with_padding(EdgeInsets::new(1.0, 1.0, 1.0, 1.0));
        map.camera_for_lat_lng_bounds(bounds, Some(&fit)).unwrap();
        map.camera_for_lat_lngs(&[LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)], Some(&fit))
            .unwrap();
        let error = map.camera_for_lat_lngs(&[], Some(&fit)).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("at least one coordinate"));
        map.camera_for_geometry(
            &Geometry::LineString(vec![LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)]),
            Some(&fit),
        )
        .unwrap();
        map.lat_lng_bounds_for_camera(&camera).unwrap();
        map.lat_lng_bounds_for_camera_unwrapped(&camera).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_state_viewport_tile_debug_and_projection_mode_round_trip() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.set_debug_options(MapDebugOptions::TILE_BORDERS | MapDebugOptions::PARSE_STATUS)
            .unwrap();
        let debug = map.debug_options().unwrap();
        assert!(debug.contains(MapDebugOptions::TILE_BORDERS));
        assert!(debug.contains(MapDebugOptions::PARSE_STATUS));

        map.set_rendering_stats_view_enabled(true).unwrap();
        assert!(map.rendering_stats_view_enabled().unwrap());
        assert!(!map.is_fully_loaded().unwrap());
        map.dump_debug_logs().unwrap();

        let viewport = MapViewportOptions::new()
            .with_north_orientation(NorthOrientation::Up)
            .with_constrain_mode(ConstrainMode::HeightOnly)
            .with_viewport_mode(ViewportMode::Default)
            .with_frustum_offset(EdgeInsets::new(0.0, 0.0, 0.0, 0.0));
        map.set_viewport_options(&viewport).unwrap();
        let copied_viewport = map.viewport_options().unwrap();
        assert_eq!(
            copied_viewport.north_orientation,
            Some(NorthOrientation::Up)
        );
        assert_eq!(
            copied_viewport.constrain_mode,
            Some(ConstrainMode::HeightOnly)
        );
        assert_eq!(copied_viewport.viewport_mode, Some(ViewportMode::Default));

        let tile = MapTileOptions::new()
            .with_prefetch_zoom_delta(1)
            .with_lod_mode(TileLodMode::Default);
        map.set_tile_options(&tile).unwrap();
        let copied_tile = map.tile_options().unwrap();
        assert_eq!(copied_tile.prefetch_zoom_delta, Some(1));
        assert_eq!(copied_tile.lod_mode, Some(TileLodMode::Default));

        let projection_mode = ProjectionMode::new()
            .with_axonometric(false)
            .with_x_skew(0.0)
            .with_y_skew(0.0);
        map.set_projection_mode(&projection_mode).unwrap();
        let copied_projection_mode = map.projection_mode().unwrap();
        assert_eq!(copied_projection_mode.axonometric, Some(false));

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn bounds_and_free_camera_operations_call_c_api() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        let bounds = BoundOptions::new()
            .with_bounds(LatLngBounds::new(
                LatLng::new(-10.0, -20.0),
                LatLng::new(10.0, 20.0),
            ))
            .with_min_zoom(0.0)
            .with_max_zoom(20.0)
            .with_min_pitch(0.0)
            .with_max_pitch(60.0);
        map.set_bounds(&bounds).unwrap();
        let copied_bounds = map.bounds().unwrap();
        assert_eq!(copied_bounds.min_zoom, Some(0.0));
        assert_eq!(copied_bounds.max_zoom, Some(20.0));

        let free = map.free_camera_options().unwrap();
        map.set_free_camera_options(&free).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }
}
