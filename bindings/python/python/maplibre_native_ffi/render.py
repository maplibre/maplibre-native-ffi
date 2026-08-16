"""Render target values and backend interop helpers."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass, field
from enum import IntFlag
from typing import Any

from . import _native
from ._enum import UnknownIntEnum
from ._lifecycle import NativeHandleMixin
from .query import (
    FeatureStateSelector,
    QueriedFeature,
    RenderedFeatureQueryOptions,
    RenderedQueryGeometry,
    SourceFeatureQueryOptions,
)

_DETACHED_RENDER_SESSION_HANDLE_CREATE_KEY = object()
_RENDER_SESSION_HANDLE_CREATE_KEY = object()
_METAL_FRAME_HANDLE_CREATE_KEY = object()
_VULKAN_FRAME_HANDLE_CREATE_KEY = object()
_OPENGL_FRAME_HANDLE_CREATE_KEY = object()


class RenderBackend(IntFlag):
    """Render backend support bits reported by the native library."""

    NONE = 0
    METAL = 1 << 0
    VULKAN = 1 << 1
    OPENGL = 1 << 2
    WEBGPU = 1 << 3


class OpenGLContextProvider(IntFlag):
    """OpenGL context provider support bits reported by the native library."""

    NONE = 0
    WGL = 1 << 0
    EGL = 1 << 1
    WEBGL = 1 << 2


class RenderResult(UnknownIntEnum):
    """Outcome reported by :meth:`RenderSessionHandle.render_update`."""

    RENDERED = 0
    NO_UPDATE = 1
    SIZE_PENDING = 2
    TARGET_NOT_READY = 3


@dataclass(frozen=True, slots=True)
class RenderUpdate:
    """Outcome of one :meth:`RenderSessionHandle.render_update` call.

    ``result`` names the wake to wait for before rendering again.
    ``needs_repaint`` reports whether the map asked for another frame while it
    rendered this one, as during an ongoing camera transition. It is set only
    when ``result`` is ``RENDERED``, and reads false for every other outcome.
    """

    result: RenderResult
    needs_repaint: bool


class OpenGLContextOwnership(UnknownIntEnum):
    """How a session's OpenGL context relates to the thread that attached it.

    A shared session leaves the thread as it found it: every render makes the
    session context current and restores whatever was current before. The
    session context joins the share group named by the descriptor, so a host may
    hand the session a texture and sample it from its own context.

    A dedicated session owns the thread. It makes its context current once and
    keeps it current between renders, and it joins no share group. Use this when
    a thread exists to drive one render session and runs no other graphics work.
    """

    SHARED = 0
    DEDICATED = 1


class OpenGLClientApi(UnknownIntEnum):
    """OpenGL client API a dedicated EGL session creates its context for."""

    UNSPECIFIED = 0
    GL = 1
    GLES = 2


class NativePointer:
    """Borrowed opaque backend-native address value."""

    __slots__ = ("_address", "_diagnostic_name", "_is_live")

    def __init__(
        self,
        address: int,
        *,
        _is_live: Callable[[], bool] | None = None,
        _diagnostic_name: str = "native pointer",
    ) -> None:
        if address < 0:
            msg = "native pointer address must be non-negative"
            raise ValueError(msg)
        self._address = address
        self._is_live = _is_live
        self._diagnostic_name = _diagnostic_name

    @classmethod
    def null(cls) -> NativePointer:
        """Return a null native pointer value."""
        return cls(0)

    @property
    def address(self) -> int:
        """Return the address while its borrowed scope is still live."""
        self._require_live()
        return self._address

    @property
    def is_null(self) -> bool:
        """Return whether this pointer stores the null address."""
        return self.address == 0

    def _require_live(self) -> None:
        if self._is_live is None or self._is_live():
            return
        from .errors import InvalidStateError

        raise InvalidStateError(None, f"{self._diagnostic_name} is no longer live")

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, NativePointer):
            return NotImplemented
        return self.address == other.address

    def __hash__(self) -> int:
        return hash(self.address)

    def __repr__(self) -> str:
        return f"{type(self).__name__}(address={self.address!r})"


class FrameOpenGLTextureName:
    """Borrowed OpenGL texture name scoped to a live frame handle."""

    __slots__ = ("_is_live", "_texture")

    def __init__(self, texture: int, *, _is_live: Callable[[], bool]) -> None:
        if texture < 0:
            msg = "OpenGL texture name must be non-negative"
            raise ValueError(msg)
        self._texture = texture
        self._is_live = _is_live

    @property
    def value(self) -> int:
        """Return the texture object name while its frame is still live."""
        if self._is_live():
            return self._texture
        from .errors import InvalidStateError

        raise InvalidStateError(None, "OpenGL texture is no longer live")

    def __int__(self) -> int:
        return self.value

    def __eq__(self, other: object) -> bool:
        if isinstance(other, FrameOpenGLTextureName):
            return self.value == other.value
        if isinstance(other, int):
            return self.value == other
        return NotImplemented

    def __hash__(self) -> int:
        return hash(self.value)

    def __repr__(self) -> str:
        return f"{type(self).__name__}(value={self.value!r})"


@dataclass(frozen=True, slots=True)
class RenderTargetExtent:
    """Logical render target size in UI pixels."""

    width: int = 256
    height: int = 256
    scale_factor: float = 1.0

    def physical_size(self) -> tuple[int, int]:
        """Return the physical device-pixel size as ceil(logical * scale_factor).

        Session-owned texture targets and surface targets are sized this way;
        borrowed texture targets state their physical size instead.
        """
        return _native.render_target_extent_physical_size(
            self.width, self.height, self.scale_factor
        )


@dataclass(frozen=True, slots=True)
class MetalContextDescriptor:
    """Borrowed Metal context values shared by Metal render targets."""

    device: NativePointer = field(default_factory=NativePointer.null)


@dataclass(frozen=True, slots=True)
class VulkanContextDescriptor:
    """Borrowed Vulkan context values shared by Vulkan render targets."""

    instance: NativePointer = field(default_factory=NativePointer.null)
    physical_device: NativePointer = field(default_factory=NativePointer.null)
    device: NativePointer = field(default_factory=NativePointer.null)
    graphics_queue: NativePointer = field(default_factory=NativePointer.null)
    graphics_queue_family_index: int = 0
    get_instance_proc_addr: NativePointer = field(default_factory=NativePointer.null)
    get_device_proc_addr: NativePointer = field(default_factory=NativePointer.null)


@dataclass(frozen=True, slots=True)
class WglContextDescriptor:
    """Borrowed WGL context values shared by OpenGL render targets.

    A shared session joins the share group named by ``share_context``, which is
    required there. A dedicated session joins no share group, so
    ``share_context`` must be null there.
    """

    device_context: NativePointer = field(default_factory=NativePointer.null)
    share_context: NativePointer = field(default_factory=NativePointer.null)
    get_proc_address: NativePointer = field(default_factory=NativePointer.null)
    ownership: OpenGLContextOwnership = OpenGLContextOwnership.SHARED


@dataclass(frozen=True, slots=True)
class EglContextDescriptor:
    """Borrowed EGL context values shared by OpenGL render targets.

    A shared session joins the share group named by ``share_context`` and takes
    its client API from that context, so ``share_context`` is required there and
    ``client_api`` is ignored. A dedicated session joins no share group, so
    ``share_context`` must be null there and ``client_api`` names GL or GLES.
    """

    display: NativePointer = field(default_factory=NativePointer.null)
    config: NativePointer = field(default_factory=NativePointer.null)
    share_context: NativePointer = field(default_factory=NativePointer.null)
    client_api: OpenGLClientApi = OpenGLClientApi.UNSPECIFIED
    get_proc_address: NativePointer = field(default_factory=NativePointer.null)
    ownership: OpenGLContextOwnership = OpenGLContextOwnership.SHARED


OpenGLContextDescriptor = WglContextDescriptor | EglContextDescriptor


@dataclass(frozen=True, slots=True)
class MetalSurfaceDescriptor:
    """Metal native surface attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: MetalContextDescriptor = MetalContextDescriptor()
    layer: NativePointer = field(default_factory=NativePointer.null)


