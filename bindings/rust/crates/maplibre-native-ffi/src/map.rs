use std::fmt;
use std::sync::{Arc, Mutex};

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::ptr::{const_ptr_or_null, option_ptr};
use maplibre_native_ffi_core::values::{lat_lngs_to_native, screen_points_to_native};
use maplibre_native_ffi_sys as sys;

#[cfg(test)]
use crate::PremultipliedRgba8Image;
use crate::camera::{
    BoundOptionsNativeExt, CameraDeltaNativeExt, CameraFitOptionsNativeExt, CameraOptionsNativeExt,
    CameraUpdateNativeExt, FreeCameraOptionsNativeExt, ProjectionModeNativeExt,
};
use crate::completion;
#[cfg(test)]
use crate::custom_geometry::CanonicalTileId;
use crate::events::MapId;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};
use crate::options::{MapOptionsNativeExt, MapTileOptionsNativeExt, MapViewportOptionsNativeExt};
use crate::render::{
    MetalBorrowedTextureDescriptor, MetalOwnedTextureDescriptor, MetalSurfaceDescriptor,
    OpenGLBorrowedTextureDescriptor, OpenGLOwnedTextureDescriptor, OpenGLSurfaceDescriptor,
    RenderSessionAttachOptions, RenderSessionAttachment, RenderSessionHandle,
    VulkanBorrowedTextureDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
    WebGpuBorrowedTextureDescriptor, WebGpuOwnedTextureDescriptor, WebGpuSurfaceDescriptor,
};
use crate::runtime::{RuntimeHandle, RuntimeState};
use crate::values::NativeValue;
use crate::{
    BoundOptions, CameraDelta, CameraFitOptions, CameraOptions, CameraSnapshot, CameraUpdate,
    Error, FreeCameraOptions, HandleOperationError, LatLng, LatLngBounds, MapDebugOptions,
    MapOptions, MapProjectionHandle, MapTileOptions, MapViewportOptions, NativeFuture,
    ProjectionMode, Result, RuntimeEventMask, ScreenPoint,
};

mod style;
pub use style::{
    GeoJsonSourceOptions, ImageContent, ImageStretch, LocationIndicatorImageKind, SourceInfo,
    SourceType, StyleImage, StyleImageInfo, StyleImageOptions, StyleImageStretches,
    StyleImageTextFit, StyleLayerInfo, StyleLayerVisibility, StyleTransitionOptions, TileJsonInfo,
    TileScheme, TileSourceOptions, VectorTileEncoding,
};

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ConcurrentNativeHandle<sys::mln_map>,
    runtime: Mutex<Option<Arc<RuntimeState>>>,
    id: MapId,
}

impl MapState {
    fn new(native: sys::mln_map, runtime: Arc<RuntimeState>, id: MapId) -> Result<Self> {
        // SAFETY: native came from a successful typed creation take and map
        // control state supports calls from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_map") }?;
        Ok(Self {
            handle,
            runtime: Mutex::new(Some(runtime)),
            id,
        })
    }

