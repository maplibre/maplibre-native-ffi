"""Generated from the C headers by tools/bindgen. Do not edit."""

from __future__ import annotations

from collections.abc import Callable
from concurrent.futures import Future
from typing import TYPE_CHECKING, NamedTuple, TypeVar

from . import _native
from ._completion import CommandCompletion
from ._future import map_future
from ._generated_values import _maybe
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
    MetalSurfaceDescriptor,
    MetalTextureFrame,
    NetworkStatus,
    OfflineRegionDefinition,
    OfflineRegionDownloadState,
    OfflineRegionInfo,
    OfflineRegionStatus,
    OpenglBorrowedTextureDescriptor,
    OpenglContextProviderFlag,
    OpenglOwnedTextureDescriptor,
    OpenglSurfaceDescriptor,
    OpenglTextureFrame,
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
    VulkanSurfaceDescriptor,
    VulkanTextureFrame,
    WebgpuBorrowedTextureDescriptor,
    WebgpuOwnedTextureDescriptor,
    WebgpuSurfaceDescriptor,
    WebgpuTextureFrame,
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

    def with_metal_texture(self, callback: Callable[[MetalTextureFrame], R]) -> R:
        """Copies Metal-native metadata from an acquired frame.

        See `mln_acquired_frame_get_metal_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Native resources remain valid only during the callback.
        """
        return _with_view(
            self,
            lambda: self._native.with_metal_texture(),
            lambda raw: MetalTextureFrame._from_native(raw),
            callback,
        )

    def with_opengl_texture(self, callback: Callable[[OpenglTextureFrame], R]) -> R:
        """Copies OpenGL-native metadata from an acquired frame.

        See `mln_acquired_frame_get_opengl_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Native resources remain valid only during the callback.
        """
        return _with_view(
            self,
            lambda: self._native.with_opengl_texture(),
            lambda raw: OpenglTextureFrame._from_native(raw),
            callback,
        )

    def with_producer_sync(self, callback: Callable[[GpuSync], R]) -> R:
        """Copies the producer synchronization for an acquired texture frame.

        See `mln_acquired_frame_get_producer_sync` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Native resources remain valid only during the callback.
        """
        return _with_view(
            self,
            lambda: self._native.with_producer_sync(),
            lambda raw: GpuSync._from_native(raw),
            callback,
        )

    def get_result(self) -> RenderFrameResult:
        """Copies common metadata for an acquired frame.

        See `mln_acquired_frame_get_result` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderFrameResult._from_native(self._native.get_result())

    def with_vulkan_texture(self, callback: Callable[[VulkanTextureFrame], R]) -> R:
        """Copies Vulkan-native metadata from an acquired frame.

        See `mln_acquired_frame_get_vulkan_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Native resources remain valid only during the callback.
        """
        return _with_view(
            self,
            lambda: self._native.with_vulkan_texture(),
            lambda raw: VulkanTextureFrame._from_native(raw),
            callback,
        )

    def with_webgpu_texture(self, callback: Callable[[WebgpuTextureFrame], R]) -> R:
        """Copies WebGPU-native metadata from an acquired frame.

        See `mln_acquired_frame_get_webgpu_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Native resources remain valid only during the callback.
        """
        return _with_view(
            self,
            lambda: self._native.with_webgpu_texture(),
            lambda raw: WebgpuTextureFrame._from_native(raw),
            callback,
        )

    def close(self, consumer_completion: GpuSync | None = None) -> None:
        """Releases an acquired frame after optional consumer GPU work.

        See `mln_acquired_frame_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.close(consumer_completion)


class _BufferHandleOperations(GeneratedOperations):
    _native: _native._BufferHandle

    def close(self) -> None:
        """Destroys an owned buffer. A null handle is a no-op.

        See `mln_buffer_destroy` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
        """
        return self._native.close()

    def get(self) -> bytes:
        """Borrows the data stored by an owned buffer.

        See `mln_buffer_get` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
        """
        return self._native.get()


class _EventBatchHandleOperations(GeneratedOperations):
    _native: _native._EventBatchHandle

    def get(self) -> RuntimeEventBatchView:
        """Borrows the event and message view stored by an owned event batch.

        See `mln_event_batch_get` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return RuntimeEventBatchView._from_native(self._native.get())

    def close(self) -> None:
        """Releases an owned event batch. A null handle is a no-op.

        See `mln_event_batch_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.close()


class _GeojsonSourceDataHandleOperations(GeneratedOperations):
    _native: _native._GeojsonSourceDataHandle

    def close(self) -> None:
        """Releases prepared GeoJSON source data.

        See `mln_geojson_source_data_destroy` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.close()


class _HttpHeaderTransformResponseScopeOperations(GeneratedOperations):
    _native: _native._HttpHeaderTransformResponseScope

    def set(self, name: str, value: str) -> None:
        """Sets one outgoing HTTP request header for the current transform
        invocation.

        See `mln_http_header_transform_response_set` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set(name, value)


