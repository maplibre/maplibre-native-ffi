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
    AndroidHardwareBufferFrame,
    AndroidHardwareBufferRingDescriptor,
    AndroidSurfaceDescriptor,
    AndroidSurfaceReplacement,
    BoundOptions,
    CacheDatabaseOperation,
    CameraDelta,
    CameraFitOptions,
    CameraFitUpdate,
    CameraOptions,
    CameraQueryResult,
    CameraUpdate,
    CanonicalTileId,
    CustomGeometrySourceOptions,
    CustomVectorSourceOptions,
    DmaBufFrame,
    DmaBufRingDescriptor,
    DxgiSharedTextureFrame,
    DxgiSharedTextureRingDescriptor,
    FeatureStateSelector,
    FrameDemand,
    FramePacerOptions,
    FreeCameraOptions,
    GeoJsonSourceOptions,
    GpuSync,
    HttpHeaderTransform,
    IoSurfaceFrame,
    IoSurfaceRingDescriptor,
    LatLng,
    LatLngBounds,
    LogHandler,
    LogicalExtent,
    LogicalSize,
    LogSeverityFlag,
    MapDebugFlag,
    MapOptions,
    MapProjectionCameraResult,
    MapSnapshot,
    MapTileOptions,
    MapView,
    MapViewportOptions,
    MetalBorrowedTextureDescriptor,
    MetalBorrowedTextureReplacement,
    MetalSurfaceDescriptor,
    MetalSurfaceReplacement,
    MetalTextureFrame,
    NetworkStatus,
    OfflineRegionDefinition,
    OfflineRegionDownloadState,
    OfflineRegionInfo,
    OfflineRegionStatus,
    OffscreenDescriptor,
    OffscreenDeviceOptions,
    OpenGlBorrowedTextureDescriptor,
    OpenGlBorrowedTextureReplacement,
    OpenGlContextProviderFlag,
    OpenGlSurfaceDescriptor,
    OpenGlSurfaceReplacement,
    OpenGlTextureFrame,
    PremultipliedRgba8Image,
    ProjectedMeters,
    ProjectionModeOptions,
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
    ResourceTransformError,
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
    VulkanBorrowedTextureReplacement,
    VulkanDeviceContextResult,
    VulkanDeviceOptions,
    VulkanSurfaceDescriptor,
    VulkanSurfaceReplacement,
    VulkanTextureFrame,
    WaylandSurfaceDescriptor,
    WaylandSurfaceReplacement,
    Win32WindowDescriptor,
    Win32WindowReplacement,
    _maybe,
)
from ._operation import GeneratedOperations, _adopt_value, _answer, _view

if TYPE_CHECKING:
    from ._generated_owners import (
        AcquiredFrameHandle,
        FramePacerHandle,
        GeoJsonSourceDataHandle,
        MapHandle,
        MapProjectionHandle,
        OffscreenDeviceHandle,
        RenderSessionHandle,
        RuntimeHandle,
        StyleExpressionHandle,
        VulkanDeviceHandle,
    )


class LogicalExtentPhysicalSizeResult(NamedTuple):
    width: int
    height: int


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

    def android_hardware_buffer(
        self,
    ) -> AbstractContextManager[AndroidHardwareBufferFrame]:
        """Gets the Android hardware buffer of an acquired frame.

        Wait for `AcquiredFrameHandle.producer_sync()` before you read the
        buffer. Once you release or discard the frame, the session may render
        into the buffer again. A reference that you take with
        `AHardwareBuffer_acquire` keeps the buffer's memory, but not its
        contents.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_android_hardware_buffer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.android_hardware_buffer(),
            lambda raw: AndroidHardwareBufferFrame._from_native(raw),
        )

    def dma_buf(self) -> AbstractContextManager[DmaBufFrame]:
        """Gets the dmabuf of an acquired frame.

        Wait for `AcquiredFrameHandle.producer_sync()` before you read the
        buffer. Once you release or discard the frame, the session may render
        into the buffer again. You can map a buffer with
        DRM_FORMAT_MOD_LINEAR, which has one plane, with `mmap`, and bracket
        your reads with DMA_BUF_IOCTL_SYNC.

        The session owns the planes' file descriptors, so don't close them.
        To keep the buffer after you release the frame, duplicate each file
        descriptor with `dup`. A duplicate keeps the buffer's memory, but not
        its contents.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_dma_buf` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.dma_buf(),
            lambda raw: DmaBufFrame._from_native(raw),
        )

    def dxgi_shared_texture(self) -> AbstractContextManager[DxgiSharedTextureFrame]:
        """Gets the DXGI shared texture of an acquired frame.

        Wait for `AcquiredFrameHandle.producer_sync()` before you read the
        texture. Once you release or discard the frame, the session may
        render into the texture again. Open the handle with
        `ID3D11Device1::OpenSharedResource1` or
        `ID3D12Device::OpenSharedHandle`. A texture that you open keeps its
        memory, but not its contents.

        The session owns the handle, so don't close it. To keep the handle
        after you release the frame, duplicate it with `DuplicateHandle`.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_dxgi_shared_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.dxgi_shared_texture(),
            lambda raw: DxgiSharedTextureFrame._from_native(raw),
        )

    def io_surface(self) -> AbstractContextManager[IoSurfaceFrame]:
        """Gets the IOSurface of an acquired frame.

        Wait for `AcquiredFrameHandle.producer_sync()` before you read the
        surface. Once you release or discard the frame, the session may
        render into the surface again. For an API that takes a
        `CVPixelBuffer`, wrap the surface with
        `CVPixelBufferCreateWithIOSurface`. A reference that you take with
        `CFRetain`, or a pixel buffer that wraps the surface, keeps the
        surface's memory, but not its contents.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_io_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.io_surface(),
            lambda raw: IoSurfaceFrame._from_native(raw),
        )

    def metal_texture(self) -> AbstractContextManager[MetalTextureFrame]:
        """Gets the Metal texture of an acquired frame.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_metal_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
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

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
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

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.producer_sync(),
            lambda raw: GpuSync._from_native(raw),
        )

    def get_result(self) -> RenderFrameResult:
        """Reads the frame result of an acquired frame.

        It succeeds until you release or discard the frame, whatever the
        session's state.

        See `mln_acquired_frame_get_result` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderFrameResult._from_native(self._native.get_result())

    def vulkan_texture(self) -> AbstractContextManager[VulkanTextureFrame]:
        """Gets the Vulkan image of an acquired frame.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_vulkan_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.vulkan_texture(),
            lambda raw: VulkanTextureFrame._from_native(raw),
        )

    def release(self, consumer_completion: GpuSync) -> None:
        """Gives an acquired frame back to the session once you're done reading
        it.

        `consumer_completion` says when your GPU work that reads the texture
        finishes. Pass `GpuSyncKind.CPU_COMPLETE` when that work has already
        finished. The session waits for that point before it renders into the
        texture again. If that wait fails, such as for an OpenGL fence
        outside the session's share group, the session fails.

        See `mln_acquired_frame_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `RenderSessionCapabilities.consumer_sync_kinds` lacks the kind of
          `consumer_completion`.
        """
        return self._native.release(consumer_completion)


class _FramePacerHandleOperations(GeneratedOperations):
    _native: _native._FramePacerHandle

    def release(self) -> None:
        """Releases a frame pacer.

        Render sessions that use the pacer stay paced until you release them
        or set another pacer.

        See `mln_frame_pacer_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.release()


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

        - `Status.INVALID_ARGUMENT` when `data` isn't valid GeoJSON, when an
          option in `options` is out of range, or when `options` turns on
          clustering and `data` isn't a feature collection of points.
        """
        return _adopt_value(
            _native._GeoJsonSourceDataHandle.create(data, options),
            "GeoJsonSourceDataHandle",
            None,
        )

    def release(self) -> None:
        """Releases prepared GeoJSON data.

        Sources that use the data keep it.

        See `mln_geo_json_source_data_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.release()


