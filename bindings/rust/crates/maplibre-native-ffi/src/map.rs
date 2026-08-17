use std::fmt;
use std::sync::{Arc, Mutex};

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::ptr::{const_ptr_or_null, option_ptr};
use maplibre_native_ffi_core::values::{
    empty_lat_lng_bounds, lat_lngs_to_native, screen_points_to_native,
};
use maplibre_native_ffi_sys as sys;

#[cfg(test)]
use crate::PremultipliedRgba8Image;
use crate::camera::{
    BoundOptionsNativeExt, CameraDeltaNativeExt, CameraFitOptionsNativeExt, CameraOptionsNativeExt,
    CameraUpdateNativeExt, FreeCameraOptionsNativeExt, ProjectionModeNativeExt,
};
#[cfg(test)]
use crate::custom_geometry::CanonicalTileId;
use crate::events::MapId;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error, out_handle};
use crate::options::{MapOptionsNativeExt, MapTileOptionsNativeExt, MapViewportOptionsNativeExt};
use crate::render::{
    MetalBorrowedTextureDescriptor, MetalOwnedTextureDescriptor, MetalSurfaceDescriptor,
    OpenGLBorrowedTextureDescriptor, OpenGLOwnedTextureDescriptor, OpenGLSurfaceDescriptor,
    RenderSessionAttachOptions, RenderSessionAttachment, RenderSessionHandle,
    VulkanBorrowedTextureDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
    WebGpuBorrowedTextureDescriptor, WebGpuOwnedTextureDescriptor, WebGpuSurfaceDescriptor,
};
use crate::runtime::{
    OperationHandle, OperationKind, RuntimeHandle, RuntimeState, wait_raw_operation_completed,
};
use crate::values::NativeValue;
use crate::{
    BoundOptions, CameraDelta, CameraFitOptions, CameraOptions, CameraSnapshot, CameraUpdate,
    Error, ErrorKind, FreeCameraOptions, HandleOperationError, LatLng, LatLngBounds,
    MapDebugOptions, MapOptions, MapProjectionHandle, MapTileOptions, MapViewportOptions,
    ProjectionMode, Result, RuntimeEventMask, ScreenPoint,
};