    pub(crate) fn native(&self) -> Result<sys::mln_map> {
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("MapHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<NativeFuture<()>> {
        let teardown = self.handle.close_with(|map| {
            // SAFETY: map is live and native consumes it only on success. A
            // rejected release leaves the completion state owned by submit.
            completion::submit(
                |completion| unsafe { sys::mln_map_release(map, completion) },
                completion::unit,
            )
        })?;
        self.runtime
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        Ok(teardown.unwrap_or_else(|| completion::ready(())))
    }
}

impl Drop for MapState {
    fn drop(&mut self) {
        if self.close().is_err() {
            self.handle.leak_for_report();
        }
        self.runtime
            .get_mut()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
    }
}

/// Any-thread map control handle.
pub struct MapHandle {
    pub(crate) inner: MapState,
}

/// Logical map extent in UI pixels and device-pixel scale.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct LogicalExtent {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

/// Immutable map state copied from the latest worker publication.
///
/// Every committed map command publishes a new generation in its completion,
/// so a snapshot whose `generation` is at or past that value observes the
/// commit.
#[derive(Debug, Clone, PartialEq)]
pub struct MapSnapshot {
    pub generation: u64,
    /// Debug overlay mask committed by [`MapHandle::set_debug_options`].
    pub debug_options: MapDebugOptions,
    pub camera: CameraOptions,
    pub logical_extent: LogicalExtent,
    pub projection_mode: ProjectionMode,
    pub viewport: MapViewportOptions,
    /// True once every requested style and tile resource finished loading.
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    pub repaint_demand: bool,
    pub event_mask: RuntimeEventMask,
    pub latest_render_update_generation: u64,
    pub tile: MapTileOptions,
    pub bounds: BoundOptions,
    pub free_camera: FreeCameraOptions,
}

impl fmt::Debug for MapHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapHandle {
    /// Creates a map on the runtime worker.
    pub fn with_options(
        runtime: &RuntimeHandle,
        options: &MapOptions,
    ) -> Result<NativeFuture<Self>> {
        let runtime_ptr = runtime.inner.native()?;
        let raw_options = options.to_native()?;
        let runtime_state = Arc::clone(&runtime.inner);
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_create(runtime_ptr, &raw_options, completion) },
            move |result| {
                let native = crate::completion::copy_value::<sys::mln_map>(result)?;
                if native.0 == 0 {
                    return Err(Error::invalid_argument(
                        "map creation returned a null handle",
                    ));
                }
                let id = MapId::new(native.0);
                Ok(Self {
                    inner: MapState::new(native, runtime_state, id)?,
                })
            },
        )
    }

    /// Returns this map's runtime-local event source identity.
    pub fn id(&self) -> MapId {
        self.inner.id
    }

    /// Explicitly releases the map's public handle.
    pub fn close(self) -> std::result::Result<NativeFuture<()>, HandleOperationError<Self>> {
        if self.inner.is_closed() {
            return Ok(completion::ready(()));
        }
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }

    /// Closes this map and waits for retirement when the test target permits a
    /// synchronous wait.
    #[cfg(test)]
    pub(crate) fn close_and_wait(self) {
        let completion = self
            .close()
            .map_err(HandleOperationError::into_error)
            .expect("native close submission failed");
        #[cfg(not(target_os = "emscripten"))]
        completion::blocking(Ok(completion));
        // Emscripten graphics teardown can require the test's pthread. The
        // browser runner isolates every integration test in its own process,
        // which provides the resource boundary without blocking that thread.
        #[cfg(target_os = "emscripten")]
        drop(completion);
    }

    /// Copies the latest immutable map publication.
    pub fn snapshot(&self) -> Result<MapSnapshot> {
        let map = self.inner.native()?;
        let mut raw = sys::mln_map_snapshot {
            size: std::mem::size_of::<sys::mln_map_snapshot>() as u32,
            debug_options: 0,
            generation: 0,
            // SAFETY: these constructors initialize the nested ABI descriptors.
            camera: unsafe { sys::mln_camera_options_default() },
            logical_extent: sys::mln_logical_extent {
                width: 0,
                height: 0,
                scale_factor: 0.0,
            },
            projection_mode: unsafe { sys::mln_projection_mode_default() },
            viewport: unsafe { sys::mln_map_viewport_options_default() },
            fully_loaded: false,
            rendering_stats_view_enabled: false,
            repaint_demand: false,
            reserved_flags: 0,
            event_mask: 0,
            latest_render_update_generation: 0,
            tile: unsafe { sys::mln_map_tile_options_default() },
            bounds: unsafe { sys::mln_bound_options_default() },
            free_camera: unsafe { sys::mln_free_camera_options_default() },
        };
        // SAFETY: map is live and raw is a size-tagged writable descriptor.
        maplibre_core::check(unsafe { sys::mln_map_snapshot_get(map, &mut raw) })?;
        Ok(MapSnapshot {
            generation: raw.generation,
            debug_options: MapDebugOptions::from_bits_retain(raw.debug_options),
            camera: CameraOptions::from_native(raw.camera),
            logical_extent: LogicalExtent {
                width: raw.logical_extent.width,
                height: raw.logical_extent.height,
                scale_factor: raw.logical_extent.scale_factor,
            },
            projection_mode: ProjectionMode::from_native(raw.projection_mode),
            viewport: MapViewportOptions::from_native(raw.viewport),
            fully_loaded: raw.fully_loaded,
            rendering_stats_view_enabled: raw.rendering_stats_view_enabled,
            repaint_demand: raw.repaint_demand,
            event_mask: RuntimeEventMask::from_bits_retain(raw.event_mask),
            latest_render_update_generation: raw.latest_render_update_generation,
            tile: MapTileOptions::from_native(raw.tile),
            bounds: BoundOptions::from_native(raw.bounds),
            free_camera: FreeCameraOptions::from_native(raw.free_camera),
        })
    }

    /// Submits the map's logical extent update.
    pub fn resize(&self, extent: LogicalExtent) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = sys::mln_logical_extent {
            width: extent.width,
            height: extent.height,
            scale_factor: extent.scale_factor,
        };
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_resize(map, raw, completion) },
            crate::completion::command,
        )
    }

    /// Requests a continuous-map repaint and returns its completion future.
    pub fn request_repaint(&self) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_request_repaint(map, completion) },
            crate::completion::command,
        )
    }

    /// Starts one noncoalescing still-image operation.
    pub fn request_still_image(&self) -> Result<NativeFuture<()>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_request_still_image(map, completion) },
            crate::completion::unit,
        )
    }

    /// Copies the latest published camera without entering the runtime queue.
    pub fn camera_snapshot(&self) -> Result<CameraSnapshot> {
        let map = self.inner.native()?;
        // SAFETY: the constructor initializes this ABI version's descriptor.
        let mut camera = unsafe { sys::mln_camera_options_default() };
        let mut generation = 0;
        // SAFETY: map is live and both outputs are writable.
        maplibre_core::check(unsafe {
            sys::mln_map_camera_snapshot_get(map, &mut camera, &mut generation)
        })?;
        Ok(CameraSnapshot {
            generation,
            camera: CameraOptions::from_native(camera),
        })
    }

    /// Submits one copied atomic camera update.
    pub fn update_camera(
        &self,
        update: &CameraUpdate,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = update.to_native();
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_update_camera(map, &raw, completion) },
            crate::completion::command,
        )
    }

    /// Submits one relative camera update.
    pub fn apply_camera_delta(
        &self,
        delta: &CameraDelta,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = delta.to_native();
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_apply_camera_delta(map, &raw, completion) },
            crate::completion::command,
        )
    }

    /// Starts an ordered camera query.
    pub fn camera_query(&self) -> Result<NativeFuture<CameraSnapshot>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_camera_query(map, completion) },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_camera_query_result>(result)?;
                Ok(CameraSnapshot {
                    generation: value.generation,
                    camera: CameraOptions::from_native(value.camera),
                })
            },
        )
    }

    /// Selects which map-originated event types this map queues and returns the
    /// completion future.
    pub fn set_event_mask(
        &self,
        mask: RuntimeEventMask,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_set_event_mask(map, mask.bits(), completion) },
            crate::completion::command,
        )
    }

    /// Submits a debug-overlay command.
    ///
    /// The committed mask is visible as [`MapSnapshot::debug_options`].
    pub fn set_debug_options(
        &self,
        options: MapDebugOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_set_debug_options(map, options.bits(), completion)
            },
            crate::completion::command,
        )
    }

    /// Submits a rendering-stats visibility command.
    ///
    /// The committed value is visible as
    /// [`MapSnapshot::rendering_stats_view_enabled`].
    pub fn set_rendering_stats_view_enabled(
        &self,
        enabled: bool,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_set_rendering_stats_view_enabled(map, enabled, completion)
            },
            crate::completion::command,
        )
    }

    /// Submits a viewport-options command; the committed options are visible
    /// as [`MapSnapshot::viewport`].
    pub fn set_viewport_options(
        &self,
        options: &MapViewportOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = options.to_native();
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_set_viewport_options(map, &raw, completion) },
            crate::completion::command,
        )
    }

    /// Submits a tile-options command; the committed options are visible as
    /// [`MapSnapshot::tile`].
    pub fn set_tile_options(
        &self,
        options: &MapTileOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = options.to_native();
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_set_tile_options(map, &raw, completion) },
            crate::completion::command,
        )
    }

    /// Submits a camera-constraint command; the committed constraints are
    /// visible as [`MapSnapshot::bounds`].
    pub fn set_bounds(
        &self,
        options: &BoundOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = options.to_native();
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_set_bounds(map, &raw, completion) },
            crate::completion::command,
        )
    }

    /// Submits a free-camera command; the committed options are visible as
    /// [`MapSnapshot::free_camera`].
    pub fn set_free_camera_options(
        &self,
        options: &FreeCameraOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = options.to_native();
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_set_free_camera_options(map, &raw, completion)
            },
            crate::completion::command,
        )
    }

    pub fn projection_mode(&self) -> Result<ProjectionMode> {
        Ok(self.snapshot()?.projection_mode)
    }

    pub fn set_projection_mode(
        &self,
        mode: &ProjectionMode,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        let map = self.inner.native()?;
        let raw = mode.to_native();
        crate::completion::submit(
            move |completion| unsafe { sys::mln_map_set_projection_mode(map, &raw, completion) },
            crate::completion::command,
        )
    }

    pub fn camera_for_lat_lng_bounds(
        &self,
        bounds: LatLngBounds,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let map = self.inner.native()?;
        let fit = fit_options.map(CameraFitOptions::to_native);
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_camera_for_lat_lng_bounds(
                    map,
                    bounds.to_native(),
                    option_ptr(fit.as_ref()),
                    completion,
                )
            },
            |result| {
                Ok(CameraOptions::from_native(crate::completion::copy_value(
                    result,
                )?))
            },
        )
    }

    pub fn camera_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        if coordinates.is_empty() {
            return Err(Error::invalid_argument(
                "camera_for_lat_lngs requires at least one coordinate",
            ));
        }
        let map = self.inner.native()?;
        let coordinates = lat_lngs_to_native(coordinates);
        let fit = fit_options.map(CameraFitOptions::to_native);
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_camera_for_lat_lngs(
                    map,
                    const_ptr_or_null(&coordinates),
                    coordinates.len(),
                    option_ptr(fit.as_ref()),
                    completion,
                )
            },
            |result| {
                Ok(CameraOptions::from_native(crate::completion::copy_value(
                    result,
                )?))
            },
        )
    }

    pub fn camera_for_geometry(
        &self,
        geometry: &[u8],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let map = self.inner.native()?;
        let fit = fit_options.map(CameraFitOptions::to_native);
        let geometry = geometry.to_vec();
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_camera_for_geometry(
                    map,
                    maplibre_core::string::buffer_view(&geometry),
                    option_ptr(fit.as_ref()),
                    completion,
                )
            },
            |result| {
                Ok(CameraOptions::from_native(crate::completion::copy_value(
                    result,
                )?))
            },
        )
    }

    fn bounds_for_camera(
        &self,
        camera: &CameraOptions,
        unwrapped: bool,
    ) -> Result<NativeFuture<LatLngBounds>> {
        let map = self.inner.native()?;
        let camera = camera.to_native();
        crate::completion::submit(
            move |completion| unsafe {
                if unwrapped {
                    sys::mln_map_lat_lng_bounds_for_camera_unwrapped(map, &camera, completion)
                } else {
                    sys::mln_map_lat_lng_bounds_for_camera(map, &camera, completion)
                }
            },
            |result| {
                Ok(LatLngBounds::from_native(crate::completion::copy_value(
                    result,
                )?))
            },
        )
    }

    pub fn lat_lng_bounds_for_camera(
        &self,
        camera: &CameraOptions,
    ) -> Result<NativeFuture<LatLngBounds>> {
        self.bounds_for_camera(camera, false)
    }

    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: &CameraOptions,
    ) -> Result<NativeFuture<LatLngBounds>> {
        self.bounds_for_camera(camera, true)
    }

    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<NativeFuture<ScreenPoint>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_pixel_for_lat_lng(map, coordinate.to_native(), completion)
            },
            |result| {
                Ok(ScreenPoint::from_native(crate::completion::copy_value(
                    result,
                )?))
            },
        )
    }

    fn coordinate_for_pixel(
        &self,
        point: ScreenPoint,
        unwrapped: bool,
    ) -> Result<NativeFuture<LatLng>> {
        let map = self.inner.native()?;
        crate::completion::submit(
            move |completion| unsafe {
                if unwrapped {
                    sys::mln_map_lat_lng_for_pixel_unwrapped(map, point.to_native(), completion)
                } else {
                    sys::mln_map_lat_lng_for_pixel(map, point.to_native(), completion)
                }
            },
            |result| Ok(LatLng::from_native(crate::completion::copy_value(result)?)),
        )
    }

    fn coordinates_for_pixels(
        &self,
        points: &[ScreenPoint],
        unwrapped: bool,
    ) -> Result<NativeFuture<Vec<LatLng>>> {
        let map = self.inner.native()?;
        let points = screen_points_to_native(points);
        crate::completion::submit(
            move |completion| unsafe {
                if unwrapped {
                    sys::mln_map_lat_lngs_for_pixels_unwrapped(
                        map,
                        const_ptr_or_null(&points),
                        points.len(),
                        completion,
                    )
                } else {
                    sys::mln_map_lat_lngs_for_pixels(
                        map,
                        const_ptr_or_null(&points),
                        points.len(),
                        completion,
                    )
                }
            },
            |result| {
                Ok(crate::completion::copy_slice::<sys::mln_lat_lng>(result)?
                    .into_iter()
                    .map(LatLng::from_native)
                    .collect())
            },
        )
    }

    /// Converts a screen point to a geographic coordinate.
    ///
    /// The longitude is wrapped to the range from -180 to 180 degrees.
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<NativeFuture<LatLng>> {
        self.coordinate_for_pixel(point, false)
    }

    /// Converts a screen point to an unwrapped geographic coordinate.
    ///
    /// The longitude preserves the visible world copy and may fall outside
    /// -180 to 180.
    pub fn lat_lng_for_pixel_unwrapped(&self, point: ScreenPoint) -> Result<NativeFuture<LatLng>> {
        self.coordinate_for_pixel(point, true)
    }

    pub fn pixels_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
    ) -> Result<NativeFuture<Vec<ScreenPoint>>> {
        let map = self.inner.native()?;
        let coordinates = lat_lngs_to_native(coordinates);
        crate::completion::submit(
            move |completion| unsafe {
                sys::mln_map_pixels_for_lat_lngs(
                    map,
                    const_ptr_or_null(&coordinates),
                    coordinates.len(),
                    completion,
                )
            },
            |result| {
                Ok(
                    crate::completion::copy_slice::<sys::mln_screen_point>(result)?
                        .into_iter()
                        .map(ScreenPoint::from_native)
                        .collect(),
                )
            },
        )
    }

    /// Converts screen points to geographic coordinates.
    ///
    /// Each longitude is wrapped to the range from -180 to 180 degrees.
    pub fn lat_lngs_for_pixels(&self, points: &[ScreenPoint]) -> Result<NativeFuture<Vec<LatLng>>> {
        self.coordinates_for_pixels(points, false)
    }

    /// Converts screen points to unwrapped geographic coordinates.
    ///
    /// Each longitude preserves its visible world copy and may fall outside
    /// -180 to 180.
    pub fn lat_lngs_for_pixels_unwrapped(
        &self,
        points: &[ScreenPoint],
    ) -> Result<NativeFuture<Vec<LatLng>>> {
        self.coordinates_for_pixels(points, true)
    }

    /// Creates a standalone projection snapshot from the current map transform.
    pub fn create_projection(&self) -> Result<NativeFuture<MapProjectionHandle>> {
        MapProjectionHandle::new(self)
    }
}

