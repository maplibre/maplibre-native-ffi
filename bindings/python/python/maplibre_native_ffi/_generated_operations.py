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
    CustomVectorSourceOptions,
    FeatureStateSelector,
    FrameDemand,
    FreeCameraOptions,
    GeoJsonSourceOptions,
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
    OffscreenDescriptor,
    OffscreenDeviceOptions,
    OpenGlBorrowedTextureDescriptor,
    OpenGlContextProviderFlag,
    OpenGlOwnedTextureDescriptor,
    OpenGlSurfaceDescriptor,
    OpenGlTextureFrame,
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
    StyleVectorTileEncoding,
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
        GeoJsonSourceDataHandle,
        MapHandle,
        MapProjectionHandle,
        OffscreenDeviceHandle,
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


class MapAttachOffscreenResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachOpenGlBorrowedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachOpenGlOwnedTextureResult(NamedTuple):
    session: RenderSessionHandle
    ready: Future[None]


class MapAttachOpenGlSurfaceResult(NamedTuple):
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
        """Gives up an acquired frame without saying when your GPU stops reading
        it.

        Because your GPU may still read the frame's texture, the session
        never renders into that texture again.

        See `mln_acquired_frame_discard` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.discard()

    def metal_texture(self) -> AbstractContextManager[MetalTextureFrame]:
        """Gets the Metal texture of an acquired frame.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_metal_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session lost its target.
        """
        return _view(
            self,
            lambda: self._native.metal_texture(),
            lambda raw: MetalTextureFrame._from_native(raw),
        )

    def open_gl_texture(self) -> AbstractContextManager[OpenGlTextureFrame]:
        """Gets the OpenGL texture of an acquired frame.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_open_gl_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session lost its target.
        """
        return _view(
            self,
            lambda: self._native.open_gl_texture(),
            lambda raw: OpenGlTextureFrame._from_native(raw),
        )

    def producer_sync(self) -> AbstractContextManager[GpuSync]:
        """Reads what to wait for before you read an acquired frame's texture.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_producer_sync` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session lost its target.
        """
        return _view(
            self,
            lambda: self._native.producer_sync(),
            lambda raw: GpuSync._from_native(raw),
        )

    def get_result(self) -> RenderFrameResult:
        """Reads the frame result of an acquired frame.

        See `mln_acquired_frame_get_result` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session lost its target.
        """
        return RenderFrameResult._from_native(self._native.get_result())

    def vulkan_texture(self) -> AbstractContextManager[VulkanTextureFrame]:
        """Gets the Vulkan image of an acquired frame.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_vulkan_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session lost its target.
        """
        return _view(
            self,
            lambda: self._native.vulkan_texture(),
            lambda raw: VulkanTextureFrame._from_native(raw),
        )

    def close(self, consumer_completion: GpuSync | None = None) -> None:
        """Gives an acquired frame back to the session once you're done reading
        it.

        `consumer_completion` says when your GPU work that reads the texture
        finishes. Null means that the work has already finished. The session
        waits for that point before it renders into the texture again.

        See `mln_acquired_frame_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `RenderSessionCapabilities.consumer_sync_kinds` lacks the kind of
          `consumer_completion`.
        """
        return self._native.close(consumer_completion)


class _GeoJsonSourceDataHandleOperations(GeneratedOperations):
    _native: _native._GeoJsonSourceDataHandle

    @classmethod
    def create(
        cls, data: Any, options: GeoJsonSourceOptions | None = None
    ) -> GeoJsonSourceDataHandle:
        """Prepares GeoJSON data for GeoJSON sources.

        `data` is a GeoJSON feature collection, feature, or geometry.
        Preparing it can be slow, but adding the result to a source is quick,
        and several sources can share it.

        See `mln_geo_json_source_data_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `data` isn't valid GeoJSON, or when
          `options` turns on clustering and `data` isn't a feature collection
          of points.
        """
        return _adopt_value(
            _native._GeoJsonSourceDataHandle.create(data, options),
            "GeoJsonSourceDataHandle",
            None,
        )

    def close(self) -> None:
        """Releases prepared GeoJSON data.

        Sources that use the data keep it.

        See `mln_geo_json_source_data_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.close()


class _MapHandleOperations(GeneratedOperations):
    _native: _native._MapHandle

    def add_custom_geometry_source(
        self, source_id: str, options: CustomGeometrySourceOptions
    ) -> Future[CommandCompletion]:
        """Adds a source whose GeoJSON tiles come from callbacks.

        See `mln_map_add_custom_geometry_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the style already has a source named
          `source_id`.
        """
        return self._native.add_custom_geometry_source(source_id, options)

    def add_custom_vector_source(
        self, source_id: str, options: CustomVectorSourceOptions
    ) -> Future[CommandCompletion]:
        """Adds a source whose vector tiles come from callbacks.

        See `mln_map_add_custom_vector_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the style already has a source named
          `source_id`.
        """
        return self._native.add_custom_vector_source(source_id, options)

    def add_geo_json_source_data(
        self, source_id: str, data: GeoJsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Adds a GeoJSON source with prepared data.

        The source uses the options that `data` was prepared with. `data` can
        be released once this function returns.

        See `mln_map_add_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the style already has a source named
          `source_id`.
        """
        return self._native.add_geo_json_source_data(source_id, data._native)

    def add_geo_json_source_url(
        self, source_id: str, url: str, options: GeoJsonSourceOptions | None = None
    ) -> Future[CommandCompletion]:
        """Adds a GeoJSON source that loads its data from `url`.

        See `mln_map_add_geo_json_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the style already has a source named
          `source_id`.
        """
        return self._native.add_geo_json_source_url(source_id, url, options)

    def add_image_source_image(
        self,
        source_id: str,
        coordinates: Sequence[LatLng],
        image: PremultipliedRgba8Image,
    ) -> Future[CommandCompletion]:
        """Adds an image source that shows `image`.

        `coordinates` are the image's four corners, clockwise from the top
        left.

        See `mln_map_add_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the style already has a source named
          `source_id`.
        """
        return self._native.add_image_source_image(source_id, coordinates, image)

    def add_image_source_url(
        self, source_id: str, coordinates: Sequence[LatLng], url: str
    ) -> Future[CommandCompletion]:
        """Adds an image source that loads its image from `url`.

        `coordinates` are the image's four corners, clockwise from the top
        left.

        See `mln_map_add_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the style already has a source named
          `source_id`.
        """
        return self._native.add_image_source_url(source_id, coordinates, url)

    def add_style_layer_json(
        self, layer_json: Any, before_layer_id: str
    ) -> Future[CommandCompletion]:
        """Adds a layer to the map's style.

        `layer_json` is a
        \\[layer\\](https://maplibre.org/maplibre-style-spec/layers/) object.
        The layer goes below the layer named `before_layer_id`, or above
        every layer when `before_layer_id` is empty.

        See `mln_map_add_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named
          `before_layer_id`, or no source with the ID that the layer uses.
        - `Status.INVALID_ARGUMENT` when the style already has a layer with
          the new layer's ID.
        """
        return self._native.add_style_layer_json(layer_json, before_layer_id)

    def add_style_source_json(
        self, source_id: str, source_json: Any
    ) -> Future[CommandCompletion]:
        """Adds a source to the map's style.

        `source_json` is a
        \\[source\\](https://maplibre.org/maplibre-style-spec/sources/) object.

        See `mln_map_add_style_source_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the style already has a source named
          `source_id`.
        """
        return self._native.add_style_source_json(source_id, source_json)

    def apply_camera_delta(self, delta: CameraDelta) -> Future[CommandCompletion]:
        """Moves the camera relative to where it is, as a gesture does.

        `delta` can pan the map's content by an offset on the screen, scale
        the zoom, and add to the bearing and pitch. An anchor keeps one
        screen point in place while the scale, bearing, or pitch changes. An
        anchor needs one of those changes, and combines with an offset only
        when the delta has no animation. The function finishes when the map
        applies the delta, before an animation ends.

        See `mln_map_apply_camera_delta` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.apply_camera_delta(delta)

    def attach_metal_borrowed_texture(
        self,
        descriptor: MetalBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> MapAttachMetalBorrowedTextureResult:
        """Creates a session that renders the map into a ring of your Metal
        textures.

        See `mln_map_attach_metal_borrowed_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.METAL`, or when the target supports none of the
          `producer_sync_kinds` in `options`.
        """
        raw = self._native.attach_metal_borrowed_texture(descriptor, options)
        return MapAttachMetalBorrowedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_metal_owned_texture(
        self,
        descriptor: MetalOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> MapAttachMetalOwnedTextureResult:
        """Creates a session that renders the map into a ring of new Metal
        textures.

        See `mln_map_attach_metal_owned_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.METAL`, or when the target supports none of the
          `producer_sync_kinds` in `options`.
        """
        raw = self._native.attach_metal_owned_texture(descriptor, options)
        return MapAttachMetalOwnedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_metal_surface(
        self, descriptor: MetalSurfaceDescriptor, options: RenderSessionAttachOptions
    ) -> MapAttachMetalSurfaceResult:
        """Creates a session that renders the map to a Metal layer.

        See `mln_map_attach_metal_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.METAL`.
        """
        raw = self._native.attach_metal_surface(descriptor, options)
        return MapAttachMetalSurfaceResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_offscreen(
        self,
        descriptor: OffscreenDescriptor,
        options: RenderSessionAttachOptions,
        device: OffscreenDeviceHandle | None = None,
    ) -> MapAttachOffscreenResult:
        """Attaches a render session that renders the map offscreen.

        Read each image's pixels with `RenderSessionHandle.read_texture()`.
        `options` must select `RenderDriverKind.CORE_WORKER`.

        The session renders on `device`. When `device` is null, the session
        creates a device with the default options, and releases it when the
        session ends.

        See `mln_map_attach_offscreen` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the map already has a render session.
        - `Status.UNSUPPORTED` when `device` is null and this build's render
          backend can't create a device.
        - `Status.NATIVE_ERROR` when `device` is null and the platform can't
          create a device.
        """
        raw = self._native.attach_offscreen(
            descriptor, options, None if device is None else device._native
        )
        return MapAttachOffscreenResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_open_gl_borrowed_texture(
        self,
        descriptor: OpenGlBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> MapAttachOpenGlBorrowedTextureResult:
        """Creates a session that renders the map into a ring of your OpenGL
        textures.

        The session needs `RenderDriverKind.CALLER_GRAPHICS_THREAD`.

        See `mln_map_attach_open_gl_borrowed_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `open_gl_supported_context_provider_mask()` lacks the context's
          platform, or when the target supports none of the
          `producer_sync_kinds` in `options`.
        """
        raw = self._native.attach_open_gl_borrowed_texture(descriptor, options)
        return MapAttachOpenGlBorrowedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_open_gl_owned_texture(
        self,
        descriptor: OpenGlOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> MapAttachOpenGlOwnedTextureResult:
        """Creates a session that renders the map into a ring of new OpenGL
        textures.

        With `OpenGlContextOwnership.SHARED`, the session needs
        `RenderDriverKind.CALLER_GRAPHICS_THREAD`, and the textures are in
        your context's share group. With `OpenGlContextOwnership.DEDICATED`,
        the session needs `RenderDriverKind.CORE_WORKER` and renders in a
        context of its own. You can't acquire its frames, but
        `RenderSessionHandle.read_texture()` reads them.

        See `mln_map_attach_open_gl_owned_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `open_gl_supported_context_provider_mask()` lacks the context's
          platform, or when the target supports none of the
          `producer_sync_kinds` in `options`.
        """
        raw = self._native.attach_open_gl_owned_texture(descriptor, options)
        return MapAttachOpenGlOwnedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_open_gl_surface(
        self, descriptor: OpenGlSurfaceDescriptor, options: RenderSessionAttachOptions
    ) -> MapAttachOpenGlSurfaceResult:
        """Creates a session that renders the map to an OpenGL surface.

        The session needs `RenderDriverKind.CORE_WORKER` for a transferred
        WebGL canvas, and `RenderDriverKind.CALLER_GRAPHICS_THREAD` for any
        other context.

        See `mln_map_attach_open_gl_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `open_gl_supported_context_provider_mask()` lacks the context's
          platform.
        """
        raw = self._native.attach_open_gl_surface(descriptor, options)
        return MapAttachOpenGlSurfaceResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_vulkan_borrowed_texture(
        self,
        descriptor: VulkanBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> MapAttachVulkanBorrowedTextureResult:
        """Creates a session that renders the map into a ring of your Vulkan
        images.

        See `mln_map_attach_vulkan_borrowed_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.VULKAN`, or when the target supports none of the
          `producer_sync_kinds` in `options`.
        """
        raw = self._native.attach_vulkan_borrowed_texture(descriptor, options)
        return MapAttachVulkanBorrowedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_vulkan_owned_texture(
        self,
        descriptor: VulkanOwnedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> MapAttachVulkanOwnedTextureResult:
        """Creates a session that renders the map into a ring of new Vulkan
        images.

        See `mln_map_attach_vulkan_owned_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.VULKAN`, or when the target supports none of the
          `producer_sync_kinds` in `options`.
        """
        raw = self._native.attach_vulkan_owned_texture(descriptor, options)
        return MapAttachVulkanOwnedTextureResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def attach_vulkan_surface(
        self, descriptor: VulkanSurfaceDescriptor, options: RenderSessionAttachOptions
    ) -> MapAttachVulkanSurfaceResult:
        """Creates a session that renders the map to a Vulkan surface.

        See `mln_map_attach_vulkan_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.VULKAN`.
        """
        raw = self._native.attach_vulkan_surface(descriptor, options)
        return MapAttachVulkanSurfaceResult(
            session=_adopt_value(raw["session"], "RenderSessionHandle", self),
            ready=raw["ready"],
        )

    def begin_command_group(self) -> Future[CommandCompletion]:
        """Holds back the map's changes from rendering until
        `MapHandle.end_command_group()`, so they show in the same frame.

        Groups nest, and the changes render when the outermost group ends.

        See `mln_map_begin_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.begin_command_group()

    def camera_for_geometry(
        self, geometry: Any, fit_options: CameraFitOptions | None = None
    ) -> Future[CameraOptions]:
        """Computes a camera that fits a GeoJSON geometry in the map's view.

        `geometry` is a GeoJSON geometry object with at least one coordinate,
        not a feature. The camera keeps the map's bearing and pitch unless
        `fit_options` sets them, and its zoom stays within the map's zoom
        limits.

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
        """Computes a camera that fits `bounds` in the map's view.

        The camera keeps the map's bearing and pitch unless `fit_options`
        sets them, and its zoom stays within the map's zoom limits.

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
        """Computes a camera that fits `coordinates` in the map's view.

        `coordinates` must not be empty. The camera keeps the map's bearing
        and pitch unless `fit_options` sets them, and its zoom stays within
        the map's zoom limits.

        See `mln_map_camera_for_lat_lngs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.camera_for_lat_lngs(coordinates, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def cancel_camera_transition(self, transition_id: int) -> Future[CommandCompletion]:
        """Stops the running camera animations whose animation options set
        `transition_id`.

        Other animations keep running. The end handler of each stopped
        animation reports `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_camera_transition` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.cancel_camera_transition(transition_id)

    def cancel_transitions(self) -> Future[CommandCompletion]:
        """Stops every running camera animation where it is.

        The end handler of each stopped animation reports
        `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_transitions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.cancel_transitions()

    def dump_debug_logs(self) -> Future[CommandCompletion]:
        """Logs debugging information about the map's style and sources.

        See `mln_map_dump_debug_logs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.dump_debug_logs()

    def end_command_group(self) -> Future[CommandCompletion]:
        """Ends the group that `MapHandle.begin_command_group()` began.

        See `mln_map_end_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.end_command_group()

    def get_camera(self) -> Future[CameraQueryResult]:
        """Reads the map's camera.

        The query reads the camera after the map applies the commands sent
        before it.

        See `mln_map_get_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.get_camera(),
            lambda value: CameraQueryResult._from_native(value),
        )

    def get_feature_state(self, selector: FeatureStateSelector) -> Future[Any]:
        """Reads the state of a feature, as a JSON object.

        The object is empty when the feature has no state.

        See `mln_map_get_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.get_feature_state(selector)

    def get_global_state(self) -> Future[Any]:
        """Reads the global state of the map's style as a JSON object.

        See `mln_map_get_global_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_global_state()

    def get_image_source_coordinates(
        self, source_id: str
    ) -> Future[tuple[LatLng, ...]]:
        """Reads the corners of an image source's image, clockwise from the top
        left.

        See `mln_map_get_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't
          an image source.
        """
        return map_future(
            self._native.get_image_source_coordinates(source_id),
            lambda value: tuple(LatLng._from_native(item) for item in value),
        )

    def get_loaded_style_json(self) -> Future[Any]:
        """Reads the JSON of the style that the map loaded.

        The JSON doesn't include the changes made to the style since it
        loaded.

        See `mln_map_get_loaded_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.get_loaded_style_json()

    def get_snapshot(self) -> MapSnapshot:
        """Copies the map's state.

        The copy doesn't wait for pending commands. To read the camera after
        they apply, use `MapHandle.get_camera()`.

        See `mln_map_get_snapshot` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return MapSnapshot._from_native(self._native.get_snapshot())

    def get_style_image(self, image_id: str) -> Future[StyleImageInfo | None]:
        """Reads an image of the map's style.

        Completes with no value when the style has no image named `image_id`.

        See `mln_map_get_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_image(image_id),
            lambda value: _maybe(StyleImageInfo._from_native, value),
        )

    def get_style_layer(self, layer_id: str) -> Future[StyleLayerInfo | None]:
        """Reads a layer of the map's style.

        Completes with no value when the style has no layer named `layer_id`.

        See `mln_map_get_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_layer(layer_id),
            lambda value: _maybe(StyleLayerInfo._from_native, value),
        )

    def get_style_layer_json(self, layer_id: str) -> Future[Any]:
        """Reads a layer of the map's style as a
        \\[layer\\](https://maplibre.org/maplibre-style-spec/layers/) object.

        Completes with no value when the style has no layer named `layer_id`.

        See `mln_map_get_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_layer_json(layer_id)

    def get_style_layer_property(
        self, layer_id: str, property_name: str
    ) -> Future[Any]:
        """Reads a property of a layer in the map's style.

        Completes with no value when the layer doesn't set the property.

        See `mln_map_get_style_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named `layer_id`.
        """
        return self._native.get_style_layer_property(layer_id, property_name)

    def get_style_light_property(self, property_name: str) -> Future[Any]:
        """Reads a property of the light of the map's style.

        Completes with no value when the light doesn't set the property.

        See `mln_map_get_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_light_property(property_name)

    def get_style_source(self, source_id: str) -> Future[StyleSourceInfo | None]:
        """Reads a source of the map's style.

        Completes with no value when the style has no source named
        `source_id`.

        See `mln_map_get_style_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_source(source_id),
            lambda value: _maybe(StyleSourceInfo._from_native, value),
        )

    def get_style_transition_options(self) -> Future[StyleTransitionOptions]:
        """Reads the default transition of the map's style.

        See `mln_map_get_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.get_style_transition_options(),
            lambda value: StyleTransitionOptions._from_native(value),
        )

    def get_style_url(self) -> Future[str]:
        """Reads the URL of the map's style.

        The URL is empty when the style came from
        `MapHandle.set_style_json()`.

        See `mln_map_get_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.get_style_url()

    def invalidate_custom_geometry_source_region(
        self, source_id: str, bounds: LatLngBounds
    ) -> Future[CommandCompletion]:
        """Makes a custom geometry source ask again for the tiles in `bounds`
        that the map still needs.

        See `mln_map_invalidate_custom_geometry_source_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom geometry source.
        """
        return self._native.invalidate_custom_geometry_source_region(source_id, bounds)

    def invalidate_custom_geometry_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> Future[CommandCompletion]:
        """Makes a custom geometry source ask for a tile again when the map
        needs it.

        See `mln_map_invalidate_custom_geometry_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom geometry source.
        """
        return self._native.invalidate_custom_geometry_source_tile(source_id, tile_id)

    def invalidate_custom_vector_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> Future[CommandCompletion]:
        """Makes a custom vector source ask for a tile again when the map needs
        it.

        See `mln_map_invalidate_custom_vector_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom vector source.
        """
        return self._native.invalidate_custom_vector_source_tile(source_id, tile_id)

    def lat_lng_bounds_for_camera(
        self, camera: CameraOptions | None = None
    ) -> Future[LatLngBounds]:
        """Computes the area that the map would show with `camera`.

        Camera fields that `camera` doesn't set come from the map's camera.
        The longitudes stay between -180 and 180.

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
        """Computes the area that the map would show with `camera`, without
        wrapping longitudes.

        Camera fields that `camera` doesn't set come from the map's camera.
        The longitudes can fall outside -180 to 180, so a view across the
        antimeridian gives one continuous range.

        See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lng_bounds_for_camera_unwrapped(camera),
            lambda value: LatLngBounds._from_native(value),
        )

    def list_style_layers(self) -> Future[tuple[StyleLayerInfo, ...]]:
        """Lists the layers of the map's style, from bottom to top.

        See `mln_map_list_style_layers` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return map_future(
            self._native.list_style_layers(),
            lambda value: tuple(StyleLayerInfo._from_native(item) for item in value),
        )

    def list_style_sources(self) -> Future[tuple[StyleSourceInfo, ...]]:
        """Lists the sources of the map's style.

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
        """Moves a layer of the map's style below the layer named
        `before_layer_id`, or above every layer when `before_layer_id` is
        empty.

        See `mln_map_move_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named `layer_id` or
          `before_layer_id`.
        """
        return self._native.move_style_layer(layer_id, before_layer_id)

    def close(self) -> Future[None]:
        """Releases the map.

        Detach the map's render session first.

        See `mln_map_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.close()

    def remove_feature_state(
        self, selector: FeatureStateSelector
    ) -> Future[CommandCompletion]:
        """Removes the feature state that `selector` selects.

        See `mln_map_remove_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.remove_feature_state(selector)

    def remove_style_image(self, image_id: str) -> Future[CommandCompletion]:
        """Removes an image from the map's style.

        See `mln_map_remove_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no image named `image_id`.
        """
        return self._native.remove_style_image(image_id)

    def remove_style_layer(self, layer_id: str) -> Future[CommandCompletion]:
        """Removes a layer from the map's style.

        See `mln_map_remove_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named `layer_id`.
        """
        return self._native.remove_style_layer(layer_id)

    def remove_style_source(self, source_id: str) -> Future[CommandCompletion]:
        """Removes a source from the map's style.

        See `mln_map_remove_style_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_STATE` when a layer still uses the source.
        """
        return self._native.remove_style_source(source_id)

    def request_repaint(self) -> Future[CommandCompletion]:
        """Renders a new frame of a continuous map, even when nothing changed.

        See `mln_map_request_repaint` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.request_repaint()

    def request_still_image(self) -> Future[None]:
        """Renders a still image of a static or tile map.

        The map's render session draws the image once every resource that the
        image needs has loaded.

        See `mln_map_request_still_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.CANCELLED` when the map is released before the image is
          done.
        - `Status.NATIVE_ERROR` when the style or a resource that the image
          needs fails to load.
        """
        return self._native.request_still_image()

    def resize(self, extent: LogicalExtent) -> Future[CommandCompletion]:
        """Sets the map's size.

        `extent` must keep the scale factor that the map was created with. A
        map with a render session takes its size from the session, so resize
        the session with `RenderSessionHandle.resize()` instead.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.resize(extent)

    def set_bounds(
        self, options: BoundOptions | None = None
    ) -> Future[CommandCompletion]:
        """Limits the camera's center to an area, and its zoom and pitch to
        ranges.

        Limits that `options` doesn't set keep their current values. When the
        camera is outside the new limits, it moves inside them.

        See `mln_map_set_bounds` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_bounds(options)

    def set_custom_geometry_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: Any
    ) -> Future[CommandCompletion]:
        """Sets the data of a tile in a custom geometry source.

        `data` is a GeoJSON feature collection. The source ignores data for a
        tile that it hasn't asked for, or has since cancelled.

        See `mln_map_set_custom_geometry_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom geometry source.
        """
        return self._native.set_custom_geometry_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_geometry_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> Future[CommandCompletion]:
        """Reports that a tile of a custom geometry source failed to load.

        See `mln_map_set_custom_geometry_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom geometry source.
        """
        return self._native.set_custom_geometry_source_tile_error(
            source_id, tile_id, message
        )

    def set_custom_vector_source_tile_data(
        self,
        source_id: str,
        tile_id: CanonicalTileId,
        encoding: StyleVectorTileEncoding,
        data: Buffer,
    ) -> Future[CommandCompletion]:
        """Sets the data of a tile in a custom vector source.

        `data` is the tile, encoded as `encoding`. The source ignores data
        for a tile that it hasn't asked for, or has since cancelled.

        See `mln_map_set_custom_vector_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom vector source.
        """
        return self._native.set_custom_vector_source_tile_data(
            source_id, tile_id, encoding, data
        )

    def set_custom_vector_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> Future[CommandCompletion]:
        """Reports that a tile of a custom vector source failed to load.

        See `mln_map_set_custom_vector_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom vector source.
        """
        return self._native.set_custom_vector_source_tile_error(
            source_id, tile_id, message
        )

    def set_debug_options(self, options: MapDebugFlag) -> Future[CommandCompletion]:
        """Draws debugging overlays, such as tile borders, on the map.

        `options` replaces the overlays set before.

        See `mln_map_set_debug_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_debug_options(options)

    def set_event_mask(self, mask: RuntimeEventFlag) -> Future[CommandCompletion]:
        """Selects the events that the map reports.

        See `mln_map_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_event_mask(mask)

    def set_feature_state(
        self, selector: FeatureStateSelector, state: Any
    ) -> Future[CommandCompletion]:
        """Merges `state` into the state of a feature.

        `state` is a JSON object. Its keys replace the same keys in the
        feature's state, and the other keys stay.

        See `mln_map_set_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_feature_state(selector, state)

    def set_free_camera_options(
        self, options: FreeCameraOptions | None = None
    ) -> Future[CommandCompletion]:
        """Places the camera at a position and orientation in space.

        The map adjusts them to a camera that it can show, and stops running
        camera animations.

        See `mln_map_set_free_camera_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_free_camera_options(options)

    def set_geo_json_source_data(
        self, source_id: str, data: GeoJsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Replaces the data of a GeoJSON source.

        `data` must have been prepared with the same options as the source.
        It can be released once this function returns.

        See `mln_map_set_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          GeoJSON source, or `data` was prepared with other options.
        """
        return self._native.set_geo_json_source_data(source_id, data._native)

    def set_geo_json_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> Future[CommandCompletion]:
        """Sets whether a GeoJSON source builds its tiles synchronously.

        This works like the synchronous tiling option of
        `GeoJsonSourceOptions`, but `enabled` can't turn off synchronous
        tiling that the source's options turned on.

        See `mln_map_set_geo_json_source_synchronous_tiling` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          GeoJSON source.
        """
        return self._native.set_geo_json_source_synchronous_tiling(source_id, enabled)

    def set_geo_json_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Makes a GeoJSON source load its data from `url`.

        See `mln_map_set_geo_json_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          GeoJSON source.
        """
        return self._native.set_geo_json_source_url(source_id, url)

    def set_global_state_property(
        self, property_name: str, value: Any
    ) -> Future[CommandCompletion]:
        """Sets a value that the style's `global-state`
        \\[expressions\\](https://maplibre.org/maplibre-style-spec/expressions/)
        read.

        See `mln_map_set_global_state_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_STATE` until the map has parsed its style, which
          `RuntimeEventType.MAP_STYLE_LOADED` reports, and again while it
          loads a new style URL.
        """
        return self._native.set_global_state_property(property_name, value)

    def set_image_source_coordinates(
        self, source_id: str, coordinates: Sequence[LatLng]
    ) -> Future[CommandCompletion]:
        """Moves the image of an image source.

        `coordinates` are the image's four corners, clockwise from the top
        left.

        See `mln_map_set_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't
          an image source.
        """
        return self._native.set_image_source_coordinates(source_id, coordinates)

    def set_image_source_image(
        self, source_id: str, image: PremultipliedRgba8Image
    ) -> Future[CommandCompletion]:
        """Replaces the image of an image source.

        See `mln_map_set_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't
          an image source.
        """
        return self._native.set_image_source_image(source_id, image)

    def set_image_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Makes an image source load its image from `url`.

        See `mln_map_set_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't
          an image source.
        """
        return self._native.set_image_source_url(source_id, url)

    def set_projection_mode(self, mode: ProjectionMode) -> Future[CommandCompletion]:
        """Sets whether the map draws in perspective or axonometric projection,
        and how much it skews the view.

        Settings that `mode` doesn't set keep their current values.

        See `mln_map_set_projection_mode` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_projection_mode(mode)

    def set_rendering_stats_view_enabled(
        self, enabled: bool
    ) -> Future[CommandCompletion]:
        """Shows or hides an overlay of rendering statistics on the map.

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
        """Adds an image to the map's style, or replaces the image named
        `image_id`.

        See `mln_map_set_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_image(image_id, image, options)

    def set_style_json(self, json: Any) -> Future[CommandCompletion]:
        """Loads the map's style from `json`.

        The style's sources, sprites, and glyphs load in the background.

        See `mln_map_set_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NATIVE_ERROR` when `json` isn't a valid style.
        """
        return self._native.set_style_json(json)

    def set_style_layer_property(
        self, layer_id: str, property_name: str, value: Any
    ) -> Future[CommandCompletion]:
        """Sets a property of a layer in the map's style.

        `property_name` is one of the layer's layout or paint
        \\[properties\\](https://maplibre.org/maplibre-style-spec/layers/), or
        `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null
        `value` resets any of these but `source` and `source-layer` to its
        default.

        See `mln_map_set_style_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named `layer_id`.
        """
        return self._native.set_style_layer_property(layer_id, property_name, value)

    def set_style_light_json(self, light_json: Any) -> Future[CommandCompletion]:
        """Replaces the
        \\[light\\](https://maplibre.org/maplibre-style-spec/light/) of the
        map's style.

        See `mln_map_set_style_light_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_light_json(light_json)

    def set_style_light_property(
        self, property_name: str, value: Any
    ) -> Future[CommandCompletion]:
        """Sets a property of the light of the map's style.

        See `mln_map_set_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_light_property(property_name, value)

    def set_style_source_volatile(
        self, source_id: str, is_volatile: bool
    ) -> Future[CommandCompletion]:
        """Sets whether a source is volatile.

        The map doesn't store the tiles of a volatile source in the
        persistent cache.

        See `mln_map_set_style_source_volatile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        """
        return self._native.set_style_source_volatile(source_id, is_volatile)

    def set_style_transition_options(
        self, options: StyleTransitionOptions | None = None
    ) -> Future[CommandCompletion]:
        """Sets the default transition of the map's style.

        See `mln_map_set_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.set_style_transition_options(options)

    def set_style_url(self, url: str) -> Future[CommandCompletion]:
        """Loads the map's style from `url`.

        The style loads in the background, and
        `RuntimeEventType.MAP_STYLE_LOADED` or
        `RuntimeEventType.MAP_LOADING_FAILED` reports how the load ends.
        Sources, layers, and images added before the style loads are lost
        when it loads.

        See `mln_map_set_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.set_style_url(url)

    def set_tile_options(
        self, options: MapTileOptions | None = None
    ) -> Future[CommandCompletion]:
        """Sets how the map chooses which tiles to load, and at what detail.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_tile_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_tile_options(options)

    def set_viewport_options(
        self, options: MapViewportOptions | None = None
    ) -> Future[CommandCompletion]:
        """Sets how the map orients and constrains its view.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_viewport_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_viewport_options(options)

    def update_camera(self, update: CameraUpdate) -> Future[CommandCompletion]:
        """Moves the camera, at once or with an animation.

        Camera fields that `update` doesn't set keep their values, and the
        update stops any running animation of the fields it sets. The
        function finishes when the map applies the update, before an
        animation ends. To learn when an animation ends, set an
        `CameraTransitionHandler` in its animation options.

        See `mln_map_update_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.update_camera(update)

    @property
    def discarding(self) -> MapHandleDiscarding:
        return MapHandleDiscarding(self)


class _MapProjectionHandleOperations(GeneratedOperations):
    _native: _native._MapProjectionHandle

    @classmethod
    def create(cls, view: MapView) -> MapProjectionHandle:
        """Creates a projection that converts coordinates for one view of a map.

        Create it from the `view` of an `MapSnapshot` or an
        `RenderFrameResult`. The projection doesn't follow the map, so create
        a new one after the map's camera or size changes.

        See `mln_map_projection_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return _adopt_value(
            _native._MapProjectionHandle.create(view), "MapProjectionHandle", None
        )

    def get_camera(self) -> CameraOptions:
        """Reads the projection's camera.

        See `mln_map_projection_get_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return CameraOptions._from_native(self._native.get_camera())

    def lat_lng_for_pixel(self, point: ScreenPoint) -> LatLng:
        """Finds the coordinate at a screen point.

        The longitude stays between -180 and 180.

        See `mln_map_projection_lat_lng_for_pixel` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return LatLng._from_native(self._native.lat_lng_for_pixel(point))

    def lat_lng_for_pixel_unwrapped(self, point: ScreenPoint) -> LatLng:
        """Finds the coordinate at a screen point, without wrapping its
        longitude.

        The longitude can fall outside -180 to 180, to show which copy of the
        world the point is in.

        See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return LatLng._from_native(self._native.lat_lng_for_pixel_unwrapped(point))

    def lat_lngs_for_pixels(self, points: Sequence[ScreenPoint]) -> tuple[LatLng, ...]:
        """Finds the coordinates at several screen points, as
        `MapProjectionHandle.lat_lng_for_pixel()` does.

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
        """Finds the coordinates at several screen points, as
        `MapProjectionHandle.lat_lng_for_pixel_unwrapped()` does.

        See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return tuple(
            LatLng._from_native(item)
            for item in self._native.lat_lngs_for_pixels_unwrapped(points)
        )

    def meters_per_pixel_at_latitude(self, latitude: float) -> float:
        """Computes how many meters one screen pixel covers at `latitude`, at
        the projection's zoom.

        The result ignores the camera's pitch.

        See `mln_map_projection_meters_per_pixel_at_latitude` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.meters_per_pixel_at_latitude(latitude)

    def pixel_for_lat_lng(self, coordinate: LatLng) -> ScreenPoint:
        """Finds the screen point of a coordinate.

        The point is in the copy of the world nearest the camera's center.

        See `mln_map_projection_pixel_for_lat_lng` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return ScreenPoint._from_native(self._native.pixel_for_lat_lng(coordinate))

    def pixels_for_lat_lngs(
        self, coordinates: Sequence[LatLng]
    ) -> tuple[ScreenPoint, ...]:
        """Finds the screen points of several coordinates, as
        `MapProjectionHandle.pixel_for_lat_lng()` does.

        See `mln_map_projection_pixels_for_lat_lngs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return tuple(
            ScreenPoint._from_native(item)
            for item in self._native.pixels_for_lat_lngs(coordinates)
        )

    def close(self) -> None:
        """Releases a projection.

        See `mln_map_projection_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.close()

    def set_camera(self, camera: CameraOptions | None = None) -> None:
        """Moves the projection's camera, without changing the map.

        Camera fields that `camera` doesn't set keep their values.

        See `mln_map_projection_set_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.set_camera(camera)


class _OffscreenDeviceHandleOperations(GeneratedOperations):
    _native: _native._OffscreenDeviceHandle

    @classmethod
    def create(
        cls, options: OffscreenDeviceOptions | None = None
    ) -> OffscreenDeviceHandle:
        """Creates a GPU device for rendering maps offscreen.

        `MapHandle.attach_offscreen()` creates a device of its own when it
        gets none. Create one to share it among the sessions on many maps, or
        to pick a GPU.

        See `mln_offscreen_device_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when this build's render backend can't create
          a device, or can't render with `client_api`.
        - `Status.NATIVE_ERROR` when `adapter_index` names no GPU, or the
          platform can't create the device.
        """
        return _adopt_value(
            _native._OffscreenDeviceHandle.create(options),
            "OffscreenDeviceHandle",
            None,
        )

    def close(self) -> None:
        """Releases an offscreen device.

        Render sessions on the device keep working until they detach.

        See `mln_offscreen_device_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
        """
        return self._native.close()


class _RenderSessionHandleOperations(GeneratedOperations):
    _native: _native._RenderSessionHandle

    def abandon(self) -> RenderAbandonResult:
        """Stops the session right away, without running its pending graphics
        work.

        Use it when the target's graphics objects are going away and a detach
        can't finish first. Pending work completes with `Status.TARGET_LOST`,
        and pending frame requests finish with
        `RenderResult.TARGET_NOT_READY`. Once the call returns, the library
        no longer uses your graphics objects, and you can attach another
        session to the map. Dispose the session afterward.

        See `mln_render_session_abandon` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderAbandonResult._from_native(self._native.abandon())

    def acquire_frame(self) -> AcquiredFrameHandle | None:
        """Takes a rendered frame from the session's texture ring, if one is
        ready.

        It takes the oldest frame that you haven't acquired. The session can
        render over a frame before you acquire it. While you hold a frame in
        every texture of the ring, new frames wait until you release one.

        See `mln_render_session_acquire_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session lacks
          `RenderSessionCapabilityFlag.FRAME_ACQUISITION`.
        - `Status.INVALID_STATE` when the session has failed.
        """
        return _maybe(
            lambda raw: _adopt_value(raw, "AcquiredFrameHandle", self),
            self._native.acquire_frame(),
        )

    def barrier(self) -> Future[None]:
        """Ends the session's pending frame requests and waits for its earlier
        work.

        Frame requests that haven't started, including those that wait for a
        map update, finish with `RenderResult.NO_UPDATE`.

        See `mln_render_session_barrier` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session loses its target before the
          barrier runs.
        """
        return self._native.barrier()

    def clear_data(self) -> Future[None]:
        """Clears the data that the session's renderer holds, such as loaded
        tiles, and draws the map again.

        See `mln_render_session_clear_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session loses its target before the
          work runs.
        """
        return self._native.clear_data()

    def detach(self) -> Future[None]:
        """Detaches the session from its map and frees its graphics objects.

        Release every acquired frame first. Pending frame requests finish
        with `RenderResult.TARGET_NOT_READY`. Once the detach completes, the
        library no longer uses your graphics objects, and you can attach
        another session to the map. Dispose the session afterward.

        See `mln_render_session_detach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session loses its target before the
          detach runs.
        """
        return self._native.detach()

    def close(self) -> Future[None]:
        """Releases the session in any state.

        An attached session driven by `RenderDriverKind.CORE_WORKER` detaches
        first unless you hold one of its frames. Any other session is
        abandoned as `RenderSessionHandle.abandon()` does. To free the
        graphics objects of a session driven by
        `RenderDriverKind.CALLER_GRAPHICS_THREAD`, detach it before you
        dispose it. Once the disposal completes, the library no longer uses
        your graphics objects.

        See `mln_render_session_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.close()

    def drain_frame_results(self) -> tuple[RenderFrameResult, ...]:
        """Takes the frame results that the session has queued.

        See `mln_render_session_drain_frame_results` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return tuple(
            RenderFrameResult._from_native(item)
            for item in self._native.drain_frame_results()
        )

    def dump_debug_logs(self) -> Future[None]:
        """Logs debugging information about the session's renderer.

        See `mln_render_session_dump_debug_logs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session loses its target before the
          work runs.
        """
        return self._native.dump_debug_logs()

    def get_capabilities(self) -> RenderSessionCapabilities:
        """Reads what the session supports.

        See `mln_render_session_get_capabilities` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderSessionCapabilities._from_native(self._native.get_capabilities())

    def get_failure(self) -> None:
        """Reports the error that made the session fail.

        The call fails with that error, and succeeds while the session hasn't
        failed.

        See `mln_render_session_get_failure` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session failed because it lost its
          target.
        - `Status.NATIVE_ERROR` when the session failed for another reason.
        """
        return self._native.get_failure()

    def get_snapshot(self) -> RenderSessionSnapshot:
        """Reads the session's current state.

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
        """Reads a value that a source computes for one of its features.

        A clustered GeoJSON source answers the `supercluster` extension for a
        cluster feature. Its `extension_field` is `children` for the
        cluster's child features, `leaves` for the points in the cluster, or
        `expansion-zoom` for the zoom level where the cluster splits. For
        `leaves`, `arguments` can set `limit` and `offset`. Without `limit`,
        the result has up to 10 points and ignores `offset`. The result is
        JSON: a GeoJSON feature collection, a number, or null when the source
        has no answer.

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
        """Finds the features that the session drew in an area of the screen.

        The query reads the last frame that the session rendered. A feature
        that several layers draw appears once for each layer.

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
        """Finds the features of the source named `source_id`.

        The query covers only the tiles that the source has loaded for the
        current view. A source loads tiles only while a visible layer uses
        it.

        See `mln_render_session_query_source_features` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
        """
        return map_future(
            self._native.query_source_features(source_id, options),
            lambda value: tuple(SourceFeature._from_native(item) for item in value),
        )

    def read_texture(self) -> Future[TextureReadbackResult]:
        """Reads the pixels of the last frame that the session rendered.

        See `mln_render_session_read_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session's capabilities lack
          `RenderSessionCapabilityFlag.READBACK`.
        - `Status.INVALID_STATE` when the session hasn't rendered a frame
          since it attached, resized, or changed its target.
        """
        return map_future(
            self._native.read_texture(),
            lambda value: TextureReadbackResult._from_native(value),
        )

    def reduce_memory_use(self) -> Future[None]:
        """Frees memory that the session's renderer can rebuild, such as caches.

        See `mln_render_session_reduce_memory_use` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session loses its target before the
          work runs.
        """
        return self._native.reduce_memory_use()

    def release_target(self) -> Future[None]:
        """Lets go of the session's surface or textures, and keeps the rest of
        the session.

        Use it when the platform takes a surface away, such as when your app
        goes to the background. Release every acquired frame first. Until you
        give the session a new target, frame requests finish with
        `RenderResult.TARGET_NOT_READY`. Once the release completes, you can
        destroy the old target.

        See `mln_render_session_release_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders into a ring that it
          created.
        - `Status.UNSUPPORTED` when the target is a WebGL canvas.
        - `Status.TARGET_LOST` when the session loses its target before the
          release runs.
        """
        return self._native.release_target()

    def request_frame(self, demand: FrameDemand) -> None:
        """Asks the session to render a frame.

        Call it once the attach completes. Every request finishes with one
        frame result that carries its token, and
        `RenderSessionHandle.drain_frame_results()` takes it. A request
        without `FrameDemandFlag.FORCE` replaces each pending request with
        the same flags.

        See `mln_render_session_request_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the session has failed.
        """
        return self._native.request_frame(demand)

    def resize(self, extent: LogicalExtent) -> Future[CommandCompletion]:
        """Resizes the session's target and its map.

        `extent` must keep the session's scale factor. To change the scale
        factor, give the session a new target or attach a new session.
        Release every acquired frame first.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders into a borrowed ring.
          Give it new textures of the new size instead.
        - `Status.TARGET_LOST` when the session loses its target before the
          resize applies.
        """
        return self._native.resize(extent)

    def service_driver_work(self) -> None:
        """Runs the session's ready graphics work on your graphics thread.

        Use it with `RenderDriverKind.CALLER_GRAPHICS_THREAD`, and call it
        from the same thread every time. Call it each time
        `RenderSessionAttachOptions.driver_work_wake` is called, even while
        you request no frames, and until a detach completes. It returns once
        no work is ready.

        See `mln_render_session_service_driver_work` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.service_driver_work()

    def set_metal_borrowed_texture_target(
        self, descriptor: MetalBorrowedTextureDescriptor
    ) -> Future[None]:
        """Replaces the Metal textures that a borrowed ring renders into.

        Use it to resize the ring, or to give the session textures again
        after `RenderSessionHandle.release_target()`. Release every acquired
        frame first. The new ring must have as many textures as the old one,
        and the same device and command queue. Once the replacement
        completes, the session no longer uses the old textures.

        See `mln_render_session_set_metal_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the new textures have a different pixel
          format than the old ones. Attach a new session to change it.
        """
        return self._native.set_metal_borrowed_texture_target(descriptor)

    def set_metal_surface_target(
        self, descriptor: MetalSurfaceDescriptor
    ) -> Future[None]:
        """Replaces the Metal layer that the session renders to.

        Use it to give the session a layer again after
        `RenderSessionHandle.release_target()`, or to move it to another
        layer. The new layer must use the same device. Once the replacement
        completes, the session no longer uses the old layer.

        See `mln_render_session_set_metal_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.set_metal_surface_target(descriptor)

    def set_open_gl_borrowed_texture_target(
        self, descriptor: OpenGlBorrowedTextureDescriptor
    ) -> Future[None]:
        """Replaces the OpenGL textures that a borrowed ring renders into.

        Use it to resize the ring, or to give the session textures again
        after `RenderSessionHandle.release_target()`. Release every acquired
        frame first. The new ring must have as many textures as the old one,
        and the same context. Once the replacement completes, the session no
        longer uses the old textures.

        See `mln_render_session_set_open_gl_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.set_open_gl_borrowed_texture_target(descriptor)

    def set_open_gl_surface_target(
        self, descriptor: OpenGlSurfaceDescriptor
    ) -> Future[None]:
        """Replaces the OpenGL surface that the session presents to.

        Use it to give the session a surface again after
        `RenderSessionHandle.release_target()`, or to move it to another
        surface. The new surface must use the same context. Once the
        replacement completes, the session no longer uses the old surface.

        See `mln_render_session_set_open_gl_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.set_open_gl_surface_target(descriptor)

    def set_vulkan_borrowed_texture_target(
        self, descriptor: VulkanBorrowedTextureDescriptor
    ) -> Future[None]:
        """Replaces the Vulkan images that a borrowed ring renders into.

        Use it to resize the ring, or to give the session images again after
        `RenderSessionHandle.release_target()`. Release every acquired frame
        first. The new ring must have as many images as the old one, and the
        same context. Once the replacement completes, the session no longer
        uses the old images.

        See `mln_render_session_set_vulkan_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the new ring has a different `format`,
          `final_layout`, or `owner` than the old one. Attach a new session
          to change them.
        """
        return self._native.set_vulkan_borrowed_texture_target(descriptor)

    def set_vulkan_surface_target(
        self, descriptor: VulkanSurfaceDescriptor
    ) -> Future[None]:
        """Replaces the Vulkan surface that the session presents to.

        Use it to give the session a surface again after
        `RenderSessionHandle.release_target()`, or to move it to another
        surface. The new surface must use the same context. Once the
        replacement completes, the session no longer uses the old surface.

        See `mln_render_session_set_vulkan_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the new surface needs a different color
          format or pre-rotation support than the old one. Attach a new
          session instead.
        """
        return self._native.set_vulkan_surface_target(descriptor)

    @property
    def discarding(self) -> RenderSessionHandleDiscarding:
        return RenderSessionHandleDiscarding(self)


class _ResourceRequestHandleOperations(GeneratedOperations):
    _native: _native._ResourceRequestHandle

    def complete(self, response: ResourceResponse) -> None:
        """Answers a resource request.

        See `mln_resource_request_complete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the library cancelled the request.
        """
        return self._native.complete(response)

    def is_cancelled(self) -> bool:
        """Reads whether the library cancelled a resource request.

        See `mln_resource_request_is_cancelled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.is_cancelled()

    def pass_through(self) -> None:
        """Hands a resource request back to the library to load as usual.

        See `mln_resource_request_pass_through` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the library cancelled the request.
        """
        return self._native.pass_through()

    def close(self) -> None:
        """Releases a resource request.

        See `mln_resource_request_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.close()

    def set_cancel_callback(self, handler: ResourceRequestCancelHandler) -> bool:
        """Calls a callback when the library cancels a resource request.

        The callback never runs after the request is answered. A request
        takes one cancel callback. When the library has already cancelled the
        request, the call reports that instead and the callback never runs.

        See `mln_resource_request_set_cancel_callback` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_cancel_callback(handler)


class _ResourceTransformRequestHandleOperations(GeneratedOperations):
    _native: _native._ResourceTransformRequestHandle

    def complete(self, url: str) -> None:
        """Answers a resource transform request with the URL to load instead.

        `url` must not be empty.

        See `mln_resource_transform_request_complete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.complete(url)

    def close(self) -> None:
        """Releases a resource transform request.

        See `mln_resource_transform_request_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.close()


class _RuntimeHandleOperations(GeneratedOperations):
    _native: _native._RuntimeHandle

    def clear_event_handler(self) -> Future[CommandCompletion]:
        """Stops sending the events of the runtime and its maps to a callback.

        See `mln_runtime_clear_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_event_handler()

    def clear_http_header_transform(self) -> Future[CommandCompletion]:
        """Stops adding headers to the runtime's HTTP requests.

        See `mln_runtime_clear_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_http_header_transform()

    def clear_resource_provider(self) -> Future[CommandCompletion]:
        """Removes the runtime's resource provider, so the library loads every
        request itself.

        See `mln_runtime_clear_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_resource_provider()

    def clear_resource_transform(self) -> Future[CommandCompletion]:
        """Stops rewriting the URLs of the runtime's network requests.

        See `mln_runtime_clear_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.clear_resource_transform()

    @classmethod
    def create(cls, options: RuntimeOptions | None = None) -> RuntimeHandle:
        """Creates a runtime.

        A runtime holds maps and the resource loading, cache, and offline
        regions that they share. In a browser, create runtimes on a Web
        Worker, not on the main thread.

        See `mln_runtime_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return _adopt_value(
            _native._RuntimeHandle.create(options), "RuntimeHandle", None
        )

    def create_map(self, options: MapOptions) -> RuntimeCreateMapResult:
        """Creates a map on `runtime`.

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
        """Creates an offline region in the runtime's cache file.

        `metadata` holds any bytes to store with the region, such as its
        name. The region doesn't download until
        `RuntimeHandle.set_offline_region_download_state()` makes its
        download active.

        See `mln_runtime_create_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.create_offline_region(definition, metadata),
            lambda value: OfflineRegionInfo._from_native(value),
        )

    def delete_offline_region(self, region_id: int) -> Future[None]:
        """Deletes the offline region `region_id` from the runtime's cache file.

        See `mln_runtime_delete_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
        """
        return self._native.delete_offline_region(region_id)

    def get_event_mask(self) -> RuntimeEventFlag:
        """Reads the runtime's event mask.

        See `mln_runtime_get_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return RuntimeEventFlag(self._native.get_event_mask())

    def get_offline_region(self, region_id: int) -> Future[OfflineRegionInfo | None]:
        """Reads the offline region `region_id`.

        The result is null when the cache file has no region with that ID.

        See `mln_runtime_get_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return map_future(
            self._native.get_offline_region(region_id),
            lambda value: _maybe(OfflineRegionInfo._from_native, value),
        )

    def get_offline_region_status(self, region_id: int) -> Future[OfflineRegionStatus]:
        """Reads the download progress of the offline region `region_id`.

        See `mln_runtime_get_offline_region_status` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
        """
        return map_future(
            self._native.get_offline_region_status(region_id),
            lambda value: OfflineRegionStatus._from_native(value),
        )

    def invalidate_offline_region(self, region_id: int) -> Future[None]:
        """Makes maps check each resource of the offline region `region_id` with
        the server before they use it.

        The region keeps its resources.

        See `mln_runtime_invalidate_offline_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
        """
        return self._native.invalidate_offline_region(region_id)

    def list_offline_regions(self) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Lists the offline regions in the runtime's cache file.

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
        """Copies the offline regions of another cache file into the runtime's
        cache file.

        A region whose definition and metadata match one that the runtime's
        file already has isn't copied again. The result lists the runtime's
        copy of each region in the other file.

        See `mln_runtime_merge_offline_regions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `side_database_path` isn't a
          readable database file.
        - `Status.NATIVE_ERROR` when the merge would exceed the limit on
          tiles from MapLibre's demo tile server that
          `RuntimeEventType.OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`
          describes.
        """
        return map_future(
            self._native.merge_offline_regions(side_database_path),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def close(self) -> Future[None]:
        """Releases a runtime.

        Release the runtime's maps first.

        See `mln_runtime_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.close()

    def run_ambient_cache_operation(
        self, operation: AmbientCacheOperation
    ) -> Future[None]:
        """Runs an action on the runtime's ambient cache.

        See `mln_runtime_run_ambient_cache_operation` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.run_ambient_cache_operation(operation)

    def set_event_handler(
        self, handler: RuntimeEventHandler
    ) -> Future[CommandCompletion]:
        """Sends the events of the runtime and its maps to a callback.

        The callback replaces the one set before.

        See `mln_runtime_set_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_event_handler(handler)

    def set_event_mask(self, mask: RuntimeEventFlag) -> None:
        """Selects the runtime's own events that reach the event callback.

        Each map selects its events with `MapHandle.set_event_mask()`
        instead. A runtime starts with every event selected.

        See `mln_runtime_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_event_mask(mask)

    def set_http_header_transform(
        self, transform: HttpHeaderTransform
    ) -> Future[CommandCompletion]:
        """Adds headers to the runtime's HTTP requests.

        The transform replaces the one set before. In a browser or on
        OpenHarmony, serve the requests with
        `RuntimeHandle.set_resource_provider()` instead.

        See `mln_runtime_set_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the library runs in a browser or on
          OpenHarmony.
        """
        return self._native.set_http_header_transform(transform)

    def set_maximum_ambient_cache_size(self, size: int) -> Future[None]:
        """Limits the size of the runtime's ambient cache.

        `size` is in bytes. Offline regions count toward it, but the library
        evicts only ambient cache resources to stay within it.

        See `mln_runtime_set_maximum_ambient_cache_size` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_maximum_ambient_cache_size(size)

    def set_offline_region_download_state(
        self, region_id: int, state: OfflineRegionDownloadState
    ) -> Future[None]:
        """Starts or pauses the download of the offline region `region_id`.

        A download stops when the runtime closes.

        See `mln_runtime_set_offline_region_download_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
        """
        return self._native.set_offline_region_download_state(region_id, state)

    def set_offline_region_observed(
        self, region_id: int, observed: bool
    ) -> Future[None]:
        """Sets whether the runtime reports the download progress of the offline
        region `region_id` as events.

        See `mln_runtime_set_offline_region_observed` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
        """
        return self._native.set_offline_region_observed(region_id, observed)

    def set_resource_provider(
        self, provider: ResourceProvider
    ) -> Future[CommandCompletion]:
        """Serves the runtime's network requests through a callback.

        The provider receives the network requests that its routes match, and
        the library loads the rest as usual. A provider can serve URL schemes
        that the library can't load itself. The provider replaces the one set
        before.

        See `mln_runtime_set_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_resource_provider(provider)

    def set_resource_transform(
        self, transform: ResourceTransform
    ) -> Future[CommandCompletion]:
        """Rewrites the URLs of the runtime's network requests through a
        callback.

        The transform replaces the one set before.

        See `mln_runtime_set_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_resource_transform(transform)

    def update_offline_region_metadata(
        self, region_id: int, metadata: Buffer
    ) -> Future[OfflineRegionInfo]:
        """Replaces the metadata of the offline region `region_id`.

        See `mln_runtime_update_offline_region_metadata` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
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

    @classmethod
    def create(
        cls, expression: Any, options: StyleExpressionOptions | None = None
    ) -> StyleExpressionHandle:
        """Parses a style
        \\[expression\\](https://maplibre.org/maplibre-style-spec/expressions/)
        to evaluate without a map.

        See `mln_style_expression_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `expression` isn't a valid
          expression, or its result can't have the expected type.
        """
        return _adopt_value(
            _native._StyleExpressionHandle.create(expression, options),
            "StyleExpressionHandle",
            None,
        )

    def evaluate(self, inputs: StyleExpressionInputs | None = None) -> Any:
        """Evaluates a style expression and writes its result as JSON.

        A number in the result that isn't finite becomes null.

        See `mln_style_expression_evaluate` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the expression fails to evaluate,
          such as when it reads an input that `inputs` doesn't set.
        """
        return self._native.evaluate(inputs)

    def get_info(self) -> StyleExpressionInfo:
        """Reads what a style expression's result depends on.

        See `mln_style_expression_get_info` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
        """
        return StyleExpressionInfo._from_native(self._native.get_info())

    def close(self) -> None:
        """Releases a parsed style expression.

        See `mln_style_expression_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
        """
        return self._native.close()


def init_android(jni_env: int, context: int) -> None:
    """Gives the library the Java VM and the app context that it needs on
    Android.

    Call it before creating a runtime. `jni_env` is the calling thread's
    `JNIEnv*`, and `context` is an Android `Context`, such as the
    application.

    See `mln_android_init` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
    """
    return _native.init_android(jni_env, context)


def c_version() -> int:
    """Returns the API epoch of the library.

    Headers and a library with different epochs aren't compatible.

    See `mln_c_version` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """
    return _native.c_version()


def lat_lng_for_projected_meters(meters: ProjectedMeters) -> LatLng:
    """Converts meters in the Web Mercator projection to a coordinate.

    See `mln_lat_lng_for_projected_meters` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    """
    return LatLng._from_native(_native.lat_lng_for_projected_meters(meters))


def clear_log_callback() -> None:
    """Sends log messages to the platform's log again, instead of a
    callback.

    See `mln_log_clear_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.clear_log_callback()


def set_log_async_severity_mask(mask: LogSeverityFlag) -> None:
    """Selects the severities that the library logs on a background thread.

    The library logs other severities on the thread that logged them.
    `mask` applies to every runtime in the process.

    See `mln_log_set_async_severity_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.set_log_async_severity_mask(mask)


def set_log_callback(handler: LogHandler) -> None:
    """Sends the log messages of every runtime in the process to a callback.

    The callback replaces the one set before.

    The callback runs on the binding's dispatcher thread, after native
    queues the call.

    See `mln_log_set_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """
    return _native.set_log_callback(handler)


def logical_extent_physical_size(
    extent: LogicalExtent,
) -> LogicalExtentPhysicalSizeResult:
    """Computes the size in pixels of a target with a logical size and scale
    factor.

    See `mln_logical_extent_physical_size` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """
    raw = _native.logical_extent_physical_size(extent)
    return LogicalExtentPhysicalSizeResult(raw["width"], raw["height"])


def get_network_status() -> NetworkStatus:
    """Reads whether the library may use the network.

    See `mln_network_get_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return NetworkStatus(_native.get_network_status())


def set_network_status(status: NetworkStatus) -> None:
    """Tells the library whether it may use the network.

    While the status is `NetworkStatus.OFFLINE`, the library starts no
    network requests. The requests that wait start when the status
    returns to `NetworkStatus.ONLINE`. The status starts as online and
    applies to every runtime in the process.

    See `mln_network_set_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """
    return _native.set_network_status(status)


def open_gl_supported_context_provider_mask() -> OpenGlContextProviderFlag:
    """Reports the OpenGL context platforms that this build of the library
    has.

    See `mln_open_gl_supported_context_provider_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """
    return OpenGlContextProviderFlag(_native.open_gl_supported_context_provider_mask())


def get_plugin_register_function_v1() -> int:
    """Returns the function that registers a plugin's style layer types.

    Pass the function to the plugin's registration entry point before
    loading a style that uses the plugin's layer types. Plugin callbacks
    run on library threads and can keep running while the process exits,
    so they must not use state that the process destroys at exit.

    See `mln_plugin_get_register_function_v1` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
    """
    return _native.get_plugin_register_function_v1()


def projected_meters_for_lat_lng(coordinate: LatLng) -> ProjectedMeters:
    """Converts a coordinate to meters in the Web Mercator projection.

    See `mln_projected_meters_for_lat_lng` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    """
    return ProjectedMeters._from_native(
        _native.projected_meters_for_lat_lng(coordinate)
    )


def supported_render_backend_mask() -> RenderBackendFlag:
    """Returns the render backends that this build of the library draws
    with.

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
        self, source_id: str, options: CustomGeometrySourceOptions
    ) -> None:
        """Adds a source whose GeoJSON tiles come from callbacks.

        See `mln_map_add_custom_geometry_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_custom_geometry_source(source_id, options)

    def add_custom_vector_source(
        self, source_id: str, options: CustomVectorSourceOptions
    ) -> None:
        """Adds a source whose vector tiles come from callbacks.

        See `mln_map_add_custom_vector_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_custom_vector_source(source_id, options)

    def add_geo_json_source_data(
        self, source_id: str, data: GeoJsonSourceDataHandle
    ) -> None:
        """Adds a GeoJSON source with prepared data.

        The source uses the options that `data` was prepared with. `data` can
        be released once this function returns.

        See `mln_map_add_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_geo_json_source_data(source_id, data._native)

    def add_geo_json_source_url(
        self, source_id: str, url: str, options: GeoJsonSourceOptions | None = None
    ) -> None:
        """Adds a GeoJSON source that loads its data from `url`.

        See `mln_map_add_geo_json_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_geo_json_source_url(source_id, url, options)

    def add_image_source_image(
        self,
        source_id: str,
        coordinates: Sequence[LatLng],
        image: PremultipliedRgba8Image,
    ) -> None:
        """Adds an image source that shows `image`.

        `coordinates` are the image's four corners, clockwise from the top
        left.

        See `mln_map_add_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_image_source_image(
            source_id, coordinates, image
        )

    def add_image_source_url(
        self, source_id: str, coordinates: Sequence[LatLng], url: str
    ) -> None:
        """Adds an image source that loads its image from `url`.

        `coordinates` are the image's four corners, clockwise from the top
        left.

        See `mln_map_add_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_image_source_url(source_id, coordinates, url)

    def add_style_layer_json(self, layer_json: Any, before_layer_id: str) -> None:
        """Adds a layer to the map's style.

        `layer_json` is a
        \\[layer\\](https://maplibre.org/maplibre-style-spec/layers/) object.
        The layer goes below the layer named `before_layer_id`, or above
        every layer when `before_layer_id` is empty.

        See `mln_map_add_style_layer_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_style_layer_json(layer_json, before_layer_id)

    def add_style_source_json(self, source_id: str, source_json: Any) -> None:
        """Adds a source to the map's style.

        `source_json` is a
        \\[source\\](https://maplibre.org/maplibre-style-spec/sources/) object.

        See `mln_map_add_style_source_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_style_source_json(source_id, source_json)

    def apply_camera_delta(self, delta: CameraDelta) -> None:
        """Moves the camera relative to where it is, as a gesture does.

        `delta` can pan the map's content by an offset on the screen, scale
        the zoom, and add to the bearing and pitch. An anchor keeps one
        screen point in place while the scale, bearing, or pitch changes. An
        anchor needs one of those changes, and combines with an offset only
        when the delta has no animation. The function finishes when the map
        applies the delta, before an animation ends.

        See `mln_map_apply_camera_delta` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_apply_camera_delta(delta)

    def begin_command_group(self) -> None:
        """Holds back the map's changes from rendering until
        `MapHandle.end_command_group()`, so they show in the same frame.

        Groups nest, and the changes render when the outermost group ends.

        See `mln_map_begin_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_begin_command_group()

    def cancel_camera_transition(self, transition_id: int) -> None:
        """Stops the running camera animations whose animation options set
        `transition_id`.

        Other animations keep running. The end handler of each stopped
        animation reports `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_camera_transition` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_cancel_camera_transition(transition_id)

    def cancel_transitions(self) -> None:
        """Stops every running camera animation where it is.

        The end handler of each stopped animation reports
        `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_transitions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_cancel_transitions()

    def dump_debug_logs(self) -> None:
        """Logs debugging information about the map's style and sources.

        See `mln_map_dump_debug_logs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_dump_debug_logs()

    def end_command_group(self) -> None:
        """Ends the group that `MapHandle.begin_command_group()` began.

        See `mln_map_end_command_group` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_end_command_group()

    def invalidate_custom_geometry_source_region(
        self, source_id: str, bounds: LatLngBounds
    ) -> None:
        """Makes a custom geometry source ask again for the tiles in `bounds`
        that the map still needs.

        See `mln_map_invalidate_custom_geometry_source_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_invalidate_custom_geometry_source_region(
            source_id, bounds
        )

    def invalidate_custom_geometry_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> None:
        """Makes a custom geometry source ask for a tile again when the map
        needs it.

        See `mln_map_invalidate_custom_geometry_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_invalidate_custom_geometry_source_tile(
            source_id, tile_id
        )

    def invalidate_custom_vector_source_tile(
        self, source_id: str, tile_id: CanonicalTileId
    ) -> None:
        """Makes a custom vector source ask for a tile again when the map needs
        it.

        See `mln_map_invalidate_custom_vector_source_tile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_invalidate_custom_vector_source_tile(
            source_id, tile_id
        )

    def move_style_layer(self, layer_id: str, before_layer_id: str) -> None:
        """Moves a layer of the map's style below the layer named
        `before_layer_id`, or above every layer when `before_layer_id` is
        empty.

        See `mln_map_move_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_move_style_layer(layer_id, before_layer_id)

    def remove_feature_state(self, selector: FeatureStateSelector) -> None:
        """Removes the feature state that `selector` selects.

        See `mln_map_remove_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_remove_feature_state(selector)

    def remove_style_image(self, image_id: str) -> None:
        """Removes an image from the map's style.

        See `mln_map_remove_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_remove_style_image(image_id)

    def remove_style_layer(self, layer_id: str) -> None:
        """Removes a layer from the map's style.

        See `mln_map_remove_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_remove_style_layer(layer_id)

    def remove_style_source(self, source_id: str) -> None:
        """Removes a source from the map's style.

        See `mln_map_remove_style_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_remove_style_source(source_id)

    def request_repaint(self) -> None:
        """Renders a new frame of a continuous map, even when nothing changed.

        See `mln_map_request_repaint` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_request_repaint()

    def resize(self, extent: LogicalExtent) -> None:
        """Sets the map's size.

        `extent` must keep the scale factor that the map was created with. A
        map with a render session takes its size from the session, so resize
        the session with `RenderSessionHandle.resize()` instead.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_resize(extent)

    def set_bounds(self, options: BoundOptions | None = None) -> None:
        """Limits the camera's center to an area, and its zoom and pitch to
        ranges.

        Limits that `options` doesn't set keep their current values. When the
        camera is outside the new limits, it moves inside them.

        See `mln_map_set_bounds` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_bounds(options)

    def set_custom_geometry_source_tile_data(
        self, source_id: str, tile_id: CanonicalTileId, data: Any
    ) -> None:
        """Sets the data of a tile in a custom geometry source.

        `data` is a GeoJSON feature collection. The source ignores data for a
        tile that it hasn't asked for, or has since cancelled.

        See `mln_map_set_custom_geometry_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_geometry_source_tile_data(
            source_id, tile_id, data
        )

    def set_custom_geometry_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> None:
        """Reports that a tile of a custom geometry source failed to load.

        See `mln_map_set_custom_geometry_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_geometry_source_tile_error(
            source_id, tile_id, message
        )

    def set_custom_vector_source_tile_data(
        self,
        source_id: str,
        tile_id: CanonicalTileId,
        encoding: StyleVectorTileEncoding,
        data: Buffer,
    ) -> None:
        """Sets the data of a tile in a custom vector source.

        `data` is the tile, encoded as `encoding`. The source ignores data
        for a tile that it hasn't asked for, or has since cancelled.

        See `mln_map_set_custom_vector_source_tile_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_vector_source_tile_data(
            source_id, tile_id, encoding, data
        )

    def set_custom_vector_source_tile_error(
        self, source_id: str, tile_id: CanonicalTileId, message: str
    ) -> None:
        """Reports that a tile of a custom vector source failed to load.

        See `mln_map_set_custom_vector_source_tile_error` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_custom_vector_source_tile_error(
            source_id, tile_id, message
        )

    def set_debug_options(self, options: MapDebugFlag) -> None:
        """Draws debugging overlays, such as tile borders, on the map.

        `options` replaces the overlays set before.

        See `mln_map_set_debug_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_debug_options(options)

    def set_event_mask(self, mask: RuntimeEventFlag) -> None:
        """Selects the events that the map reports.

        See `mln_map_set_event_mask` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_event_mask(mask)

    def set_feature_state(self, selector: FeatureStateSelector, state: Any) -> None:
        """Merges `state` into the state of a feature.

        `state` is a JSON object. Its keys replace the same keys in the
        feature's state, and the other keys stay.

        See `mln_map_set_feature_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_feature_state(selector, state)

    def set_free_camera_options(self, options: FreeCameraOptions | None = None) -> None:
        """Places the camera at a position and orientation in space.

        The map adjusts them to a camera that it can show, and stops running
        camera animations.

        See `mln_map_set_free_camera_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_free_camera_options(options)

    def set_geo_json_source_data(
        self, source_id: str, data: GeoJsonSourceDataHandle
    ) -> None:
        """Replaces the data of a GeoJSON source.

        `data` must have been prepared with the same options as the source.
        It can be released once this function returns.

        See `mln_map_set_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geo_json_source_data(source_id, data._native)

    def set_geo_json_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> None:
        """Sets whether a GeoJSON source builds its tiles synchronously.

        This works like the synchronous tiling option of
        `GeoJsonSourceOptions`, but `enabled` can't turn off synchronous
        tiling that the source's options turned on.

        See `mln_map_set_geo_json_source_synchronous_tiling` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geo_json_source_synchronous_tiling(
            source_id, enabled
        )

    def set_geo_json_source_url(self, source_id: str, url: str) -> None:
        """Makes a GeoJSON source load its data from `url`.

        See `mln_map_set_geo_json_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geo_json_source_url(source_id, url)

    def set_global_state_property(self, property_name: str, value: Any) -> None:
        """Sets a value that the style's `global-state`
        \\[expressions\\](https://maplibre.org/maplibre-style-spec/expressions/)
        read.

        See `mln_map_set_global_state_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_global_state_property(property_name, value)

    def set_image_source_coordinates(
        self, source_id: str, coordinates: Sequence[LatLng]
    ) -> None:
        """Moves the image of an image source.

        `coordinates` are the image's four corners, clockwise from the top
        left.

        See `mln_map_set_image_source_coordinates` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_image_source_coordinates(
            source_id, coordinates
        )

    def set_image_source_image(
        self, source_id: str, image: PremultipliedRgba8Image
    ) -> None:
        """Replaces the image of an image source.

        See `mln_map_set_image_source_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_image_source_image(source_id, image)

    def set_image_source_url(self, source_id: str, url: str) -> None:
        """Makes an image source load its image from `url`.

        See `mln_map_set_image_source_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_image_source_url(source_id, url)

    def set_projection_mode(self, mode: ProjectionMode) -> None:
        """Sets whether the map draws in perspective or axonometric projection,
        and how much it skews the view.

        Settings that `mode` doesn't set keep their current values.

        See `mln_map_set_projection_mode` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_projection_mode(mode)

    def set_rendering_stats_view_enabled(self, enabled: bool) -> None:
        """Shows or hides an overlay of rendering statistics on the map.

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
        """Adds an image to the map's style, or replaces the image named
        `image_id`.

        See `mln_map_set_style_image` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_image(image_id, image, options)

    def set_style_json(self, json: Any) -> None:
        """Loads the map's style from `json`.

        The style's sources, sprites, and glyphs load in the background.

        See `mln_map_set_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_style_json(json)

    def set_style_layer_property(
        self, layer_id: str, property_name: str, value: Any
    ) -> None:
        """Sets a property of a layer in the map's style.

        `property_name` is one of the layer's layout or paint
        \\[properties\\](https://maplibre.org/maplibre-style-spec/layers/), or
        `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null
        `value` resets any of these but `source` and `source-layer` to its
        default.

        See `mln_map_set_style_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_layer_property(
            layer_id, property_name, value
        )

    def set_style_light_json(self, light_json: Any) -> None:
        """Replaces the
        \\[light\\](https://maplibre.org/maplibre-style-spec/light/) of the
        map's style.

        See `mln_map_set_style_light_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_light_json(light_json)

    def set_style_light_property(self, property_name: str, value: Any) -> None:
        """Sets a property of the light of the map's style.

        See `mln_map_set_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_light_property(property_name, value)

    def set_style_source_volatile(self, source_id: str, is_volatile: bool) -> None:
        """Sets whether a source is volatile.

        The map doesn't store the tiles of a volatile source in the
        persistent cache.

        See `mln_map_set_style_source_volatile` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_source_volatile(source_id, is_volatile)

    def set_style_transition_options(
        self, options: StyleTransitionOptions | None = None
    ) -> None:
        """Sets the default transition of the map's style.

        See `mln_map_set_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_transition_options(options)

    def set_style_url(self, url: str) -> None:
        """Loads the map's style from `url`.

        The style loads in the background, and
        `RuntimeEventType.MAP_STYLE_LOADED` or
        `RuntimeEventType.MAP_LOADING_FAILED` reports how the load ends.
        Sources, layers, and images added before the style loads are lost
        when it loads.

        See `mln_map_set_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_style_url(url)

    def set_tile_options(self, options: MapTileOptions | None = None) -> None:
        """Sets how the map chooses which tiles to load, and at what detail.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_tile_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_tile_options(options)

    def set_viewport_options(self, options: MapViewportOptions | None = None) -> None:
        """Sets how the map orients and constrains its view.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_viewport_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_viewport_options(options)

    def update_camera(self, update: CameraUpdate) -> None:
        """Moves the camera, at once or with an animation.

        Camera fields that `update` doesn't set keep their values, and the
        update stops any running animation of the fields it sets. The
        function finishes when the map applies the update, before an
        animation ends. To learn when an animation ends, set an
        `CameraTransitionHandler` in its animation options.

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
        """Resizes the session's target and its map.

        `extent` must keep the session's scale factor. To change the scale
        factor, give the session a new target or attach a new session.
        Release every acquired frame first.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders into a borrowed ring.
          Give it new textures of the new size instead.
        """
        return self._native._discard_resize(extent)


class RuntimeHandleDiscarding:
    __slots__ = ("_native",)
    _native: _native._RuntimeHandle

    def __init__(self, owner: _RuntimeHandleOperations):
        self._native = owner._native

    def clear_event_handler(self) -> None:
        """Stops sending the events of the runtime and its maps to a callback.

        See `mln_runtime_clear_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_event_handler()

    def clear_http_header_transform(self) -> None:
        """Stops adding headers to the runtime's HTTP requests.

        See `mln_runtime_clear_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_http_header_transform()

    def clear_resource_provider(self) -> None:
        """Removes the runtime's resource provider, so the library loads every
        request itself.

        See `mln_runtime_clear_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_resource_provider()

    def clear_resource_transform(self) -> None:
        """Stops rewriting the URLs of the runtime's network requests.

        See `mln_runtime_clear_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_clear_resource_transform()

    def set_event_handler(self, handler: RuntimeEventHandler) -> None:
        """Sends the events of the runtime and its maps to a callback.

        The callback replaces the one set before.

        See `mln_runtime_set_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_event_handler(handler)

    def set_http_header_transform(self, transform: HttpHeaderTransform) -> None:
        """Adds headers to the runtime's HTTP requests.

        The transform replaces the one set before. In a browser or on
        OpenHarmony, serve the requests with
        `RuntimeHandle.set_resource_provider()` instead.

        See `mln_runtime_set_http_header_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the library runs in a browser or on
          OpenHarmony.
        """
        return self._native._discard_set_http_header_transform(transform)

    def set_resource_provider(self, provider: ResourceProvider) -> None:
        """Serves the runtime's network requests through a callback.

        The provider receives the network requests that its routes match, and
        the library loads the rest as usual. A provider can serve URL schemes
        that the library can't load itself. The provider replaces the one set
        before.

        See `mln_runtime_set_resource_provider` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_resource_provider(provider)

    def set_resource_transform(self, transform: ResourceTransform) -> None:
        """Rewrites the URLs of the runtime's network requests through a
        callback.

        The transform replaces the one set before.

        See `mln_runtime_set_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_resource_transform(transform)