@dataclass(frozen=True, slots=True)
class VulkanSurfaceDescriptor:
    """Vulkan native surface attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: VulkanContextDescriptor = VulkanContextDescriptor()
    surface: NativePointer = field(default_factory=NativePointer.null)


@dataclass(frozen=True, slots=True)
class OpenGLSurfaceDescriptor:
    """OpenGL native surface attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: OpenGLContextDescriptor = EglContextDescriptor()
    surface: NativePointer = field(default_factory=NativePointer.null)


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureDescriptor:
    """Metal session-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: MetalContextDescriptor = MetalContextDescriptor()


@dataclass(frozen=True, slots=True)
class MetalBorrowedTextureDescriptor:
    """Metal caller-owned texture attachment descriptor."""

    extent: RenderTargetExtent
    # Sized by its owner, not derived from extent.
    physical_width: int
    physical_height: int
    texture: NativePointer = field(default_factory=NativePointer.null)


@dataclass(frozen=True, slots=True)
class VulkanOwnedTextureDescriptor:
    """Vulkan session-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: VulkanContextDescriptor = VulkanContextDescriptor()


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTextureDescriptor:
    """Vulkan caller-owned texture attachment descriptor."""

    extent: RenderTargetExtent
    # Sized by its owner, not derived from extent.
    physical_width: int
    physical_height: int
    context: VulkanContextDescriptor = VulkanContextDescriptor()
    image: NativePointer = field(default_factory=NativePointer.null)
    image_view: NativePointer = field(default_factory=NativePointer.null)
    format: int = 0
    initial_layout: int = 0
    final_layout: int = 0