impl MapHandle {
    pub fn attach_metal_surface(
        &self,
        value: &MetalSurfaceDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_metal_surface_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_vulkan_surface(
        &self,
        value: &VulkanSurfaceDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_vulkan_surface_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_webgpu_surface(
        &self,
        value: &WebGpuSurfaceDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_webgpu_surface_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_opengl_surface(
        &self,
        value: &OpenGLSurfaceDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_opengl_surface_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_metal_owned_texture(
        &self,
        value: &MetalOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_metal_owned_texture_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_metal_borrowed_texture(
        &self,
        value: &MetalBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_metal_borrowed_texture_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_vulkan_owned_texture(
        &self,
        value: &VulkanOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_vulkan_owned_texture_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_vulkan_borrowed_texture(
        &self,
        value: &VulkanBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_vulkan_borrowed_texture_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_webgpu_owned_texture(
        &self,
        value: &WebGpuOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_webgpu_owned_texture_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_webgpu_borrowed_texture(
        &self,
        value: &WebGpuBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_webgpu_borrowed_texture_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_opengl_owned_texture(
        &self,
        value: &OpenGLOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_opengl_owned_texture_attach(map, &raw, options, session, operation)
        })
    }

    pub fn attach_opengl_borrowed_texture(
        &self,
        value: &OpenGLBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_opengl_borrowed_texture_attach(map, &raw, options, session, operation)
        })
    }
}

#[cfg(test)]
mod tests;