class _MapHandleOperations(GeneratedOperations):
    _native: _native._MapHandle

    def add_color_relief_layer(
        self, layer_id: str, source_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Adds a color-relief layer for a raster DEM source.

        See `mln_map_add_color_relief_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_color_relief_layer(layer_id, source_id, before_layer_id)

    def add_custom_geometry_source(
        self, source_id: str, options: CustomGeometrySourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Adds a custom geometry source.

        See `mln_map_add_custom_geometry_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_custom_geometry_source(source_id, options)

    def add_custom_mvt_vector_source(
        self, source_id: str, options: CustomMvtVectorSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Adds a custom MVT vector source.

        See `mln_map_add_custom_mvt_vector_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_custom_mvt_vector_source(source_id, options)

    def add_geojson_source_data(
        self, source_id: str, data: GeojsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Adds a GeoJSON source with prepared inline data.

        See `mln_map_add_geojson_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_geojson_source_data(source_id, data._native)

    def add_geojson_source_url(
        self, source_id: str, url: str, options: GeojsonSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Adds a GeoJSON source with URL data.

        See `mln_map_add_geojson_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_geojson_source_url(source_id, url, options)

    def add_hillshade_layer(
        self, layer_id: str, source_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Adds a hillshade layer for a raster DEM source.

        See `mln_map_add_hillshade_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_hillshade_layer(layer_id, source_id, before_layer_id)

    def add_image_source_image(
        self,
        source_id: str,
        coordinates: tuple[LatLng, ...],
        image: PremultipliedRgba8Image | None = None,
    ) -> Future[CommandCompletion]:
        """Adds an image source with inline image pixels.

        See `mln_map_add_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_image_source_image(source_id, coordinates, image)

    def add_image_source_url(
        self, source_id: str, coordinates: tuple[LatLng, ...], url: str
    ) -> Future[CommandCompletion]:
        """Adds an image source that loads its image from a URL.

        See `mln_map_add_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_image_source_url(source_id, coordinates, url)

    def add_location_indicator_layer(
        self, layer_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Adds a source-free location indicator layer.

        See `mln_map_add_location_indicator_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_location_indicator_layer(layer_id, before_layer_id)

    def add_raster_dem_source_tiles(
        self,
        source_id: str,
        tiles: tuple[str, ...],
        options: StyleTileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Adds a raster DEM source with inline tile URLs.

        See `mln_map_add_raster_dem_source_tiles` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_raster_dem_source_tiles(source_id, tiles, options)

    def add_raster_dem_source_url(
        self, source_id: str, url: str, options: StyleTileSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Adds a raster DEM source with a TileJSON URL.

        See `mln_map_add_raster_dem_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_raster_dem_source_url(source_id, url, options)

    def add_raster_source_tiles(
        self,
        source_id: str,
        tiles: tuple[str, ...],
        options: StyleTileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Adds a raster source with inline tile URLs.

        See `mln_map_add_raster_source_tiles` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_raster_source_tiles(source_id, tiles, options)

    def add_raster_source_url(
        self, source_id: str, url: str, options: StyleTileSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Adds a raster source with a TileJSON URL.

        See `mln_map_add_raster_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_raster_source_url(source_id, url, options)

    def add_style_layer_json(
        self, layer_json: bytes, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Adds one style layer from a full style-spec layer JSON object.

        See `mln_map_add_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_style_layer_json(layer_json, before_layer_id)

    def add_style_source_json(
        self, source_id: str, source_json: bytes
    ) -> Future[CommandCompletion]:
        """Adds one style source from a style-spec source JSON object.

        See `mln_map_add_style_source_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_style_source_json(source_id, source_json)

    def add_vector_source_tiles(
        self,
        source_id: str,
        tiles: tuple[str, ...],
        options: StyleTileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Adds a vector source with inline tile URLs.

        See `mln_map_add_vector_source_tiles` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_vector_source_tiles(source_id, tiles, options)

    def add_vector_source_url(
        self, source_id: str, url: str, options: StyleTileSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Adds a vector source with a TileJSON URL.

        See `mln_map_add_vector_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_vector_source_url(source_id, url, options)

    def apply_camera_delta(
        self, delta: CameraDelta | None = None
    ) -> Future[CommandCompletion]:
        """Submits one copied relative camera update.

        See `mln_map_apply_camera_delta` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.apply_camera_delta(delta)

    def camera_for_geometry(
        self, geometry: bytes, fit_options: CameraFitOptions | None = None
    ) -> Future[CameraOptions]:
        """Starts an ordered query for a camera that fits a GeoJSON geometry.

        See `mln_map_camera_for_geometry` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.camera_for_geometry(geometry, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def camera_for_lat_lng_bounds(
        self, bounds: LatLngBounds, fit_options: CameraFitOptions | None = None
    ) -> Future[CameraOptions]:
        """Starts an ordered query for a camera that fits geographic bounds.

        See `mln_map_camera_for_lat_lng_bounds` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.camera_for_lat_lng_bounds(bounds, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def camera_for_lat_lngs(
        self,
        coordinates: tuple[LatLng, ...],
        fit_options: CameraFitOptions | None = None,
    ) -> Future[CameraOptions]:
        """Starts an ordered query for a camera that fits geographic
        coordinates.

        See `mln_map_camera_for_lat_lngs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.camera_for_lat_lngs(coordinates, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def camera_query(self) -> Future[CameraQueryResult]:
        """Starts an ordered camera read.

        See `mln_map_camera_query` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.camera_query(),
            lambda value: CameraQueryResult._from_native(value),
        )

    def camera_snapshot_get(self) -> MapCameraSnapshotGetResult:
        """Copies the camera from the latest immutable map snapshot.

        See `mln_map_camera_snapshot_get` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        raw = self._native.camera_snapshot_get()
        return MapCameraSnapshotGetResult(
            CameraOptions._from_native(raw["camera"]), raw["generation"]
        )

    def cancel_transitions(self) -> Future[CommandCompletion]:
        """Cancels the camera transitions running when this command commits.

        See `mln_map_cancel_transitions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.cancel_transitions()

    def copy_layer_source_id(self, layer_id: str) -> Future[str | None]:
        """Copies one layer's source ID.

        See `mln_map_copy_layer_source_id` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.copy_layer_source_id(layer_id)

    def copy_layer_source_layer(self, layer_id: str) -> Future[str | None]:
        """Copies one layer's source-layer ID.

        See `mln_map_copy_layer_source_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.copy_layer_source_layer(layer_id)

    def copy_style_image_premultiplied_rgba8(
        self, image_id: str
    ) -> Future[bytes | None]:
        """Copies one runtime style image as tightly packed premultiplied RGBA8
        pixels.

        See `mln_map_copy_style_image_premultiplied_rgba8` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.copy_style_image_premultiplied_rgba8(image_id)

    def copy_style_image_stretches(
        self, image_id: str
    ) -> Future[StyleImageStretchesResult | None]:
        """Copies one runtime style image's stretchable intervals.

        See `mln_map_copy_style_image_stretches` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.copy_style_image_stretches(image_id),
            lambda value: _maybe(StyleImageStretchesResult._from_native, value),
        )

    def copy_style_source_attribution(self, source_id: str) -> Future[str | None]:
        """Copies one style source attribution string.

        See `mln_map_copy_style_source_attribution` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.copy_style_source_attribution(source_id)

    def copy_style_source_url(self, source_id: str) -> Future[str | None]:
        """Copies one style source URL.

        See `mln_map_copy_style_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.copy_style_source_url(source_id)

    def dump_debug_logs(self) -> Future[CommandCompletion]:
        """Submits an ordered debug-log command.

        See `mln_map_dump_debug_logs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.dump_debug_logs()

    def get_feature_state(self, selector: FeatureStateSelector) -> Future[bytes]:
        """Starts an ordered read of per-feature state from this map.

        See `mln_map_get_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.get_feature_state(selector)

    def get_global_state(self) -> Future[bytes]:
        """Queries the global-state JSON object, including style defaults.
        Completion borrows one `mln_buffer_view` for the duration of the
        callback.

        See `mln_map_get_global_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_global_state()

    def get_image_source_coordinates(
        self, source_id: str
    ) -> Future[tuple[LatLng, ...] | None]:
        """Copies image source coordinates.

        See `mln_map_get_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_image_source_coordinates(source_id),
            lambda value: (
                None
                if value is None
                else (tuple(LatLng._from_native(item) for item in value))
            ),
        )

    def get_layer_filter(self, layer_id: str) -> Future[bytes | None]:
        """Serializes one layer filter as a style-spec JSON value.

        See `mln_map_get_layer_filter` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_layer_filter(layer_id)

    def get_layer_property(
        self, layer_id: str, property_name: str
    ) -> Future[bytes | None]:
        """Serializes one layer property as a style-spec JSON value.

        See `mln_map_get_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_layer_property(layer_id, property_name)

    def get_style_image_info(self, image_id: str) -> Future[StyleImageResult | None]:
        """Copies one complete runtime style image.

        See `mln_map_get_style_image_info` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_image_info(image_id),
            lambda value: _maybe(StyleImageResult._from_native, value),
        )

    def get_style_layer_info(self, layer_id: str) -> Future[StyleLayerResult | None]:
        """Copies complete metadata for one style layer.

        See `mln_map_get_style_layer_info` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_layer_info(layer_id),
            lambda value: _maybe(StyleLayerResult._from_native, value),
        )

    def get_style_layer_json(self, layer_id: str) -> Future[bytes | None]:
        """Serializes one style layer as a full style-spec layer JSON object.

        See `mln_map_get_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_layer_json(layer_id)

    def get_style_light_property(self, property_name: str) -> Future[bytes | None]:
        """Serializes one style light property as a style-spec JSON value.

        See `mln_map_get_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_light_property(property_name)

    def get_style_source_info(self, source_id: str) -> Future[StyleSourceResult | None]:
        """Copies complete metadata for one style source.

        See `mln_map_get_style_source_info` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_source_info(source_id),
            lambda value: _maybe(StyleSourceResult._from_native, value),
        )

    def get_style_source_tile_urls(
        self, source_id: str
    ) -> Future[StyleSourceTileUrlsResult | None]:
        """Copies one style source's inline TileJSON tile URLs.

        See `mln_map_get_style_source_tile_urls` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_source_tile_urls(source_id),
            lambda value: _maybe(StyleSourceTileUrlsResult._from_native, value),
        )

    def get_style_transition_options(self) -> Future[StyleTransitionOptions]:
        """Reads the style's global transition options.

        See `mln_map_get_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_transition_options(),
            lambda value: StyleTransitionOptions._from_native(value),
        )

    def invalidate_custom_geometry_source_region(
        self, source_id: str, bounds: LatLngBounds
    ) -> Future[CommandCompletion]:
        """Invalidates custom geometry source data inside one geographic region.

        See `mln_map_invalidate_custom_geometry_source_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.invalidate_custom_geometry_source_region(source_id, bounds)

    def invalidate_custom_geometry_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> Future[CommandCompletion]:
        """Invalidates custom geometry source data for one canonical tile.

        See `mln_map_invalidate_custom_geometry_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.invalidate_custom_geometry_source_tile(source_id, tile_id)

    def invalidate_custom_mvt_vector_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> Future[CommandCompletion]:
        """Invalidates custom MVT vector source data for one canonical tile.

        See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.invalidate_custom_mvt_vector_source_tile(source_id, tile_id)

    def lat_lng_bounds_for_camera(
        self, camera: CameraOptions | None = None
    ) -> Future[LatLngBounds]:
        """Starts an ordered wrapped-bounds query for a copied camera.

        See `mln_map_lat_lng_bounds_for_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lng_bounds_for_camera(camera),
            lambda value: LatLngBounds._from_native(value),
        )

    def lat_lng_bounds_for_camera_unwrapped(
        self, camera: CameraOptions | None = None
    ) -> Future[LatLngBounds]:
        """Starts an ordered unwrapped-bounds query for a copied camera.

        See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lng_bounds_for_camera_unwrapped(camera),
            lambda value: LatLngBounds._from_native(value),
        )

    def lat_lng_for_pixel(self, point: ScreenPoint) -> Future[LatLng]:
        """Starts an ordered conversion from a screen point to a geographic
        coordinate.

        See `mln_map_lat_lng_for_pixel` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lng_for_pixel(point),
            lambda value: LatLng._from_native(value),
        )

    def lat_lng_for_pixel_unwrapped(self, point: ScreenPoint) -> Future[LatLng]:
        """Starts an ordered conversion from a screen point to an unwrapped
        geographic coordinate.

        See `mln_map_lat_lng_for_pixel_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lng_for_pixel_unwrapped(point),
            lambda value: LatLng._from_native(value),
        )

    def lat_lngs_for_pixels(
        self, points: tuple[ScreenPoint, ...]
    ) -> Future[tuple[LatLng, ...]]:
        """Starts an ordered conversion of copied screen points to coordinates.

        See `mln_map_lat_lngs_for_pixels` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lngs_for_pixels(points),
            lambda value: tuple(LatLng._from_native(item) for item in value),
        )

    def lat_lngs_for_pixels_unwrapped(
        self, points: tuple[ScreenPoint, ...]
    ) -> Future[tuple[LatLng, ...]]:
        """Starts an ordered conversion of copied screen points to unwrapped
        coordinates.

        See `mln_map_lat_lngs_for_pixels_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lngs_for_pixels_unwrapped(points),
            lambda value: tuple(LatLng._from_native(item) for item in value),
        )

    def list_style_layer_ids(self) -> Future[tuple[str, ...]]:
        """Copies style layer IDs in style order.

        See `mln_map_list_style_layer_ids` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.list_style_layer_ids(),
            lambda value: tuple(item for item in value),
        )

    def list_style_layers(self) -> Future[tuple[StyleLayerEntry, ...]]:
        """Starts an ordered query of every style layer in style order.

        See `mln_map_list_style_layers` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.list_style_layers(),
            lambda value: tuple(StyleLayerEntry._from_native(item) for item in value),
        )

    def list_style_source_ids(self) -> Future[tuple[str, ...]]:
        """Copies style source IDs in style order.

        See `mln_map_list_style_source_ids` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.list_style_source_ids(),
            lambda value: tuple(item for item in value),
        )

    def loaded_style_json(self) -> Future[bytes]:
        """Starts an ordered copy of the last successfully parsed style
        document.

        See `mln_map_loaded_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.loaded_style_json()

    def meters_per_pixel_at_latitude(self, latitude: float) -> Future[float]:
        """Starts an ordered query of meters per logical pixel at a latitude and
        the current map zoom. The completion borrows one double.

        See `mln_map_meters_per_pixel_at_latitude` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.meters_per_pixel_at_latitude(latitude)

    def move_style_layer(
        self, layer_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Moves one style layer before another layer or to the top.

        See `mln_map_move_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.move_style_layer(layer_id, before_layer_id)

    def pixel_for_lat_lng(self, coordinate: LatLng) -> Future[ScreenPoint]:
        """Starts an ordered conversion from a geographic coordinate to a screen
        point.

        See `mln_map_pixel_for_lat_lng` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.pixel_for_lat_lng(coordinate),
            lambda value: ScreenPoint._from_native(value),
        )

    def pixels_for_lat_lngs(
        self, coordinates: tuple[LatLng, ...]
    ) -> Future[tuple[ScreenPoint, ...]]:
        """Starts an ordered conversion of copied coordinates to screen points.

        See `mln_map_pixels_for_lat_lngs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.pixels_for_lat_lngs(coordinates),
            lambda value: tuple(ScreenPoint._from_native(item) for item in value),
        )

    def projection_create(self) -> Future[MapProjectionHandle]:
        """Starts creation of a standalone projection from the map's ordered
        transform state.

        See `mln_map_projection_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return _adopt_future(
            self._native.projection_create(), "MapProjectionHandle", None
        )

    def close(self) -> Future[None]:
        """Releases a map after synchronous state preflight.

        See `mln_map_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.close()

    def remove_feature_state(
        self, selector: FeatureStateSelector
    ) -> Future[CommandCompletion]:
        """Removes per-feature state from this map.

        See `mln_map_remove_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.remove_feature_state(selector)

    def remove_style_image(self, image_id: str) -> Future[CommandCompletion]:
        """Removes one runtime style image by ID.

        See `mln_map_remove_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.remove_style_image(image_id)

    def remove_style_layer(self, layer_id: str) -> Future[CommandCompletion]:
        """Removes one style layer by ID.

        See `mln_map_remove_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.remove_style_layer(layer_id)

    def remove_style_source(self, source_id: str) -> Future[CommandCompletion]:
        """Removes one style source by ID.

        See `mln_map_remove_style_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.remove_style_source(source_id)

    def request_repaint(self) -> Future[CommandCompletion]:
        """Requests a repaint for a continuous map.

        See `mln_map_request_repaint` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.request_repaint()

    def request_still_image(self) -> Future[None]:
        """Requests one still image for a static or tile map.

        See `mln_map_request_still_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.request_still_image()

    def resize(self, extent: LogicalExtent) -> Future[CommandCompletion]:
        """Submits the sole post-creation logical extent update.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.resize(extent)

    def set_bounds(
        self, options: BoundOptions | None = None
    ) -> Future[CommandCompletion]:
        """Submits a copied camera-constraint command.

        See `mln_map_set_bounds` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_bounds(options)

    def set_custom_geometry_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: bytes
    ) -> Future[CommandCompletion]:
        """Sets custom geometry source data for one canonical tile.

        See `mln_map_set_custom_geometry_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_custom_geometry_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_mvt_vector_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: bytes
    ) -> Future[CommandCompletion]:
        """Sets custom MVT vector source data for one canonical tile.

        See `mln_map_set_custom_mvt_vector_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_custom_mvt_vector_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_mvt_vector_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> Future[CommandCompletion]:
        """Reports a custom MVT vector source error for one canonical tile.

        See `mln_map_set_custom_mvt_vector_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_custom_mvt_vector_source_tile_error(
            source_id, tile_id, message
        )

    def set_debug_options(self, options: MapDebugOption) -> Future[CommandCompletion]:
        """Submits a debug-overlay command.

        See `mln_map_set_debug_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_debug_options(options)

    def set_event_mask(self, mask: RuntimeEventMask) -> Future[CommandCompletion]:
        """Selects which map-originated event types this map queues.

        See `mln_map_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_event_mask(mask)

    def set_feature_state(
        self, selector: FeatureStateSelector, input_state: bytes
    ) -> Future[CommandCompletion]:
        """Submits a copied per-feature-state command.

        See `mln_map_set_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_feature_state(selector, input_state)

    def set_free_camera_options(
        self, options: FreeCameraOptions | None = None
    ) -> Future[CommandCompletion]:
        """Submits a copied free-camera command.

        See `mln_map_set_free_camera_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_free_camera_options(options)

    def set_geojson_source_data(
        self, source_id: str, data: GeojsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Updates one GeoJSON source with prepared inline data.

        See `mln_map_set_geojson_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_geojson_source_data(source_id, data._native)

    def set_geojson_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> Future[CommandCompletion]:
        """Overrides one GeoJSON source's synchronous tiling at runtime.

        See `mln_map_set_geojson_source_synchronous_tiling` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_geojson_source_synchronous_tiling(source_id, enabled)

    def set_geojson_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Updates one GeoJSON source to load data from a URL.

        See `mln_map_set_geojson_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_geojson_source_url(source_id, url)

    def set_global_state_property(
        self, property_name: str, value: bytes
    ) -> Future[CommandCompletion]:
        """Submits a global-state JSON value. JSON null restores the style
        default. Input is copied before return.

        See `mln_map_set_global_state_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_global_state_property(property_name, value)

    def set_image_source_coordinates(
        self, source_id: str, coordinates: tuple[LatLng, ...]
    ) -> Future[CommandCompletion]:
        """Updates image source coordinates.

        See `mln_map_set_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_image_source_coordinates(source_id, coordinates)

    def set_image_source_image(
        self, source_id: str, image: PremultipliedRgba8Image | None = None
    ) -> Future[CommandCompletion]:
        """Updates an image source with inline image pixels.

        See `mln_map_set_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_image_source_image(source_id, image)

    def set_image_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Updates an image source to load its image from a URL.

        See `mln_map_set_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_image_source_url(source_id, url)

    def set_layer_filter(
        self, layer_id: str, filter: bytes | None = None
    ) -> Future[CommandCompletion]:
        """Sets or clears one layer filter.

        See `mln_map_set_layer_filter` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_layer_filter(layer_id, filter)

    def set_layer_max_zoom(
        self, layer_id: str, max_zoom: float
    ) -> Future[CommandCompletion]:
        """Sets the highest zoom at which one layer draws.

        See `mln_map_set_layer_max_zoom` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_layer_max_zoom(layer_id, max_zoom)

    def set_layer_min_zoom(
        self, layer_id: str, min_zoom: float
    ) -> Future[CommandCompletion]:
        """Sets the lowest zoom at which one layer draws.

        See `mln_map_set_layer_min_zoom` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_layer_min_zoom(layer_id, min_zoom)

    def set_layer_property(
        self, layer_id: str, property_name: str, value: bytes
    ) -> Future[CommandCompletion]:
        """Sets one layer property using its MapLibre style-spec property name.

        See `mln_map_set_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_layer_property(layer_id, property_name, value)

    def set_layer_source_id(
        self, layer_id: str, source_id: str
    ) -> Future[CommandCompletion]:
        """Sets one layer's source ID.

        See `mln_map_set_layer_source_id` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_layer_source_id(layer_id, source_id)

    def set_layer_source_layer(
        self, layer_id: str, source_layer: str | None = None
    ) -> Future[CommandCompletion]:
        """Sets one layer's source-layer ID.

        See `mln_map_set_layer_source_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_layer_source_layer(layer_id, source_layer)

    def set_layer_visibility(
        self, layer_id: str, visibility: StyleLayerVisibility
    ) -> Future[CommandCompletion]:
        """Sets whether one layer draws.

        See `mln_map_set_layer_visibility` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_layer_visibility(layer_id, visibility)

    def set_location_indicator_accuracy_radius(
        self, layer_id: str, radius: float
    ) -> Future[CommandCompletion]:
        """Sets a location indicator layer accuracy radius in meters.

        See `mln_map_set_location_indicator_accuracy_radius` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_location_indicator_accuracy_radius(layer_id, radius)

    def set_location_indicator_bearing(
        self, layer_id: str, bearing: float
    ) -> Future[CommandCompletion]:
        """Sets a location indicator layer bearing in degrees.

        See `mln_map_set_location_indicator_bearing` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_location_indicator_bearing(layer_id, bearing)

    def set_location_indicator_image_name(
        self, layer_id: str, image_kind: LocationIndicatorImageKind, image_id: str
    ) -> Future[CommandCompletion]:
        """Sets one location indicator image-name property.

        See `mln_map_set_location_indicator_image_name` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_location_indicator_image_name(
            layer_id, image_kind, image_id
        )

    def set_location_indicator_location(
        self, layer_id: str, coordinate: LatLng, altitude: float
    ) -> Future[CommandCompletion]:
        """Sets a location indicator layer location.

        See `mln_map_set_location_indicator_location` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_location_indicator_location(
            layer_id, coordinate, altitude
        )

    def set_projection_mode(
        self, mode: ProjectionMode | None = None
    ) -> Future[CommandCompletion]:
        """Submits copied axonometric rendering option fields.

        See `mln_map_set_projection_mode` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_projection_mode(mode)

    def set_rendering_stats_view_enabled(
        self, enabled: bool
    ) -> Future[CommandCompletion]:
        """Submits a rendering-stats visibility command.

        See `mln_map_set_rendering_stats_view_enabled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_rendering_stats_view_enabled(enabled)

    def set_style_image(
        self,
        image_id: str,
        image: PremultipliedRgba8Image | None = None,
        options: StyleImageOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Sets one runtime style image.

        See `mln_map_set_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_image(image_id, image, options)

    def set_style_json(self, json: bytes) -> Future[CommandCompletion]:
        """Queues an inline style JSON command.

        See `mln_map_set_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_style_json(json)

    def set_style_light_json(self, light_json: bytes) -> Future[CommandCompletion]:
        """Sets the style light from a style-spec light JSON object.

        See `mln_map_set_style_light_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_light_json(light_json)

    def set_style_light_property(
        self, property_name: str, value: bytes
    ) -> Future[CommandCompletion]:
        """Sets one style light property using its MapLibre style-spec property
        name.

        See `mln_map_set_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_light_property(property_name, value)

    def set_style_source_volatile(
        self, source_id: str, is_volatile: bool
    ) -> Future[CommandCompletion]:
        """Sets whether one style source stores fetched tiles in the persistent
        cache.

        See `mln_map_set_style_source_volatile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_source_volatile(source_id, is_volatile)

    def set_style_transition_options(
        self, options: StyleTransitionOptions | None = None
    ) -> Future[CommandCompletion]:
        """Sets the style's global transition options.

        See `mln_map_set_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_transition_options(options)

    def set_style_url(self, url: str) -> Future[CommandCompletion]:
        """Queues a style URL command.

        See `mln_map_set_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_style_url(url)

    def set_tile_options(
        self, options: MapTileOptions | None = None
    ) -> Future[CommandCompletion]:
        """Submits a copied tile-options command.

        See `mln_map_set_tile_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_tile_options(options)

    def set_viewport_options(
        self, options: MapViewportOptions | None = None
    ) -> Future[CommandCompletion]:
        """Submits a copied viewport-options command.

        See `mln_map_set_viewport_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_viewport_options(options)

    def snapshot_get(self) -> MapSnapshot:
        """Copies the latest immutable state published by the map worker.

        See `mln_map_snapshot_get` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return MapSnapshot._from_native(self._native.snapshot_get())

    def style_url(self) -> Future[str]:
        """Starts an ordered copy of the last requested style URL.

        See `mln_map_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.style_url()

    def update_camera(
        self, update: CameraUpdate | None = None
    ) -> Future[CommandCompletion]:
        """Submits one atomic camera update.

        See `mln_map_update_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.update_camera(update)

    def metal_borrowed_texture_attach(
        self,
        descriptor: MetalBorrowedTextureDescriptor | None = None,
        options: RenderSessionAttachOptions | None = None,
    ) -> MetalBorrowedTextureAttachResult:
        """Starts attachment of a ring of caller-owned Metal textures.

        See `mln_metal_borrowed_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of a session-owned Metal texture ring.

        See `mln_metal_owned_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of a Metal surface target.

        See `mln_metal_surface_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
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
        """Starts attachment of a ring of caller-owned OpenGL textures.

        See `mln_opengl_borrowed_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of a session-owned OpenGL texture ring.

        See `mln_opengl_owned_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of an OpenGL surface target.

        See `mln_opengl_surface_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
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
        """Starts attachment of a ring of caller-owned Vulkan images.

        See `mln_vulkan_borrowed_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of a session-owned Vulkan texture ring.

        See `mln_vulkan_owned_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of a Vulkan surface target.

        See `mln_vulkan_surface_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
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
        """Starts attachment of a ring of caller-owned WebGPU textures.

        See `mln_webgpu_borrowed_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of a session-owned WebGPU texture ring.

        See `mln_webgpu_owned_texture_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
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
        """Starts attachment of a WebGPU surface target.

        See `mln_webgpu_surface_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        raw = self._native.webgpu_surface_attach(descriptor, options)
        return WebgpuSurfaceAttachResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            completion=raw["completion"],
        )


class _MapProjectionHandleOperations(GeneratedOperations):
    _native: _native._MapProjectionHandle

    def close(self) -> None:
        """Closes a standalone projection.

        See `mln_map_projection_close` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.close()

    def get_camera(self) -> CameraOptions:
        """Copies the projection camera into out_camera.

        See `mln_map_projection_get_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return CameraOptions._from_native(self._native.get_camera())

    def lat_lng_for_pixel(self, point: ScreenPoint) -> LatLng:
        """Converts a screen point to a geographic coordinate.

        See `mln_map_projection_lat_lng_for_pixel` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return LatLng._from_native(self._native.lat_lng_for_pixel(point))

    def lat_lng_for_pixel_unwrapped(self, point: ScreenPoint) -> LatLng:
        """Converts a screen point to an unwrapped geographic coordinate.

        See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return LatLng._from_native(self._native.lat_lng_for_pixel_unwrapped(point))

    def meters_per_pixel_at_latitude(self, latitude: float) -> float:
        """Reads the ground distance covered by one logical map pixel at a
        latitude for the helper camera zoom.

        See `mln_map_projection_meters_per_pixel_at_latitude` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.meters_per_pixel_at_latitude(latitude)

    def pixel_for_lat_lng(self, coordinate: LatLng) -> ScreenPoint:
        """Converts a geographic coordinate to a screen point.

        See `mln_map_projection_pixel_for_lat_lng` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return ScreenPoint._from_native(self._native.pixel_for_lat_lng(coordinate))

    def set_camera(self, camera: CameraOptions | None = None) -> None:
        """Applies a camera update to a standalone projection.

        See `mln_map_projection_set_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.set_camera(camera)

    def set_visible_coordinates(
        self, coordinates: tuple[LatLng, ...], padding: EdgeInsets
    ) -> None:
        """Applies a camera fit for geographic coordinates.

        See `mln_map_projection_set_visible_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.set_visible_coordinates(coordinates, padding)

    def set_visible_geometry(self, geometry: bytes, padding: EdgeInsets) -> None:
        """Applies a camera fit for GeoJSON Geometry bytes.

        See `mln_map_projection_set_visible_geometry` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.set_visible_geometry(geometry, padding)


class _RenderFrameBatchHandleOperations(GeneratedOperations):
    _native: _native._RenderFrameBatchHandle

    def count(self) -> int:
        """Returns the number of records in an owned frame-result batch.

        See `mln_render_frame_batch_count` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.count()

    def get(self, index: int) -> RenderFrameResult:
        """Copies one frame-result record.

        See `mln_render_frame_batch_get` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderFrameResult._from_native(self._native.get(index))

    def close(self) -> None:
        """Releases a frame-result batch.

        See `mln_render_frame_batch_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.close()


class _RenderSessionHandleOperations(GeneratedOperations):
    _native: _native._RenderSessionHandle

    def metal_borrowed_texture_set_target(
        self, descriptor: MetalBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered replacement of every texture of a caller-owned
        Metal ring.

        See `mln_metal_borrowed_texture_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.metal_borrowed_texture_set_target(descriptor)

    def metal_surface_set_target(
        self, descriptor: MetalSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered Metal surface replacement.

        See `mln_metal_surface_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.metal_surface_set_target(descriptor)

    def opengl_borrowed_texture_set_target(
        self, descriptor: OpenglBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered replacement of every texture of a caller-owned
        OpenGL ring.

        See `mln_opengl_borrowed_texture_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.opengl_borrowed_texture_set_target(descriptor)

    def opengl_surface_set_target(
        self, descriptor: OpenglSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered OpenGL surface replacement.

        See `mln_opengl_surface_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.opengl_surface_set_target(descriptor)

    def abandon(self) -> RenderAbandonResult:
        """Irreversibly closes control and mailboxes and disposes of the
        session's graphics objects.

        See `mln_render_session_abandon` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderAbandonResult._from_native(self._native.abandon())

    def acquire_frame(self) -> AcquiredFrameHandle | None:
        """Acquires the oldest rendered frame that is not already acquired. The
        frame owns its slot until release. The call is nonblocking.

        See `mln_render_session_acquire_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return _maybe(
            lambda raw: _adopt_value(raw, "AcquiredFrameHandle", self),
            self._native.acquire_frame(),
        )

    def barrier(self) -> Future[None]:
        """Starts a barrier that completes after all render work accepted before
        it has a terminal result. A barrier does not request a frame.

        See `mln_render_session_barrier` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.barrier()

    def clear_data(self) -> Future[None]:
        """Starts asynchronous renderer-data clearing.

        See `mln_render_session_clear_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.clear_data()

    def close(self) -> None:
        """Retires a detached or abandoned session handle. The call is CPU-only
        and may run on any native thread, including from one of the session's
        own completions. If an abandonment is still in progress on another
        thread, this waits for it to finish before consuming the session
        owner.

        See `mln_render_session_destroy` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.close()

    def detach(self) -> Future[None]:
        """Starts normal graphics-owner teardown and map detachment.

        See `mln_render_session_detach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.detach()

    def drain_frame_results(self) -> RenderFrameBatchHandle | None:
        """Drains every currently queued terminal frame result into an
        independently owned batch. The records remain stable until the batch
        is released.

        See `mln_render_session_drain_frame_results` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return _maybe(
            lambda raw: _adopt_value(raw, "RenderFrameBatchHandle", None),
            self._native.drain_frame_results(),
        )

    def dump_debug_logs(self) -> Future[None]:
        """Starts asynchronous renderer diagnostic-log emission.

        See `mln_render_session_dump_debug_logs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.dump_debug_logs()

    def get_capabilities(self) -> RenderSessionCapabilities:
        """Returns the immutable capabilities fixed during attachment.

        See `mln_render_session_get_capabilities` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderSessionCapabilities._from_native(self._native.get_capabilities())

    def get_snapshot(self) -> RenderSessionSnapshot:
        """Copies the latest render-session snapshot from any native thread.

        See `mln_render_session_get_snapshot` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderSessionSnapshot._from_native(self._native.get_snapshot())

    def projection_create(self) -> MapProjectionHandle:
        """Copies the last completed rendered transform into an independent
        projection. Callable from any thread. Returns invalid state before a
        completed render, after an extent or target change, or after
        detachment. The caller owns the returned projection, which remains
        usable after the session is released. out_projection must point to a
        null handle.

        See `mln_render_session_projection_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
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
        """Starts a feature-extension query against the latest driver state. The
        completion borrows one `mln_buffer_view` holding UTF-8 JSON
        (value_count 1), valid only for the callback.

        See `mln_render_session_query_feature_extensions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
        """
        return self._native.query_feature_extensions(
            source_id, feature, extension, extension_field, arguments
        )

    def query_rendered_features(
        self,
        geometry: RenderedQueryGeometry,
        options: RenderedFeatureQueryOptions | None = None,
    ) -> Future[tuple[QueriedFeature, ...]]:
        """Starts a rendered-feature query against the session's latest driver
        state.

        See `mln_render_session_query_rendered_features` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
        """
        return map_future(
            self._native.query_rendered_features(geometry, options),
            lambda value: tuple(QueriedFeature._from_native(item) for item in value),
        )

    def query_source_features(
        self, source_id: str, options: SourceFeatureQueryOptions | None = None
    ) -> Future[tuple[QueriedFeature, ...]]:
        """Starts a source-feature query against the session's latest driver
        state. The completion borrows an array of `mln_queried_feature`
        values (value_count entries), valid only for the callback.

        See `mln_render_session_query_source_features` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
        """
        return map_future(
            self._native.query_source_features(source_id, options),
            lambda value: tuple(QueriedFeature._from_native(item) for item in value),
        )

    def reduce_memory_use(self) -> Future[None]:
        """Starts best-effort release of renderer caches.

        See `mln_render_session_reduce_memory_use` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.reduce_memory_use()

    def request_frame(self, demand: FrameDemand | None = None) -> None:
        """Requests a frame without waiting. Every accepted demand produces one
        terminal result record. A core worker wakes itself; a caller driver
        publishes its driver-work endpoint.

        See `mln_render_session_request_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.request_frame(demand)

    def resize(self, extent: RenderTargetExtent) -> Future[CommandCompletion]:
        """Starts an ordered logical resize. The completion runs after the
        selected driver applies the extent and updates the map viewport.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.resize(extent)

    def service_driver_work(self, max_work: int) -> int:
        """Services up to max_work items for a caller-graphics-thread driver;
        zero services every item currently queued. The first successful
        service call fixes the session's graphics-thread identity; later
        calls from another native thread return `MLN_STATUS_WRONG_THREAD`.
        The target context must be current. Core-worker sessions return
        `MLN_STATUS_INVALID_STATE`.

        See `mln_render_session_service_driver_work` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.service_driver_work(max_work)

    def texture_read_premultiplied_rgba8(self) -> Future[TextureReadbackResult]:
        """Starts readback of the latest rendered texture frame.

        See `mln_texture_read_premultiplied_rgba8` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return map_future(
            self._native.texture_read_premultiplied_rgba8(),
            lambda value: TextureReadbackResult._from_native(value),
        )

    def vulkan_borrowed_texture_set_target(
        self, descriptor: VulkanBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered replacement of every image of a caller-owned Vulkan
        ring.

        See `mln_vulkan_borrowed_texture_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.vulkan_borrowed_texture_set_target(descriptor)

    def vulkan_surface_set_target(
        self, descriptor: VulkanSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered Vulkan surface replacement.

        See `mln_vulkan_surface_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.vulkan_surface_set_target(descriptor)

    def webgpu_borrowed_texture_set_target(
        self, descriptor: WebgpuBorrowedTextureDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered replacement of every texture of a caller-owned
        WebGPU ring.

        See `mln_webgpu_borrowed_texture_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.webgpu_borrowed_texture_set_target(descriptor)

    def webgpu_surface_set_target(
        self, descriptor: WebgpuSurfaceDescriptor | None = None
    ) -> Future[None]:
        """Starts an ordered WebGPU surface replacement.

        See `mln_webgpu_surface_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.webgpu_surface_set_target(descriptor)


class _ResourceRequestHandleOperations(GeneratedOperations):
    _native: _native._ResourceRequestHandle

    def cancelled(self) -> bool:
        """Reports whether MapLibre has cancelled a C API resource provider
        request.

        See `mln_resource_request_cancelled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.cancelled()

    def complete(self, response: ResourceResponse) -> None:
        """Completes a C API resource provider request.

        See `mln_resource_request_complete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.complete(response)

    def close(self) -> None:
        return self._native.close()

    def set_cancel_callback(self, callback: Callable[[], None]) -> bool:
        """Registers a callback that runs when MapLibre cancels a C API resource
        provider request.

        See `mln_resource_request_set_cancel_callback` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_cancel_callback(callback)

    def wait_until_retired(self) -> None:
        """Blocks until a resource request is released and its cancel callback
        registration has retired: the callback, if it ran, and
        release_user_data have both returned. Completing a request does not
        release its owner.

        See `mln_resource_request_wait_until_retired` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.wait_until_retired()


class _ResourceTransformResponseScopeOperations(GeneratedOperations):
    _native: _native._ResourceTransformResponseScope

    def set_url(self, url: str) -> None:
        """Copies a replacement URL into C API-managed storage for the current
        callback.

        See `mln_resource_transform_response_set_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_url(url)


class _RuntimeHandleOperations(GeneratedOperations):
    _native: _native._RuntimeHandle

    def map_create(self, options: MapOptions | None = None) -> Future[MapHandle]:
        """Creates a map on the runtime worker.

        See `mln_map_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return _adopt_future(self._native.map_create(options), "MapHandle", self)

    def barrier(self) -> Future[None]:
        """Starts an ordered runtime barrier.

        See `mln_runtime_barrier` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.barrier()

    def clear_http_header_transform(self) -> Future[None]:
        """Clears the runtime-scoped outgoing HTTP header transform.

        See `mln_runtime_clear_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_http_header_transform()

    def clear_resource_provider(self) -> Future[None]:
        """Clears the runtime-scoped network resource provider.

        See `mln_runtime_clear_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_resource_provider()

    def clear_resource_transform(self) -> Future[None]:
        """Clears the runtime-scoped URL transform for network resources.

        See `mln_runtime_clear_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_resource_transform()

    def drain_events(self) -> EventBatchHandle:
        """Drains this runtime's queued events into a new owned batch.

        See `mln_runtime_drain_events` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return _adopt_value(self._native.drain_events(), "EventBatchHandle", None)

    def get_event_mask(self) -> RuntimeEventMask:
        """Reports which runtime-scoped event types this runtime queues.

        See `mln_runtime_get_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return RuntimeEventMask(self._native.get_event_mask())

    def offline_region_create(
        self, definition: OfflineRegionDefinition, metadata: bytes
    ) -> Future[OfflineRegionInfo]:
        """Starts creating an offline region.

        See `mln_runtime_offline_region_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.offline_region_create(definition, metadata),
            lambda value: OfflineRegionInfo._from_native(value),
        )

    def offline_region_delete(self, region_id: int) -> Future[None]:
        """Deletes an offline region.

        See `mln_runtime_offline_region_delete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.offline_region_delete(region_id)

    def offline_region_get(self, region_id: int) -> Future[OfflineRegionInfo | None]:
        """Starts getting one offline region by ID.

        See `mln_runtime_offline_region_get` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.offline_region_get(region_id),
            lambda value: _maybe(OfflineRegionInfo._from_native, value),
        )

    def offline_region_get_status(self, region_id: int) -> Future[OfflineRegionStatus]:
        """Starts getting the current download status for an offline region.

        See `mln_runtime_offline_region_get_status` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.offline_region_get_status(region_id),
            lambda value: OfflineRegionStatus._from_native(value),
        )

    def offline_region_invalidate(self, region_id: int) -> Future[None]:
        """Invalidates cached resources for an offline region.

        See `mln_runtime_offline_region_invalidate` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.offline_region_invalidate(region_id)

    def offline_region_set_download_state(
        self, region_id: int, input_state: OfflineRegionDownloadState
    ) -> Future[None]:
        """Sets an offline region's native download state.

        See `mln_runtime_offline_region_set_download_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.offline_region_set_download_state(region_id, input_state)

    def offline_region_set_observed(
        self, region_id: int, observed: bool
    ) -> Future[None]:
        """Enables or disables runtime events for an offline region.

        See `mln_runtime_offline_region_set_observed` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.offline_region_set_observed(region_id, observed)

    def offline_region_update_metadata(
        self, region_id: int, metadata: bytes
    ) -> Future[OfflineRegionInfo]:
        """Starts updating opaque binary metadata for an offline region.

        See `mln_runtime_offline_region_update_metadata` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.offline_region_update_metadata(region_id, metadata),
            lambda value: OfflineRegionInfo._from_native(value),
        )

    def offline_regions_list(self) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Starts listing the offline regions in the runtime database.

        See `mln_runtime_offline_regions_list` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.offline_regions_list(),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def offline_regions_merge_database(
        self, side_database_path: str
    ) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Starts merging offline regions from another MapLibre offline
        database.

        See `mln_runtime_offline_regions_merge_database` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.offline_regions_merge_database(side_database_path),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def close(self) -> Future[None]:
        """Releases a runtime after synchronous child preflight.

        See `mln_runtime_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.close()

    def run_ambient_cache_operation(
        self, operation: AmbientCacheOperation
    ) -> Future[None]:
        """Starts a MapLibre ambient cache maintenance operation for this
        runtime.

        See `mln_runtime_run_ambient_cache_operation` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.run_ambient_cache_operation(operation)

    def set_event_mask(self, mask: RuntimeEventMask) -> None:
        """Selects which runtime-scoped event types this runtime queues.

        See `mln_runtime_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_event_mask(mask)

    def set_http_header_transform(self, transform: HttpHeaderTransform) -> Future[None]:
        """Registers or replaces the runtime-scoped outgoing HTTP header
        transform.

        See `mln_runtime_set_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_http_header_transform(transform)

    def set_maximum_ambient_cache_size(self, size: int) -> Future[None]:
        """Starts a change to this runtime's maximum ambient cache size.

        See `mln_runtime_set_maximum_ambient_cache_size` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_maximum_ambient_cache_size(size)

    def set_resource_provider(self, provider: ResourceProvider) -> Future[None]:
        """Registers or replaces a runtime-scoped network resource provider.

        See `mln_runtime_set_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_resource_provider(provider)

    def set_resource_transform(self, transform: ResourceTransform) -> Future[None]:
        """Registers or updates a runtime-scoped URL transform for network
        resources.

        See `mln_runtime_set_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_resource_transform(transform)


def android_init(jni_env: int, jni_class: int, context: int) -> None:
    """Initializes Android platform services.

    See `mln_android_init` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
    """
    return _native.android_init(jni_env, jni_class, context)


def c_version() -> int:
    """Reports the C ABI contract version. The value is 0 while the ABI is
    unstable, and will increment on each SemVer major release.

    See `mln_c_version` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """
    return _native.c_version()


def geojson_source_data_create(
    data: bytes, options: GeojsonSourceOptions | None = None
) -> GeojsonSourceDataHandle:
    """Prepares GeoJSON source data for installation on a map.

    See `mln_geojson_source_data_create` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """
    return _adopt_value(
        _native.geojson_source_data_create(data, options),
        "GeojsonSourceDataHandle",
        None,
    )


def lat_lng_for_projected_meters(meters: ProjectedMeters) -> LatLng:
    """Converts spherical Mercator projected meters to a geographic
    coordinate.

    See `mln_lat_lng_for_projected_meters` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    """
    return LatLng._from_native(_native.lat_lng_for_projected_meters(meters))


def log_clear_callback() -> None:
    """Clears the process-global log callback.

    See `mln_log_clear_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.log_clear_callback()


def log_set_async_severity_mask(mask: LogSeverityMask) -> None:
    """Controls which log severities MapLibre Native may dispatch
    asynchronously.

    See `mln_log_set_async_severity_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.log_set_async_severity_mask(mask)


def log_set_callback(
    callback: Callable[[LogSeverity, LogEvent, int, str], int] | None = None,
) -> None:
    """Installs a process-global MapLibre Native log callback.

    See `mln_log_set_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.log_set_callback(LogSetCallbackRegistration(callback))


def network_status_get() -> NetworkStatus:
    """Reads MapLibre Native's process-global network status.

    See `mln_network_status_get` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return NetworkStatus(_native.network_status_get())


def network_status_set(input_status: NetworkStatus) -> None:
    """Sets MapLibre Native's process-global network status.

    See `mln_network_status_set` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return _native.network_status_set(input_status)


def opengl_supported_context_provider_mask() -> OpenglContextProviderFlag:
    """Returns OpenGL context providers supported by this build.

    See `mln_opengl_supported_context_provider_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """
    return OpenglContextProviderFlag(_native.opengl_supported_context_provider_mask())


def plugin_get_register_function_v1() -> int:
    """Returns the process-wide `mln_plugin_register_v1` entry point; never
    null.

    See `mln_plugin_get_register_function_v1` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
    """
    return _native.plugin_get_register_function_v1()


def projected_meters_for_lat_lng(coordinate: LatLng) -> ProjectedMeters:
    """Converts a geographic coordinate to spherical Mercator projected
    meters.

    See `mln_projected_meters_for_lat_lng` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    """
    return ProjectedMeters._from_native(
        _native.projected_meters_for_lat_lng(coordinate)
    )


def render_target_extent_physical_size(
    extent: RenderTargetExtent,
) -> RenderTargetExtentPhysicalSizeResult:
    """Computes the physical device-pixel size of a logical render target
    extent.

    See `mln_render_target_extent_physical_size` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """
    raw = _native.render_target_extent_physical_size(extent)
    return RenderTargetExtentPhysicalSizeResult(raw["width"], raw["height"])


def rendered_query_geometry_box(input_box: ScreenBox) -> RenderedQueryGeometry:
    """Returns a rendered box query geometry descriptor.

    See `mln_rendered_query_geometry_box` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """
    return RenderedQueryGeometry._from_native(
        _native.rendered_query_geometry_box(input_box)
    )


def rendered_query_geometry_line_string(
    points: tuple[ScreenPoint, ...],
) -> RenderedQueryGeometry:
    """Returns a rendered line-string query geometry descriptor.

    See `mln_rendered_query_geometry_line_string` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """
    return RenderedQueryGeometry._from_native(
        _native.rendered_query_geometry_line_string(points)
    )


def rendered_query_geometry_point(point: ScreenPoint) -> RenderedQueryGeometry:
    """Returns a rendered point query geometry descriptor.

    See `mln_rendered_query_geometry_point` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """
    return RenderedQueryGeometry._from_native(
        _native.rendered_query_geometry_point(point)
    )


def runtime_create(options: RuntimeOptions | None = None) -> RuntimeHandle:
    """Creates a runtime with a new core-owned worker.

    See `mln_runtime_create` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return _adopt_value(_native.runtime_create(options), "RuntimeHandle", None)


def supported_render_backend_mask() -> RenderBackendFlag:
    """Reports the render backends available in this native library build.

    See `mln_supported_render_backend_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """
    return RenderBackendFlag(_native.supported_render_backend_mask())