@dataclass(frozen=True, slots=True)
class OpenGLOwnedTextureDescriptor:
    """OpenGL session-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: OpenGLContextDescriptor = EglContextDescriptor()


@dataclass(frozen=True, slots=True)
class OpenGLBorrowedTextureDescriptor:
    """OpenGL caller-owned texture attachment descriptor."""

    extent: RenderTargetExtent
    # Sized by its owner, not derived from extent.
    physical_width: int
    physical_height: int
    context: OpenGLContextDescriptor = EglContextDescriptor()
    texture: int = 0
    target: int = 0


@dataclass(frozen=True, slots=True)
class TextureImageInfo:
    """CPU image readback metadata for a texture session frame."""

    width: int
    height: int
    stride: int
    byte_length: int

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> TextureImageInfo:
        return cls(
            width=raw["width"],
            height=raw["height"],
            stride=raw["stride"],
            byte_length=raw["byte_length"],
        )


@dataclass(frozen=True, slots=True)
class PremultipliedRgba8Image:
    """Copied premultiplied RGBA8 image bytes and metadata."""

    info: TextureImageInfo
    data: bytes

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> PremultipliedRgba8Image:
        return cls(info=TextureImageInfo._from_native(raw["info"]), data=raw["data"])


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureFrame:
    """Copied metadata for an acquired Metal session-owned texture frame."""

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    pixel_format: int

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> MetalOwnedTextureFrame:
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            pixel_format=raw["pixel_format"],
        )


@dataclass(frozen=True, slots=True)
class VulkanOwnedTextureFrame:
    """Copied metadata for an acquired Vulkan session-owned texture frame."""

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    format: int
    layout: int

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> VulkanOwnedTextureFrame:
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            format=raw["format"],
            layout=raw["layout"],
        )


@dataclass(frozen=True, slots=True)
class OpenGLOwnedTextureFrame:
    """Copied metadata for an acquired OpenGL session-owned texture frame."""

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    target: int
    internal_format: int
    format: int
    type: int

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> OpenGLOwnedTextureFrame:
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            target=raw["target"],
            internal_format=raw["internal_format"],
            format=raw["format"],
            type=raw["type"],
        )


class DetachedRenderSessionHandle(NativeHandleMixin):
    """Close-only render session handle after backend resources detach.

    A detached session holds no reference to its former map, so it stays
    destroyable after that map closes.
    """

    _handle_name = "DetachedRenderSessionHandle"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _DETACHED_RENDER_SESSION_HANDLE_CREATE_KEY:
            msg = "DetachedRenderSessionHandle instances are created by RenderSessionHandle"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> DetachedRenderSessionHandle:
        return cls(native, _create_key=_DETACHED_RENDER_SESSION_HANDLE_CREATE_KEY)


class RenderSessionHandle(NativeHandleMixin):
    """Render session handle affine to the thread that attached it."""

    _handle_name = "RenderSessionHandle"

    def __init__(
        self,
        native: Any,
        map_handle: MapHandle,
        *,
        _create_key: object | None = None,
    ) -> None:
        if _create_key is not _RENDER_SESSION_HANDLE_CREATE_KEY:
            msg = "RenderSessionHandle instances are created by MapHandle"
            raise TypeError(msg)
        self._native = native
        self._map: MapHandle | None = map_handle

    @classmethod
    def _from_native(cls, native: Any, map_handle: MapHandle) -> RenderSessionHandle:
        return cls(native, map_handle, _create_key=_RENDER_SESSION_HANDLE_CREATE_KEY)

    @property
    def detached(self) -> bool:
        """Return whether backend resources have been detached."""
        return bool(self._native.detached)

    def resize(self, width: int, height: int, scale_factor: float) -> None:
        """Resize this attached render session.

        Surface and session-owned texture targets resize in place. A
        caller-owned texture target is sized by its owner and reports
        :class:`UnsupportedFeatureError`; hand over a new texture with the
        backend's ``set_*_borrowed_texture_target`` method instead.

        The session keeps its renderer, and renderer-held state such as feature
        state carries over. A new scale factor is the exception: it starts a
        fresh renderer with that state empty. The same exception applies to
        every ``set_*_target`` method.
        """
        self._native.resize(width, height, scale_factor)

    def _set_target(
        self,
        set_target: Callable[..., None],
        descriptor: Any,
        *args: object,
    ) -> None:
        extent = descriptor.extent
        set_target(extent.width, extent.height, extent.scale_factor, *args)

    def set_metal_surface_target(self, descriptor: MetalSurfaceDescriptor) -> None:
        """Present this attached surface session through a new surface.

        The session keeps its renderer, and with it the tile pyramid, atlases,
        symbol placement, and feature state. The descriptor's extent applies as
        a resize does. A ``context.device`` that is neither null nor this
        session's device raises :class:`InvalidArgumentError` and leaves this
        session rendering into the surface it has. The session assigns the layer
        its own device and pixel format.
        """
        self._set_target(
            self._native.set_metal_surface_target,
            descriptor,
            descriptor.context.device.address,
            descriptor.layer.address,
        )

    def set_vulkan_surface_target(self, descriptor: VulkanSurfaceDescriptor) -> None:
        """Present this attached surface session through a new surface.

        See :meth:`set_metal_surface_target` for what replacing a surface
        preserves. The outgoing ``VkSurfaceKHR`` must still be valid, since this
        session holds a swapchain built from it.
        """
        self._set_target(
            self._native.set_vulkan_surface_target,
            descriptor,
            descriptor.context.instance.address,
            descriptor.context.physical_device.address,
            descriptor.context.device.address,
            descriptor.context.graphics_queue.address,
            descriptor.context.graphics_queue_family_index,
            descriptor.context.get_instance_proc_addr.address,
            descriptor.context.get_device_proc_addr.address,
            descriptor.surface.address,
        )

    def set_opengl_surface_target(self, descriptor: OpenGLSurfaceDescriptor) -> None:
        """Present this attached surface session through a new surface.

        See :meth:`set_metal_surface_target` for what replacing a surface
        preserves. The new surface is made current on the next render, so a host
        may hand over a replacement for one it has already destroyed, and an
        unusable surface is reported by the next :meth:`render_update`.
        """
        platform, ownership, first, second, share, client_api, get_proc = (
            _opengl_context_parts(descriptor.context)
        )
        self._set_target(
            self._native.set_opengl_surface_target,
            descriptor,
            platform,
            ownership,
            first,
            second,
            share,
            client_api,
            get_proc,
            descriptor.surface.address,
        )

    def set_metal_borrowed_texture_target(
        self, descriptor: MetalBorrowedTextureDescriptor
    ) -> None:
        """Render this attached texture session into a new caller-owned texture.

        This is how a caller-owned texture target resizes, and the session keeps
        its renderer. The replacement must belong to the device this session
        attached with, which raises :class:`InvalidArgumentError` otherwise, and
        carry the pixel format it attached with, which raises
        :class:`UnsupportedFeatureError` otherwise; both leave this session
        rendering into the texture it has. The caller owns the replacement and
        keeps it valid until the next replacement, detach, or close. The session
        never retains or reads the outgoing texture.
        """
        self._set_target(
            self._native.set_metal_borrowed_texture_target,
            descriptor,
            descriptor.physical_width,
            descriptor.physical_height,
            descriptor.texture.address,
        )

    def set_vulkan_borrowed_texture_target(
        self, descriptor: VulkanBorrowedTextureDescriptor
    ) -> None:
        """Render this attached texture session into a new caller-owned image.

        See :meth:`set_metal_borrowed_texture_target` for what replacing a
        target preserves. The replacement must carry the format and both layouts
        this session attached with, since its render pass was built around them.
        """
        self._set_target(
            self._native.set_vulkan_borrowed_texture_target,
            descriptor,
            descriptor.physical_width,
            descriptor.physical_height,
            descriptor.context.instance.address,
            descriptor.context.physical_device.address,
            descriptor.context.device.address,
            descriptor.context.graphics_queue.address,
            descriptor.context.graphics_queue_family_index,
            descriptor.context.get_instance_proc_addr.address,
            descriptor.context.get_device_proc_addr.address,
            descriptor.image.address,
            descriptor.image_view.address,
            descriptor.format,
            descriptor.initial_layout,
            descriptor.final_layout,
        )

    def set_opengl_borrowed_texture_target(
        self, descriptor: OpenGLBorrowedTextureDescriptor
    ) -> None:
        """Render this attached texture session into a new caller-owned texture.

        See :meth:`set_metal_borrowed_texture_target` for what replacing a
        target preserves. The replacement must belong to the context this
        session attached with or one in its share group, and that context must
        be current on this thread.
        """
        platform, ownership, first, second, share, client_api, get_proc = (
            _opengl_context_parts(descriptor.context)
        )
        self._set_target(
            self._native.set_opengl_borrowed_texture_target,
            descriptor,
            descriptor.physical_width,
            descriptor.physical_height,
            platform,
            ownership,
            first,
            second,
            share,
            client_api,
            get_proc,
            descriptor.texture,
            descriptor.target,
        )

    def render_update(self) -> RenderUpdate:
        """Render the latest map update into this session's render target.

        The returned :class:`RenderUpdate` carries a :class:`RenderResult` that
        names the wake to wait for before calling again:

        - ``RENDERED``: the target holds a new frame. The map retains its latest
          update, so redraw on demand after a resize or a surface expose, and
          gate a frame loop on
          ``RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE``.
        - ``NO_UPDATE``: the call produced no frame. The map either has no
          update yet, or the Metal backend has not created an owned texture
          because content is not ready. Wait for
          ``RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE``.
        - ``SIZE_PENDING``: this session resized and the map, which applies its
          size on its own thread, is still behind. The map publishes an update
          for the new size on its own, so wait for the next
          ``RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE``.
        - ``TARGET_NOT_READY``: the render target had no frame available, such
          as a Metal surface whose next drawable is nil. No map update resolves
          this, so wait for a host event that changes the target, or back off
          and retry.

        ``needs_repaint`` reports whether the map asked for another frame
        while it rendered this one, as during an ongoing camera transition.
        It is set only when ``result`` is ``RENDERED``. This is the same signal
        the ``RuntimeEventType.MAP_RENDER_FRAME_FINISHED`` event carries in its
        ``needs_repaint`` field, delivered here without the event round trip,
        so a host can re-arm its frame loop before it drains events.
        """
        result, needs_repaint = self._native.render_update()
        return RenderUpdate(result=RenderResult(result), needs_repaint=needs_repaint)

    def detach(self) -> DetachedRenderSessionHandle:
        """Detach backend resources and return a close-only handle.

        Detach ends this session's hold on the map, leaving the map free to
        close. The returned handle stays destroyable after the map closes.
        """
        native = self._native.detach()
        self._map = None
        return DetachedRenderSessionHandle._from_native(native)

    def reduce_memory_use(self) -> None:
        """Ask the session renderer to release cached resources where possible."""
        self._native.reduce_memory_use()

    def clear_data(self) -> None:
        """Clear renderer data for the session."""
        self._native.clear_data()

    def dump_debug_logs(self) -> None:
        """Dump renderer debug logs through MapLibre Native logging."""
        self._native.dump_debug_logs()

    def texture_image_info(self) -> TextureImageInfo:
        """Return readback metadata for the latest texture frame."""
        return TextureImageInfo._from_native(self._native.texture_image_info())

    def read_premultiplied_rgba8_into(self, buffer: object) -> TextureImageInfo:
        """Read the latest texture frame into caller-owned writable storage."""
        return TextureImageInfo._from_native(
            self._native.read_premultiplied_rgba8_into(buffer)
        )

    def acquire_metal_owned_texture_frame(self) -> MetalOwnedTextureFrameHandle:
        """Acquire a borrowed Metal frame from a session-owned texture target."""
        return MetalOwnedTextureFrameHandle._from_native(
            self._native.acquire_metal_owned_texture_frame()
        )

    def acquire_vulkan_owned_texture_frame(self) -> VulkanOwnedTextureFrameHandle:
        """Acquire a borrowed Vulkan frame from a session-owned texture target."""
        return VulkanOwnedTextureFrameHandle._from_native(
            self._native.acquire_vulkan_owned_texture_frame()
        )

    def acquire_opengl_owned_texture_frame(self) -> OpenGLOwnedTextureFrameHandle:
        """Acquire a borrowed OpenGL frame from a session-owned texture target."""
        return OpenGLOwnedTextureFrameHandle._from_native(
            self._native.acquire_opengl_owned_texture_frame()
        )

    def query_rendered_features(
        self,
        geometry: RenderedQueryGeometry,
        options: RenderedFeatureQueryOptions | None = None,
    ) -> list[QueriedFeature]:
        """Query rendered features as copied queried-feature values."""
        from .query import _geometry_to_native_wire

        return [
            QueriedFeature._from_native(raw)
            for raw in self._native.query_rendered_features(
                _geometry_to_native_wire(geometry),
                options.layer_ids if options is not None else None,
                options.filter if options is not None else None,
            )
        ]

    def query_source_features(
        self,
        source_id: str,
        options: SourceFeatureQueryOptions | None = None,
    ) -> list[QueriedFeature]:
        """Query source features as copied queried-feature values."""
        return [
            QueriedFeature._from_native(raw)
            for raw in self._native.query_source_features(
                source_id,
                options.source_layer_ids if options is not None else None,
                options.filter if options is not None else None,
            )
        ]

    def query_feature_extensions(
        self,
        source_id: str,
        feature: bytes,
        extension: str,
        extension_field: str,
        arguments: bytes | None = None,
    ) -> bytes:
        """Query a feature extension as UTF-8 JSON or GeoJSON bytes."""
        return self._native.query_feature_extensions(
            source_id,
            feature,
            extension,
            extension_field,
            arguments,
        )

    def set_feature_state(
        self,
        selector: FeatureStateSelector,
        state: bytes,
    ) -> None:
        """Set per-feature state on a render source for this render session."""
        self._native.set_feature_state(
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
            state,
        )

    def get_feature_state(self, selector: FeatureStateSelector) -> bytes:
        """Return copied per-feature state JSON from a render source."""
        return self._native.get_feature_state(
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
        )

    def remove_feature_state(self, selector: FeatureStateSelector) -> None:
        """Remove per-feature state from a render source."""
        self._native.remove_feature_state(
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
        )


class MetalOwnedTextureFrameHandle(NativeHandleMixin):
    """Scoped handle for an acquired Metal session-owned texture frame."""

    _handle_name = "MetalOwnedTextureFrameHandle"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _METAL_FRAME_HANDLE_CREATE_KEY:
            msg = "MetalOwnedTextureFrameHandle instances are created by RenderSessionHandle"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> MetalOwnedTextureFrameHandle:
        return cls(native, _create_key=_METAL_FRAME_HANDLE_CREATE_KEY)

    @property
    def frame(self) -> MetalOwnedTextureFrame:
        """Return copied frame metadata."""
        return MetalOwnedTextureFrame._from_native(self._native.frame())

    @property
    def texture(self) -> NativePointer:
        """Return the borrowed Metal texture pointer while the frame is open."""
        return NativePointer(
            self._native.texture_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Metal texture",
        )

    @property
    def device(self) -> NativePointer:
        """Return the borrowed Metal device pointer while the frame is open."""
        return NativePointer(
            self._native.device_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Metal device",
        )


class VulkanOwnedTextureFrameHandle(NativeHandleMixin):
    """Scoped handle for an acquired Vulkan session-owned texture frame."""

    _handle_name = "VulkanOwnedTextureFrameHandle"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _VULKAN_FRAME_HANDLE_CREATE_KEY:
            msg = "VulkanOwnedTextureFrameHandle instances are created by RenderSessionHandle"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> VulkanOwnedTextureFrameHandle:
        return cls(native, _create_key=_VULKAN_FRAME_HANDLE_CREATE_KEY)

    @property
    def frame(self) -> VulkanOwnedTextureFrame:
        """Return copied frame metadata."""
        return VulkanOwnedTextureFrame._from_native(self._native.frame())

    @property
    def image(self) -> NativePointer:
        """Return the borrowed Vulkan image pointer while the frame is open."""
        return NativePointer(
            self._native.image_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Vulkan image",
        )

    @property
    def image_view(self) -> NativePointer:
        """Return the borrowed Vulkan image-view pointer while the frame is open."""
        return NativePointer(
            self._native.image_view_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Vulkan image view",
        )

    @property
    def device(self) -> NativePointer:
        """Return the borrowed Vulkan device pointer while the frame is open."""
        return NativePointer(
            self._native.device_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Vulkan device",
        )


class OpenGLOwnedTextureFrameHandle(NativeHandleMixin):
    """Scoped handle for an acquired OpenGL session-owned texture frame."""

    _handle_name = "OpenGLOwnedTextureFrameHandle"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _OPENGL_FRAME_HANDLE_CREATE_KEY:
            msg = "OpenGLOwnedTextureFrameHandle instances are created by RenderSessionHandle"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> OpenGLOwnedTextureFrameHandle:
        return cls(native, _create_key=_OPENGL_FRAME_HANDLE_CREATE_KEY)

    @property
    def frame(self) -> OpenGLOwnedTextureFrame:
        """Return copied frame metadata."""
        return OpenGLOwnedTextureFrame._from_native(self._native.frame())

    @property
    def texture(self) -> FrameOpenGLTextureName:
        """Return the borrowed OpenGL texture object name while the frame is open."""
        return FrameOpenGLTextureName(
            int(self._native.texture()),
            _is_live=lambda: not self.closed,
        )


def _opengl_context_parts(
    context: EglContextDescriptor | WglContextDescriptor,
) -> tuple[int, int, int, int, int, int, int]:
    if isinstance(context, WglContextDescriptor):
        return (
            1,
            context.ownership.native_code,
            context.device_context.address,
            0,
            context.share_context.address,
            OpenGLClientApi.UNSPECIFIED.native_code,
            context.get_proc_address.address,
        )
    if isinstance(context, EglContextDescriptor):
        return (
            2,
            context.ownership.native_code,
            context.display.address,
            context.config.address,
            context.share_context.address,
            context.client_api.native_code,
            context.get_proc_address.address,
        )
    msg = f"unsupported OpenGL context descriptor: {type(context)!r}"
    raise TypeError(msg)


__all__ = [
    "DetachedRenderSessionHandle",
    "EglContextDescriptor",
    "FrameOpenGLTextureName",
    "MetalBorrowedTextureDescriptor",
    "MetalContextDescriptor",
    "MetalOwnedTextureDescriptor",
    "MetalOwnedTextureFrame",
    "MetalOwnedTextureFrameHandle",
    "MetalSurfaceDescriptor",
    "NativePointer",
    "OpenGLBorrowedTextureDescriptor",
    "OpenGLClientApi",
    "OpenGLContextDescriptor",
    "OpenGLContextOwnership",
    "OpenGLContextProvider",
    "OpenGLOwnedTextureDescriptor",
    "OpenGLOwnedTextureFrame",
    "OpenGLOwnedTextureFrameHandle",
    "OpenGLSurfaceDescriptor",
    "PremultipliedRgba8Image",
    "RenderBackend",
    "RenderResult",
    "RenderSessionHandle",
    "RenderTargetExtent",
    "RenderUpdate",
    "TextureImageInfo",
    "VulkanBorrowedTextureDescriptor",
    "VulkanContextDescriptor",
    "VulkanOwnedTextureDescriptor",
    "VulkanOwnedTextureFrame",
    "VulkanOwnedTextureFrameHandle",
    "VulkanSurfaceDescriptor",
    "WglContextDescriptor",
]

from .map import MapHandle