mod style;
pub use style::{
    GeoJsonSourceOptions, ImageContent, ImageStretch, LocationIndicatorImageKind, SourceInfo,
    SourceType, StyleImage, StyleImageInfo, StyleImageOperation, StyleImageOptions,
    StyleImageStretches, StyleImageTextFit, StyleLayerInfo, StyleLayerInfoOperation,
    StyleLayerVisibility, StyleSourceInfoOperation, StyleTransitionOptions, TileJsonInfo,
    TileScheme, TileSourceOptions, VectorTileEncoding,
};

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ConcurrentNativeHandle<sys::mln_map>,
    pub(crate) runtime: Mutex<Option<Arc<RuntimeState>>>,
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

    fn close(&self) -> Result<()> {
        let map = self.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: map is live and operation is null writable storage.
        maplibre_core::check(unsafe { sys::mln_map_close_start(map, &mut operation) })?;
        let result = wait_raw_operation_completed(operation);
        // SAFETY: this call owns the close observer.
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

/// Any-thread map control handle bound to a retained runtime.
pub struct MapHandle {
    pub(crate) inner: Arc<MapState>,
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
/// Every committed map command publishes a new generation and reports it in
/// its command-finished event, so a snapshot whose `generation` is at or past
/// a commit's observes that commit.
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
    /// Creates a map and waits for the worker-side creation operation.
    pub fn with_options(runtime: &RuntimeHandle, options: &MapOptions) -> Result<Self> {
        let runtime_ptr = runtime.inner.native()?;
        let raw_options = options.to_native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: inputs remain readable and operation is null writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_create_start(runtime_ptr, &raw_options, &mut operation)
        })?;
        let result = (|| {
            wait_raw_operation_completed(operation)?;
            let mut out = maplibre_core::ptr::OutHandle::<sys::mln_map>::new();
            // SAFETY: creation completed successfully and out is null writable.
            maplibre_core::check(unsafe {
                sys::mln_map_create_take_result(operation, out.as_mut_ptr())
            })?;
            let native = out_handle(out, "mln_map")?;
            let id = MapId::new(native.0);
            Ok(Self {
                inner: Arc::new(MapState::new(native, Arc::clone(&runtime.inner), id)?),
            })
        })();
        // SAFETY: this call owns the creation observer.
        unsafe { sys::mln_operation_release(operation) };
        result
    }

    pub(crate) fn operation_registry(&self) -> Result<Arc<crate::runtime::OperationRegistry>> {
        let runtime = self
            .inner
            .runtime
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        Ok(Arc::clone(
            &runtime
                .as_ref()
                .ok_or_else(|| closed_handle_error("MapHandle"))?
                .operations,
        ))
    }

    fn start_operation<T>(
        &self,
        operation: sys::mln_operation,
        kind: OperationKind,
    ) -> Result<OperationHandle<T>> {
        OperationHandle::new(operation, kind, self.operation_registry()?)
    }

    /// Returns this map's runtime-local event source identity.
    pub fn id(&self) -> MapId {
        self.inner.id
    }

    /// Explicitly closes the map and waits for worker-side retirement.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.inner.is_closed() {
            return Ok(());
        }
        if Arc::strong_count(&self.inner) > 1 {
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

    /// Submits the map's logical extent update and returns its command ID.
    pub fn resize(&self, extent: LogicalExtent) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        let raw = sys::mln_logical_extent {
            width: extent.width,
            height: extent.height,
            scale_factor: extent.scale_factor,
        };
        // SAFETY: map is live and command_id is zero writable storage.
        maplibre_core::check(unsafe { sys::mln_map_resize(map, raw, &mut command_id) })?;
        Ok(command_id)
    }

    /// Requests a continuous-map repaint and returns its command ID.
    pub fn request_repaint(&self) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        // SAFETY: map is live and command_id is zero writable storage.
        maplibre_core::check(unsafe { sys::mln_map_request_repaint(map, &mut command_id) })?;
        Ok(command_id)
    }

    /// Starts one noncoalescing still-image operation.
    pub fn request_still_image(&self) -> Result<OperationHandle<()>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: map is live and operation is null writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_request_still_image_start(map, &mut operation)
        })?;
        let registry = {
            let runtime = self
                .inner
                .runtime
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            Arc::clone(
                &runtime
                    .as_ref()
                    .ok_or_else(|| closed_handle_error("MapHandle"))?
                    .operations,
            )
        };
        OperationHandle::new(operation, OperationKind::MapStillImage, registry)
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

    /// Submits one copied atomic camera update and returns its command ID.
    pub fn update_camera(&self, update: &CameraUpdate) -> Result<u64> {
        let map = self.inner.native()?;
        let raw = update.to_native();
        let mut command_id = 0;
        // SAFETY: raw is readable for this call and command_id is writable.
        maplibre_core::check(unsafe { sys::mln_map_update_camera(map, &raw, &mut command_id) })?;
        Ok(command_id)
    }

    /// Submits one relative camera operation and returns its command ID.
    pub fn apply_camera_delta(&self, delta: &CameraDelta) -> Result<u64> {
        let map = self.inner.native()?;
        let raw = delta.to_native();
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_apply_camera_delta(map, &raw, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Starts an ordered camera query.
    pub fn start_camera_query(&self) -> Result<OperationHandle<CameraSnapshot>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: map is live and operation is null writable storage.
        maplibre_core::check(unsafe { sys::mln_map_camera_query_start(map, &mut operation) })?;
        let registry = {
            let runtime = self
                .inner
                .runtime
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            Arc::clone(
                &runtime
                    .as_ref()
                    .ok_or_else(|| closed_handle_error("MapHandle"))?
                    .operations,
            )
        };
        OperationHandle::new(operation, OperationKind::CameraQuery, registry)
    }

    /// Selects which map-originated event types this map queues and returns the
    /// accepted command ID.
    pub fn set_event_mask(&self, mask: RuntimeEventMask) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        // SAFETY: map is live and command_id is zero writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_set_event_mask(map, mask.bits(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Submits a debug-overlay command and returns its command ID.
    ///
    /// The committed mask is visible as [`MapSnapshot::debug_options`].
    pub fn set_debug_options(&self, options: MapDebugOptions) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        // SAFETY: map is live and command_id is zero writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_set_debug_options(map, options.bits(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Submits a rendering-stats visibility command and returns its command ID.
    ///
    /// The committed value is visible as
    /// [`MapSnapshot::rendering_stats_view_enabled`].
    pub fn set_rendering_stats_view_enabled(&self, enabled: bool) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        // SAFETY: map is live and command_id is zero writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(map, enabled, &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Submits a viewport-options command; the committed options are visible
    /// as [`MapSnapshot::viewport`].
    pub fn set_viewport_options(&self, options: &MapViewportOptions) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_set_viewport_options(map, &options.to_native(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Submits a tile-options command; the committed options are visible as
    /// [`MapSnapshot::tile`].
    pub fn set_tile_options(&self, options: &MapTileOptions) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_set_tile_options(map, &options.to_native(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Submits a camera-constraint command; the committed constraints are
    /// visible as [`MapSnapshot::bounds`].
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_set_bounds(map, &options.to_native(), &mut command_id)
        })?;
        Ok(command_id)
    }

    /// Submits a free-camera command; the committed options are visible as
    /// [`MapSnapshot::free_camera`].
    pub fn set_free_camera_options(&self, options: &FreeCameraOptions) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_set_free_camera_options(map, &options.to_native(), &mut command_id)
        })?;
        Ok(command_id)
    }

    pub fn projection_mode(&self) -> Result<ProjectionMode> {
        Ok(self.snapshot()?.projection_mode)
    }

    pub fn set_projection_mode(&self, mode: &ProjectionMode) -> Result<u64> {
        let map = self.inner.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_set_projection_mode(map, &mode.to_native(), &mut command_id)
        })?;
        Ok(command_id)
    }

    pub fn start_camera_for_lat_lng_bounds(
        &self,
        bounds: LatLngBounds,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<OperationHandle<CameraOptions>> {
        let map = self.inner.native()?;
        let fit = fit_options.map(CameraFitOptions::to_native);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lng_bounds_start(
                map,
                bounds.to_native(),
                option_ptr(fit.as_ref()),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::CameraFitBounds)
    }

    pub fn start_camera_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<OperationHandle<CameraOptions>> {
        if coordinates.is_empty() {
            return Err(Error::invalid_argument(
                "start_camera_for_lat_lngs requires at least one coordinate",
            ));
        }
        let map = self.inner.native()?;
        let coordinates = lat_lngs_to_native(coordinates);
        let fit = fit_options.map(CameraFitOptions::to_native);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lngs_start(
                map,
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                option_ptr(fit.as_ref()),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::CameraFitCoordinates)
    }

    pub fn start_camera_for_geometry(
        &self,
        geometry: &[u8],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<OperationHandle<CameraOptions>> {
        let map = self.inner.native()?;
        let fit = fit_options.map(CameraFitOptions::to_native);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_geometry_start(
                map,
                maplibre_core::string::buffer_view(geometry),
                option_ptr(fit.as_ref()),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::CameraFitGeometry)
    }

    fn start_bounds_for_camera(
        &self,
        camera: &CameraOptions,
        kind: OperationKind,
    ) -> Result<OperationHandle<LatLngBounds>> {
        let map = self.inner.native()?;
        let camera = camera.to_native();
        let mut operation = sys::mln_operation(0);
        let status = unsafe {
            match kind {
                OperationKind::BoundsForCamera => {
                    sys::mln_map_lat_lng_bounds_for_camera_start(map, &camera, &mut operation)
                }
                OperationKind::BoundsForCameraUnwrapped => {
                    sys::mln_map_lat_lng_bounds_for_camera_unwrapped_start(
                        map,
                        &camera,
                        &mut operation,
                    )
                }
                _ => sys::MLN_STATUS_INVALID_STATE,
            }
        };
        maplibre_core::check(status)?;
        self.start_operation(operation, kind)
    }

    pub fn start_lat_lng_bounds_for_camera(
        &self,
        camera: &CameraOptions,
    ) -> Result<OperationHandle<LatLngBounds>> {
        self.start_bounds_for_camera(camera, OperationKind::BoundsForCamera)
    }

    pub fn start_lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: &CameraOptions,
    ) -> Result<OperationHandle<LatLngBounds>> {
        self.start_bounds_for_camera(camera, OperationKind::BoundsForCameraUnwrapped)
    }

    pub fn start_pixel_for_lat_lng(
        &self,
        coordinate: LatLng,
    ) -> Result<OperationHandle<ScreenPoint>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_pixel_for_lat_lng_start(map, coordinate.to_native(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::PixelForLatLng)
    }

    pub fn start_lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<OperationHandle<LatLng>> {
        let map = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lng_for_pixel_start(map, point.to_native(), &mut operation)
        })?;
        self.start_operation(operation, OperationKind::LatLngForPixel)
    }

    pub fn start_pixels_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
    ) -> Result<OperationHandle<Vec<ScreenPoint>>> {
        let map = self.inner.native()?;
        let coordinates = lat_lngs_to_native(coordinates);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_pixels_for_lat_lngs_start(
                map,
                const_ptr_or_null(&coordinates),
                coordinates.len(),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::PixelsForLatLngs)
    }

    pub fn start_lat_lngs_for_pixels(
        &self,
        points: &[ScreenPoint],
    ) -> Result<OperationHandle<Vec<LatLng>>> {
        let map = self.inner.native()?;
        let points = screen_points_to_native(points);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lngs_for_pixels_start(
                map,
                const_ptr_or_null(&points),
                points.len(),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::LatLngsForPixels)
    }

    /// Creates a standalone projection snapshot from the current map transform.
    pub fn create_projection(&self) -> Result<MapProjectionHandle> {
        MapProjectionHandle::new(self)
    }
}

