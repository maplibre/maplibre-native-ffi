"""Generated from the C headers by tools/bindgen. Do not edit."""

from __future__ import annotations

from collections.abc import Buffer, Sequence
from concurrent.futures import Future
from contextlib import AbstractContextManager
from typing import TYPE_CHECKING, Any, NamedTuple

from . import _native
from ._completion import CommandCompletion
from ._future import map_future
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
    FeatureStateSelector,
    FrameDemand,
    FreeCameraOptions,
    GeojsonSourceOptions,
    GpuSync,
    HttpHeaderTransform,
    LatLng,
    LatLngBounds,
    LogHandler,
    LogicalExtent,
    LogSeverityFlag,
    MapDebugFlag,
    MapOptions,
    MapSnapshot,
    MapTileOptions,
    MapView,
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
    RenderAbandonResult,
    RenderBackendFlag,
    RenderedFeature,
    RenderedFeatureQueryOptions,
    RenderedQueryGeometry,
    RenderFrameResult,
    RenderSessionAttachOptions,
    RenderSessionCapabilities,
    RenderSessionSnapshot,
    ResourceProvider,
    ResourceRequestCancelHandler,
    ResourceResponse,
    ResourceTransform,
    RuntimeEventFlag,
    RuntimeEventHandler,
    RuntimeOptions,
    ScreenPoint,
    SourceFeature,
    SourceFeatureQueryOptions,
    StyleExpressionInfo,
    StyleExpressionInputs,
    StyleExpressionOptions,
    StyleImageInfo,
    StyleImageOptions,
    StyleLayerInfo,
    StyleSourceInfo,
    StyleTransitionOptions,
    TextureReadbackResult,
    VulkanBorrowedTextureDescriptor,
    VulkanOwnedTextureDescriptor,
    VulkanSurfaceDescriptor,
    VulkanTextureFrame,
    _maybe,
)
from ._operation import GeneratedOperations, _adopt_value, _view

if TYPE_CHECKING:
    from ._generated_owners import (
        AcquiredFrameHandle,
        GeojsonSourceDataHandle,
        MapHandle,
        MapProjectionHandle,
        RenderSessionHandle,
        RuntimeHandle,
        StyleExpressionHandle,
    )


class LogicalExtentPhysicalSizeResult(NamedTuple):
    width: int
    height: int


class MapAttachMetalBorrowedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachMetalOwnedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachMetalSurfaceResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachOpenglBorrowedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachOpenglOwnedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachOpenglSurfaceResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachVulkanBorrowedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachVulkanOwnedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachVulkanSurfaceResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class RuntimeCreateMapResult(NamedTuple):
    map: MapHandle
    ready: Future[None]


