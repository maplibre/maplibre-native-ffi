"""Generated from the C headers by tools/bindgen. Do not edit."""

from __future__ import annotations

from collections.abc import Callable
from concurrent.futures import Future
from typing import TYPE_CHECKING, NamedTuple, TypeVar

from . import _native
from ._completion import CommandCompletion
from ._future import map_future
from ._operation import GeneratedOperations, _adopt_future, _adopt_value, _with_view

R = TypeVar("R")
from ._generated_values import (
    AmbientCacheOperation,
    BoundOptions,
    CameraDelta,
    CameraFitOptions,
    CameraOptions,
    CameraQueryResult,
    CameraUpdate,
    CanonicalTileId,
    CustomGeometrySourceOptions,
    CustomMvtVectorSourceOptions,
    EdgeInsets,
    FeatureStateSelector,
    FrameDemand,
    FreeCameraOptions,
    GeojsonSourceOptions,
    GpuSync,
    HttpHeaderTransform,
    LatLng,
    LatLngBounds,
    LocationIndicatorImageKind,
    LogEvent,
    LogicalExtent,
    LogSetCallbackRegistration,
    LogSeverity,
    LogSeverityMask,
    MapDebugOption,
    MapOptions,
    MapSnapshot,
    MapTileOptions,
    MapViewportOptions,
    MetalBorrowedTextureDescriptor,
    MetalOwnedTextureDescriptor,
    MetalOwnedTextureFrame,
    MetalSurfaceDescriptor,
    NetworkStatus,
    OfflineRegionDefinition,
    OfflineRegionDownloadState,
    OfflineRegionInfo,
    OfflineRegionStatus,
    OpenglBorrowedTextureDescriptor,
    OpenglContextProviderFlag,
    OpenglOwnedTextureDescriptor,
    OpenglOwnedTextureFrame,
    OpenglSurfaceDescriptor,
    PremultipliedRgba8Image,
    ProjectedMeters,
    ProjectionMode,
    QueriedFeature,
    RenderAbandonResult,
    RenderBackendFlag,
    RenderedFeatureQueryOptions,
    RenderedQueryGeometry,
    RenderFrameResult,
    RenderSessionAttachOptions,
    RenderSessionCapabilities,
    RenderSessionSnapshot,
    RenderTargetExtent,
    ResourceProvider,
    ResourceResponse,
    ResourceTransform,
    RuntimeEventBatchView,
    RuntimeEventMask,
    RuntimeOptions,
    ScreenBox,
    ScreenPoint,
    SourceFeatureQueryOptions,
    StyleImageOptions,
    StyleImageResult,
    StyleImageStretchesResult,
    StyleLayerEntry,
    StyleLayerResult,
    StyleLayerVisibility,
    StyleSourceResult,
    StyleSourceTileUrlsResult,
    StyleTileSourceOptions,
    StyleTransitionOptions,
    TextureReadbackResult,
    VulkanBorrowedTextureDescriptor,
    VulkanOwnedTextureDescriptor,
    VulkanOwnedTextureFrame,
    VulkanSurfaceDescriptor,
    WebgpuBorrowedTextureDescriptor,
    WebgpuOwnedTextureDescriptor,
    WebgpuOwnedTextureFrame,
    WebgpuSurfaceDescriptor,
)

if TYPE_CHECKING:
    from ._generated_owners import (
        AcquiredFrameHandle,
        EventBatchHandle,
        GeojsonSourceDataHandle,
        MapHandle,
        MapProjectionHandle,
        RenderFrameBatchHandle,
        RenderSessionHandle,
        RuntimeHandle,
    )


class MapCameraSnapshotGetResult(NamedTuple):
    camera: CameraOptions
    generation: int


class MetalBorrowedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class MetalOwnedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class MetalSurfaceAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class OpenglBorrowedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class OpenglOwnedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class OpenglSurfaceAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class RenderTargetExtentPhysicalSizeResult(NamedTuple):
    width: int
    height: int


class VulkanBorrowedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class VulkanOwnedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class VulkanSurfaceAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class WebgpuBorrowedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class WebgpuOwnedTextureAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class WebgpuSurfaceAttachResult(NamedTuple):
    session: RenderSessionHandle
    completion: Future[None]


class _AcquiredFrameHandleOperations(GeneratedOperations):
    _native: _native._AcquiredFrameHandle

    def with_metal_texture(self, callback: Callable[[MetalOwnedTextureFrame], R]) -> R:
        """Call mln_acquired_frame_get_metal_texture. Native resources remain valid only during the callback."""
        return _with_view(
            self,
            lambda: self._native.with_metal_texture(),
            lambda raw: MetalOwnedTextureFrame._from_native(raw),
            callback,
        )

    def with_opengl_texture(
        self, callback: Callable[[OpenglOwnedTextureFrame], R]
    ) -> R:
        """Call mln_acquired_frame_get_opengl_texture. Native resources remain valid only during the callback."""
        return _with_view(
            self,
            lambda: self._native.with_opengl_texture(),
            lambda raw: OpenglOwnedTextureFrame._from_native(raw),
            callback,
        )

    def with_producer_sync(self, callback: Callable[[GpuSync], R]) -> R:
        """Call mln_acquired_frame_get_producer_sync. Native resources remain valid only during the callback."""
        return _with_view(
            self,
            lambda: self._native.with_producer_sync(),
            lambda raw: GpuSync._from_native(raw),
            callback,
        )

    def get_result(self) -> RenderFrameResult:
        """Call mln_acquired_frame_get_result."""
        return RenderFrameResult._from_native(self._native.get_result())

    def with_vulkan_texture(
        self, callback: Callable[[VulkanOwnedTextureFrame], R]
    ) -> R:
        """Call mln_acquired_frame_get_vulkan_texture. Native resources remain valid only during the callback."""
        return _with_view(
            self,
            lambda: self._native.with_vulkan_texture(),
            lambda raw: VulkanOwnedTextureFrame._from_native(raw),
            callback,
        )

    def with_webgpu_texture(
        self, callback: Callable[[WebgpuOwnedTextureFrame], R]
    ) -> R:
        """Call mln_acquired_frame_get_webgpu_texture. Native resources remain valid only during the callback."""
        return _with_view(
            self,
            lambda: self._native.with_webgpu_texture(),
            lambda raw: WebgpuOwnedTextureFrame._from_native(raw),
            callback,
        )

    def close(self, consumer_completion: GpuSync | None = None) -> None:
        """Call mln_acquired_frame_release."""
        return self._native.close(consumer_completion)


