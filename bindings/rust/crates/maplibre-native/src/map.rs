use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::fmt;
use std::marker::PhantomData;
use std::ptr::NonNull;
use std::rc::Rc;
use std::sync::{Arc, RwLock};

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
    GeoJsonSourceOptions, LocationIndicatorImageKind, RasterDemEncoding, SourceInfo, SourceType,
    StyleImage, StyleImageInfo, StyleImageOptions, TileScheme, TileSourceOptions,
    VectorTileEncoding,
};

/// Cross-thread liveness for one map address.
///
/// The map handle publishes its address here and retires it when the native map
/// is destroyed, so a [`MapAttachRef`] on another thread observes a closed map
/// rather than an address a later map could reuse. Without this the reference
/// would carry a bare pointer, and the C API's registry lookup keys on the
/// pointer value, so an address reused by a new map would attach to the wrong
/// one.
#[derive(Debug)]
pub(crate) struct MapAddress(RwLock<Option<NonNull<sys::mln_map>>>);

// SAFETY: the pointer is only ever read out and handed to the C API, which
// validates it under its own registry lock. The `RwLock` is what makes the read
// and the native call that consumes it atomic against `retire`.
unsafe impl Send for MapAddress {}
unsafe impl Sync for MapAddress {}

impl MapAddress {
    fn new(ptr: NonNull<sys::mln_map>) -> Self {
        Self(RwLock::new(Some(ptr)))
    }

    /// Runs `use_ptr` with the map held live, or reports a closed handle.
    ///
    /// The guard spans the call, so a `close` on the map's owner thread waits
    /// rather than destroying the map midway through. Without that, the address
    /// could be freed between the read and the C API's registry lookup, and a
    /// map allocated at the same address would be attached to instead.
    fn with_live<T>(&self, use_ptr: impl FnOnce(*mut sys::mln_map) -> T) -> Option<T> {
        let guard = self
            .0
            .read()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        (*guard).map(|ptr| use_ptr(ptr.as_ptr()))
    }

    fn is_retired(&self) -> bool {
        self.0
            .read()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .is_none()
    }

    /// Blocks until no attach is in flight, then runs `close` and retires the
    /// address only if it succeeded.
    fn retire_with(&self, close: impl FnOnce() -> Result<()>) -> Result<()> {
        let mut guard = self
            .0
            .write()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        close()?;
        *guard = None;
        Ok(())
    }

    /// Retires without closing, for a drop whose destroy already succeeded.
    fn retire(&self) {
        let mut guard = self
            .0
            .write()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        *guard = None;
    }
}

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ThreadAffineNativeHandle<sys::mln_map>,
    address: Arc<MapAddress>,
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
            address: Arc::new(MapAddress::new(ptr)),
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
        // Retire under the same guard as the destroy, so an attach already
        // inside C finishes first and a later one sees a closed handle. A failed
        // close leaves the native map live and the address published.
        self.address.retire_with(|| self.handle.close())?;
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
        // Destroy here, under the address guard, so the address retires exactly
        // when the native map goes away. Leaving it to the handle's own `Drop`
        // would run the destroy after this body, so the address would stay
        // published for a map that no longer exists. A failed destroy, which is
        // what happens while a render session is still attached, leaves both the
        // map and the address live and is reported through the leak channel.
        let _ = self.address.retire_with(|| self.handle.close());
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
        let raw_options = options.to_native()?;

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
    ///
    /// Closing discards this map's queued runtime events and its recorded
    /// loading failure. There is no flush and no terminal event, so read any
    /// state you mirror from events before closing, and treat close as the end
    /// of this map's event stream rather than awaiting an event during
    /// teardown. Dropping the handle ends the stream the same way.
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

    /// Reads the map's logical viewport size in UI pixels and its pixel ratio.
    ///
    /// The size starts at the creation width and height, and follows the attach
    /// and resize rules documented on [`MapOptions`]. The scale factor is fixed
    /// for the lifetime of the map and is independent of any render target's
    /// scale factor.
    pub fn size(&self) -> Result<(u32, u32, f64)> {
        let map = self.inner.as_ptr()?;
        let mut width = 0u32;
        let mut height = 0u32;
        let mut scale_factor = 0f64;
        // SAFETY: map is live and all three out pointers reference live locals
        // for the duration of the call.
        maplibre_core::check(unsafe {
            sys::mln_map_get_size(map, &mut width, &mut height, &mut scale_factor)
        })?;
        Ok((width, height, scale_factor))
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
    ///
    /// An absent `animation`, or an animation with no duration, eases over zero
    /// duration: the camera reaches the target before this call returns, with
    /// no runtime pump in between. Set a duration to animate over time.
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
    ///
    /// Fly is the one camera command that animates by default. When duration is
    /// absent, native derives it from `AnimationOptions::velocity`; when
    /// velocity is absent too, native defaults to 1.2 screenfuls per second.
    /// The camera is therefore still en route when this call returns and
    /// advances as the runtime is pumped.
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
    ///
    /// Native routes this delta through the ease transition, so an absent
    /// `animation`, or an animation with no duration, applies the pan instantly
    /// like [`Self::ease_to`]. Set a duration to animate over time.
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
    ///
    /// Native routes this delta through the ease transition, so an absent
    /// `animation`, or an animation with no duration, applies the zoom
    /// instantly like [`Self::ease_to`]. Set a duration to animate over time.
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
    ///
    /// Native routes this delta through the ease transition, so an absent
    /// `animation`, or an animation with no duration, applies the rotation
    /// instantly like [`Self::ease_to`]. Set a duration to animate over time.
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
    ///
    /// Native routes this delta through the ease transition, so an absent
    /// `animation`, or an animation with no duration, applies the pitch
    /// instantly like [`Self::ease_to`]. Set a duration to animate over time.
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

    /// Produces a [`Send`] reference to this map for attaching a render session.
    ///
    /// A render session is owned by the thread that attaches it, which need not
    /// be the map's owner thread. [`MapHandle`] is `!Send`, so this is how the
    /// thread that drives a render loop names the map it renders while the map
    /// itself stays on the runtime owner thread.
    pub fn attach_ref(&self) -> Result<MapAttachRef> {
        self.inner.as_ptr()?;
        Ok(MapAttachRef {
            address: Arc::clone(&self.inner.address),
            _not_sync: PhantomData,
        })
    }
}