impl OperationHandle<CameraOptions> {
    pub fn take(&self) -> Result<CameraOptions> {
        let mut raw = unsafe { sys::mln_camera_options_default() };
        self.with_result_operation(|operation| {
            let status = unsafe {
                match self.operation_kind {
                    OperationKind::CameraFitBounds => {
                        sys::mln_map_camera_for_lat_lng_bounds_take_result(operation, &mut raw)
                    }
                    OperationKind::CameraFitCoordinates => {
                        sys::mln_map_camera_for_lat_lngs_take_result(operation, &mut raw)
                    }
                    OperationKind::CameraFitGeometry => {
                        sys::mln_map_camera_for_geometry_take_result(operation, &mut raw)
                    }
                    _ => sys::MLN_STATUS_INVALID_STATE,
                }
            };
            maplibre_core::check(status)
        })?;
        Ok(CameraOptions::from_native(raw))
    }
}

impl OperationHandle<LatLngBounds> {
    pub fn take(&self) -> Result<LatLngBounds> {
        let mut raw = empty_lat_lng_bounds();
        self.with_result_operation(|operation| {
            let status = unsafe {
                match self.operation_kind {
                    OperationKind::BoundsForCamera => {
                        sys::mln_map_lat_lng_bounds_for_camera_take_result(operation, &mut raw)
                    }
                    OperationKind::BoundsForCameraUnwrapped => {
                        sys::mln_map_lat_lng_bounds_for_camera_unwrapped_take_result(
                            operation, &mut raw,
                        )
                    }
                    _ => sys::MLN_STATUS_INVALID_STATE,
                }
            };
            maplibre_core::check(status)
        })?;
        Ok(LatLngBounds::from_native(raw))
    }
}