class _BufferHandleOperations(GeneratedOperations):
    _native: _native._BufferHandle

    def close(self) -> None:
        """Call mln_buffer_destroy."""
        return self._native.close()

    def get(self) -> bytes:
        """Call mln_buffer_get."""
        return self._native.get()


class _EventBatchHandleOperations(GeneratedOperations):
    _native: _native._EventBatchHandle

    def get(self) -> RuntimeEventBatchView:
        """Call mln_event_batch_get."""
        return RuntimeEventBatchView._from_native(self._native.get())

    def close(self) -> None:
        """Call mln_event_batch_release."""
        return self._native.close()


class _GeojsonSourceDataHandleOperations(GeneratedOperations):
    _native: _native._GeojsonSourceDataHandle

    def close(self) -> None:
        """Call mln_geojson_source_data_destroy."""
        return self._native.close()


class _HttpHeaderTransformResponseScopeOperations(GeneratedOperations):
    _native: _native._HttpHeaderTransformResponseScope

    def set(self, name: str, value: str) -> None:
        """Call mln_http_header_transform_response_set."""
        return self._native.set(name, value)


class _MapHandleOperations(GeneratedOperations):
    _native: _native._MapHandle

    def add_color_relief_layer(
        self, layer_id: str, source_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_color_relief_layer."""
        return self._native.add_color_relief_layer(layer_id, source_id, before_layer_id)

    def add_custom_geometry_source(
        self, source_id: str, options: CustomGeometrySourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_custom_geometry_source."""
        return self._native.add_custom_geometry_source(source_id, options)

    def add_custom_mvt_vector_source(
        self, source_id: str, options: CustomMvtVectorSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_custom_mvt_vector_source."""
        return self._native.add_custom_mvt_vector_source(source_id, options)

    def add_geojson_source_data(
        self, source_id: str, data: GeojsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_geojson_source_data."""
        return self._native.add_geojson_source_data(source_id, data._native)

    def add_geojson_source_url(
        self, source_id: str, url: str, options: GeojsonSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_geojson_source_url."""
        return self._native.add_geojson_source_url(source_id, url, options)

    def add_hillshade_layer(
        self, layer_id: str, source_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_hillshade_layer."""
        return self._native.add_hillshade_layer(layer_id, source_id, before_layer_id)

    def add_image_source_image(
        self,
        source_id: str,
        coordinates: tuple[LatLng, ...],
        image: PremultipliedRgba8Image | None = None,
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_image_source_image."""
        return self._native.add_image_source_image(source_id, coordinates, image)

    def add_image_source_url(
        self, source_id: str, coordinates: tuple[LatLng, ...], url: str
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_image_source_url."""
        return self._native.add_image_source_url(source_id, coordinates, url)

    def add_location_indicator_layer(
        self, layer_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_location_indicator_layer."""
        return self._native.add_location_indicator_layer(layer_id, before_layer_id)

    def add_raster_dem_source_tiles(
        self,
        source_id: str,
        tiles: tuple[str, ...],
        options: StyleTileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_raster_dem_source_tiles."""
        return self._native.add_raster_dem_source_tiles(source_id, tiles, options)

    def add_raster_dem_source_url(
        self, source_id: str, url: str, options: StyleTileSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_raster_dem_source_url."""
        return self._native.add_raster_dem_source_url(source_id, url, options)

    def add_raster_source_tiles(
        self,
        source_id: str,
        tiles: tuple[str, ...],
        options: StyleTileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_raster_source_tiles."""
        return self._native.add_raster_source_tiles(source_id, tiles, options)

    def add_raster_source_url(
        self, source_id: str, url: str, options: StyleTileSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_raster_source_url."""
        return self._native.add_raster_source_url(source_id, url, options)

    def add_style_layer_json(
        self, layer_json: bytes, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_style_layer_json."""
        return self._native.add_style_layer_json(layer_json, before_layer_id)

    def add_style_source_json(
        self, source_id: str, source_json: bytes
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_style_source_json."""
        return self._native.add_style_source_json(source_id, source_json)

    def add_vector_source_tiles(
        self,
        source_id: str,
        tiles: tuple[str, ...],
        options: StyleTileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_vector_source_tiles."""
        return self._native.add_vector_source_tiles(source_id, tiles, options)

    def add_vector_source_url(
        self, source_id: str, url: str, options: StyleTileSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_add_vector_source_url."""
        return self._native.add_vector_source_url(source_id, url, options)

    def apply_camera_delta(
        self, delta: CameraDelta | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_apply_camera_delta."""
        return self._native.apply_camera_delta(delta)

    def camera_for_geometry(
        self, geometry: bytes, fit_options: CameraFitOptions | None = None
    ) -> Future[CameraOptions]:
        """Call mln_map_camera_for_geometry."""
        return map_future(
            self._native.camera_for_geometry(geometry, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def camera_for_lat_lng_bounds(
        self, bounds: LatLngBounds, fit_options: CameraFitOptions | None = None
    ) -> Future[CameraOptions]:
        """Call mln_map_camera_for_lat_lng_bounds."""
        return map_future(
            self._native.camera_for_lat_lng_bounds(bounds, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def camera_for_lat_lngs(
        self,
        coordinates: tuple[LatLng, ...],
        fit_options: CameraFitOptions | None = None,
    ) -> Future[CameraOptions]:
        """Call mln_map_camera_for_lat_lngs."""
        return map_future(
            self._native.camera_for_lat_lngs(coordinates, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def camera_query(self) -> Future[CameraQueryResult]:
        """Call mln_map_camera_query."""
        return map_future(
            self._native.camera_query(),
            lambda value: CameraQueryResult._from_native(value),
        )

    def camera_snapshot_get(self) -> MapCameraSnapshotGetResult:
        """Call mln_map_camera_snapshot_get."""
        raw = self._native.camera_snapshot_get()
        return MapCameraSnapshotGetResult(
            CameraOptions._from_native(raw["camera"]), raw["generation"]
        )

    def cancel_transitions(self) -> Future[CommandCompletion]:
        """Call mln_map_cancel_transitions."""
        return self._native.cancel_transitions()

    def copy_layer_source_id(self, layer_id: str) -> Future[str | None]:
        """Call mln_map_copy_layer_source_id."""
        return map_future(
            self._native.copy_layer_source_id(layer_id),
            lambda value: None if value is None else (value),
        )

    def copy_layer_source_layer(self, layer_id: str) -> Future[str | None]:
        """Call mln_map_copy_layer_source_layer."""
        return map_future(
            self._native.copy_layer_source_layer(layer_id),
            lambda value: None if value is None else (value),
        )

    def copy_style_image_premultiplied_rgba8(
        self, image_id: str
    ) -> Future[bytes | None]:
        """Call mln_map_copy_style_image_premultiplied_rgba8."""
        return map_future(
            self._native.copy_style_image_premultiplied_rgba8(image_id),
            lambda value: None if value is None else (value),
        )

    def copy_style_image_stretches(
        self, image_id: str
    ) -> Future[StyleImageStretchesResult | None]:
        """Call mln_map_copy_style_image_stretches."""
        return map_future(
            self._native.copy_style_image_stretches(image_id),
            lambda value: (
                None
                if value is None
                else (StyleImageStretchesResult._from_native(value))
            ),
        )

    def copy_style_source_attribution(self, source_id: str) -> Future[str | None]:
        """Call mln_map_copy_style_source_attribution."""
        return map_future(
            self._native.copy_style_source_attribution(source_id),
            lambda value: None if value is None else (value),
        )

    def copy_style_source_url(self, source_id: str) -> Future[str | None]:
        """Call mln_map_copy_style_source_url."""
        return map_future(
            self._native.copy_style_source_url(source_id),
            lambda value: None if value is None else (value),
        )

    def dump_debug_logs(self) -> Future[CommandCompletion]:
        """Call mln_map_dump_debug_logs."""
        return self._native.dump_debug_logs()

    def get_feature_state(self, selector: FeatureStateSelector) -> Future[bytes]:
        """Call mln_map_get_feature_state."""
        return self._native.get_feature_state(selector)

    def get_global_state(self) -> Future[bytes]:
        """Call mln_map_get_global_state."""
        return self._native.get_global_state()

    def get_image_source_coordinates(
        self, source_id: str
    ) -> Future[tuple[LatLng, ...] | None]:
        """Call mln_map_get_image_source_coordinates."""
        return map_future(
            self._native.get_image_source_coordinates(source_id),
            lambda value: (
                None
                if value is None
                else (tuple(LatLng._from_native(item) for item in value))
            ),
        )

    def get_layer_filter(self, layer_id: str) -> Future[bytes | None]:
        """Call mln_map_get_layer_filter."""
        return map_future(
            self._native.get_layer_filter(layer_id),
            lambda value: None if value is None else (value),
        )

    def get_layer_property(
        self, layer_id: str, property_name: str
    ) -> Future[bytes | None]:
        """Call mln_map_get_layer_property."""
        return map_future(
            self._native.get_layer_property(layer_id, property_name),
            lambda value: None if value is None else (value),
        )

    def get_style_image_info(self, image_id: str) -> Future[StyleImageResult | None]:
        """Call mln_map_get_style_image_info."""
        return map_future(
            self._native.get_style_image_info(image_id),
            lambda value: (
                None if value is None else (StyleImageResult._from_native(value))
            ),
        )

    def get_style_layer_info(self, layer_id: str) -> Future[StyleLayerResult | None]:
        """Call mln_map_get_style_layer_info."""
        return map_future(
            self._native.get_style_layer_info(layer_id),
            lambda value: (
                None if value is None else (StyleLayerResult._from_native(value))
            ),
        )

    def get_style_layer_json(self, layer_id: str) -> Future[bytes | None]:
        """Call mln_map_get_style_layer_json."""
        return map_future(
            self._native.get_style_layer_json(layer_id),
            lambda value: None if value is None else (value),
        )

    def get_style_light_property(self, property_name: str) -> Future[bytes | None]:
        """Call mln_map_get_style_light_property."""
        return map_future(
            self._native.get_style_light_property(property_name),
            lambda value: None if value is None else (value),
        )

    def get_style_source_info(self, source_id: str) -> Future[StyleSourceResult | None]:
        """Call mln_map_get_style_source_info."""
        return map_future(
            self._native.get_style_source_info(source_id),
            lambda value: (
                None if value is None else (StyleSourceResult._from_native(value))
            ),
        )

    def get_style_source_tile_urls(
        self, source_id: str
    ) -> Future[StyleSourceTileUrlsResult | None]:
        """Call mln_map_get_style_source_tile_urls."""
        return map_future(
            self._native.get_style_source_tile_urls(source_id),
            lambda value: (
                None
                if value is None
                else (StyleSourceTileUrlsResult._from_native(value))
            ),
        )

    def get_style_transition_options(self) -> Future[StyleTransitionOptions]:
        """Call mln_map_get_style_transition_options."""
        return map_future(
            self._native.get_style_transition_options(),
            lambda value: StyleTransitionOptions._from_native(value),
        )

    def invalidate_custom_geometry_source_region(
        self, source_id: str, bounds: LatLngBounds
    ) -> Future[CommandCompletion]:
        """Call mln_map_invalidate_custom_geometry_source_region."""
        return self._native.invalidate_custom_geometry_source_region(source_id, bounds)

    def invalidate_custom_geometry_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> Future[CommandCompletion]:
        """Call mln_map_invalidate_custom_geometry_source_tile."""
        return self._native.invalidate_custom_geometry_source_tile(source_id, tile_id)

    def invalidate_custom_mvt_vector_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> Future[CommandCompletion]:
        """Call mln_map_invalidate_custom_mvt_vector_source_tile."""
        return self._native.invalidate_custom_mvt_vector_source_tile(source_id, tile_id)

    def lat_lng_bounds_for_camera(
        self, camera: CameraOptions | None = None
    ) -> Future[LatLngBounds]:
        """Call mln_map_lat_lng_bounds_for_camera."""
        return map_future(
            self._native.lat_lng_bounds_for_camera(camera),
            lambda value: LatLngBounds._from_native(value),
        )

    def lat_lng_bounds_for_camera_unwrapped(
        self, camera: CameraOptions | None = None
    ) -> Future[LatLngBounds]:
        """Call mln_map_lat_lng_bounds_for_camera_unwrapped."""
        return map_future(
            self._native.lat_lng_bounds_for_camera_unwrapped(camera),
            lambda value: LatLngBounds._from_native(value),
        )

    def lat_lng_for_pixel(self, point: ScreenPoint) -> Future[LatLng]:
        """Call mln_map_lat_lng_for_pixel."""
        return map_future(
            self._native.lat_lng_for_pixel(point),
            lambda value: LatLng._from_native(value),
        )

    def lat_lng_for_pixel_unwrapped(self, point: ScreenPoint) -> Future[LatLng]:
        """Call mln_map_lat_lng_for_pixel_unwrapped."""
        return map_future(
            self._native.lat_lng_for_pixel_unwrapped(point),
            lambda value: LatLng._from_native(value),
        )

    def lat_lngs_for_pixels(
        self, points: tuple[ScreenPoint, ...]
    ) -> Future[tuple[LatLng, ...]]:
        """Call mln_map_lat_lngs_for_pixels."""
        return map_future(
            self._native.lat_lngs_for_pixels(points),
            lambda value: tuple(LatLng._from_native(item) for item in value),
        )

    def lat_lngs_for_pixels_unwrapped(
        self, points: tuple[ScreenPoint, ...]
    ) -> Future[tuple[LatLng, ...]]:
        """Call mln_map_lat_lngs_for_pixels_unwrapped."""
        return map_future(
            self._native.lat_lngs_for_pixels_unwrapped(points),
            lambda value: tuple(LatLng._from_native(item) for item in value),
        )

    def list_style_layer_ids(self) -> Future[tuple[str, ...]]:
        """Call mln_map_list_style_layer_ids."""
        return map_future(
            self._native.list_style_layer_ids(),
            lambda value: tuple(item for item in value),
        )

    def list_style_layers(self) -> Future[tuple[StyleLayerEntry, ...]]:
        """Call mln_map_list_style_layers."""
        return map_future(
            self._native.list_style_layers(),
            lambda value: tuple(StyleLayerEntry._from_native(item) for item in value),
        )

    def list_style_source_ids(self) -> Future[tuple[str, ...]]:
        """Call mln_map_list_style_source_ids."""
        return map_future(
            self._native.list_style_source_ids(),
            lambda value: tuple(item for item in value),
        )

    def loaded_style_json(self) -> Future[bytes]:
        """Call mln_map_loaded_style_json."""
        return self._native.loaded_style_json()

    def meters_per_pixel_at_latitude(self, latitude: float) -> Future[float]:
        """Call mln_map_meters_per_pixel_at_latitude."""
        return self._native.meters_per_pixel_at_latitude(latitude)

    def move_style_layer(
        self, layer_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_move_style_layer."""
        return self._native.move_style_layer(layer_id, before_layer_id)

    def pixel_for_lat_lng(self, coordinate: LatLng) -> Future[ScreenPoint]:
        """Call mln_map_pixel_for_lat_lng."""
        return map_future(
            self._native.pixel_for_lat_lng(coordinate),
            lambda value: ScreenPoint._from_native(value),
        )

    def pixels_for_lat_lngs(
        self, coordinates: tuple[LatLng, ...]
    ) -> Future[tuple[ScreenPoint, ...]]:
        """Call mln_map_pixels_for_lat_lngs."""
        return map_future(
            self._native.pixels_for_lat_lngs(coordinates),
            lambda value: tuple(ScreenPoint._from_native(item) for item in value),
        )

    def projection_create(self) -> Future[MapProjectionHandle]:
        """Call mln_map_projection_create."""
        return _adopt_future(
            self._native.projection_create(), "MapProjectionHandle", None
        )

    def close(self) -> Future[None]:
        """Call mln_map_release."""
        return self._native.close()

    def remove_feature_state(
        self, selector: FeatureStateSelector
    ) -> Future[CommandCompletion]:
        """Call mln_map_remove_feature_state."""
        return self._native.remove_feature_state(selector)

    def remove_style_image(self, image_id: str) -> Future[CommandCompletion]:
        """Call mln_map_remove_style_image."""
        return self._native.remove_style_image(image_id)

    def remove_style_layer(self, layer_id: str) -> Future[CommandCompletion]:
        """Call mln_map_remove_style_layer."""
        return self._native.remove_style_layer(layer_id)

    def remove_style_source(self, source_id: str) -> Future[CommandCompletion]:
        """Call mln_map_remove_style_source."""
        return self._native.remove_style_source(source_id)

    def request_repaint(self) -> Future[CommandCompletion]:
        """Call mln_map_request_repaint."""
        return self._native.request_repaint()

    def request_still_image(self) -> Future[None]:
        """Call mln_map_request_still_image."""
        return self._native.request_still_image()

    def resize(self, extent: LogicalExtent) -> Future[CommandCompletion]:
        """Call mln_map_resize."""
        return self._native.resize(extent)

    def set_bounds(
        self, options: BoundOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_bounds."""
        return self._native.set_bounds(options)

    def set_custom_geometry_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: bytes
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_custom_geometry_source_tile_data."""
        return self._native.set_custom_geometry_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_mvt_vector_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: bytes
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_custom_mvt_vector_source_tile_data."""
        return self._native.set_custom_mvt_vector_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_mvt_vector_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_custom_mvt_vector_source_tile_error."""
        return self._native.set_custom_mvt_vector_source_tile_error(
            source_id, tile_id, message
        )

    def set_debug_options(self, options: MapDebugOption) -> Future[CommandCompletion]:
        """Call mln_map_set_debug_options."""
        return self._native.set_debug_options(options)

    def set_event_mask(self, mask: RuntimeEventMask) -> Future[CommandCompletion]:
        """Call mln_map_set_event_mask."""
        return self._native.set_event_mask(mask)

    def set_feature_state(
        self, selector: FeatureStateSelector, input_state: bytes
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_feature_state."""
        return self._native.set_feature_state(selector, input_state)

    def set_free_camera_options(
        self, options: FreeCameraOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_free_camera_options."""
        return self._native.set_free_camera_options(options)

    def set_geojson_source_data(
        self, source_id: str, data: GeojsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_geojson_source_data."""
        return self._native.set_geojson_source_data(source_id, data._native)

    def set_geojson_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_geojson_source_synchronous_tiling."""
        return self._native.set_geojson_source_synchronous_tiling(source_id, enabled)

    def set_geojson_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_geojson_source_url."""
        return self._native.set_geojson_source_url(source_id, url)

    def set_global_state_property(
        self, property_name: str, value: bytes
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_global_state_property."""
        return self._native.set_global_state_property(property_name, value)

    def set_image_source_coordinates(
        self, source_id: str, coordinates: tuple[LatLng, ...]
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_image_source_coordinates."""
        return self._native.set_image_source_coordinates(source_id, coordinates)

    def set_image_source_image(
        self, source_id: str, image: PremultipliedRgba8Image | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_image_source_image."""
        return self._native.set_image_source_image(source_id, image)

    def set_image_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_image_source_url."""
        return self._native.set_image_source_url(source_id, url)

    def set_layer_filter(
        self, layer_id: str, filter: bytes | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_layer_filter."""
        return self._native.set_layer_filter(layer_id, filter)

    def set_layer_max_zoom(
        self, layer_id: str, max_zoom: float
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_layer_max_zoom."""
        return self._native.set_layer_max_zoom(layer_id, max_zoom)

    def set_layer_min_zoom(
        self, layer_id: str, min_zoom: float
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_layer_min_zoom."""
        return self._native.set_layer_min_zoom(layer_id, min_zoom)

    def set_layer_property(
        self, layer_id: str, property_name: str, value: bytes
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_layer_property."""
        return self._native.set_layer_property(layer_id, property_name, value)

    def set_layer_source_id(
        self, layer_id: str, source_id: str
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_layer_source_id."""
        return self._native.set_layer_source_id(layer_id, source_id)

    def set_layer_source_layer(
        self, layer_id: str, source_layer: str | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_layer_source_layer."""
        return self._native.set_layer_source_layer(layer_id, source_layer)

    def set_layer_visibility(
        self, layer_id: str, visibility: StyleLayerVisibility
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_layer_visibility."""
        return self._native.set_layer_visibility(layer_id, visibility)

    def set_location_indicator_accuracy_radius(
        self, layer_id: str, radius: float
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_location_indicator_accuracy_radius."""
        return self._native.set_location_indicator_accuracy_radius(layer_id, radius)

    def set_location_indicator_bearing(
        self, layer_id: str, bearing: float
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_location_indicator_bearing."""
        return self._native.set_location_indicator_bearing(layer_id, bearing)

    def set_location_indicator_image_name(
        self, layer_id: str, image_kind: LocationIndicatorImageKind, image_id: str
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_location_indicator_image_name."""
        return self._native.set_location_indicator_image_name(
            layer_id, image_kind, image_id
        )

    def set_location_indicator_location(
        self, layer_id: str, coordinate: LatLng, altitude: float
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_location_indicator_location."""
        return self._native.set_location_indicator_location(
            layer_id, coordinate, altitude
        )

    def set_projection_mode(
        self, mode: ProjectionMode | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_projection_mode."""
        return self._native.set_projection_mode(mode)

    def set_rendering_stats_view_enabled(
        self, enabled: bool
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_rendering_stats_view_enabled."""
        return self._native.set_rendering_stats_view_enabled(enabled)

    def set_style_image(
        self,
        image_id: str,
        image: PremultipliedRgba8Image | None = None,
        options: StyleImageOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_style_image."""
        return self._native.set_style_image(image_id, image, options)

    def set_style_json(self, json: bytes) -> Future[CommandCompletion]:
        """Call mln_map_set_style_json."""
        return self._native.set_style_json(json)

    def set_style_light_json(self, light_json: bytes) -> Future[CommandCompletion]:
        """Call mln_map_set_style_light_json."""
        return self._native.set_style_light_json(light_json)

    def set_style_light_property(
        self, property_name: str, value: bytes
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_style_light_property."""
        return self._native.set_style_light_property(property_name, value)

    def set_style_source_volatile(
        self, source_id: str, is_volatile: bool
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_style_source_volatile."""
        return self._native.set_style_source_volatile(source_id, is_volatile)

    def set_style_transition_options(
        self, options: StyleTransitionOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_style_transition_options."""
        return self._native.set_style_transition_options(options)

    def set_style_url(self, url: str) -> Future[CommandCompletion]:
        """Call mln_map_set_style_url."""
        return self._native.set_style_url(url)

    def set_tile_options(
        self, options: MapTileOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_tile_options."""
        return self._native.set_tile_options(options)

    def set_viewport_options(
        self, options: MapViewportOptions | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_set_viewport_options."""
        return self._native.set_viewport_options(options)

    def snapshot_get(self) -> MapSnapshot:
        """Call mln_map_snapshot_get."""
        return MapSnapshot._from_native(self._native.snapshot_get())

    def style_url(self) -> Future[str]:
        """Call mln_map_style_url."""
        return self._native.style_url()

    def update_camera(
        self, update: CameraUpdate | None = None
    ) -> Future[CommandCompletion]:
        """Call mln_map_update_camera."""
        return self._native.update_camera(update)

    def metal_borrowed_texture_attach(
        self,
        descriptor: MetalBorrowedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> MetalBorrowedTextureAttachResult:
        """Call mln_metal_borrowed_texture_attach."""
        raw = self._native.metal_borrowed_texture_attach(descriptor, options)
        return MetalBorrowedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def metal_owned_texture_attach(
        self,
        descriptor: MetalOwnedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> MetalOwnedTextureAttachResult:
        """Call mln_metal_owned_texture_attach."""
        raw = self._native.metal_owned_texture_attach(descriptor, options)
        return MetalOwnedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def metal_surface_attach(
        self,
        descriptor: MetalSurfaceDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> MetalSurfaceAttachResult:
        """Call mln_metal_surface_attach."""
        raw = self._native.metal_surface_attach(descriptor, options)
        return MetalSurfaceAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def opengl_borrowed_texture_attach(
        self,
        descriptor: OpenglBorrowedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> OpenglBorrowedTextureAttachResult:
        """Call mln_opengl_borrowed_texture_attach."""
        raw = self._native.opengl_borrowed_texture_attach(descriptor, options)
        return OpenglBorrowedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def opengl_owned_texture_attach(
        self,
        descriptor: OpenglOwnedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> OpenglOwnedTextureAttachResult:
        """Call mln_opengl_owned_texture_attach."""
        raw = self._native.opengl_owned_texture_attach(descriptor, options)
        return OpenglOwnedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def opengl_surface_attach(
        self,
        descriptor: OpenglSurfaceDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> OpenglSurfaceAttachResult:
        """Call mln_opengl_surface_attach."""
        raw = self._native.opengl_surface_attach(descriptor, options)
        return OpenglSurfaceAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def vulkan_borrowed_texture_attach(
        self,
        descriptor: VulkanBorrowedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> VulkanBorrowedTextureAttachResult:
        """Call mln_vulkan_borrowed_texture_attach."""
        raw = self._native.vulkan_borrowed_texture_attach(descriptor, options)
        return VulkanBorrowedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def vulkan_owned_texture_attach(
        self,
        descriptor: VulkanOwnedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> VulkanOwnedTextureAttachResult:
        """Call mln_vulkan_owned_texture_attach."""
        raw = self._native.vulkan_owned_texture_attach(descriptor, options)
        return VulkanOwnedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def vulkan_surface_attach(
        self,
        descriptor: VulkanSurfaceDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> VulkanSurfaceAttachResult:
        """Call mln_vulkan_surface_attach."""
        raw = self._native.vulkan_surface_attach(descriptor, options)
        return VulkanSurfaceAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def webgpu_borrowed_texture_attach(
        self,
        descriptor: WebgpuBorrowedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> WebgpuBorrowedTextureAttachResult:
        """Call mln_webgpu_borrowed_texture_attach."""
        raw = self._native.webgpu_borrowed_texture_attach(descriptor, options)
        return WebgpuBorrowedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def webgpu_owned_texture_attach(
        self,
        descriptor: WebgpuOwnedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> WebgpuOwnedTextureAttachResult:
        """Call mln_webgpu_owned_texture_attach."""
        raw = self._native.webgpu_owned_texture_attach(descriptor, options)
        return WebgpuOwnedTextureAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )

    def webgpu_surface_attach(
        self,
        descriptor: WebgpuSurfaceDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> WebgpuSurfaceAttachResult:
        """Call mln_webgpu_surface_attach."""
        raw = self._native.webgpu_surface_attach(descriptor, options)
        return WebgpuSurfaceAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )


class _MapProjectionHandleOperations(GeneratedOperations):
    _native: _native._MapProjectionHandle

    def close(self) -> None:
        """Call mln_map_projection_close."""
        return self._native.close()

    def get_camera(self) -> CameraOptions:
        """Call mln_map_projection_get_camera."""
        return CameraOptions._from_native(self._native.get_camera())

    def lat_lng_for_pixel(self, point: ScreenPoint) -> LatLng:
        """Call mln_map_projection_lat_lng_for_pixel."""
        return LatLng._from_native(self._native.lat_lng_for_pixel(point))

    def lat_lng_for_pixel_unwrapped(self, point: ScreenPoint) -> LatLng:
        """Call mln_map_projection_lat_lng_for_pixel_unwrapped."""
        return LatLng._from_native(self._native.lat_lng_for_pixel_unwrapped(point))

    def meters_per_pixel_at_latitude(self, latitude: float) -> float:
        """Call mln_map_projection_meters_per_pixel_at_latitude."""
        return self._native.meters_per_pixel_at_latitude(latitude)

    def pixel_for_lat_lng(self, coordinate: LatLng) -> ScreenPoint:
        """Call mln_map_projection_pixel_for_lat_lng."""
        return ScreenPoint._from_native(self._native.pixel_for_lat_lng(coordinate))

    def set_camera(self, camera: CameraOptions | None = None) -> None:
        """Call mln_map_projection_set_camera."""
        return self._native.set_camera(camera)

    def set_visible_coordinates(
        self, coordinates: tuple[LatLng, ...], padding: EdgeInsets
    ) -> None:
        """Call mln_map_projection_set_visible_coordinates."""
        return self._native.set_visible_coordinates(coordinates, padding)

    def set_visible_geometry(self, geometry: bytes, padding: EdgeInsets) -> None:
        """Call mln_map_projection_set_visible_geometry."""
        return self._native.set_visible_geometry(geometry, padding)


class _RenderFrameBatchHandleOperations(GeneratedOperations):
    _native: _native._RenderFrameBatchHandle

    def count(self) -> int:
        """Call mln_render_frame_batch_count."""
        return self._native.count()

    def get(self, index: int) -> RenderFrameResult:
        """Call mln_render_frame_batch_get."""
        return RenderFrameResult._from_native(self._native.get(index))

    def close(self) -> None:
        """Call mln_render_frame_batch_release."""
        return self._native.close()


class _RenderSessionHandleOperations(GeneratedOperations):
    _native: _native._RenderSessionHandle

    def metal_borrowed_texture_set_target(
        self, descriptor: MetalBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Call mln_metal_borrowed_texture_set_target."""
        return self._native.metal_borrowed_texture_set_target(descriptor)

    def metal_surface_set_target(
        self, descriptor: MetalSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Call mln_metal_surface_set_target."""
        return self._native.metal_surface_set_target(descriptor)

    def opengl_borrowed_texture_set_target(
        self, descriptor: OpenglBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Call mln_opengl_borrowed_texture_set_target."""
        return self._native.opengl_borrowed_texture_set_target(descriptor)

    def opengl_surface_set_target(
        self, descriptor: OpenglSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Call mln_opengl_surface_set_target."""
        return self._native.opengl_surface_set_target(descriptor)

    def abandon(self) -> RenderAbandonResult:
        """Call mln_render_session_abandon."""
        return RenderAbandonResult._from_native(self._native.abandon())

    def acquire_frame(self) -> AcquiredFrameHandle:
        """Call mln_render_session_acquire_frame."""
        return _adopt_value(self._native.acquire_frame(), "AcquiredFrameHandle", self)

    def barrier(self) -> Future[None]:
        """Call mln_render_session_barrier."""
        return self._native.barrier()

    def clear_data(self) -> Future[None]:
        """Call mln_render_session_clear_data."""
        return self._native.clear_data()

    def close(self) -> None:
        """Call mln_render_session_destroy."""
        return self._native.close()

    def detach(self) -> Future[None]:
        """Call mln_render_session_detach."""
        return self._native.detach()

    def drain_frame_results(self) -> RenderFrameBatchHandle:
        """Call mln_render_session_drain_frame_results."""
        return _adopt_value(
            self._native.drain_frame_results(), "RenderFrameBatchHandle", None
        )

    def dump_debug_logs(self) -> Future[None]:
        """Call mln_render_session_dump_debug_logs."""
        return self._native.dump_debug_logs()

    def get_capabilities(self) -> RenderSessionCapabilities:
        """Call mln_render_session_get_capabilities."""
        return RenderSessionCapabilities._from_native(self._native.get_capabilities())

    def get_snapshot(self) -> RenderSessionSnapshot:
        """Call mln_render_session_get_snapshot."""
        return RenderSessionSnapshot._from_native(self._native.get_snapshot())

    def projection_create(self) -> MapProjectionHandle:
        """Call mln_render_session_projection_create."""
        return _adopt_value(
            self._native.projection_create(), "MapProjectionHandle", None
        )

    def query_feature_extensions(
        self,
        source_id: str,
        feature: bytes,
        extension: str,
        extension_field: str,
        arguments: bytes | None = None,
    ) -> Future[bytes]:
        """Call mln_render_session_query_feature_extensions."""
        return self._native.query_feature_extensions(
            source_id, feature, extension, extension_field, arguments
        )

    def query_rendered_features(
        self,
        geometry: RenderedQueryGeometry,
        options: RenderedFeatureQueryOptions | None = None,
    ) -> Future[tuple[QueriedFeature, ...]]:
        """Call mln_render_session_query_rendered_features."""
        return map_future(
            self._native.query_rendered_features(geometry, options),
            lambda value: tuple(QueriedFeature._from_native(item) for item in value),
        )

    def query_source_features(
        self, source_id: str, options: SourceFeatureQueryOptions | None = None
    ) -> Future[tuple[QueriedFeature, ...]]:
        """Call mln_render_session_query_source_features."""
        return map_future(
            self._native.query_source_features(source_id, options),
            lambda value: tuple(QueriedFeature._from_native(item) for item in value),
        )

    def reduce_memory_use(self) -> Future[None]:
        """Call mln_render_session_reduce_memory_use."""
        return self._native.reduce_memory_use()

    def request_frame(self, demand: FrameDemand | None = None) -> None:
        """Call mln_render_session_request_frame."""
        return self._native.request_frame(demand)

    def resize(self, extent: RenderTargetExtent) -> Future[CommandCompletion]:
        """Call mln_render_session_resize."""
        return self._native.resize(extent)

    def service_driver_work(self, max_work: int) -> int:
        """Call mln_render_session_service_driver_work."""
        return self._native.service_driver_work(max_work)

    def texture_read_premultiplied_rgba8(self) -> Future[TextureReadbackResult]:
        """Call mln_texture_read_premultiplied_rgba8."""
        return map_future(
            self._native.texture_read_premultiplied_rgba8(),
            lambda value: TextureReadbackResult._from_native(value),
        )

    def vulkan_borrowed_texture_set_target(
        self, descriptor: VulkanBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Call mln_vulkan_borrowed_texture_set_target."""
        return self._native.vulkan_borrowed_texture_set_target(descriptor)

    def vulkan_surface_set_target(
        self, descriptor: VulkanSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Call mln_vulkan_surface_set_target."""
        return self._native.vulkan_surface_set_target(descriptor)

    def webgpu_borrowed_texture_set_target(
        self, descriptor: WebgpuBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Call mln_webgpu_borrowed_texture_set_target."""
        return self._native.webgpu_borrowed_texture_set_target(descriptor)

    def webgpu_surface_set_target(
        self, descriptor: WebgpuSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Call mln_webgpu_surface_set_target."""
        return self._native.webgpu_surface_set_target(descriptor)


class _ResourceRequestHandleOperations(GeneratedOperations):
    _native: _native._ResourceRequestHandle

    def cancelled(self) -> bool:
        """Call mln_resource_request_cancelled."""
        return self._native.cancelled()

    def complete(self, response: ResourceResponse) -> None:
        """Call mln_resource_request_complete."""
        return self._native.complete(response)

    def close(self) -> None:
        return self._native.close()

    def set_cancel_callback(self, callback: Callable[[], None]) -> bool:
        return self._native.set_cancel_callback(callback)

    def wait_until_retired(self) -> None:
        """Call mln_resource_request_wait_until_retired."""
        return self._native.wait_until_retired()


class _ResourceTransformResponseScopeOperations(GeneratedOperations):
    _native: _native._ResourceTransformResponseScope

    def set_url(self, url: str) -> None:
        """Call mln_resource_transform_response_set_url."""
        return self._native.set_url(url)


class _RuntimeHandleOperations(GeneratedOperations):
    _native: _native._RuntimeHandle

    def map_create(self, options: MapOptions | None = None) -> Future[MapHandle]:
        """Call mln_map_create."""
        return _adopt_future(self._native.map_create(options), "MapHandle", self)

    def barrier(self) -> Future[None]:
        """Call mln_runtime_barrier."""
        return self._native.barrier()

    def clear_http_header_transform(self) -> Future[None]:
        """Call mln_runtime_clear_http_header_transform."""
        return self._native.clear_http_header_transform()

    def clear_resource_provider(self) -> Future[None]:
        """Call mln_runtime_clear_resource_provider."""
        return self._native.clear_resource_provider()

    def clear_resource_transform(self) -> Future[None]:
        """Call mln_runtime_clear_resource_transform."""
        return self._native.clear_resource_transform()

    def drain_events(self) -> EventBatchHandle:
        """Call mln_runtime_drain_events."""
        return _adopt_value(self._native.drain_events(), "EventBatchHandle", None)

    def get_event_mask(self) -> RuntimeEventMask:
        """Call mln_runtime_get_event_mask."""
        return RuntimeEventMask(self._native.get_event_mask())

    def offline_region_create(
        self, definition: OfflineRegionDefinition, metadata: bytes
    ) -> Future[OfflineRegionInfo]:
        """Call mln_runtime_offline_region_create."""
        return map_future(
            self._native.offline_region_create(definition, metadata),
            lambda value: OfflineRegionInfo._from_native(value),
        )

    def offline_region_delete(self, region_id: int) -> Future[None]:
        """Call mln_runtime_offline_region_delete."""
        return self._native.offline_region_delete(region_id)

    def offline_region_get(self, region_id: int) -> Future[OfflineRegionInfo | None]:
        """Call mln_runtime_offline_region_get."""
        return map_future(
            self._native.offline_region_get(region_id),
            lambda value: (
                None if value is None else (OfflineRegionInfo._from_native(value))
            ),
        )

    def offline_region_get_status(self, region_id: int) -> Future[OfflineRegionStatus]:
        """Call mln_runtime_offline_region_get_status."""
        return map_future(
            self._native.offline_region_get_status(region_id),
            lambda value: OfflineRegionStatus._from_native(value),
        )

    def offline_region_invalidate(self, region_id: int) -> Future[None]:
        """Call mln_runtime_offline_region_invalidate."""
        return self._native.offline_region_invalidate(region_id)

    def offline_region_set_download_state(
        self, region_id: int, input_state: OfflineRegionDownloadState
    ) -> Future[None]:
        """Call mln_runtime_offline_region_set_download_state."""
        return self._native.offline_region_set_download_state(region_id, input_state)

    def offline_region_set_observed(
        self, region_id: int, observed: bool
    ) -> Future[None]:
        """Call mln_runtime_offline_region_set_observed."""
        return self._native.offline_region_set_observed(region_id, observed)

    def offline_region_update_metadata(
        self, region_id: int, metadata: bytes
    ) -> Future[OfflineRegionInfo]:
        """Call mln_runtime_offline_region_update_metadata."""
        return map_future(
            self._native.offline_region_update_metadata(region_id, metadata),
            lambda value: OfflineRegionInfo._from_native(value),
        )

    def offline_regions_list(self) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Call mln_runtime_offline_regions_list."""
        return map_future(
            self._native.offline_regions_list(),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def offline_regions_merge_database(
        self, side_database_path: str
    ) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Call mln_runtime_offline_regions_merge_database."""
        return map_future(
            self._native.offline_regions_merge_database(side_database_path),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def close(self) -> Future[None]:
        """Call mln_runtime_release."""
        return self._native.close()

    def run_ambient_cache_operation(
        self, operation: AmbientCacheOperation
    ) -> Future[None]:
        """Call mln_runtime_run_ambient_cache_operation."""
        return self._native.run_ambient_cache_operation(operation)

    def set_event_mask(self, mask: RuntimeEventMask) -> None:
        """Call mln_runtime_set_event_mask."""
        return self._native.set_event_mask(mask)

    def set_http_header_transform(self, transform: HttpHeaderTransform) -> Future[None]:
        """Call mln_runtime_set_http_header_transform."""
        return self._native.set_http_header_transform(transform)

    def set_maximum_ambient_cache_size(self, size: int) -> Future[None]:
        """Call mln_runtime_set_maximum_ambient_cache_size."""
        return self._native.set_maximum_ambient_cache_size(size)

    def set_resource_provider(self, provider: ResourceProvider) -> Future[None]:
        """Call mln_runtime_set_resource_provider."""
        return self._native.set_resource_provider(provider)

    def set_resource_transform(self, transform: ResourceTransform) -> Future[None]:
        """Call mln_runtime_set_resource_transform."""
        return self._native.set_resource_transform(transform)


def android_init(jni_env: int, jni_class: int, context: int) -> None:
    """Call mln_android_init."""
    return _native.android_init(jni_env, jni_class, context)


def c_version() -> int:
    """Call mln_c_version."""
    return _native.c_version()


def geojson_source_data_create(
    data: bytes, options: GeojsonSourceOptions | None = None
) -> GeojsonSourceDataHandle:
    """Call mln_geojson_source_data_create."""
    return _adopt_value(
        _native.geojson_source_data_create(data, options),
        "GeojsonSourceDataHandle",
        None,
    )


def lat_lng_for_projected_meters(meters: ProjectedMeters) -> LatLng:
    """Call mln_lat_lng_for_projected_meters."""
    return LatLng._from_native(_native.lat_lng_for_projected_meters(meters))


def log_clear_callback() -> None:
    """Call mln_log_clear_callback."""
    return _native.log_clear_callback()


def log_set_async_severity_mask(mask: LogSeverityMask) -> None:
    """Call mln_log_set_async_severity_mask."""
    return _native.log_set_async_severity_mask(mask)


def log_set_callback(
    callback: Callable[[LogSeverity, LogEvent, int, str], int] | None = None,
) -> None:
    return _native.log_set_callback(LogSetCallbackRegistration(callback))


def network_status_get() -> NetworkStatus:
    """Call mln_network_status_get."""
    return NetworkStatus(_native.network_status_get())


def network_status_set(input_status: NetworkStatus) -> None:
    """Call mln_network_status_set."""
    return _native.network_status_set(input_status)


def opengl_supported_context_provider_mask() -> OpenglContextProviderFlag:
    """Call mln_opengl_supported_context_provider_mask."""
    return OpenglContextProviderFlag(_native.opengl_supported_context_provider_mask())


def plugin_get_register_function_v1() -> int:
    """Call mln_plugin_get_register_function_v1."""
    return _native.plugin_get_register_function_v1()


def projected_meters_for_lat_lng(coordinate: LatLng) -> ProjectedMeters:
    """Call mln_projected_meters_for_lat_lng."""
    return ProjectedMeters._from_native(
        _native.projected_meters_for_lat_lng(coordinate)
    )


def render_target_extent_physical_size(
    extent: RenderTargetExtent,
) -> RenderTargetExtentPhysicalSizeResult:
    """Call mln_render_target_extent_physical_size."""
    raw = _native.render_target_extent_physical_size(extent)
    return RenderTargetExtentPhysicalSizeResult(raw["width"], raw["height"])


def rendered_query_geometry_box(input_box: ScreenBox) -> RenderedQueryGeometry:
    """Call mln_rendered_query_geometry_box."""
    return RenderedQueryGeometry._from_native(
        _native.rendered_query_geometry_box(input_box)
    )


def rendered_query_geometry_line_string(
    points: tuple[ScreenPoint, ...],
) -> RenderedQueryGeometry:
    """Call mln_rendered_query_geometry_line_string."""
    return RenderedQueryGeometry._from_native(
        _native.rendered_query_geometry_line_string(points)
    )


def rendered_query_geometry_point(point: ScreenPoint) -> RenderedQueryGeometry:
    """Call mln_rendered_query_geometry_point."""
    return RenderedQueryGeometry._from_native(
        _native.rendered_query_geometry_point(point)
    )


def runtime_create(options: RuntimeOptions | None = None) -> RuntimeHandle:
    """Call mln_runtime_create."""
    return _adopt_value(_native.runtime_create(options), "RuntimeHandle", None)


def supported_render_backend_mask() -> RenderBackendFlag:
    """Call mln_supported_render_backend_mask."""
    return RenderBackendFlag(_native.supported_render_backend_mask())