class _AcquiredFrameHandleOperations(GeneratedOperations):
    _native: _native._AcquiredFrameHandle

    def discard(self) -> None:
        """Consumes an acquired frame without consumer GPU synchronization, so
        it quarantines the frame's slot of the texture ring.

        See `mln_acquired_frame_discard` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.discard()

    def metal_texture(self) -> AbstractContextManager[MetalTextureFrame]:
        """Copies Metal-native metadata from an acquired frame.

        See `mln_acquired_frame_get_metal_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Native resources remain valid only inside the `with` block.
        """
        return _view(
            self,
            lambda: self._native.metal_texture(),
            lambda raw: MetalTextureFrame._from_native(raw),
        )

    def opengl_texture(self) -> AbstractContextManager[OpenglTextureFrame]:
        """Copies OpenGL-native metadata from an acquired frame.

        See `mln_acquired_frame_get_opengl_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Native resources remain valid only inside the `with` block.
        """
        return _view(
            self,
            lambda: self._native.opengl_texture(),
            lambda raw: OpenglTextureFrame._from_native(raw),
        )

    def producer_sync(self) -> AbstractContextManager[GpuSync]:
        """Copies the producer synchronization for an acquired texture frame.

        See `mln_acquired_frame_get_producer_sync` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Native resources remain valid only inside the `with` block.
        """
        return _view(
            self,
            lambda: self._native.producer_sync(),
            lambda raw: GpuSync._from_native(raw),
        )

    def get_result(self) -> RenderFrameResult:
        """Copies common metadata for an acquired frame.

        See `mln_acquired_frame_get_result` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderFrameResult._from_native(self._native.get_result())

    def vulkan_texture(self) -> AbstractContextManager[VulkanTextureFrame]:
        """Copies Vulkan-native metadata from an acquired frame.

        See `mln_acquired_frame_get_vulkan_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Native resources remain valid only inside the `with` block.
        """
        return _view(
            self,
            lambda: self._native.vulkan_texture(),
            lambda raw: VulkanTextureFrame._from_native(raw),
        )

    def close(self, consumer_completion: GpuSync | None = None) -> None:
        """Releases an acquired frame after the host's GPU work that reads it.

        See `mln_acquired_frame_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.close(consumer_completion)


class _GeojsonSourceDataHandleOperations(GeneratedOperations):
    _native: _native._GeojsonSourceDataHandle

    def close(self) -> None:
        """Releases prepared GeoJSON source data.

        See `mln_geojson_source_data_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.close()


class _HttpHeaderTransformResponseScopeOperations(GeneratedOperations):
    _native: _native._HttpHeaderTransformResponseScope

    def set_header(self, name: str, value: str) -> None:
        """Sets one outgoing HTTP request header for the current transform
        invocation.

        See `mln_http_header_transform_response_set_header` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_header(name, value)


class _MapHandleOperations(GeneratedOperations):
    _native: _native._MapHandle

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

    def add_image_source_image(
        self,
        source_id: str,
        coordinates: Sequence[LatLng],
        image: PremultipliedRgba8Image,
    ) -> Future[CommandCompletion]:
        """Adds an image source with inline image pixels.

        See `mln_map_add_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_image_source_image(source_id, coordinates, image)

    def add_image_source_url(
        self, source_id: str, coordinates: Sequence[LatLng], url: str
    ) -> Future[CommandCompletion]:
        """Adds an image source that loads its image from a URL.

        See `mln_map_add_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_image_source_url(source_id, coordinates, url)

    def add_style_layer_json(
        self, layer_json: Any, before_layer_id: str
    ) -> Future[CommandCompletion]:
        """Adds one style layer from a full style-spec layer JSON object.

        See `mln_map_add_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_style_layer_json(layer_json, before_layer_id)

    def add_style_source_json(
        self, source_id: str, source_json: Any
    ) -> Future[CommandCompletion]:
        """Adds one style source from a style-spec source JSON object.

        See `mln_map_add_style_source_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.add_style_source_json(source_id, source_json)

    def apply_camera_delta(self, delta: CameraDelta) -> Future[CommandCompletion]:
        """Submits one atomic relative camera update.

        See `mln_map_apply_camera_delta` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.apply_camera_delta(delta)

    def attach_metal_borrowed_texture(
        self,
        descriptor: MetalBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachMetalBorrowedTextureResult:
        """Starts attachment of a ring of caller-owned Metal textures.

        See `mln_map_attach_metal_borrowed_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        raw = self._native.attach_metal_borrowed_texture(descriptor, options)
        return MapAttachMetalBorrowedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_metal_owned_texture(
        self,
        descriptor: MetalOwnedTextureDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachMetalOwnedTextureResult:
        """Starts attachment of a session-owned Metal texture ring.

        See `mln_map_attach_metal_owned_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        raw = self._native.attach_metal_owned_texture(descriptor, options)
        return MapAttachMetalOwnedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_metal_surface(
        self,
        descriptor: MetalSurfaceDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachMetalSurfaceResult:
        """Starts attachment of a Metal surface target.

        See `mln_map_attach_metal_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        raw = self._native.attach_metal_surface(descriptor, options)
        return MapAttachMetalSurfaceResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_opengl_borrowed_texture(
        self,
        descriptor: OpenglBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachOpenglBorrowedTextureResult:
        """Starts attachment of a ring of caller-owned OpenGL textures.

        See `mln_map_attach_opengl_borrowed_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        raw = self._native.attach_opengl_borrowed_texture(descriptor, options)
        return MapAttachOpenglBorrowedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_opengl_owned_texture(
        self,
        descriptor: OpenglOwnedTextureDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachOpenglOwnedTextureResult:
        """Starts attachment of a session-owned OpenGL texture ring.

        See `mln_map_attach_opengl_owned_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        raw = self._native.attach_opengl_owned_texture(descriptor, options)
        return MapAttachOpenglOwnedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_opengl_surface(
        self,
        descriptor: OpenglSurfaceDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachOpenglSurfaceResult:
        """Starts attachment of an OpenGL surface target.

        See `mln_map_attach_opengl_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        raw = self._native.attach_opengl_surface(descriptor, options)
        return MapAttachOpenglSurfaceResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_vulkan_borrowed_texture(
        self,
        descriptor: VulkanBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachVulkanBorrowedTextureResult:
        """Starts attachment of a ring of caller-owned Vulkan images.

        See `mln_map_attach_vulkan_borrowed_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        raw = self._native.attach_vulkan_borrowed_texture(descriptor, options)
        return MapAttachVulkanBorrowedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_vulkan_owned_texture(
        self,
        descriptor: VulkanOwnedTextureDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachVulkanOwnedTextureResult:
        """Starts attachment of a session-owned Vulkan texture ring.

        See `mln_map_attach_vulkan_owned_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        raw = self._native.attach_vulkan_owned_texture(descriptor, options)
        return MapAttachVulkanOwnedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_vulkan_surface(
        self,
        descriptor: VulkanSurfaceDescriptor,
        options: RenderSessionAttachOptions | None = None,
    ) -> MapAttachVulkanSurfaceResult:
        """Starts attachment of a Vulkan surface target.

        See `mln_map_attach_vulkan_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        raw = self._native.attach_vulkan_surface(descriptor, options)
        return MapAttachVulkanSurfaceResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def begin_command_group(self) -> Future[CommandCompletion]:
        """Begins a command group, which holds this map's render updates until
        the group ends.

        See `mln_map_begin_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.begin_command_group()

    def camera_for_geometry(
        self, geometry: Any, fit_options: CameraFitOptions | None = None
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
        self, coordinates: Sequence[LatLng], fit_options: CameraFitOptions | None = None
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

    def cancel_camera_transition(self, transition_id: int) -> Future[CommandCompletion]:
        """Cancels the camera transitions of the commands whose animation
        carried transition_id, and leaves every other transition running.

        See `mln_map_cancel_camera_transition` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.cancel_camera_transition(transition_id)

    def cancel_transitions(self) -> Future[CommandCompletion]:
        """Cancels the camera transitions running when this command commits.

        See `mln_map_cancel_transitions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.cancel_transitions()

    def dump_debug_logs(self) -> Future[CommandCompletion]:
        """Submits an ordered debug-log command.

        See `mln_map_dump_debug_logs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.dump_debug_logs()

    def end_command_group(self) -> Future[CommandCompletion]:
        """Ends the innermost command group that `mln_map_begin_command_group()`
        began.

        See `mln_map_end_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.end_command_group()

    def get_camera(self) -> Future[CameraQueryResult]:
        """Starts an ordered camera read.

        See `mln_map_get_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.get_camera(),
            lambda value: CameraQueryResult._from_native(value),
        )

    def get_feature_state(self, selector: FeatureStateSelector) -> Future[Any]:
        """Starts an ordered read of per-feature state from this map.

        See `mln_map_get_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.get_feature_state(selector)

    def get_global_state(self) -> Future[Any]:
        """Queries the global-state JSON object, including style defaults.
        Completion borrows one `mln_buffer_view` for the duration of the
        callback.

        See `mln_map_get_global_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_global_state()

    def get_image_source_coordinates(
        self, source_id: str
    ) -> Future[tuple[LatLng, ...]]:
        """Copies image source coordinates.

        See `mln_map_get_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_image_source_coordinates(source_id),
            lambda value: tuple(LatLng._from_native(item) for item in value),
        )

    def get_loaded_style_json(self) -> Future[Any]:
        """Starts an ordered copy of the last successfully parsed style
        document.

        See `mln_map_get_loaded_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.get_loaded_style_json()

    def get_snapshot(self) -> MapSnapshot:
        """Copies the map's latest published snapshot.

        See `mln_map_get_snapshot` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return MapSnapshot._from_native(self._native.get_snapshot())

    def get_style_image(self, image_id: str) -> Future[StyleImageInfo | None]:
        """Copies one complete runtime style image.

        See `mln_map_get_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_image(image_id),
            lambda value: _maybe(StyleImageInfo._from_native, value),
        )

    def get_style_layer(self, layer_id: str) -> Future[StyleLayerInfo | None]:
        """Copies the complete metadata of one style layer.

        See `mln_map_get_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_layer(layer_id),
            lambda value: _maybe(StyleLayerInfo._from_native, value),
        )

    def get_style_layer_json(self, layer_id: str) -> Future[Any]:
        """Serializes one style layer as a full style-spec layer JSON object.

        See `mln_map_get_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_layer_json(layer_id)

    def get_style_layer_property(
        self, layer_id: str, property_name: str
    ) -> Future[Any]:
        """Serializes one layer property as a style-spec JSON value.

        See `mln_map_get_style_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_layer_property(layer_id, property_name)

    def get_style_light_property(self, property_name: str) -> Future[Any]:
        """Serializes one style light property as a style-spec JSON value.

        See `mln_map_get_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_light_property(property_name)

    def get_style_source(self, source_id: str) -> Future[StyleSourceInfo | None]:
        """Copies the complete metadata of one style source.

        See `mln_map_get_style_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_source(source_id),
            lambda value: _maybe(StyleSourceInfo._from_native, value),
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

    def get_style_url(self) -> Future[str]:
        """Starts an ordered copy of the last requested style URL.

        See `mln_map_get_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.get_style_url()

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

    def list_style_layers(self) -> Future[tuple[StyleLayerInfo, ...]]:
        """Starts an ordered query of every style layer in style order.

        See `mln_map_list_style_layers` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.list_style_layers(),
            lambda value: tuple(StyleLayerInfo._from_native(item) for item in value),
        )

    def list_style_sources(self) -> Future[tuple[StyleSourceInfo, ...]]:
        """Lists every style source in style order.

        See `mln_map_list_style_sources` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.list_style_sources(),
            lambda value: tuple(StyleSourceInfo._from_native(item) for item in value),
        )

    def move_style_layer(
        self, layer_id: str, before_layer_id: str
    ) -> Future[CommandCompletion]:
        """Moves one style layer before another layer or to the top.

        See `mln_map_move_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.move_style_layer(layer_id, before_layer_id)

    def close(self) -> Future[None]:
        """Releases a map asynchronously.

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
        """Requests one still image for a static or tile map, which renders
        itself.

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
        self, source_id: str, tile_id: CanonicalTileId, data: Any
    ) -> Future[CommandCompletion]:
        """Sets custom geometry source data for one canonical tile.

        See `mln_map_set_custom_geometry_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_custom_geometry_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_geometry_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> Future[CommandCompletion]:
        """Reports a custom geometry source error for one canonical tile.

        See `mln_map_set_custom_geometry_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_custom_geometry_source_tile_error(
            source_id, tile_id, message
        )

    def set_custom_mvt_vector_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: Buffer
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

    def set_debug_options(self, options: MapDebugFlag) -> Future[CommandCompletion]:
        """Submits a debug-overlay command.

        See `mln_map_set_debug_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_debug_options(options)

    def set_event_mask(self, mask: RuntimeEventFlag) -> Future[CommandCompletion]:
        """Selects which map-originated event types this map queues.

        See `mln_map_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_event_mask(mask)

    def set_feature_state(
        self, selector: FeatureStateSelector, state: Any
    ) -> Future[CommandCompletion]:
        """Submits a copied per-feature-state command.

        See `mln_map_set_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_feature_state(selector, state)

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
        self, property_name: str, value: Any
    ) -> Future[CommandCompletion]:
        """Submits a global-state JSON value. JSON null restores the style
        default.

        See `mln_map_set_global_state_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_global_state_property(property_name, value)

    def set_image_source_coordinates(
        self, source_id: str, coordinates: Sequence[LatLng]
    ) -> Future[CommandCompletion]:
        """Updates image source coordinates.

        See `mln_map_set_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_image_source_coordinates(source_id, coordinates)

    def set_image_source_image(
        self, source_id: str, image: PremultipliedRgba8Image
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

    def set_projection_mode(self, mode: ProjectionMode) -> Future[CommandCompletion]:
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
        image: PremultipliedRgba8Image,
        options: StyleImageOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Sets one runtime style image.

        See `mln_map_set_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_image(image_id, image, options)

    def set_style_json(self, json: Any) -> Future[CommandCompletion]:
        """Queues an inline style JSON command.

        See `mln_map_set_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_style_json(json)

    def set_style_layer_property(
        self, layer_id: str, property_name: str, value: Any
    ) -> Future[CommandCompletion]:
        """Sets one layer property using its MapLibre style-spec property name.

        See `mln_map_set_style_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_layer_property(layer_id, property_name, value)

    def set_style_light_json(self, light_json: Any) -> Future[CommandCompletion]:
        """Sets the style light from a style-spec light JSON object.

        See `mln_map_set_style_light_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_light_json(light_json)

    def set_style_light_property(
        self, property_name: str, value: Any
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

    def update_camera(self, update: CameraUpdate) -> Future[CommandCompletion]:
        """Submits one atomic camera update.

        See `mln_map_update_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.update_camera(update)

    @property
    def discarding(self) -> MapHandleDiscarding:
        return MapHandleDiscarding(self)


class _MapProjectionHandleOperations(GeneratedOperations):
    _native: _native._MapProjectionHandle

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

    def lat_lngs_for_pixels(self, points: Sequence[ScreenPoint]) -> tuple[LatLng, ...]:
        """Converts screen points to geographic coordinates.

        See `mln_map_projection_lat_lngs_for_pixels` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return tuple(
            LatLng._from_native(item)
            for item in self._native.lat_lngs_for_pixels(points)
        )

    def lat_lngs_for_pixels_unwrapped(
        self, points: Sequence[ScreenPoint]
    ) -> tuple[LatLng, ...]:
        """Converts screen points to unwrapped geographic coordinates.

        See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return tuple(
            LatLng._from_native(item)
            for item in self._native.lat_lngs_for_pixels_unwrapped(points)
        )

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

    def pixels_for_lat_lngs(
        self, coordinates: Sequence[LatLng]
    ) -> tuple[ScreenPoint, ...]:
        """Converts geographic coordinates to screen points.

        See `mln_map_projection_pixels_for_lat_lngs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return tuple(
            ScreenPoint._from_native(item)
            for item in self._native.pixels_for_lat_lngs(coordinates)
        )

    def close(self) -> None:
        """Releases a standalone projection.

        See `mln_map_projection_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.close()

    def set_camera(self, camera: CameraOptions | None = None) -> None:
        """Applies a camera update to a standalone projection.

        See `mln_map_projection_set_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.set_camera(camera)


class _RenderSessionHandleOperations(GeneratedOperations):
    _native: _native._RenderSessionHandle

    def abandon(self) -> RenderAbandonResult:
        """Irreversibly ends session work and destroys the session's graphics
        objects where that is safe.

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
        """Ends every frame demand that has not started with
        `MLN_RENDER_RESULT_NO_UPDATE` and completes after earlier driver work
        has finished. A running demand finishes its attempt and ends any wait
        for a map update or a free slot. After completion, a new frame demand
        is required to render again, except that a pending still image
        submits its next demand itself; see `mln_map_request_still_image()`.

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

    def detach(self) -> Future[None]:
        """Starts normal graphics-owner teardown and map detachment.

        See `mln_render_session_detach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.detach()

    def close(self) -> Future[None]:
        """Consumes a session in any state and retires it, then runs completion,
        if given, once the host may destroy the target's graphics objects.

        See `mln_render_session_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.close()

    def drain_frame_results(self) -> tuple[RenderFrameResult, ...]:
        """Drains every currently queued terminal frame result into an
        independently owned batch and borrows its view. The records remain
        stable until the batch is released.

        See `mln_render_session_drain_frame_results` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return tuple(
            RenderFrameResult._from_native(item)
            for item in self._native.drain_frame_results()
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

    def query_feature_extensions(
        self,
        source_id: str,
        feature: Any,
        extension: str,
        extension_field: str,
        arguments: Any,
    ) -> Future[Any]:
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
    ) -> Future[tuple[RenderedFeature, ...]]:
        """Starts a rendered-feature query against the session's latest driver
        state.

        See `mln_render_session_query_rendered_features` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
        """
        return map_future(
            self._native.query_rendered_features(geometry, options),
            lambda value: tuple(RenderedFeature._from_native(item) for item in value),
        )

    def query_source_features(
        self, source_id: str, options: SourceFeatureQueryOptions | None = None
    ) -> Future[tuple[SourceFeature, ...]]:
        """Starts a source-feature query against the session's latest driver
        state.

        See `mln_render_session_query_source_features` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
        """
        return map_future(
            self._native.query_source_features(source_id, options),
            lambda value: tuple(SourceFeature._from_native(item) for item in value),
        )

    def read_texture(self) -> Future[TextureReadbackResult]:
        """Reads back the latest frame of the session's owned texture as
        premultiplied RGBA8.

        See `mln_render_session_read_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return map_future(
            self._native.read_texture(),
            lambda value: TextureReadbackResult._from_native(value),
        )

    def reduce_memory_use(self) -> Future[None]:
        """Starts best-effort release of renderer caches.

        See `mln_render_session_reduce_memory_use` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.reduce_memory_use()

    def request_frame(self, demand: FrameDemand) -> None:
        """Requests a frame without waiting. Every accepted demand produces one
        terminal result record. Caller-driver sessions require service on the
        graphics thread. Core-worker sessions need no host service.

        See `mln_render_session_request_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.request_frame(demand)

    def resize(self, extent: LogicalExtent) -> Future[CommandCompletion]:
        """Starts an ordered logical resize of the driver and the map. The
        driver applies the extent in order with its other work, and later
        driver work does not wait for the map. The completion runs after both
        have applied it and the map has published its snapshot. Frames render
        once a map update matches the applied extent; static and tile maps
        publish that update for their next still image.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.resize(extent)

    def service_driver_work(self) -> None:
        """Services every queued item for a caller-graphics-thread driver. The
        first successful service call fixes the session's graphics-thread
        identity; later calls from another native thread return
        `MLN_STATUS_WRONG_THREAD`. The target context must be current.
        Core-worker sessions return `MLN_STATUS_INVALID_STATE`.

        See `mln_render_session_service_driver_work` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.service_driver_work()

    def set_metal_borrowed_texture_target(
        self, descriptor: MetalBorrowedTextureDescriptor
    ) -> Future[None]:
        """Starts an ordered replacement of every texture of a caller-owned
        Metal ring.

        See `mln_render_session_set_metal_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.set_metal_borrowed_texture_target(descriptor)

    def set_metal_surface_target(
        self, descriptor: MetalSurfaceDescriptor
    ) -> Future[None]:
        """Starts an ordered Metal surface replacement.

        See `mln_render_session_set_metal_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.set_metal_surface_target(descriptor)

    def set_opengl_borrowed_texture_target(
        self, descriptor: OpenglBorrowedTextureDescriptor
    ) -> Future[None]:
        """Starts an ordered replacement of every texture of a caller-owned
        OpenGL ring.

        See `mln_render_session_set_opengl_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.set_opengl_borrowed_texture_target(descriptor)

    def set_opengl_surface_target(
        self, descriptor: OpenglSurfaceDescriptor
    ) -> Future[None]:
        """Starts an ordered OpenGL surface replacement.

        See `mln_render_session_set_opengl_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.set_opengl_surface_target(descriptor)

    def set_vulkan_borrowed_texture_target(
        self, descriptor: VulkanBorrowedTextureDescriptor
    ) -> Future[None]:
        """Starts an ordered replacement of every image of a caller-owned Vulkan
        ring.

        See `mln_render_session_set_vulkan_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.set_vulkan_borrowed_texture_target(descriptor)

    def set_vulkan_surface_target(
        self, descriptor: VulkanSurfaceDescriptor
    ) -> Future[None]:
        """Starts an ordered Vulkan surface replacement.

        See `mln_render_session_set_vulkan_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.set_vulkan_surface_target(descriptor)

    @property
    def discarding(self) -> RenderSessionHandleDiscarding:
        return RenderSessionHandleDiscarding(self)


class _ResourceRequestHandleOperations(GeneratedOperations):
    _native: _native._ResourceRequestHandle

    def complete(self, response: ResourceResponse) -> None:
        """Completes a C API resource provider request and consumes its handle.

        See `mln_resource_request_complete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.complete(response)

    def is_cancelled(self) -> bool:
        """Reports whether MapLibre has cancelled a C API resource provider
        request.

        See `mln_resource_request_is_cancelled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.is_cancelled()

    def pass_through(self) -> None:
        """Hands an outstanding provider request to the built-in network loader
        and consumes its handle.

        See `mln_resource_request_pass_through` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.pass_through()

    def close(self) -> None:
        """Releases the provider's resource request handle without answering it.

        See `mln_resource_request_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.close()

    def set_cancel_callback(self, handler: ResourceRequestCancelHandler) -> bool:
        """Registers a callback that runs when MapLibre cancels a C API resource
        provider request.

        See `mln_resource_request_set_cancel_callback` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_cancel_callback(handler)


class _ResourceTransformRequestHandleOperations(GeneratedOperations):
    _native: _native._ResourceTransformRequestHandle

    def complete(self, url: str) -> None:
        """Answers a resource transform request with a replacement URL and
        consumes its handle.

        See `mln_resource_transform_request_complete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.complete(url)

    def close(self) -> None:
        """Releases a resource transform request without answering it.

        See `mln_resource_transform_request_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.close()


class _RuntimeHandleOperations(GeneratedOperations):
    _native: _native._RuntimeHandle

    def clear_event_handler(self) -> Future[CommandCompletion]:
        """Stops event delivery and retires the previous handler.

        See `mln_runtime_clear_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_event_handler()

    def clear_http_header_transform(self) -> Future[CommandCompletion]:
        """Clears the runtime-scoped outgoing HTTP header transform.

        See `mln_runtime_clear_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_http_header_transform()

    def clear_resource_provider(self) -> Future[CommandCompletion]:
        """Clears the runtime-scoped network resource provider.

        See `mln_runtime_clear_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_resource_provider()

    def clear_resource_transform(self) -> Future[CommandCompletion]:
        """Clears the runtime-scoped URL transform for network resources.

        See `mln_runtime_clear_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_resource_transform()

    def create_map(self, options: MapOptions | None = None) -> RuntimeCreateMapResult:
        """Starts creation of a map on the runtime.

        See `mln_runtime_create_map` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        raw = self._native.create_map(options)
        return RuntimeCreateMapResult(
            map=_adopt_value(raw["map"], "MapHandle", self), ready=raw["ready"]
        )

    def create_offline_region(
        self, definition: OfflineRegionDefinition, metadata: Buffer
    ) -> Future[OfflineRegionInfo]:
        """Starts creating an offline region.

        See `mln_runtime_create_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.create_offline_region(definition, metadata),
            lambda value: OfflineRegionInfo._from_native(value),
        )

    def delete_offline_region(self, region_id: int) -> Future[None]:
        """Deletes an offline region.

        See `mln_runtime_delete_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.delete_offline_region(region_id)

    def get_event_mask(self) -> RuntimeEventFlag:
        """Reports which runtime-scoped event types this runtime queues.

        See `mln_runtime_get_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return RuntimeEventFlag(self._native.get_event_mask())

    def get_offline_region(self, region_id: int) -> Future[OfflineRegionInfo | None]:
        """Starts getting one offline region by ID.

        See `mln_runtime_get_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.get_offline_region(region_id),
            lambda value: _maybe(OfflineRegionInfo._from_native, value),
        )

    def get_offline_region_status(self, region_id: int) -> Future[OfflineRegionStatus]:
        """Starts getting the current download status for an offline region.

        See `mln_runtime_get_offline_region_status` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.get_offline_region_status(region_id),
            lambda value: OfflineRegionStatus._from_native(value),
        )

    def invalidate_offline_region(self, region_id: int) -> Future[None]:
        """Invalidates cached resources for an offline region.

        See `mln_runtime_invalidate_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.invalidate_offline_region(region_id)

    def list_offline_regions(self) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Starts listing the offline regions in the runtime database.

        See `mln_runtime_list_offline_regions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.list_offline_regions(),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def merge_offline_regions(
        self, side_database_path: str
    ) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Starts merging offline regions from another MapLibre offline
        database.

        See `mln_runtime_merge_offline_regions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.merge_offline_regions(side_database_path),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def close(self) -> Future[None]:
        """Starts release of a runtime after checking its children.

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

    def set_event_handler(
        self, handler: RuntimeEventHandler
    ) -> Future[CommandCompletion]:
        """Selects how runtime and map events reach the host.

        See `mln_runtime_set_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_event_handler(handler)

    def set_event_mask(self, mask: RuntimeEventFlag) -> None:
        """Selects which runtime-scoped event types this runtime queues.

        See `mln_runtime_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_event_mask(mask)

    def set_http_header_transform(
        self, transform: HttpHeaderTransform
    ) -> Future[CommandCompletion]:
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

    def set_offline_region_download_state(
        self, region_id: int, state: OfflineRegionDownloadState
    ) -> Future[None]:
        """Sets an offline region's native download state.

        See `mln_runtime_set_offline_region_download_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_offline_region_download_state(region_id, state)

    def set_offline_region_observed(
        self, region_id: int, observed: bool
    ) -> Future[None]:
        """Enables or disables runtime events for an offline region.

        See `mln_runtime_set_offline_region_observed` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_offline_region_observed(region_id, observed)

    def set_resource_provider(
        self, provider: ResourceProvider
    ) -> Future[CommandCompletion]:
        """Registers or replaces a runtime-scoped network resource provider.

        See `mln_runtime_set_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_resource_provider(provider)

    def set_resource_transform(
        self, transform: ResourceTransform
    ) -> Future[CommandCompletion]:
        """Registers or updates a runtime-scoped URL transform for network
        resources.

        See `mln_runtime_set_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_resource_transform(transform)

    def update_offline_region_metadata(
        self, region_id: int, metadata: Buffer
    ) -> Future[OfflineRegionInfo]:
        """Starts updating opaque binary metadata for an offline region.

        See `mln_runtime_update_offline_region_metadata` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.update_offline_region_metadata(region_id, metadata),
            lambda value: OfflineRegionInfo._from_native(value),
        )

    @property
    def discarding(self) -> RuntimeHandleDiscarding:
        return RuntimeHandleDiscarding(self)


class _StyleExpressionHandleOperations(GeneratedOperations):
    _native: _native._StyleExpressionHandle

    def evaluate(self, inputs: StyleExpressionInputs | None = None) -> Any:
        """Evaluates a parsed expression and writes its result as UTF-8 JSON.

        See `mln_style_expression_evaluate` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
        """
        return self._native.evaluate(inputs)

    def get_info(self) -> StyleExpressionInfo:
        """Copies what a parsed expression depends on into out_info.

        See `mln_style_expression_get_info` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
        """
        return StyleExpressionInfo._from_native(self._native.get_info())

    def close(self) -> None:
        """Releases a style expression.

        See `mln_style_expression_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
        """
        return self._native.close()


def android_init(jni_env: int, context: int) -> None:
    """Initializes Android platform services.

    See `mln_android_init` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
    """
    return _native.android_init(jni_env, context)


def c_version() -> int:
    """Reports the C ABI contract version. The value is 0 while the ABI is
    unstable, and will increment on each SemVer major release.

    See `mln_c_version` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """
    return _native.c_version()


def geojson_source_data_create(
    data: Any, options: GeojsonSourceOptions | None = None
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


def log_set_async_severity_mask(mask: LogSeverityFlag) -> None:
    """Controls which log severities MapLibre Native may dispatch
    asynchronously.

    See `mln_log_set_async_severity_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.log_set_async_severity_mask(mask)


def log_set_callback(handler: LogHandler) -> None:
    """Installs a process-global MapLibre Native log callback.

    See `mln_log_set_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.log_set_callback(handler)


def logical_extent_physical_size(
    extent: LogicalExtent,
) -> LogicalExtentPhysicalSizeResult:
    """Computes the physical device-pixel size of a logical extent.

    See `mln_logical_extent_physical_size` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """
    raw = _native.logical_extent_physical_size(extent)
    return LogicalExtentPhysicalSizeResult(raw["width"], raw["height"])


def map_projection_create(view: MapView) -> MapProjectionHandle:
    """Creates a standalone projection of a view, such as the view of a
    rendered frame or of a map snapshot.

    See `mln_map_projection_create` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    """
    return _adopt_value(
        _native.map_projection_create(view), "MapProjectionHandle", None
    )


def network_get_status() -> NetworkStatus:
    """Reads MapLibre Native's process-global network status.

    See `mln_network_get_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return NetworkStatus(_native.network_get_status())


def network_set_status(status: NetworkStatus) -> None:
    """Sets MapLibre Native's process-global network status.

    See `mln_network_set_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return _native.network_set_status(status)


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


def runtime_create(options: RuntimeOptions | None = None) -> RuntimeHandle:
    """Creates a runtime.

    See `mln_runtime_create` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return _adopt_value(_native.runtime_create(options), "RuntimeHandle", None)


def style_expression_create(
    expression: Any, options: StyleExpressionOptions | None = None
) -> StyleExpressionHandle:
    """Parses a style expression.

    See `mln_style_expression_create` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """
    return _adopt_value(
        _native.style_expression_create(expression, options),
        "StyleExpressionHandle",
        None,
    )


def supported_render_backend_mask() -> RenderBackendFlag:
    """Reports the render backends available in this native library build.

    See `mln_supported_render_backend_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """
    return RenderBackendFlag(_native.supported_render_backend_mask())


class MapHandleDiscarding:
    __slots__ = ("_native",)
    _native: _native._MapHandle

    def __init__(self, owner: _MapHandleOperations):
        self._native = owner._native

    def add_custom_geometry_source(
        self, source_id: str, options: CustomGeometrySourceOptions | None = None
    ) -> None:
        """Adds a custom geometry source.

        See `mln_map_add_custom_geometry_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_custom_geometry_source(source_id, options)

    def add_custom_mvt_vector_source(
        self, source_id: str, options: CustomMvtVectorSourceOptions | None = None
    ) -> None:
        """Adds a custom MVT vector source.

        See `mln_map_add_custom_mvt_vector_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_custom_mvt_vector_source(source_id, options)

    def add_geojson_source_data(
        self, source_id: str, data: GeojsonSourceDataHandle
    ) -> None:
        """Adds a GeoJSON source with prepared inline data.

        See `mln_map_add_geojson_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_geojson_source_data(source_id, data._native)

    def add_geojson_source_url(
        self, source_id: str, url: str, options: GeojsonSourceOptions | None = None
    ) -> None:
        """Adds a GeoJSON source with URL data.

        See `mln_map_add_geojson_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_geojson_source_url(source_id, url, options)

    def add_image_source_image(
        self,
        source_id: str,
        coordinates: Sequence[LatLng],
        image: PremultipliedRgba8Image,
    ) -> None:
        """Adds an image source with inline image pixels.

        See `mln_map_add_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_image_source_image(
            source_id, coordinates, image
        )

    def add_image_source_url(
        self, source_id: str, coordinates: Sequence[LatLng], url: str
    ) -> None:
        """Adds an image source that loads its image from a URL.

        See `mln_map_add_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_image_source_url(source_id, coordinates, url)

    def add_style_layer_json(self, layer_json: Any, before_layer_id: str) -> None:
        """Adds one style layer from a full style-spec layer JSON object.

        See `mln_map_add_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_style_layer_json(layer_json, before_layer_id)

    def add_style_source_json(self, source_id: str, source_json: Any) -> None:
        """Adds one style source from a style-spec source JSON object.

        See `mln_map_add_style_source_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_style_source_json(source_id, source_json)

    def apply_camera_delta(self, delta: CameraDelta) -> None:
        """Submits one atomic relative camera update.

        See `mln_map_apply_camera_delta` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_apply_camera_delta(delta)

    def begin_command_group(self) -> None:
        """Begins a command group, which holds this map's render updates until
        the group ends.

        See `mln_map_begin_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_begin_command_group()

    def cancel_camera_transition(self, transition_id: int) -> None:
        """Cancels the camera transitions of the commands whose animation
        carried transition_id, and leaves every other transition running.

        See `mln_map_cancel_camera_transition` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_cancel_camera_transition(transition_id)

    def cancel_transitions(self) -> None:
        """Cancels the camera transitions running when this command commits.

        See `mln_map_cancel_transitions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_cancel_transitions()

    def dump_debug_logs(self) -> None:
        """Submits an ordered debug-log command.

        See `mln_map_dump_debug_logs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_dump_debug_logs()

    def end_command_group(self) -> None:
        """Ends the innermost command group that `mln_map_begin_command_group()`
        began.

        See `mln_map_end_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_end_command_group()

    def invalidate_custom_geometry_source_region(
        self, source_id: str, bounds: LatLngBounds
    ) -> None:
        """Invalidates custom geometry source data inside one geographic region.

        See `mln_map_invalidate_custom_geometry_source_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_invalidate_custom_geometry_source_region(
            source_id, bounds
        )

    def invalidate_custom_geometry_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> None:
        """Invalidates custom geometry source data for one canonical tile.

        See `mln_map_invalidate_custom_geometry_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_invalidate_custom_geometry_source_tile(
            source_id, tile_id
        )

    def invalidate_custom_mvt_vector_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> None:
        """Invalidates custom MVT vector source data for one canonical tile.

        See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_invalidate_custom_mvt_vector_source_tile(
            source_id, tile_id
        )

    def move_style_layer(self, layer_id: str, before_layer_id: str) -> None:
        """Moves one style layer before another layer or to the top.

        See `mln_map_move_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_move_style_layer(layer_id, before_layer_id)

    def remove_feature_state(self, selector: FeatureStateSelector) -> None:
        """Removes per-feature state from this map.

        See `mln_map_remove_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_remove_feature_state(selector)

    def remove_style_image(self, image_id: str) -> None:
        """Removes one runtime style image by ID.

        See `mln_map_remove_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_remove_style_image(image_id)

    def remove_style_layer(self, layer_id: str) -> None:
        """Removes one style layer by ID.

        See `mln_map_remove_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_remove_style_layer(layer_id)

    def remove_style_source(self, source_id: str) -> None:
        """Removes one style source by ID.

        See `mln_map_remove_style_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_remove_style_source(source_id)

    def request_repaint(self) -> None:
        """Requests a repaint for a continuous map.

        See `mln_map_request_repaint` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_request_repaint()

    def resize(self, extent: LogicalExtent) -> None:
        """Submits the sole post-creation logical extent update.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_resize(extent)

    def set_bounds(self, options: BoundOptions | None = None) -> None:
        """Submits a copied camera-constraint command.

        See `mln_map_set_bounds` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_bounds(options)

    def set_custom_geometry_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: Any
    ) -> None:
        """Sets custom geometry source data for one canonical tile.

        See `mln_map_set_custom_geometry_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_geometry_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_geometry_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> None:
        """Reports a custom geometry source error for one canonical tile.

        See `mln_map_set_custom_geometry_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_geometry_source_tile_error(
            source_id, tile_id, message
        )

    def set_custom_mvt_vector_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: Buffer
    ) -> None:
        """Sets custom MVT vector source data for one canonical tile.

        See `mln_map_set_custom_mvt_vector_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_mvt_vector_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_mvt_vector_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> None:
        """Reports a custom MVT vector source error for one canonical tile.

        See `mln_map_set_custom_mvt_vector_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_mvt_vector_source_tile_error(
            source_id, tile_id, message
        )

    def set_debug_options(self, options: MapDebugFlag) -> None:
        """Submits a debug-overlay command.

        See `mln_map_set_debug_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_debug_options(options)

    def set_event_mask(self, mask: RuntimeEventFlag) -> None:
        """Selects which map-originated event types this map queues.

        See `mln_map_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_event_mask(mask)

    def set_feature_state(self, selector: FeatureStateSelector, state: Any) -> None:
        """Submits a copied per-feature-state command.

        See `mln_map_set_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_feature_state(selector, state)

    def set_free_camera_options(self, options: FreeCameraOptions | None = None) -> None:
        """Submits a copied free-camera command.

        See `mln_map_set_free_camera_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_free_camera_options(options)

    def set_geojson_source_data(
        self, source_id: str, data: GeojsonSourceDataHandle
    ) -> None:
        """Updates one GeoJSON source with prepared inline data.

        See `mln_map_set_geojson_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geojson_source_data(source_id, data._native)

    def set_geojson_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> None:
        """Overrides one GeoJSON source's synchronous tiling at runtime.

        See `mln_map_set_geojson_source_synchronous_tiling` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geojson_source_synchronous_tiling(
            source_id, enabled
        )

    def set_geojson_source_url(self, source_id: str, url: str) -> None:
        """Updates one GeoJSON source to load data from a URL.

        See `mln_map_set_geojson_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geojson_source_url(source_id, url)

    def set_global_state_property(self, property_name: str, value: Any) -> None:
        """Submits a global-state JSON value. JSON null restores the style
        default.

        See `mln_map_set_global_state_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_global_state_property(property_name, value)

    def set_image_source_coordinates(
        self, source_id: str, coordinates: Sequence[LatLng]
    ) -> None:
        """Updates image source coordinates.

        See `mln_map_set_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_image_source_coordinates(
            source_id, coordinates
        )

    def set_image_source_image(
        self, source_id: str, image: PremultipliedRgba8Image
    ) -> None:
        """Updates an image source with inline image pixels.

        See `mln_map_set_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_image_source_image(source_id, image)

    def set_image_source_url(self, source_id: str, url: str) -> None:
        """Updates an image source to load its image from a URL.

        See `mln_map_set_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_image_source_url(source_id, url)

    def set_projection_mode(self, mode: ProjectionMode) -> None:
        """Submits copied axonometric rendering option fields.

        See `mln_map_set_projection_mode` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_projection_mode(mode)

    def set_rendering_stats_view_enabled(self, enabled: bool) -> None:
        """Submits a rendering-stats visibility command.

        See `mln_map_set_rendering_stats_view_enabled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_rendering_stats_view_enabled(enabled)

    def set_style_image(
        self,
        image_id: str,
        image: PremultipliedRgba8Image,
        options: StyleImageOptions | None = None,
    ) -> None:
        """Sets one runtime style image.

        See `mln_map_set_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_image(image_id, image, options)

    def set_style_json(self, json: Any) -> None:
        """Queues an inline style JSON command.

        See `mln_map_set_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_style_json(json)

    def set_style_layer_property(
        self, layer_id: str, property_name: str, value: Any
    ) -> None:
        """Sets one layer property using its MapLibre style-spec property name.

        See `mln_map_set_style_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_layer_property(
            layer_id, property_name, value
        )

    def set_style_light_json(self, light_json: Any) -> None:
        """Sets the style light from a style-spec light JSON object.

        See `mln_map_set_style_light_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_light_json(light_json)

    def set_style_light_property(self, property_name: str, value: Any) -> None:
        """Sets one style light property using its MapLibre style-spec property
        name.

        See `mln_map_set_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_light_property(property_name, value)

    def set_style_source_volatile(self, source_id: str, is_volatile: bool) -> None:
        """Sets whether one style source stores fetched tiles in the persistent
        cache.

        See `mln_map_set_style_source_volatile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_source_volatile(source_id, is_volatile)

    def set_style_transition_options(
        self, options: StyleTransitionOptions | None = None
    ) -> None:
        """Sets the style's global transition options.

        See `mln_map_set_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_transition_options(options)

    def set_style_url(self, url: str) -> None:
        """Queues a style URL command.

        See `mln_map_set_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_style_url(url)

    def set_tile_options(self, options: MapTileOptions | None = None) -> None:
        """Submits a copied tile-options command.

        See `mln_map_set_tile_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_tile_options(options)

    def set_viewport_options(self, options: MapViewportOptions | None = None) -> None:
        """Submits a copied viewport-options command.

        See `mln_map_set_viewport_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_viewport_options(options)

    def update_camera(self, update: CameraUpdate) -> None:
        """Submits one atomic camera update.

        See `mln_map_update_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_update_camera(update)


class RenderSessionHandleDiscarding:
    __slots__ = ("_native",)
    _native: _native._RenderSessionHandle

    def __init__(self, owner: _RenderSessionHandleOperations):
        self._native = owner._native

    def resize(self, extent: LogicalExtent) -> None:
        """Starts an ordered logical resize of the driver and the map. The
        driver applies the extent in order with its other work, and later
        driver work does not wait for the map. The completion runs after both
        have applied it and the map has published its snapshot. Frames render
        once a map update matches the applied extent; static and tile maps
        publish that update for their next still image.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native._discard_resize(extent)


class RuntimeHandleDiscarding:
    __slots__ = ("_native",)
    _native: _native._RuntimeHandle

    def __init__(self, owner: _RuntimeHandleOperations):
        self._native = owner._native

    def clear_event_handler(self) -> None:
        """Stops event delivery and retires the previous handler.

        See `mln_runtime_clear_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_event_handler()

    def clear_http_header_transform(self) -> None:
        """Clears the runtime-scoped outgoing HTTP header transform.

        See `mln_runtime_clear_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_http_header_transform()

    def clear_resource_provider(self) -> None:
        """Clears the runtime-scoped network resource provider.

        See `mln_runtime_clear_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_resource_provider()

    def clear_resource_transform(self) -> None:
        """Clears the runtime-scoped URL transform for network resources.

        See `mln_runtime_clear_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_resource_transform()

    def set_event_handler(self, handler: RuntimeEventHandler) -> None:
        """Selects how runtime and map events reach the host.

        See `mln_runtime_set_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_event_handler(handler)

    def set_http_header_transform(self, transform: HttpHeaderTransform) -> None:
        """Registers or replaces the runtime-scoped outgoing HTTP header
        transform.

        See `mln_runtime_set_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_http_header_transform(transform)

    def set_resource_provider(self, provider: ResourceProvider) -> None:
        """Registers or replaces a runtime-scoped network resource provider.

        See `mln_runtime_set_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_resource_provider(provider)

    def set_resource_transform(self, transform: ResourceTransform) -> None:
        """Registers or updates a runtime-scoped URL transform for network
        resources.

        See `mln_runtime_set_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_resource_transform(transform)