impl OperationHandle<ScreenPoint> {
    pub fn take(&self) -> Result<ScreenPoint> {
        let mut raw = sys::mln_screen_point { x: 0.0, y: 0.0 };
        self.with_result_operation(|operation| {
            if self.operation_kind != OperationKind::PixelForLatLng {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "operation does not contain a screen point",
                ));
            }
            maplibre_core::check(unsafe {
                sys::mln_map_pixel_for_lat_lng_take_result(operation, &mut raw)
            })
        })?;
        Ok(ScreenPoint::from_native(raw))
    }
}

impl OperationHandle<LatLng> {
    pub fn take(&self) -> Result<LatLng> {
        let mut raw = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        self.with_result_operation(|operation| {
            if self.operation_kind != OperationKind::LatLngForPixel {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "operation does not contain a geographic coordinate",
                ));
            }
            maplibre_core::check(unsafe {
                sys::mln_map_lat_lng_for_pixel_take_result(operation, &mut raw)
            })
        })?;
        Ok(LatLng::from_native(raw))
    }
}

impl OperationHandle<Vec<ScreenPoint>> {
    pub fn take(&self) -> Result<Vec<ScreenPoint>> {
        let mut count = 0;
        self.with_operation(|operation| {
            // SAFETY: null/zero is a required-count probe. Native may report a
            // capacity status while still filling count and preserving result.
            unsafe {
                sys::mln_map_pixels_for_lat_lngs_take_result(
                    operation,
                    std::ptr::null_mut(),
                    0,
                    &mut count,
                );
            }
            Ok(())
        })?;
        let mut raw = vec![sys::mln_screen_point { x: 0.0, y: 0.0 }; count];
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_pixels_for_lat_lngs_take_result(
                    operation,
                    raw.as_mut_ptr(),
                    raw.len(),
                    &mut count,
                )
            })
        })?;
        Ok(raw.into_iter().map(ScreenPoint::from_native).collect())
    }
}

