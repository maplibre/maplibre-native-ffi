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
    CustomLayerHost,
    CustomVectorSourceOptions,
    DmaBufFrame,
    DxgiSharedTextureFrame,
    FeatureStateSelector,
    FrameDemand,
    FramePacerOptions,
    FreeCameraOptions,
    GeoJsonSourceOptions,
    GpuDeviceCapabilities,
    GpuDeviceOptions,
    GpuDeviceVulkanContextResult,
    GpuSync,
    HttpHeaderTransform,
    IoSurfaceFrame,
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
    MetalDeviceDescriptor,
    MetalTextureFrame,
    NetworkStatus,
    OfflineRegionDefinition,
    OfflineRegionDownloadState,
    OfflineRegionInfo,
    OfflineRegionStatus,
    OpaqueFdFrame,
    OpenGlContextDescriptor,
    OpenGlContextProviderFlag,
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
    RenderTarget,
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
    VulkanDeviceDescriptor,
    VulkanTextureFrame,
    _maybe,
)
from ._operation import GeneratedOperations, _adopt_value, _answer, _view

if TYPE_CHECKING:
    from ._generated_owners import (
        AcquiredFrameHandle,
        FramePacerHandle,
        GpuDeviceHandle,
        MapHandle,
        MapProjectionHandle,
        RenderSessionHandle,
        RuntimeHandle,
        StyleExpressionHandle,
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
        never renders into that texture again, even when
        `RenderSessionHandle.set_target()` gives it the texture again, until
        `RenderSessionHandle.release_target()` releases the ring. A view that
        is open on the frame keeps the frame's sync objects in place until it
        ends.

        See `mln_acquired_frame_discard` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.discard()

    def android_hardware_buffer(
        self,
    ) -> AbstractContextManager[AndroidHardwareBufferFrame]:
        """Gets the Android hardware buffer of an exported ring's acquired
        frame.

        Wait for `AcquiredFrameHandle.producer_sync()` before you read the
        buffer. Once you release or discard the frame, the session may render
        into the buffer again. A reference that you take with
        `AHardwareBuffer_acquire` keeps the buffer's memory, but not its
        contents.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_android_hardware_buffer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session's ring shares its textures in
          another way.
        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.android_hardware_buffer(),
            lambda raw: AndroidHardwareBufferFrame._from_native(raw),
        )

    def dma_buf(self) -> AbstractContextManager[DmaBufFrame]:
        """Gets the dmabuf of an exported ring's acquired frame.

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
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session's ring shares its textures in
          another way.
        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.dma_buf(),
            lambda raw: DmaBufFrame._from_native(raw),
        )

    def dxgi_shared_texture(self) -> AbstractContextManager[DxgiSharedTextureFrame]:
        """Gets the DXGI shared texture of an exported ring's acquired frame.

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
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session's ring shares its textures in
          another way.
        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.dxgi_shared_texture(),
            lambda raw: DxgiSharedTextureFrame._from_native(raw),
        )

    def io_surface(self) -> AbstractContextManager[IoSurfaceFrame]:
        """Gets the IOSurface of an exported ring's acquired frame.

        Wait for `AcquiredFrameHandle.producer_sync()` before you read the
        surface. Once you release or discard the frame, the session may
        render into the surface again. For an API that takes a
        `CVPixelBuffer`, wrap the surface with
        `CVPixelBufferCreateWithIOSurface`. A reference that you take with
        `CFRetain`, or a pixel buffer that wraps the surface, keeps the
        surface's memory, but not its contents.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_io_surface` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session's ring shares its textures in
          another way.
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

        It gets a borrowed ring's texture, or an exported ring's on a Metal
        device: the texture of the IOSurface that
        `AcquiredFrameHandle.io_surface()` gets, and the frame's depth
        texture with `RenderSessionAttachOptions.keep_depth`. Wait for
        `AcquiredFrameHandle.producer_sync()` before you use them.

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

    def opaque_fd(self) -> AbstractContextManager[OpaqueFdFrame]:
        """Gets the memory of an exported ring's acquired frame, as an opaque
        file descriptor.

        Wait for `AcquiredFrameHandle.producer_sync()` before you read the
        image. Once you release or discard the frame, the session may render
        into the image again.

        The session owns the file descriptor, so don't close it. An import
        takes the descriptor, so import a duplicate from `dup`. Memory that
        you import keeps the image's memory, but not its contents.

        Native resources remain valid only inside the `with` block.

        See `mln_acquired_frame_get_opaque_fd` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session's ring shares its textures in
          another way.
        - `Status.TARGET_LOST` when the session failed, or you detached,
          abandoned, or disposed it.
        """
        return _view(
            self,
            lambda: self._native.opaque_fd(),
            lambda raw: OpaqueFdFrame._from_native(raw),
        )

    def open_gl_texture(self) -> AbstractContextManager[OpenGlTextureFrame]:
        """Gets the OpenGL texture of an acquired frame.

        It gets a borrowed ring's texture, or an exported ring's on an OpenGL
        device: the texture of the dmabuf that
        `AcquiredFrameHandle.dma_buf()` gets, and the frame's depth texture
        with `RenderSessionAttachOptions.keep_depth`, in the device's share
        group. An exported ring's textures hold the frame upside down from
        OpenGL's rows, top row first, as a dmabuf's readers take it, so
        negate clip y to draw into them. Wait for
        `AcquiredFrameHandle.producer_sync()` before you use them.

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

        It gets a borrowed ring's image, or an exported ring's on a Vulkan
        device: the image whose memory the ring shares, and the frame's depth
        image with `RenderSessionAttachOptions.keep_depth`. An exported
        ring's frame releases them to VK_QUEUE_FAMILY_EXTERNAL, so acquire
        them from there. Wait for `AcquiredFrameHandle.producer_sync()`
        before you use them.

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

        `consumer_completion` says when your GPU work on the texture, and on
        its depth texture, finishes. Pass `GpuSyncKind.CPU_COMPLETE` when
        that work has already finished. The session waits for that point
        before it renders into the textures again. If that wait fails, such
        as for an OpenGL fence outside the session's share group, the session
        fails.

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

    @classmethod
    def create(cls, options: FramePacerOptions | None = None) -> FramePacerHandle:
        """Creates a frame pacer that follows the platform's display.

        On iOS, macOS, and tvOS, the pacer follows the display through a
        `CADisplayLink`. You can call it on any thread, and the pacer needs
        no run loop of yours. On macOS, it follows the main display, the one
        with the menu bar. With
        `FramePacerOptions.maximum_frames_per_second`, the pacer also asks
        the system for that refresh rate. On iPhone, a rate above 60 needs
        `CADisableMinimumFrameDurationOnPhone` in the app's `Info.plist`.

        On Android, the pacer follows the display through the calling
        thread's `AChoreographer`. Call it on a thread with an `ALooper`,
        such as the app's main thread, and keep that thread running its
        looper while sessions use the pacer. `AChoreographer` can stop while
        the phone's display is off, even while a Surface on another display,
        such as an Android Auto projection, still shows the map. The pacer
        then keeps demanding frames from a timer on the looper, within the
        frame rate limit, until vsyncs arrive again.

        On OpenHarmony, the pacer follows the display through
        `OH_NativeVSync`. You can call it on any thread, and the pacer needs
        no event loop of yours.

        See `mln_frame_pacer_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` on the other platforms, such as visionOS,
          Linux, and Windows, where the library has no source of display
          refreshes.
        - `Status.INVALID_STATE` on Android when the calling thread has no
          `ALooper`.
        - `Status.NATIVE_ERROR` on iOS, macOS, and tvOS when the system has
          no display to follow, and on OpenHarmony when the system can't
          report the display's refreshes to the process.
        """
        return _adopt_value(
            _native._FramePacerHandle.create(options), "FramePacerHandle", None
        )

    def release(self) -> None:
        """Releases a frame pacer.

        Render sessions that use the pacer stay paced until you release them
        or set another pacer.

        See `mln_frame_pacer_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        return self._native.release()

    def set_options(self, options: FramePacerOptions) -> None:
        """Changes a frame pacer's settings.

        Settings that `options` doesn't set keep their current values. To
        remove the frame rate limit, set
        `FramePacerOptions.unlimited_frames_per_second`. You can call it on
        any thread.

        The sessions that the pacer serves follow the new settings from the
        next display refresh, including a refresh that they already wait for.
        The pacer counts a new limit from the last frames it demanded, so a
        higher limit can demand the next frames sooner. On iOS, macOS, and
        tvOS, the pacer also asks the system for the new limit's refresh
        rate, or for the display's own rate once the limit is removed.

        See `mln_frame_pacer_set_options` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.INVALID_STATE` when you released `pacer`, even while
          sessions still use it.
        """
        return self._native.set_options(options)


class _GpuDeviceHandleOperations(GeneratedOperations):
    _native: _native._GpuDeviceHandle

    @classmethod
    def create(cls, options: GpuDeviceOptions | None = None) -> GpuDeviceHandle:
        """Creates a GPU device that the library owns, for render sessions to
        share.

        The sessions on a device share its GPU and queue: one `VkDevice` and
        queue, one `MTLDevice` and command queue, or one OpenGL share group.
        `MapHandle.attach()` creates a device of its own when it gets none.
        Create one to share it among the sessions of many maps, or to pick
        the backend or the GPU. A Vulkan device enables the presentation,
        timeline semaphore, external memory, and external semaphore
        extensions that its GPU offers.

        See `mln_gpu_device_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when this build or platform lacks `backend`,
          when the device can't render with `open_gl_client_api`, when a
          device that doesn't render with Vulkan gets a `queue_lock`, or in a
          browser for every backend: WebGPU makes devices asynchronously, and
          a WebGL2 context needs a canvas. Attach without a device, which the
          session makes itself, or wrap your WebGL2 context.
        - `Status.NOT_FOUND` when no GPU matches `adapter_index`,
          `adapter_uuid`, or `adapter_luid`.
        - `Status.NATIVE_ERROR` when the platform can't create the device.
        """
        return _adopt_value(
            _native._GpuDeviceHandle.create(options), "GpuDeviceHandle", None
        )

    def get_capabilities(self) -> GpuDeviceCapabilities:
        """Reads what the device can do.

        Check `export_kinds` before you attach an exported ring, to choose
        between the ring and a way of your own to show the map.

        See `mln_gpu_device_get_capabilities` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
        """
        return GpuDeviceCapabilities._from_native(self._native.get_capabilities())

    def get_vulkan_context(self) -> GpuDeviceVulkanContextResult:
        """Gets the Vulkan instance, device, and graphics queue of a Vulkan
        device.

        Use them to make the images of a borrowed ring, or a surface, that
        sessions on the device render to. The sessions on the device share
        its graphics queue. To submit to the queue yourself, create the
        device with `GpuDeviceOptions.queue_lock`.

        See `mln_gpu_device_get_vulkan_context` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the device doesn't render with Vulkan.
        """
        return GpuDeviceVulkanContextResult._from_native(
            self._native.get_vulkan_context()
        )

    def release(self) -> None:
        """Releases a GPU device.

        Render sessions on the device keep using it until they detach, or
        their release finishes.

        See `mln_gpu_device_release` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
        """
        return self._native.release()

    @classmethod
    def wrap_metal(cls, descriptor: MetalDeviceDescriptor) -> GpuDeviceHandle:
        """Wraps your Metal device and command queue, so that sessions render on
        them.

        See `mln_gpu_device_wrap_metal` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.METAL`.
        """
        return _adopt_value(
            _native._GpuDeviceHandle.wrap_metal(descriptor), "GpuDeviceHandle", None
        )

    @classmethod
    def wrap_open_gl(cls, context: OpenGlContextDescriptor) -> GpuDeviceHandle:
        """Makes an OpenGL device whose share group is your context's.

        Each session on the device makes a context of its own that shares
        objects with yours, so that it can render into your textures and
        present to your surfaces. A device that the library creates is a
        share group of its own.

        WebGL has no share groups: a WebGL device is your context. Its
        sessions present to a web canvas target whose canvas is the context's
        own, and take turns in the context. The library resets the context's
        state that it tracks at the start of each frame, and leaves it
        undefined after.

        See `mln_gpu_device_wrap_open_gl` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when
          `open_gl_supported_context_provider_mask()` lacks the context's
          platform.
        - `Status.NATIVE_ERROR` when the platform rejects the context.
        """
        return _adopt_value(
            _native._GpuDeviceHandle.wrap_open_gl(context), "GpuDeviceHandle", None
        )

    @classmethod
    def wrap_vulkan(cls, descriptor: VulkanDeviceDescriptor) -> GpuDeviceHandle:
        """Wraps your Vulkan device and queue, so that sessions render on them.

        Sessions on the device render into your images, present to your
        surfaces, and submit to your queue, under your queue lock when you
        give one. Sessions of several devices that wrap one queue never
        submit to it at once.

        See `mln_gpu_device_wrap_vulkan` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when `supported_render_backend_mask()` lacks
          `RenderBackendFlag.VULKAN`.
        - `Status.NATIVE_ERROR` when the library can't load the device's
          functions.
        """
        return _adopt_value(
            _native._GpuDeviceHandle.wrap_vulkan(descriptor), "GpuDeviceHandle", None
        )


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

    def add_custom_layer(
        self,
        layer_id: str,
        before_layer_id: str | None,
        host: CustomLayerHost | NativeCustomLayerHost,
    ) -> Future[CommandCompletion]:
        """Adds a layer that draws with your own GPU commands, through `host`.

        The layer goes below the layer named `before_layer_id`, or above
        every layer when `before_layer_id` is empty. Its type is `custom`,
        and it takes the `visibility`, `minzoom`, and `maxzoom` properties,
        but no paint properties. `MapHandle.remove_style_layer()` removes it.

        Runs on the thread that runs the session's graphics work: a library
        thread, or yours with `RenderDriverKind.CALLER_GRAPHICS_THREAD`. It
        can't call the library.

        It can't call the library.

        See `mln_map_add_custom_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no layer named
          `before_layer_id`.
        - `Status.ALREADY_EXISTS` when the style already has a layer named
          `layer_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.add_custom_layer(layer_id, before_layer_id, host)

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

    def add_geo_json_source(
        self,
        source_id: str,
        options: GeoJsonSourceOptions | None = None,
        attribution: str | None = None,
    ) -> Future[CommandCompletion]:
        """Adds a GeoJSON source with no data.

        Give it data with `MapHandle.set_geo_json_source_data()` or
        `MapHandle.set_geo_json_source_url()`. Its options stay fixed for as
        long as the style holds it.

        `attribution` is the source's
        \\[attribution\\](https://maplibre.org/maplibre-style-spec/sources/#attribution),
        or empty for none.

        See `mln_map_add_geo_json_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when an option in `options` is out of
          range.
        - `Status.ALREADY_EXISTS` when the style already has a source named
          `source_id`.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.add_geo_json_source(source_id, options, attribution)

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

        A delta in the end phase can carry momentum, which keeps the camera
        moving from the gesture's release velocities and slows it on each
        rendered frame. The gesture lasts until the momentum ends: the
        snapshot keeps `gesture_in_progress` set, and camera events report
        `CameraChangeSource.MOMENTUM`. With momentum, `animation` sets only
        `transition_id` and the end handler, which runs when the gesture
        ends.

        A delta in the begin phase, `MapHandle.cancel_camera_transitions()`,
        or a cancellation by that ID stops the momentum. A later command that
        sets a field stops only the parts of the momentum that move that
        field. On a tilted map, the pan moves the map's content the same
        distance on the screen in every direction.

        See `mln_map_apply_camera_delta` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.apply_camera_delta(delta)

    def attach(
        self,
        target: RenderTarget,
        device: GpuDeviceHandle | None,
        options: RenderSessionAttachOptions,
    ) -> RenderSessionHandle:
        """Attaches a render session that renders the map into `target`.

        The session renders on `device`. When `device` is null, the session
        creates a device with the default options, and releases it when the
        session ends.

        See `mln_map_attach` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when a borrowed ring has depth textures,
          but not one for each of its textures.
        - `Status.INVALID_STATE` when the map already has a render session,
          or a window is minimized or has no client area.
        - `Status.UNSUPPORTED` when the device's
          `GpuDeviceCapabilities.target_kinds` lacks the target's kind, or,
          for a null `device`, when no backend of this build renders to it,
          when `options` sets `keep_depth` for a target other than an
          exported ring, or when a borrowed ring's depth format isn't one of
          the device's `GpuDeviceCapabilities.depth_stencil_formats`.
        - `Status.INVALID_STATE` when a window was minimized or had no client
          area when the attach ran.
        - `Status.UNSUPPORTED` when the GPU or the graphics driver can't
          present to the window.
        - `Status.NATIVE_ERROR` when `device` is null and the platform can't
          create a device, or when the library can't create its graphics
          context or present to the window.
        """
        raw = self._native.attach(
            target, None if device is None else device._native, options
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

        The object is empty when the feature has no state, including after
        its source leaves the style or a style loads, as
        `MapHandle.set_feature_state()` describes.

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

        Errors:

        - `Status.INVALID_STATE` until `RuntimeHandle.create_map()` completes
          with `Status.OK`.
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

    def get_style_root_property(self, property_name: str) -> Future[Any]:
        """Reads a \\[root
        property\\](https://maplibre.org/maplibre-style-spec/root/) of the
        map's style.

        The value can differ in form from the value that was set: a color set
        as `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.

        Completes with no value when neither the style nor the library sets
        the property. The library can give a property a value of its own,
        such as a default `transition`.

        See `mln_map_get_style_root_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `property_name` isn't a root
          property, or is `sources` or `layers`.
        """
        return self._native.get_style_root_property(property_name)

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
        image needs has loaded. While the image waits, the session holds a
        demand of its own for it, whose frame result carries
        `FRAME_DEMAND_TOKEN_STILL_IMAGE`:

        - It waits for the map to load what the image needs, and while every
          texture of a borrowed ring holds a frame.
        - It finishes with the image's frame, and the image completes once
          that frame's result is queued. Frames of your own demands complete
          no image.
        - Your demands never replace it, nor it them.
        - `RenderSessionHandle.barrier()` ends it with
          `RenderResult.NO_UPDATE`, and a detach or
          `RenderSessionHandle.release_target()` ends it with
          `RenderResult.TARGET_NOT_READY`. The image gets a new demand once
          the barrier runs, the session has a target again, or the next
          session attaches.
        - An image that fails ends its demand with `RenderResult.NO_UPDATE`.

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
        """Sets the map's size and scale factor.

        The map keeps its loaded tiles. A map with a render session takes its
        size and scale factor from the session, so resize the session with
        `RenderSessionHandle.resize()` instead.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the map has a render session.
        - `Status.INVALID_STATE` when a render session attached to the map
          before the resize applied.
        """
        return self._native.resize(extent)

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

        The map keeps the state until it is removed, its source leaves the
        style, or a style loads. `MapHandle.remove_style_source()` removes
        the state of the source's features, and a style that loads removes
        the state of every source, so a later source with the same ID starts
        with no state. To set state for a new style's sources, wait until
        `MapHandle.set_style_url()` or `MapHandle.set_style_json()`
        completes. The style load removes state set before then.

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
        self, source_id: str, data: Any
    ) -> Future[CommandCompletion]:
        """Replaces the data of a GeoJSON source.

        `data` is a GeoJSON feature collection, feature, or geometry. The
        library prepares it for the source with the source's options, off the
        threads that the caller and the runtime use, and the command
        completes once the source holds it. Later commands don't wait for it.
        While the source is still preparing earlier data, only the newest
        data waits to follow it.

        With synchronous tiling on, as
        `MapHandle.set_geo_json_source_synchronous_tiling()` describes, the
        command prepares `data` before later commands run, so the data shows
        in the next frame.

        See `mln_map_set_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.NOT_FOUND` when the style has no source named `source_id`.
        - `Status.INVALID_ARGUMENT` when the source named `source_id` isn't a
          GeoJSON source, when `data` isn't valid GeoJSON, or when the source
          clusters and `data` isn't a feature collection of points. The
          source keeps its data.
        - `Status.CANCELLED` when newer data for the source replaces `data`
          while it waits, or when the source's URL, its removal, a style
          load, or the map's release replaces the source's data before `data`
          is ready.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_geo_json_source_data(source_id, data)

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

    def set_globe_free_bearing_enabled(
        self, enabled: bool
    ) -> Future[CommandCompletion]:
        """Sets whether drags and zooms on a globe turn the bearing.

        When it's on, a drag or a zoom around a point on the globe turns the
        bearing so the planet keeps its orientation, and a drag can carry the
        camera over a pole. When it's off, the default, they keep the
        bearing, as MapLibre GL JS does. Turn it on for a map whose users can
        rotate it. It changes nothing while the map draws Mercator.

        See `mln_map_set_globe_free_bearing_enabled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native.set_globe_free_bearing_enabled(enabled)

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

        - `Status.INVALID_ARGUMENT` when `json` isn't a valid style. The map
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

    def set_style_root_property(
        self, property_name: str, value: Any
    ) -> Future[CommandCompletion]:
        """Sets a \\[root
        property\\](https://maplibre.org/maplibre-style-spec/root/) of the
        map's style, such as `light`, `sky`, `projection`, or `terrain`,
        without reloading the style.

        `property_name` is a root property other than `sources` and `layers`,
        which the source and layer calls change. A null `value` resets the
        property to its default, as if the style didn't set it.

        See `mln_map_set_style_root_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `property_name` isn't a root
          property, is `sources` or `layers`, or when `value` isn't a valid
          value for it.
        - `Status.UNSUPPORTED` when the library can't change the property
          while the style is loaded. Load a style that sets it instead.
        - `Status.INVALID_STATE` while the style isn't loaded, as
          `MapHandle.set_style_url()` describes.
        """
        return self._native.set_style_root_property(property_name, value)

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
        a new one after the map's camera, size, or style projection changes.

        See `mln_map_projection_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the view's style projection draws a globe
          and the library can't project one.
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

    def is_location_occluded(self, coordinate: LatLng) -> bool:
        """Reports whether the globe hides a coordinate from the camera.

        Mercator hides nothing, so this reports false unless the view's style
        projection draws a globe.

        See `mln_map_projection_is_location_occluded` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.is_location_occluded(coordinate)

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

        The point is in the copy of the world nearest the camera's center. On
        a globe, a coordinate on the far side also gets a point;
        `MapProjectionHandle.is_location_occluded()` says whether the globe
        hides it.

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

        Camera fields that `camera` doesn't set keep their values. The style
        projection stays as the view had it, even at another zoom.

        See `mln_map_projection_set_camera` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
        """
        return self._native.set_camera(camera)


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

        A session lets go of your graphics objects later when you call this
        on the thread that runs its graphics work: from a callback that the
        session runs there, once the callback returns, and for a session
        driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD`, at your next
        `RenderSessionHandle.service_driver_work()` for it.

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

        The frame shows the map commands that you sent before the call,
        except the changes that an open command group holds back.

        You can call it once the attach call returns. While the attach runs,
        the demand waits for it to complete. Every demand finishes with one
        frame result that carries its token, and
        `RenderSessionHandle.drain_frame_results()` takes it. A demand
        without `FrameDemandFlag.FORCE` replaces each pending demand with the
        same flags.

        See `mln_render_session_demand_frame` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when `demand.token` is
          `FRAME_DEMAND_TOKEN_STILL_IMAGE`.
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
        Once the release completes, you can destroy the old target. An
        Android or OpenHarmony window can go away sooner, as
        `AndroidWindowTarget` and `OhosWindowTarget` say.

        For a borrowed ring, a completed release also means that the session
        has stopped waiting on the consumer sync that you released its frames
        with, so you can reuse or destroy the textures. The session waits
        about a second at most for that sync, and logs a warning when the
        wait runs out. It checks the sync without blocking a thread: keep
        servicing a session driven by
        `RenderDriverKind.CALLER_GRAPHICS_THREAD` until the release
        completes. The session keeps nothing of the released textures: a
        texture that a later target gives it again is a new one, even one
        that you discarded a frame of.

        See `mln_render_session_release_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders offscreen or into an
          exported ring.
        - `Status.TARGET_LOST` when the session loses its target before the
          release runs.
        """
        return self._native.release_target()

    def resize(self, extent: LogicalExtent) -> Future[CommandCompletion]:
        """Resizes the session's target and its map, and sets the map's scale
        factor.

        Use it when the window changes size, or moves to a display with
        another scale factor. The map keeps its loaded tiles, and draws its
        next frame at the new size and scale factor. You can call it once the
        attach call returns. While the attach runs, the resize waits for it
        to complete.

        An exported ring gets textures of the new size. The frames that you
        hold keep their textures, of the old size, until you release or
        discard them.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders into a borrowed ring.
          Give it new textures of the new size instead.
        - `Status.INVALID_STATE` when the session failed, or you detached,
          abandoned, released, or disposed it.
        - `Status.TARGET_LOST` when the session loses its target before the
          resize applies.
        """
        return self._native.resize(extent)

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

    def set_target(self, target: RenderTarget) -> Future[None]:
        """Replaces the window, surface, or borrowed ring that the session
        renders to, and resizes the map to it.

        Use it to give the session a target again after
        `RenderSessionHandle.release_target()`, to move it to another window
        or surface, or to give a borrowed ring new textures, such as of a new
        size. The session renders to `target` with the device that it
        attached with. Once the replacement completes, the session no longer
        uses the old target. A borrowed ring takes only a ring of the kind it
        attached with, with as many textures, with depth textures only when
        it attached with them, and with the same formats, final layouts, and
        owner. Release every acquired frame of a borrowed ring first. Once
        the replacement completes, the session has also stopped waiting on
        the consumer sync of the textures that the new ring doesn't have, as
        `RenderSessionHandle.release_target()` says, so you can reuse or
        destroy them.

        See `mln_render_session_set_target` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when a borrowed ring gets more or fewer
          textures than it attached with, or depth textures but not one for
          each texture.
        - `Status.INVALID_STATE` when a window is minimized or has no client
          area, or while you hold one of a borrowed ring's acquired frames.
        - `Status.UNSUPPORTED` when the session renders offscreen or into an
          exported ring, when `target` is offscreen or an exported ring, when
          a session that presents gets a borrowed ring or a borrowed ring
          gets anything else, or when no backend of this build renders to the
          target's kind. An exported ring takes a new size from
          `RenderSessionHandle.resize()` instead.
        - `Status.INVALID_STATE` when a window was minimized or had no client
          area when the replacement ran.
        - `Status.UNSUPPORTED` when the session's device can't render to
          `target`, the new target needs a different color format or
          pre-rotation support than the old one, or a borrowed ring's new
          textures differ from the old ones in their formats, final layouts,
          or owner, or in having depth textures. Attach a new session
          instead.
        - `Status.NATIVE_ERROR` when the library can't render to the new
          target.
        """
        return self._native.set_target(target)

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
        regions that they share.

        In a browser, the library runs on the thread that loaded its module
        and never blocks it: commands, render sessions, completions, wakes
        and callbacks all run on that thread, in tasks of the library's own
        after the call that started them returns. Wait for a completion with
        a callback, or yield to the event loop; a blocking wait can't end.
        Load the module on a dedicated Web Worker, so that the map's work
        doesn't delay the page. On the page's thread it works too, but every
        frame and style parse runs there.

        See `mln_runtime_create` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the cache database file that
          `options` names can't be opened or created.
        - `Status.UNSUPPORTED` in a browser for a cache path: the browser
          caches the runtime's requests.
        """
        return _adopt_value(
            _native._RuntimeHandle.create(options), "RuntimeHandle", None
        )

    def create_map(self, options: MapOptions) -> MapHandle:
        """Creates a map on `runtime`.

        Asynchronous calls on the map before the completion runs wait for it,
        and complete with `Status.INVALID_STATE` when creation fails.

        See `mln_runtime_create_map` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when the `fallback_font_faces` of
          `options` isn't an array of font files.
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

    def add_custom_layer(
        self,
        layer_id: str,
        before_layer_id: str | None,
        host: CustomLayerHost | NativeCustomLayerHost,
    ) -> None:
        """Adds a layer that draws with your own GPU commands, through `host`.

        The layer goes below the layer named `before_layer_id`, or above
        every layer when `before_layer_id` is empty. Its type is `custom`,
        and it takes the `visibility`, `minzoom`, and `maxzoom` properties,
        but no paint properties. `MapHandle.remove_style_layer()` removes it.

        Runs on the thread that runs the session's graphics work: a library
        thread, or yours with `RenderDriverKind.CALLER_GRAPHICS_THREAD`. It
        can't call the library.

        It can't call the library.

        See `mln_map_add_custom_layer` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_custom_layer(layer_id, before_layer_id, host)

    def add_custom_vector_source(
        self, source_id: str, options: CustomVectorSourceOptions
    ) -> None:
        """Adds a source whose vector tiles come from callbacks.

        See `mln_map_add_custom_vector_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_add_custom_vector_source(source_id, options)

    def add_geo_json_source(
        self,
        source_id: str,
        options: GeoJsonSourceOptions | None = None,
        attribution: str | None = None,
    ) -> None:
        """Adds a GeoJSON source with no data.

        Give it data with `MapHandle.set_geo_json_source_data()` or
        `MapHandle.set_geo_json_source_url()`. Its options stay fixed for as
        long as the style holds it.

        `attribution` is the source's
        \\[attribution\\](https://maplibre.org/maplibre-style-spec/sources/#attribution),
        or empty for none.

        See `mln_map_add_geo_json_source` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).

        Errors:

        - `Status.INVALID_ARGUMENT` when an option in `options` is out of
          range.
        """
        return self._native._discard_add_geo_json_source(
            source_id, options, attribution
        )

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

        A delta in the end phase can carry momentum, which keeps the camera
        moving from the gesture's release velocities and slows it on each
        rendered frame. The gesture lasts until the momentum ends: the
        snapshot keeps `gesture_in_progress` set, and camera events report
        `CameraChangeSource.MOMENTUM`. With momentum, `animation` sets only
        `transition_id` and the end handler, which runs when the gesture
        ends.

        A delta in the begin phase, `MapHandle.cancel_camera_transitions()`,
        or a cancellation by that ID stops the momentum. A later command that
        sets a field stops only the parts of the momentum that move that
        field. On a tilted map, the pan moves the map's content the same
        distance on the screen in every direction.

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

    def resize(self, extent: LogicalExtent) -> None:
        """Sets the map's size and scale factor.

        The map keeps its loaded tiles. A map with a render session takes its
        size and scale factor from the session, so resize the session with
        `RenderSessionHandle.resize()` instead.

        See `mln_map_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).

        Errors:

        - `Status.INVALID_STATE` when the map has a render session.
        """
        return self._native._discard_resize(extent)

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

        The map keeps the state until it is removed, its source leaves the
        style, or a style loads. `MapHandle.remove_style_source()` removes
        the state of the source's features, and a style that loads removes
        the state of every source, so a later source with the same ID starts
        with no state. To set state for a new style's sources, wait until
        `MapHandle.set_style_url()` or `MapHandle.set_style_json()`
        completes. The style load removes state set before then.

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

    def set_geo_json_source_data(self, source_id: str, data: Any) -> None:
        """Replaces the data of a GeoJSON source.

        `data` is a GeoJSON feature collection, feature, or geometry. The
        library prepares it for the source with the source's options, off the
        threads that the caller and the runtime use, and the command
        completes once the source holds it. Later commands don't wait for it.
        While the source is still preparing earlier data, only the newest
        data waits to follow it.

        With synchronous tiling on, as
        `MapHandle.set_geo_json_source_synchronous_tiling()` describes, the
        command prepares `data` before later commands run, so the data shows
        in the next frame.

        See `mln_map_set_geo_json_source_data` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_geo_json_source_data(source_id, data)

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

    def set_globe_free_bearing_enabled(self, enabled: bool) -> None:
        """Sets whether drags and zooms on a globe turn the bearing.

        When it's on, a drag or a zoom around a point on the globe turns the
        bearing so the planet keeps its orientation, and a drag can carry the
        camera over a pole. When it's off, the default, they keep the
        bearing, as MapLibre GL JS does. Turn it on for a map whose users can
        rotate it. It changes nothing while the map draws Mercator.

        See `mln_map_set_globe_free_bearing_enabled` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
        """
        return self._native._discard_set_globe_free_bearing_enabled(enabled)

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

    def set_style_light_property(self, property_name: str, value: Any) -> None:
        """Sets a property of the light of the map's style.

        See `mln_map_set_style_light_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_light_property(property_name, value)

    def set_style_root_property(self, property_name: str, value: Any) -> None:
        """Sets a \\[root
        property\\](https://maplibre.org/maplibre-style-spec/root/) of the
        map's style, such as `light`, `sky`, `projection`, or `terrain`,
        without reloading the style.

        `property_name` is a root property other than `sources` and `layers`,
        which the source and layer calls change. A null `value` resets the
        property to its default, as if the style didn't set it.

        See `mln_map_set_style_root_property` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
        """
        return self._native._discard_set_style_root_property(property_name, value)

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

    def resize(self, extent: LogicalExtent) -> None:
        """Resizes the session's target and its map, and sets the map's scale
        factor.

        Use it when the window changes size, or moves to a display with
        another scale factor. The map keeps its loaded tiles, and draws its
        next frame at the new size and scale factor. You can call it once the
        attach call returns. While the attach runs, the resize waits for it
        to complete.

        An exported ring gets textures of the new size. The frames that you
        hold keep their textures, of the old size, until you release or
        discard them.

        See `mln_render_session_resize` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).

        Errors:

        - `Status.UNSUPPORTED` when the session renders into a borrowed ring.
          Give it new textures of the new size instead.
        - `Status.INVALID_STATE` when the session failed, or you detached,
          abandoned, released, or disposed it.
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
        """Rewrites the URLs of the runtime's network requests.

        The transform replaces the one set before.

        See `mln_runtime_set_resource_transform` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        return self._native._discard_set_resource_transform(transform)