class _MapHandleOperations(GeneratedOperations):
    _native: _native._MapHandle

    def add_custom_geometry_source(
        self, source_id: str, options: CustomGeometrySourceOptions
    ) -> Future[CommandCompletion]:
        """Adds a source whose GeoJSON tiles come from callbacks.

        See `mln_map_add_custom_geometry_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.ALREADY_EXISTS` when the style already has a source named
          `source_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.add_custom_geometry_source(source_id, options)

    def add_custom_vector_source(
        self, source_id: str, options: CustomVectorSourceOptions
    ) -> Future[CommandCompletion]:
        """Adds a source whose vector tiles come from callbacks.

        See `mln_map_add_custom_vector_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.ALREADY_EXISTS` when the style already has a source named
          `source_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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

        - `Status.ALREADY_EXISTS` when the style already has a source named
          `source_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.add_geo_json_source_data(source_id, data._native)

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

        - `Status.ALREADY_EXISTS` when the style already has a source named
          `source_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.add_image_source_image(source_id, coordinates, image)

    def add_style_layer_json(
        self, layer_json: Any, before_layer_id: str | None = None
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
        - `Status.ALREADY_EXISTS` when the style already has a layer with the
          new layer's ID.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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

        - `Status.ALREADY_EXISTS` when the style already has a source named
          `source_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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

        A delta that replaces an animated one starts from where that
        animation left the camera, so the rest of the replaced change never
        happens. To reach an exact camera, use `MapHandle.update_camera()`.

        See `mln_map_apply_camera_delta` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.apply_camera_delta(delta)

    def attach_android_hardware_buffer_ring(
        self,
        descriptor: AndroidHardwareBufferRingDescriptor,
        device: VulkanDeviceHandle | None,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Creates a session that renders the map into a ring of Android
        hardware buffers that the library allocates.

        Acquire each frame with `RenderSessionHandle.acquire_frame()`, and
        get its buffer with `AcquiredFrameHandle.android_hardware_buffer()`.
        `RenderSessionHandle.resize()` gives the ring new buffers of the new
        size.

        A Vulkan build renders with `device`, or, when `device` is null, with
        a device that the library creates. An OpenGL build uses an EGL
        context on the default display, and `device` must be null. Attach
        with `RenderDriverKind.CORE_WORKER`.

        To show the buffers through Android's UI, such as with
        `Bitmap.wrapHardwareBuffer`, attach without `GpuSyncKindFlag.SYNC_FD`
        in `RenderSessionAttachOptions.producer_sync_kinds`, so that each
        frame finishes before you acquire it. Keep each frame acquired until
        the next one replaces it on screen, then release it with
        `GpuSyncKind.CPU_COMPLETE`. Give the ring 3 buffers, so the session
        can render while you hold two frames.

        See `mln_map_attach_android_hardware_buffer_ring` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when Android is older than 8.0, API level 26,
          or when the graphics context can't render into hardware buffers:
          the Vulkan device lacks
          VK_ANDROID_external_memory_android_hardware_buffer, or the EGL
          display lacks EGL_ANDROID_get_native_client_buffer or
          EGL_ANDROID_image_native_buffer.
        - `Status.NATIVE_ERROR` when the library can't create its Vulkan
          device or initialize the default EGL display.
        """
        raw = self._native.attach_android_hardware_buffer_ring(
            descriptor, None if device is None else device._native, options
        )
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_android_surface(
        self,
        descriptor: AndroidSurfaceDescriptor,
        device: VulkanDeviceHandle | None,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Creates a session that presents the map to an Android window, with a
        graphics context that the library creates.

        A Vulkan build presents with `device`, or, when `device` is null,
        with a device that the library creates. An OpenGL build uses an EGL
        context on the default display, and `device` must be null. Attach
        with `RenderDriverKind.CORE_WORKER`.

        When the platform destroys the window, such as in
        `SurfaceHolder.Callback.surfaceDestroyed`, release the target with
        `RenderSessionHandle.release_target()`, or give the session a new
        window with `RenderSessionHandle.set_android_surface_target()`,
        without waiting for that to complete. The session keeps its own
        reference to the window and stays usable for the next window. A frame
        to the destroyed window finishes with
        `RenderResult.TARGET_NOT_READY`, or with `RenderResult.RENDERED` if
        the window went away while the frame was presenting.

        See `mln_map_attach_android_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `surface` has no window, such as
          after the platform destroyed it.
        - `Status.UNSUPPORTED` when you created `device` without
          `VulkanDeviceOptions.presentation`.
        - `Status.UNSUPPORTED` when the GPU or the Vulkan loader can't
          present.
        - `Status.NATIVE_ERROR` when the library can't create the graphics
          context or the window's surface.
        """
        raw = self._native.attach_android_surface(
            descriptor, None if device is None else device._native, options
        )
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_dma_buf_ring(
        self,
        descriptor: DmaBufRingDescriptor,
        device: VulkanDeviceHandle | None,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Creates a session that renders the map into a ring of dmabufs that
        the library allocates.

        Acquire each frame with `RenderSessionHandle.acquire_frame()`, and
        get its buffer with `AcquiredFrameHandle.dma_buf()`.
        `RenderSessionHandle.resize()` gives the ring new buffers of the new
        size.

        A Vulkan build renders with `device`, or, when `device` is null, with
        a device that the library creates. An OpenGL build uses an EGL
        context of its own, and `device` must be null. Attach with
        `RenderDriverKind.CORE_WORKER`.

        For a reader that waits on no sync file, such as a Wayland compositor
        without explicit synchronization, attach without
        `GpuSyncKindFlag.SYNC_FD` in
        `RenderSessionAttachOptions.producer_sync_kinds`, so that each frame
        finishes before you acquire it. Keep each frame acquired until its
        reader is done with the buffer. Give the ring 3 buffers, so the
        session can render while you hold two frames.

        See `mln_map_attach_dma_buf_ring` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the graphics context can't render into a
          dmabuf of any of `formats`.
        - `Status.NATIVE_ERROR` when the library can't create its Vulkan
          device or initialize an EGL display.
        """
        raw = self._native.attach_dma_buf_ring(
            descriptor, None if device is None else device._native, options
        )
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_dxgi_shared_texture_ring(
        self,
        descriptor: DxgiSharedTextureRingDescriptor,
        device: VulkanDeviceHandle | None,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Creates a session that renders the map into a ring of DXGI shared
        textures that the library allocates.

        Acquire each frame with `RenderSessionHandle.acquire_frame()`, and
        get its texture with `AcquiredFrameHandle.dxgi_shared_texture()`.
        `RenderSessionHandle.resize()` gives the ring new textures of the new
        size.

        The session renders with the Vulkan device `device`, or, when
        `device` is null, with one that the library creates. The textures
        belong to the device's GPU, so open them on a D3D11 or D3D12 device
        of the same adapter. To pick the GPU, create `device` with
        `VulkanDeviceOptions.device_luid` set to your adapter's LUID. Attach
        with `RenderDriverKind.CORE_WORKER`.

        For a reader that waits on no fence, such as Flutter's texture
        registrar, attach without `GpuSyncKindFlag.D3D12_FENCE` in
        `RenderSessionAttachOptions.producer_sync_kinds`, so that each frame
        finishes before you acquire it. Release a frame with
        `GpuSyncKind.CPU_COMPLETE` only once the reader has finished with its
        texture.

        See `mln_map_attach_dxgi_shared_texture_ring` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).

        Errors:

        - `Status.UNSUPPORTED` in an OpenGL build, or when the GPU can't
          share the textures: D3D11 has no adapter with the Vulkan device's
          LUID or can't share textures through NT handles, or the device
          lacks VK_KHR_external_memory_win32 or can't import D3D11 textures.
        - `Status.NATIVE_ERROR` when the library can't create its Vulkan
          device or a D3D11 device on the adapter.
        """
        raw = self._native.attach_dxgi_shared_texture_ring(
            descriptor, None if device is None else device._native, options
        )
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_io_surface_ring(
        self, descriptor: IoSurfaceRingDescriptor, options: RenderSessionAttachOptions
    ) -> RenderSessionHandle:
        """Creates a session that renders the map into a ring of IOSurfaces that
        the library allocates.

        Acquire each frame with `RenderSessionHandle.acquire_frame()`, and
        get its surface with `AcquiredFrameHandle.io_surface()`.
        `RenderSessionHandle.resize()` gives the ring new surfaces of the new
        size. Attach with `RenderDriverKind.CORE_WORKER`.

        To hand frames to a reader that can't wait on an event, such as
        Flutter's `copyPixelBuffer`, attach without
        `GpuSyncKindFlag.METAL_SHARED_EVENT` in
        `RenderSessionAttachOptions.producer_sync_kinds`, so that each frame
        finishes before you acquire it. Release a frame with
        `GpuSyncKind.CPU_COMPLETE` only once the reader has finished with its
        surface.

        See `mln_map_attach_io_surface_ring` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.METAL`.
        - `Status.NATIVE_ERROR` when `context` names no device and the system
          has no Metal device.
        """
        raw = self._native.attach_io_surface_ring(descriptor, options)
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_metal_borrowed_texture(
        self,
        descriptor: MetalBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
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
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_metal_surface(
        self, descriptor: MetalSurfaceDescriptor, options: RenderSessionAttachOptions
    ) -> RenderSessionHandle:
        """Creates a session that renders the map to a Metal layer.

        See `mln_map_attach_metal_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.METAL`.
        """
        raw = self._native.attach_metal_surface(descriptor, options)
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_offscreen(
        self,
        descriptor: OffscreenDescriptor,
        options: RenderSessionAttachOptions,
        device: OffscreenDeviceHandle | None = None,
    ) -> RenderSessionHandle:
        """Attaches a render session that renders the map offscreen.

        Read each image's pixels with `RenderSessionHandle.read_texture()`.

        The session renders on `device`. When `device` is null, the session
        creates a device with the default options, and releases it when the
        session ends.

        See `mln_map_attach_offscreen` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the map already has a render session.
        - `Status.NATIVE_ERROR` when `device` is null and the platform can't
          create a device.
        """
        raw = self._native.attach_offscreen(
            descriptor, options, None if device is None else device._native
        )
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_open_gl_borrowed_texture(
        self,
        descriptor: OpenGlBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Creates a session that renders the map into a ring of your OpenGL
        textures.

        See `mln_map_attach_open_gl_borrowed_texture` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `open_gl_supported_context_provider_mask()` lacks the context's
          platform, or when the target supports none of the
          `producer_sync_kinds` in `options`.
        """
        raw = self._native.attach_open_gl_borrowed_texture(descriptor, options)
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_open_gl_surface(
        self, descriptor: OpenGlSurfaceDescriptor, options: RenderSessionAttachOptions
    ) -> RenderSessionHandle:
        """Creates a session that renders the map to an OpenGL surface.

        See `mln_map_attach_open_gl_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `open_gl_supported_context_provider_mask()` lacks the context's
          platform.
        """
        raw = self._native.attach_open_gl_surface(descriptor, options)
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_vulkan_borrowed_texture(
        self,
        descriptor: VulkanBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
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
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_vulkan_surface(
        self, descriptor: VulkanSurfaceDescriptor, options: RenderSessionAttachOptions
    ) -> RenderSessionHandle:
        """Creates a session that renders the map to a Vulkan surface.

        See `mln_map_attach_vulkan_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.VULKAN`.
        """
        raw = self._native.attach_vulkan_surface(descriptor, options)
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_wayland_surface(
        self,
        descriptor: WaylandSurfaceDescriptor,
        device: VulkanDeviceHandle | None,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Creates a session that presents the map to a Wayland surface, with a
        graphics context that the library creates.

        A Vulkan build presents with `device`, or, when `device` is null,
        with a device that the library creates. An OpenGL build uses an EGL
        context on the display, and `device` must be null. Attach with
        `RenderDriverKind.CORE_WORKER`.

        The session presents without waiting for the compositor, so demand at
        most one frame per display refresh.

        Keep the surface until a release or replacement of the target, or a
        detach of the session, completes. Keep the display connected until
        the session's detach completes. In an OpenGL build, if you also use
        EGL on the display, initialize it before you attach.

        See `mln_map_attach_wayland_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when you created `device` without
          `VulkanDeviceOptions.presentation`.
        - `Status.UNSUPPORTED` when the GPU or the Vulkan loader can't
          present to Wayland.
        - `Status.NATIVE_ERROR` when the library can't create the graphics
          context or present to the surface.
        """
        raw = self._native.attach_wayland_surface(
            descriptor, None if device is None else device._native, options
        )
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

    def attach_win32_window(
        self,
        descriptor: Win32WindowDescriptor,
        device: VulkanDeviceHandle | None,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Creates a session that presents the map to a Win32 window, with a
        graphics context that the library creates.

        A Vulkan build presents with `device`, or, when `device` is null,
        with a device that the library creates. An OpenGL build uses a WGL
        context, and `device` must be null. Attach with
        `RenderDriverKind.CORE_WORKER`.

        An OpenGL build sets the window's pixel format, so don't set one
        yourself.

        The session presents without waiting for the display, so demand at
        most one frame per display refresh. While the window is minimized,
        frame demands finish with `RenderResult.TARGET_NOT_READY`.

        Keep the window until a release or replacement of the target, or a
        detach of the session, completes.

        See `mln_map_attach_win32_window` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `window` isn't a window, or, in an
          OpenGL build, already has another pixel format.
        - `Status.INVALID_STATE` when `window` is minimized or has no client
          area.
        - `Status.UNSUPPORTED` when you created `device` without
          `VulkanDeviceOptions.presentation`.
        - `Status.INVALID_STATE` when `window` was minimized before the
          attach ran.
        - `Status.UNSUPPORTED` when the GPU or the Vulkan loader can't
          present.
        - `Status.NATIVE_ERROR` when the library can't create the graphics
          context or present to the window.
        """
        raw = self._native.attach_win32_window(
            descriptor, None if device is None else device._native, options
        )
        return _adopt_value(raw["session"], "RenderSessionHandle", self, raw["ready"])

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

        Errors:

        - `Status.INVALID_ARGUMENT` when the padding in `fit_options` leaves
          no room for the area.
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

        Errors:

        - `Status.INVALID_ARGUMENT` when the padding in `fit_options` leaves
          no room for the area.
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

        Errors:

        - `Status.INVALID_ARGUMENT` when the padding in `fit_options` leaves
          no room for the area.
        """
        return map_future(
            self._native.camera_for_lat_lngs(coordinates, fit_options),
            lambda value: CameraOptions._from_native(value),
        )

    def cancel_camera_transitions(self) -> Future[CommandCompletion]:
        """Stops every running camera animation where it is.

        The end handler of each stopped animation reports
        `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_camera_transitions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.cancel_camera_transitions()

    def cancel_camera_transitions_with_id(
        self, transition_id: int
    ) -> Future[CommandCompletion]:
        """Stops the running camera animations whose animation options set
        `transition_id`.

        Other animations keep running. The end handler of each stopped
        animation reports `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_camera_transitions_with_id` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.cancel_camera_transitions_with_id(transition_id)

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

    def fit_camera_to_geometry(
        self, geometry: Any, update: CameraFitUpdate
    ) -> Future[CommandCompletion]:
        """Moves the camera to fit a GeoJSON geometry in the map's view, at once
        or with an animation.

        `geometry` is a GeoJSON geometry object with at least one coordinate,
        not a feature. The camera is the one that
        `MapHandle.camera_for_geometry()` computes after the commands sent
        before this one apply. The function finishes when the map applies the
        change, before an animation ends.

        See `mln_map_fit_camera_to_geometry` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the padding in `update` leaves no
          room for the area. The camera doesn't move.
        """
        return self._native.fit_camera_to_geometry(geometry, update)

    def fit_camera_to_lat_lng_bounds(
        self, bounds: LatLngBounds, update: CameraFitUpdate
    ) -> Future[CommandCompletion]:
        """Moves the camera to fit `bounds` in the map's view, at once or with
        an animation.

        The camera is the one that `MapHandle.camera_for_lat_lng_bounds()`
        computes after the commands sent before this one apply. The function
        finishes when the map applies the change, before an animation ends.

        See `mln_map_fit_camera_to_lat_lng_bounds` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the padding in `update` leaves no
          room for the area. The camera doesn't move.
        """
        return self._native.fit_camera_to_lat_lng_bounds(bounds, update)

    def fit_camera_to_lat_lngs(
        self, coordinates: Sequence[LatLng], update: CameraFitUpdate
    ) -> Future[CommandCompletion]:
        """Moves the camera to fit `coordinates` in the map's view, at once or
        with an animation.

        `coordinates` must not be empty. The camera is the one that
        `MapHandle.camera_for_lat_lngs()` computes after the commands sent
        before this one apply. The function finishes when the map applies the
        change, before an animation ends.

        See `mln_map_fit_camera_to_lat_lngs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the padding in `update` leaves no
          room for the area. The camera doesn't move.
        """
        return self._native.fit_camera_to_lat_lngs(coordinates, update)

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

        The value can differ in form from the value that was set: a color set
        as `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.

        Completes with no value when the layer doesn't set the property.

        See `mln_map_get_style_layer_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named `layer_id`.
        """
        return self._native.get_style_layer_property(layer_id, property_name)

    def get_style_light_property(self, property_name: str) -> Future[Any]:
        """Reads a property of the light of the map's style.

        The value can differ in form from the value that was set: a color set
        as `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.

        Completes with no value when the light doesn't set the property.

        See `mln_map_get_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native.get_style_light_property(property_name)

    def get_style_source(self, source_id: str) -> Future[StyleSourceInfo | None]:
        """Reads a source of the map's style.

        A tile source with a `url` has an empty attribution until its
        TileJSON loads, which raises `RuntimeEventType.MAP_SOURCE_CHANGED`.

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

    def invalidate_custom_vector_source_region(
        self, source_id: str, bounds: LatLngBounds
    ) -> Future[CommandCompletion]:
        """Makes a custom vector source ask again for the tiles in `bounds` that
        the map still needs.

        See `mln_map_invalidate_custom_vector_source_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          custom vector source.
        """
        return self._native.invalidate_custom_vector_source_region(source_id, bounds)

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

    def lat_lng_bounds_for_camera(self, camera: CameraOptions) -> Future[LatLngBounds]:
        """Computes the area that the map would show with `camera`.

        Camera fields that `camera` doesn't set come from the map's camera.
        The longitudes can fall outside -180 to 180, so a view across the
        antimeridian gives one continuous range.

        See `mln_map_lat_lng_bounds_for_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return map_future(
            self._native.lat_lng_bounds_for_camera(camera),
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
        self, layer_id: str, before_layer_id: str | None = None
    ) -> Future[CommandCompletion]:
        """Moves a layer of the map's style below the layer named
        `before_layer_id`, or above every layer when `before_layer_id` is
        empty.

        See `mln_map_move_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named `layer_id` or
          `before_layer_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.move_style_layer(layer_id, before_layer_id)

    def release(self) -> Future[None]:
        """Releases the map.

        Detach the map's render session first.

        See `mln_map_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native.release()

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
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.remove_style_image(image_id)

    def remove_style_layer(self, layer_id: str) -> Future[CommandCompletion]:
        """Removes a layer from the map's style.

        See `mln_map_remove_style_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named `layer_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.remove_style_layer(layer_id)

    def remove_style_source(self, source_id: str) -> Future[CommandCompletion]:
        """Removes a source from the map's style.

        See `mln_map_remove_style_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_STATE` when a layer still uses the source, or while
          the style isn't loaded, as `MapHandle.set_style_url()` describes.
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

    def resize(self, logical_size: LogicalSize) -> Future[CommandCompletion]:
        """Sets the map's size.

        A map with a render session takes its size from the session, so
        resize the session with `RenderSessionHandle.resize()` instead.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the map has a render session.
        - `Status.INVALID_STATE` when a render session attached to the map
          before the resize applied.
        """
        return self._native.resize(logical_size)

    def set_bounds(self, options: BoundOptions) -> Future[CommandCompletion]:
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

        `data` is a GeoJSON feature collection, feature, or geometry. The
        source ignores data for a tile that it hasn't asked for, or has since
        cancelled.

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

    def set_debug_flags(self, flags: MapDebugFlag) -> Future[CommandCompletion]:
        """Draws debugging overlays, such as tile borders, on the map.

        `flags` replaces the overlays set before.

        See `mln_map_set_debug_flags` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_debug_flags(flags)

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
        self, options: FreeCameraOptions
    ) -> Future[CommandCompletion]:
        """Places the camera at a position and orientation in space.

        The map adjusts them to a camera that it can show. It stops running
        camera animations first, and their end handlers report
        `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_set_free_camera_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_free_camera_options(options)

    def set_geo_json_source_data(
        self, source_id: str, data: GeoJsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Replaces the data of a GeoJSON source.

        `data` must have been prepared with the source's options, which
        `MapHandle.get_style_source()` reports, though `min_zoom` may differ.
        `data` can be released once this function returns.

        See `mln_map_set_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          GeoJSON source, or `data` was prepared with other options.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_geo_json_source_data(source_id, data._native)

    def set_geo_json_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> Future[CommandCompletion]:
        """Sets whether a GeoJSON source builds its tiles while the map renders.

        Building tiles while the map renders, instead of in the background,
        makes new data show in the next frame. The setting replaces the
        `synchronousUpdate` member of the source's JSON, which defaults to
        false.

        See `mln_map_set_geo_json_source_synchronous_tiling` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          GeoJSON source.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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

        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_image_source_url(source_id, url)

    def set_projection_mode(
        self, options: ProjectionModeOptions
    ) -> Future[CommandCompletion]:
        """Sets whether the map draws in perspective or axonometric projection,
        and how much it skews the view.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_projection_mode` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_projection_mode(options)

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

        Errors:

        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_style_image(image_id, image, options)

    def set_style_json(self, json: Any) -> Future[CommandCompletion]:
        """Loads the map's style from `json`.

        The command completes once the style has parsed, and the map's style
        is then loaded, as `MapHandle.set_style_url()` describes. The style's
        sources, sprites, and glyphs load in the background.

        See `mln_map_set_style_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NATIVE_ERROR` when `json` isn't a valid style. The map
          keeps its current style.
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
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_style_layer_property(layer_id, property_name, value)

    def set_style_light_json(self, light_json: Any) -> Future[CommandCompletion]:
        """Replaces the
        \\[light\\](https://maplibre.org/maplibre-style-spec/light/) of the
        map's style.

        See `mln_map_set_style_light_json` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_style_light_json(light_json)

    def set_style_light_property(
        self, property_name: str, value: Any
    ) -> Future[CommandCompletion]:
        """Sets a property of the light of the map's style.

        See `mln_map_set_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
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
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_style_source_volatile(source_id, is_volatile)

    def set_style_transition_options(
        self, options: StyleTransitionOptions
    ) -> Future[CommandCompletion]:
        """Sets the default transition of the map's style.

        See `mln_map_set_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_style_transition_options(options)

    def set_style_url(self, url: str) -> Future[CommandCompletion]:
        """Loads the map's style from `url`.

        The command completes once the style document has loaded and parsed.
        The style's sources, sprites, and glyphs then load in the background.

        The map's style is loaded from the time a style document parses until
        the next call to this function. While the style isn't loaded, the
        commands that change it complete with `Status.INVALID_STATE`. To
        change a new map's style without a style document, first load an
        empty style, such as `{"version":8,"sources":{},"layers":[]}`.

        See `mln_map_set_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NATIVE_ERROR` when the style document fails to load or
          isn't a valid style.
        - `Status.CANCELLED` when the map is released, or this function or
          `MapHandle.set_style_json()` loads another style, before the style
          document parses.
        """
        return self._native.set_style_url(url)

    def set_tile_options(self, options: MapTileOptions) -> Future[CommandCompletion]:
        """Sets how the map chooses which tiles to load, and at what detail.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_tile_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_tile_options(options)

    def set_viewport_options(
        self, options: MapViewportOptions
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

    def get_camera(self) -> MapProjectionCameraResult:
        """Reads the projection's camera.

        See `mln_map_projection_get_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return MapProjectionCameraResult._from_native(self._native.get_camera())

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

    def release(self) -> None:
        """Releases a projection.

        See `mln_map_projection_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.release()

    def set_camera(self, camera: CameraOptions) -> None:
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

        In a browser WebGPU build, attach without a device instead.

        See `mln_offscreen_device_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).

        Errors:

        - `Status.UNSUPPORTED` in a browser WebGPU build, or when the device
          can't render with `client_api`.
        - `Status.NATIVE_ERROR` when `adapter_index` names no GPU, or the
          platform can't create the device.
        """
        return _adopt_value(
            _native._OffscreenDeviceHandle.create(options),
            "OffscreenDeviceHandle",
            None,
        )

    def release(self) -> None:
        """Releases an offscreen device.

        Render sessions on the device keep working until they detach.

        See `mln_offscreen_device_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
        """
        return self._native.release()


class _RenderSessionHandleOperations(GeneratedOperations):
    _native: _native._RenderSessionHandle

    def abandon(self) -> RenderAbandonResult:
        """Stops the session right away, without running its pending graphics
        work.

        Use it when the target's graphics objects are going away and a detach
        can't finish first. Pending work completes with `Status.TARGET_LOST`,
        and pending frame demands finish with
        `RenderResult.TARGET_NOT_READY`. Once the call returns, the library
        no longer uses your graphics objects, and you can attach another
        session to the map. Release the session afterward.

        Abandon keeps the session's OpenGL and WebGPU objects until the
        process exits, and reports how many it kept. Detach the session
        instead to free them.

        See `mln_render_session_abandon` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderAbandonResult._from_native(self._native.abandon())

    def acquire_frame(self) -> AcquiredFrameHandle | None:
        """Takes the newest rendered frame from the session's texture ring, if
        one is ready.

        Older frames that you haven't acquired are dropped, and the session
        can render over a frame before you acquire it. While you hold a frame
        in every texture of the ring, new frames wait until you release one.
        When no frame is ready, it succeeds and writes the null handle.

        See `mln_render_session_acquire_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session lacks
          `RenderSessionCapabilityFlag.FRAME_ACQUISITION`.
        - `Status.TARGET_LOST` or `Status.NATIVE_ERROR`, as
          `RenderSessionSnapshot.failure_status` reports, when the session
          has failed.
        - `Status.INVALID_STATE` when you detached, abandoned, released, or
          disposed the session.
        """
        return _maybe(
            lambda raw: _adopt_value(raw, "AcquiredFrameHandle", self),
            self._native.acquire_frame(),
        )

    def barrier(self) -> Future[None]:
        """Ends the session's pending frame demands and waits for its earlier
        work.

        Frame demands that haven't started, including those that wait for a
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

    def demand_frame(self, demand: FrameDemand) -> None:
        """Asks the session to render a frame.

        You can call it once the attach call returns. While the attach runs,
        the demand waits for it to complete. Every demand finishes with one
        frame result that carries its token, and
        `RenderSessionHandle.drain_frame_results()` takes it. A demand
        without `FrameDemandFlag.FORCE` replaces each pending demand with the
        same flags.

        See `mln_render_session_demand_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` or `Status.NATIVE_ERROR`, as
          `RenderSessionSnapshot.failure_status` reports, when the session
          has failed.
        - `Status.INVALID_STATE` when you detached, abandoned, released, or
          disposed the session, or discarded a frame from every texture of
          its ring.
        """
        return self._native.demand_frame(demand)

    def detach(self) -> Future[None]:
        """Detaches the session from its map and frees its graphics objects.

        Release every acquired frame first. Pending frame demands finish with
        `RenderResult.TARGET_NOT_READY`. Once the detach completes, the
        library no longer uses your graphics objects, and you can attach
        another session to the map. Release the session afterward.

        See `mln_render_session_detach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` when the session loses its target before the
          detach runs.
        """
        return self._native.detach()

    def drain_frame_results(self) -> tuple[RenderFrameResult, ...]:
        """Takes the frame results that the session has queued.

        When no result is queued, it succeeds and writes the null handle and
        an empty view.

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

    def get_cluster_children(self, source_id: str, cluster_id: int) -> Future[Any]:
        """Reads the children of a cluster in a clustered GeoJSON source.

        `cluster_id` is the "cluster_id" property of a cluster feature. The
        result is a GeoJSON feature collection of the clusters and points
        that the cluster splits into at the next zoom level.

        The query reads the source's data from the session's most recent
        render attempt, so a change to the data shows once the session tries
        to render again.

        A cluster ID belongs to one version of the source's data. After the
        data changes, feature queries can return IDs from the earlier version
        until the source's tiles reload, and such an ID completes with
        `Status.NOT_FOUND` or names a different cluster. Query the features
        again for current IDs.

        See `mln_render_session_get_cluster_children` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the session's most recent render attempt
          had no source named `source_id`, or the source has no cluster
          `cluster_id`.
        - `Status.INVALID_ARGUMENT` when the source isn't a GeoJSON source
          with clustering turned on.
        """
        return self._native.get_cluster_children(source_id, cluster_id)

    def get_cluster_expansion_zoom(
        self, source_id: str, cluster_id: int
    ) -> Future[int]:
        """Reads the zoom level at which a cluster of a clustered GeoJSON source
        splits.

        `cluster_id` is the "cluster_id" property of a cluster feature. At
        the result's zoom level, the cluster splits into two or more clusters
        or points.

        A cluster ID can go out of date as
        `RenderSessionHandle.get_cluster_children()` describes.

        See `mln_render_session_get_cluster_expansion_zoom` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the session's most recent render attempt
          had no source named `source_id`, or the source has no cluster
          `cluster_id`.
        - `Status.INVALID_ARGUMENT` when the source isn't a GeoJSON source
          with clustering turned on.
        """
        return self._native.get_cluster_expansion_zoom(source_id, cluster_id)

    def get_cluster_leaves(
        self, source_id: str, cluster_id: int, limit: int, offset: int
    ) -> Future[Any]:
        """Reads the points in a cluster of a clustered GeoJSON source.

        `cluster_id` is the "cluster_id" property of a cluster feature. The
        result is a GeoJSON feature collection of the original point
        features. It skips the first `offset` points and then holds up to
        `limit` of them, so a `limit` of 0 gives an empty collection.

        A cluster ID can go out of date as
        `RenderSessionHandle.get_cluster_children()` describes.

        See `mln_render_session_get_cluster_leaves` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the session's most recent render attempt
          had no source named `source_id`, or the source has no cluster
          `cluster_id`.
        - `Status.INVALID_ARGUMENT` when the source isn't a GeoJSON source
          with clustering turned on.
        """
        return self._native.get_cluster_leaves(source_id, cluster_id, limit, offset)

    def get_failure(self) -> str:
        """Reads the message of the error that made the session fail.

        `RenderSessionSnapshot.failure_status` reports the error's status.
        The message is empty while the session hasn't failed, and doesn't
        change once it fails.

        See `mln_render_session_get_failure` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.get_failure()

    def get_snapshot(self) -> RenderSessionSnapshot:
        """Reads the session's current state.

        See `mln_render_session_get_snapshot` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return RenderSessionSnapshot._from_native(self._native.get_snapshot())

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

    def release(self) -> Future[None]:
        """Releases the session, detaching it first when it is attached.

        Pending frame demands finish with `RenderResult.TARGET_NOT_READY`.
        Once the release completes, the library no longer uses your graphics
        objects, and you can attach another session to the map.

        A session driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD`
        detaches at your next `RenderSessionHandle.service_driver_work()`, so
        keep servicing it until the release completes. When the session isn't
        attached, such as while it attaches or after it fails, or when you
        release or dispose its map before that service, the release abandons
        it as `RenderSessionHandle.abandon()` does.

        See `mln_render_session_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.INVALID_STATE` while you hold one of the session's acquired
          frames. Release or discard them first.
        """
        return self._native.release()

    def release_target(self) -> Future[None]:
        """Lets go of the session's surface or textures, and keeps the rest of
        the session.

        Use it when the platform takes a surface away, such as when your app
        goes to the background. Release every acquired frame first. Frame
        demands that are still pending, and later ones until you give the
        session a new target, finish with `RenderResult.TARGET_NOT_READY`.
        Once the release completes, you can destroy the old target.

        See `mln_render_session_release_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders offscreen or into
          Android hardware buffers, dmabufs, DXGI shared textures, or
          IOSurfaces that it allocates.
        - `Status.UNSUPPORTED` when the target is a WebGL canvas.
        - `Status.TARGET_LOST` when the session loses its target before the
          release runs.
        """
        return self._native.release_target()

    def resize(self, logical_size: LogicalSize) -> Future[CommandCompletion]:
        """Resizes the session's target and its map.

        The session renders at the map's scale factor. Release every acquired
        frame first.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders into a borrowed ring.
          Give it new textures of the new size instead.
        - `Status.TARGET_LOST` when the session loses its target before the
          resize applies.
        """
        return self._native.resize(logical_size)

    def service_driver_work(self) -> None:
        """Runs the session's ready graphics work on your graphics thread.

        Use it with `RenderDriverKind.CALLER_GRAPHICS_THREAD`, and call it
        from the same thread every time. Call it each time
        `RenderSessionAttachOptions.driver_work_wake` is called, even while
        you demand no frames, until a detach, release, or disposal of the
        session finishes. It returns once no work is ready.

        It accepts a released or disposed session until that release or
        disposal finishes.

        See `mln_render_session_service_driver_work` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.service_driver_work()

    def set_android_surface_target(
        self, replacement: AndroidSurfaceReplacement
    ) -> Future[None]:
        """Replaces the Android window that the session presents to.

        Use it to give the session a window again after
        `RenderSessionHandle.release_target()`, to move it to another window,
        or to resize it. Once the replacement completes, the session no
        longer uses the old window.

        See `mln_render_session_set_android_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `surface` has no window, such as
          after the platform destroyed it.
        - `Status.UNSUPPORTED` when, in a Vulkan build, the new window needs
          a different color format or pre-rotation support than the old one.
          Attach a new session instead.
        - `Status.NATIVE_ERROR` when the library can't create the window's
          surface.
        """
        return self._native.set_android_surface_target(replacement)

    def set_frame_pacer(self, pacer: FramePacerHandle | None = None) -> None:
        """Sets the frame pacer that demands the session's frames.

        At the first display refresh after the session has something new to
        draw, `pacer` demands a frame without flags and with token 0. One
        pacer can serve many sessions. A null `pacer` stops pacing the
        session. You can call it once the attach call returns.

        See `mln_render_session_set_frame_pacer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.TARGET_LOST` or `Status.NATIVE_ERROR`, as
          `RenderSessionSnapshot.failure_status` reports, when the session
          has failed.
        - `Status.INVALID_STATE` when you attached the session with a redraw
          wake, or detached, abandoned, released, or disposed it.
        """
        return self._native.set_frame_pacer(None if pacer is None else pacer._native)

    def set_metal_borrowed_texture_target(
        self, replacement: MetalBorrowedTextureReplacement
    ) -> Future[None]:
        """Replaces the Metal textures that a borrowed ring renders into.

        Use it to resize the ring, or to give the session textures again
        after `RenderSessionHandle.release_target()`. Release every acquired
        frame first. Once the replacement completes, the session no longer
        uses the old textures.

        See `mln_render_session_set_metal_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the new textures have a different pixel
          format than the old ones. Attach a new session to change it.
        """
        return self._native.set_metal_borrowed_texture_target(replacement)

    def set_metal_surface_target(
        self, replacement: MetalSurfaceReplacement
    ) -> Future[None]:
        """Replaces the Metal layer that the session renders to.

        Use it to give the session a layer again after
        `RenderSessionHandle.release_target()`, to move it to another layer,
        or to resize it. Once the replacement completes, the session no
        longer uses the old layer.

        See `mln_render_session_set_metal_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.set_metal_surface_target(replacement)

    def set_open_gl_borrowed_texture_target(
        self, replacement: OpenGlBorrowedTextureReplacement
    ) -> Future[None]:
        """Replaces the OpenGL textures that a borrowed ring renders into.

        Use it to resize the ring, or to give the session textures again
        after `RenderSessionHandle.release_target()`. Release every acquired
        frame first. Once the replacement completes, the session no longer
        uses the old textures.

        See `mln_render_session_set_open_gl_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.set_open_gl_borrowed_texture_target(replacement)

    def set_open_gl_surface_target(
        self, replacement: OpenGlSurfaceReplacement
    ) -> Future[None]:
        """Replaces the OpenGL surface that the session presents to.

        Use it to give the session a surface again after
        `RenderSessionHandle.release_target()`, to move it to another
        surface, or to resize it. Once the replacement completes, the session
        no longer uses the old surface.

        See `mln_render_session_set_open_gl_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
        """
        return self._native.set_open_gl_surface_target(replacement)

    def set_vulkan_borrowed_texture_target(
        self, replacement: VulkanBorrowedTextureReplacement
    ) -> Future[None]:
        """Replaces the Vulkan images that a borrowed ring renders into.

        Use it to resize the ring, or to give the session images again after
        `RenderSessionHandle.release_target()`. Release every acquired frame
        first. Once the replacement completes, the session no longer uses the
        old images.

        See `mln_render_session_set_vulkan_borrowed_texture_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
        """
        return self._native.set_vulkan_borrowed_texture_target(replacement)

    def set_vulkan_surface_target(
        self, replacement: VulkanSurfaceReplacement
    ) -> Future[None]:
        """Replaces the Vulkan surface that the session presents to.

        Use it to give the session a surface again after
        `RenderSessionHandle.release_target()`, to move it to another
        surface, or to resize it. Once the replacement completes, the session
        no longer uses the old surface.

        See `mln_render_session_set_vulkan_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the new surface needs a different color
          format or pre-rotation support than the old one. Attach a new
          session instead.
        """
        return self._native.set_vulkan_surface_target(replacement)

    def set_wayland_surface_target(
        self, replacement: WaylandSurfaceReplacement
    ) -> Future[None]:
        """Replaces the Wayland surface that the session presents to.

        Use it to give the session a surface again after
        `RenderSessionHandle.release_target()`, to move it to another
        surface, or to resize it. Once the replacement completes, the session
        no longer uses the old surface.

        See `mln_render_session_set_wayland_surface_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when, in a Vulkan build, the new surface needs
          a different color format than the old one. Attach a new session
          instead.
        - `Status.NATIVE_ERROR` when the library can't present to the new
          surface.
        """
        return self._native.set_wayland_surface_target(replacement)

    def set_win32_window_target(
        self, replacement: Win32WindowReplacement
    ) -> Future[None]:
        """Replaces the Win32 window that the session presents to.

        Use it to give the session a window again after
        `RenderSessionHandle.release_target()`, to move it to another window,
        or to resize it. Once the replacement completes, the session no
        longer uses the old window.

        See `mln_render_session_set_win32_window_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `window` isn't a window, or, in an
          OpenGL build, already has another pixel format.
        - `Status.INVALID_STATE` when `window` is minimized or has no client
          area.
        - `Status.INVALID_STATE` when `window` was minimized before the
          replacement ran.
        - `Status.UNSUPPORTED` when, in a Vulkan build, the new window needs
          a different color format than the old one. Attach a new session
          instead.
        - `Status.NATIVE_ERROR` when the library can't present to the new
          window.
        """
        return self._native.set_win32_window_target(replacement)

    @property
    def discarding(self) -> RenderSessionHandleDiscarding:
        return RenderSessionHandleDiscarding(self)


class _ResourceRequestHandleOperations(GeneratedOperations):
    _native: _native._ResourceRequestHandle

    def complete(self, response: ResourceResponse) -> None:
        """Answers a resource request.

        A failed answer fails the request with `ResourceErrorReason.OTHER`,
        as `ResourceRequestHandle.release()` does.

        See `mln_resource_request_complete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the library cancelled the request.
        """
        return _answer(self, lambda: self._native.complete(response))

    def is_cancelled(self) -> bool:
        """Reads whether the library cancelled a resource request.

        See `mln_resource_request_is_cancelled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.is_cancelled()

    def pass_through(self) -> None:
        """Hands a resource request back to the library to load as usual.

        A failed call fails the request with `ResourceErrorReason.OTHER`, as
        `ResourceRequestHandle.release()` does.

        See `mln_resource_request_pass_through` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the library cancelled the request.
        """
        return _answer(self, lambda: self._native.pass_through())

    def release(self) -> None:
        """Releases a resource request.

        Releasing a request without an answer fails it with
        `ResourceErrorReason.OTHER`.

        See `mln_resource_request_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.release()

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

        `url` must not be empty. A failed answer fails the request with
        `ResourceErrorReason.OTHER`, as
        `ResourceTransformRequestHandle.release()` does.

        See `mln_resource_transform_request_complete` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return _answer(self, lambda: self._native.complete(url))

    def fail(self, error: ResourceTransformError) -> None:
        """Fails a resource transform request without sending it.

        The library retries a request that failed for a reason it retries,
        such as `ResourceErrorReason.CONNECTION`, and the retry calls the
        transform again. When this call fails, the request fails with
        `ResourceErrorReason.OTHER`, as
        `ResourceTransformRequestHandle.release()` does.

        See `mln_resource_transform_request_fail` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return _answer(self, lambda: self._native.fail(error))

    def release(self) -> None:
        """Releases a resource transform request.

        Releasing a request without an answer fails it with
        `ResourceErrorReason.OTHER`.

        See `mln_resource_transform_request_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.release()


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

    def create_map(self, options: MapOptions) -> MapHandle:
        """Creates a map on `runtime`.

        See `mln_runtime_create_map` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        raw = self._native.create_map(options)
        return _adopt_value(raw["map"], "MapHandle", self, raw["ready"])

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

        While the download is active, the counts cover what it has visited.
        Once it's inactive, they cover every resource the region holds,
        including those that only earlier versions of its style listed, so
        the completed counts can exceed the required counts.

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

        The region keeps its resources. Starting its download again fetches
        the invalidated resources for offline use. If the style or a TileJSON
        document is then missing or invalid, the download reports an offline
        region response error and goes on from the stored copy. Resources
        that the current style no longer lists stay in the region until it's
        deleted.

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
        - `Status.NATIVE_ERROR` when the merge would exceed the tile limit
          that `RuntimeEventType.OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`
          describes.
        """
        return map_future(
            self._native.merge_offline_regions(side_database_path),
            lambda value: tuple(OfflineRegionInfo._from_native(item) for item in value),
        )

    def release(self) -> Future[None]:
        """Releases a runtime.

        Release the runtime's maps first.

        See `mln_runtime_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.release()

    def run_cache_database_operation(
        self, operation: CacheDatabaseOperation
    ) -> Future[None]:
        """Runs a maintenance action on the runtime's cache database.

        See `mln_runtime_run_cache_database_operation` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.run_cache_database_operation(operation)

    def set_event_handler(
        self, handler: RuntimeEventHandler
    ) -> Future[CommandCompletion]:
        """Sends the events of the runtime and its maps to a callback.

        The callback replaces the one set before.

        See `mln_runtime_set_event_handler` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_event_handler(handler)

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

        `size` is in bytes. Offline regions don't count toward it. It
        defaults to 50 MB, or to 0 in the browser when the runtime has no
        cache file.

        See `mln_runtime_set_maximum_ambient_cache_size` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native.set_maximum_ambient_cache_size(size)

    def set_offline_region_download_state(
        self, region_id: int, state: OfflineRegionDownloadState
    ) -> Future[None]:
        """Starts or pauses the download of the offline region `region_id`.

        Setting the state that the download already has does nothing. A
        download stops when the runtime closes.

        See `mln_runtime_set_offline_region_download_state` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
        """
        return self._native.set_offline_region_download_state(region_id, state)

    def set_offline_region_observed(
        self, region_id: int, events: RuntimeEventFlag
    ) -> Future[None]:
        """Selects the events that the runtime reports about the download of the
        offline region `region_id`.

        `events` holds bits of `RuntimeEventFlag.ALL_RUNTIME_EVENTS` and
        replaces the region's previous selection. `RuntimeEventFlag.NONE`
        stops the region's events.

        See `mln_runtime_set_offline_region_observed` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the cache file has no region `region_id`.
        """
        return self._native.set_offline_region_observed(region_id, events)

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
        """Rewrites the URLs of the runtime's network requests.

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

        A number in the result that isn't finite becomes null. A color is
        `["rgba", r, g, b, a]`, with red, green, and blue from 0 to 255 and
        alpha from 0 to 1, each exact only to single precision. Formatted
        text is a `["format", …]` expression, an image is `{"name": …,
        "available": …}`, padding is `[top, right, bottom, left]`, and a
        variable anchor offset is `[anchor, [x, y], …]`.

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

    def release(self) -> None:
        """Releases a parsed style expression.

        See `mln_style_expression_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
        """
        return self._native.release()


class _VulkanDeviceHandleOperations(GeneratedOperations):
    _native: _native._VulkanDeviceHandle

    @classmethod
    def create(cls, options: VulkanDeviceOptions | None = None) -> VulkanDeviceHandle:
        """Creates a Vulkan instance, device, and graphics queue for render
        sessions.

        Attach sessions with the context from
        `VulkanDeviceHandle.get_context()`, in place of a device of your own.
        Without `device_uuid` or `device_luid`, it prefers a discrete GPU.
        The device enables the external memory and semaphore extensions that
        the GPU offers.

        See `mln_vulkan_device_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.VULKAN`, or when `presentation` is true and the
          Vulkan loader or the GPU can't present.
        - `Status.NOT_FOUND` when no GPU matches `device_uuid` and
          `device_luid`.
        - `Status.NATIVE_ERROR` when there is no Vulkan loader, no GPU has a
          graphics queue, or Vulkan can't create the instance or the device.
        """
        return _adopt_value(
            _native._VulkanDeviceHandle.create(options), "VulkanDeviceHandle", None
        )

    def get_context(self) -> VulkanDeviceContextResult:
        """Gets the context to attach render sessions to the device with.

        Pass it as the context of `MapHandle.attach_vulkan_surface()` or
        `MapHandle.attach_vulkan_borrowed_texture()`. The sessions on the
        device share its graphics queue. To submit to the queue yourself, set
        `RenderSessionAttachOptions.queue_lock` on each session.

        See `mln_vulkan_device_get_context` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
        """
        return VulkanDeviceContextResult._from_native(self._native.get_context())

    def release(self) -> None:
        """Releases a Vulkan device.

        Render sessions that attached with the device's context keep using it
        until they detach or their release finishes.

        See `mln_vulkan_device_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
        """
        return self._native.release()


def android_frame_pacer_create(
    options: FramePacerOptions | None = None,
) -> FramePacerHandle:
    """Creates a frame pacer that follows the display through the calling
    thread's `AChoreographer`.

    Call it on a thread with an `ALooper`, such as the app's main thread,
    and keep that thread running its looper while sessions use the pacer.

    See `mln_android_frame_pacer_create` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).

    Errors:

    - `Status.INVALID_STATE` when the calling thread has no `ALooper`.
    """
    return _adopt_value(
        _native.android_frame_pacer_create(options), "FramePacerHandle", None
    )


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


def apple_frame_pacer_create(
    options: FramePacerOptions | None = None,
) -> FramePacerHandle:
    """Creates a frame pacer that follows the display through a
    `CADisplayLink`.

    You can call it on any thread. The pacer needs no run loop of yours.
    On macOS, it follows the main display, the one with the menu bar.

    With `FramePacerOptions.maximum_frames_per_second`, the pacer also
    asks the system for that refresh rate. On iPhone, a rate above 60
    needs `CADisableMinimumFrameDurationOnPhone` in the app's
    `Info.plist`.

    See `mln_apple_frame_pacer_create` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).

    Errors:

    - `Status.NATIVE_ERROR` when the system has no display to follow.
    """
    return _adopt_value(
        _native.apple_frame_pacer_create(options), "FramePacerHandle", None
    )


def c_version() -> int:
    """Returns the API epoch of the library.

    Headers and a library with different epochs aren't compatible.

    See `mln_c_version` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """
    return _native.c_version()


def camera_for_tile(tile: CanonicalTileId, logical_size: LogicalSize) -> CameraOptions:
    """Computes the camera that shows exactly one tile on a square map.

    Jump a map in `MapMode.TILE` to the camera to render `tile`, where
    `logical_size` is the map's size. The camera centers the tile, zooms
    until the tile fills the map, and looks straight down with north up
    and no padding. The map keeps its camera within its zoom limits, so
    for a deep tile, raise the maximum zoom past the camera's with
    `MapHandle.set_bounds()`.

    See `mln_camera_for_tile` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).

    Errors:

    - `Status.INVALID_ARGUMENT` when `logical_size` isn't square, or is
      wider than 512 pixels. For denser tiles, raise the map's scale
      factor instead.
    - `Status.INVALID_ARGUMENT` when `tile.x` or `tile.y` isn't less than
      2 to the power `tile.z`, or when the tile can't fill the map at a
      zoom from 0 to 25.5, as for tile 0/0/0 on a 256-pixel map.
    """
    return CameraOptions._from_native(_native.camera_for_tile(tile, logical_size))


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

    def add_style_layer_json(
        self, layer_json: Any, before_layer_id: str | None = None
    ) -> None:
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

        A delta that replaces an animated one starts from where that
        animation left the camera, so the rest of the replaced change never
        happens. To reach an exact camera, use `MapHandle.update_camera()`.

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

    def cancel_camera_transitions(self) -> None:
        """Stops every running camera animation where it is.

        The end handler of each stopped animation reports
        `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_camera_transitions` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_cancel_camera_transitions()

    def cancel_camera_transitions_with_id(self, transition_id: int) -> None:
        """Stops the running camera animations whose animation options set
        `transition_id`.

        Other animations keep running. The end handler of each stopped
        animation reports `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_cancel_camera_transitions_with_id` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_cancel_camera_transitions_with_id(transition_id)

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

    def fit_camera_to_geometry(self, geometry: Any, update: CameraFitUpdate) -> None:
        """Moves the camera to fit a GeoJSON geometry in the map's view, at once
        or with an animation.

        `geometry` is a GeoJSON geometry object with at least one coordinate,
        not a feature. The camera is the one that
        `MapHandle.camera_for_geometry()` computes after the commands sent
        before this one apply. The function finishes when the map applies the
        change, before an animation ends.

        See `mln_map_fit_camera_to_geometry` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_fit_camera_to_geometry(geometry, update)

    def fit_camera_to_lat_lng_bounds(
        self, bounds: LatLngBounds, update: CameraFitUpdate
    ) -> None:
        """Moves the camera to fit `bounds` in the map's view, at once or with
        an animation.

        The camera is the one that `MapHandle.camera_for_lat_lng_bounds()`
        computes after the commands sent before this one apply. The function
        finishes when the map applies the change, before an animation ends.

        See `mln_map_fit_camera_to_lat_lng_bounds` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_fit_camera_to_lat_lng_bounds(bounds, update)

    def fit_camera_to_lat_lngs(
        self, coordinates: Sequence[LatLng], update: CameraFitUpdate
    ) -> None:
        """Moves the camera to fit `coordinates` in the map's view, at once or
        with an animation.

        `coordinates` must not be empty. The camera is the one that
        `MapHandle.camera_for_lat_lngs()` computes after the commands sent
        before this one apply. The function finishes when the map applies the
        change, before an animation ends.

        See `mln_map_fit_camera_to_lat_lngs` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_fit_camera_to_lat_lngs(coordinates, update)

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

    def invalidate_custom_vector_source_region(
        self, source_id: str, bounds: LatLngBounds
    ) -> None:
        """Makes a custom vector source ask again for the tiles in `bounds` that
        the map still needs.

        See `mln_map_invalidate_custom_vector_source_region` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_invalidate_custom_vector_source_region(
            source_id, bounds
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

    def move_style_layer(
        self, layer_id: str, before_layer_id: str | None = None
    ) -> None:
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

    def resize(self, logical_size: LogicalSize) -> None:
        """Sets the map's size.

        A map with a render session takes its size from the session, so
        resize the session with `RenderSessionHandle.resize()` instead.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the map has a render session.
        """
        return self._native._discard_resize(logical_size)

    def set_bounds(self, options: BoundOptions) -> None:
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

        `data` is a GeoJSON feature collection, feature, or geometry. The
        source ignores data for a tile that it hasn't asked for, or has since
        cancelled.

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

    def set_debug_flags(self, flags: MapDebugFlag) -> None:
        """Draws debugging overlays, such as tile borders, on the map.

        `flags` replaces the overlays set before.

        See `mln_map_set_debug_flags` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_debug_flags(flags)

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

    def set_free_camera_options(self, options: FreeCameraOptions) -> None:
        """Places the camera at a position and orientation in space.

        The map adjusts them to a camera that it can show. It stops running
        camera animations first, and their end handlers report
        `CameraTransitionOutcome.CANCELLED`.

        See `mln_map_set_free_camera_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_free_camera_options(options)

    def set_geo_json_source_data(
        self, source_id: str, data: GeoJsonSourceDataHandle
    ) -> None:
        """Replaces the data of a GeoJSON source.

        `data` must have been prepared with the source's options, which
        `MapHandle.get_style_source()` reports, though `min_zoom` may differ.
        `data` can be released once this function returns.

        See `mln_map_set_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geo_json_source_data(source_id, data._native)

    def set_geo_json_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> None:
        """Sets whether a GeoJSON source builds its tiles while the map renders.

        Building tiles while the map renders, instead of in the background,
        makes new data show in the next frame. The setting replaces the
        `synchronousUpdate` member of the source's JSON, which defaults to
        false.

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

    def set_projection_mode(self, options: ProjectionModeOptions) -> None:
        """Sets whether the map draws in perspective or axonometric projection,
        and how much it skews the view.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_projection_mode` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_projection_mode(options)

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

        The command completes once the style has parsed, and the map's style
        is then loaded, as `MapHandle.set_style_url()` describes. The style's
        sources, sprites, and glyphs load in the background.

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

    def set_style_transition_options(self, options: StyleTransitionOptions) -> None:
        """Sets the default transition of the map's style.

        See `mln_map_set_style_transition_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_transition_options(options)

    def set_style_url(self, url: str) -> None:
        """Loads the map's style from `url`.

        The command completes once the style document has loaded and parsed.
        The style's sources, sprites, and glyphs then load in the background.

        The map's style is loaded from the time a style document parses until
        the next call to this function. While the style isn't loaded, the
        commands that change it complete with `Status.INVALID_STATE`. To
        change a new map's style without a style document, first load an
        empty style, such as `{"version":8,"sources":{},"layers":[]}`.

        See `mln_map_set_style_url` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        return self._native._discard_set_style_url(url)

    def set_tile_options(self, options: MapTileOptions) -> None:
        """Sets how the map chooses which tiles to load, and at what detail.

        Settings that `options` doesn't set keep their current values.

        See `mln_map_set_tile_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_tile_options(options)

    def set_viewport_options(self, options: MapViewportOptions) -> None:
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

    def resize(self, logical_size: LogicalSize) -> None:
        """Resizes the session's target and its map.

        The session renders at the map's scale factor. Release every acquired
        frame first.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders into a borrowed ring.
          Give it new textures of the new size instead.
        """
        return self._native._discard_resize(logical_size)


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
        """Rewrites the URLs of the runtime's network requests.

        The transform replaces the one set before.

        See `mln_runtime_set_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_resource_transform(transform)