/// A reference to a map for the sole purpose of attaching a render session.
///
/// Produced by [`MapHandle::attach_ref`]. Every attach function lives here
/// rather than on [`MapHandle`], because attaching is the one map operation
/// that runs on the render session's thread instead of the map's.
///
/// This carries no Rust retention of the map, because [`MapHandle`] is `!Send`.
/// Native keeps the map alive instead: destroying a map fails while a render
/// session is attached to it. Closing the map retires the shared address, so a
/// reference that outlives its map reports a closed handle rather than binding
/// a session to whatever the allocator put at that address next.
///
/// Dropping a [`MapHandle`] instead of closing it, while a session is still
/// attached, leaks the native map: the destroy fails and an infallible `Drop`
/// cannot return the error. It reports the address through
/// [`set_leak_reporter`](crate::set_leak_reporter) instead, and the runtime
/// stays undestroyable until that map is destroyed. Close the session first,
/// then the map.
///
/// This is `Send` and deliberately not `Sync`: one thread holds it at a time.
/// It needs no `unsafe impl`, because the address it shares lives in an
/// `AtomicUsize` and the attach it performs reaches no thread-affine map state
/// — the C API claims the map's render-session slot under its registry lock and
/// posts the new size to the map's own owner thread.
#[derive(Clone)]
pub struct MapAttachRef {
    address: Arc<MapAddress>,
    _not_sync: PhantomData<Cell<()>>,
}

impl fmt::Debug for MapAttachRef {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapAttachRef").finish()
    }
}

impl MapAttachRef {
    /// Whether the map this reference names has been closed.
    ///
    /// A reference can outlive its [`MapHandle`], so a host that keeps one
    /// across a map's lifetime can check here instead of relying on the error
    /// from a failed attach.
    pub fn is_map_closed(&self) -> bool {
        self.address.is_retired()
    }

    /// Runs `attach` with the map held live for the duration of the call.
    pub(crate) fn with_live<T>(&self, attach: impl FnOnce(*mut sys::mln_map) -> T) -> Result<T> {
        self.address
            .with_live(attach)
            .ok_or_else(|| closed_handle_error("MapHandle"))
    }

    /// Attaches a Metal native surface render target to the map.
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

    /// Attaches a Vulkan native surface render target to the map.
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

    /// Attaches an OpenGL native surface render target to the map.
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

    /// Attaches a Metal session-owned texture render target to the map.
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

    /// Attaches a Metal caller-owned texture render target to the map.
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

    /// Attaches a Vulkan session-owned texture render target to the map.
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

    /// Attaches a Vulkan caller-owned texture render target to the map.
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

    /// Attaches an OpenGL session-owned texture render target to the map.
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

    /// Attaches an OpenGL caller-owned texture render target to the map.
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