impl OperationHandle<Vec<LatLng>> {
    pub fn take(&self) -> Result<Vec<LatLng>> {
        let mut count = 0;
        self.with_operation(|operation| {
            // SAFETY: null/zero is a required-count probe. Native may report a
            // capacity status while still filling count and preserving result.
            unsafe {
                sys::mln_map_lat_lngs_for_pixels_take_result(
                    operation,
                    std::ptr::null_mut(),
                    0,
                    &mut count,
                );
            }
            Ok(())
        })?;
        let mut raw = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            };
            count
        ];
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_map_lat_lngs_for_pixels_take_result(
                    operation,
                    raw.as_mut_ptr(),
                    raw.len(),
                    &mut count,
                )
            })
        })?;
        Ok(raw.into_iter().map(LatLng::from_native).collect())
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
            sys::mln_metal_surface_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_vulkan_surface(
        &self,
        value: &VulkanSurfaceDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_vulkan_surface_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_webgpu_surface(
        &self,
        value: &WebGpuSurfaceDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_webgpu_surface_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_opengl_surface(
        &self,
        value: &OpenGLSurfaceDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_opengl_surface_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_metal_owned_texture(
        &self,
        value: &MetalOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_metal_owned_texture_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_metal_borrowed_texture(
        &self,
        value: &MetalBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_metal_borrowed_texture_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_vulkan_owned_texture(
        &self,
        value: &VulkanOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_vulkan_owned_texture_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_vulkan_borrowed_texture(
        &self,
        value: &VulkanBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_vulkan_borrowed_texture_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_webgpu_owned_texture(
        &self,
        value: &WebGpuOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_webgpu_owned_texture_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_webgpu_borrowed_texture(
        &self,
        value: &WebGpuBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_webgpu_borrowed_texture_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_opengl_owned_texture(
        &self,
        value: &OpenGLOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_opengl_owned_texture_attach_start(map, &raw, options, session, operation)
        })
    }

    pub fn attach_opengl_borrowed_texture(
        &self,
        value: &OpenGLBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> Result<RenderSessionAttachment> {
        let raw = value.to_native();
        RenderSessionHandle::attach(self, options, |map, options, session, operation| unsafe {
            sys::mln_opengl_borrowed_texture_attach_start(map, &raw, options, session, operation)
        })
    }
}

#[cfg(test)]
mod tests;
