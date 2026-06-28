"""Render target values and backend interop helpers."""

from __future__ import annotations

from ._lifecycle import NativeHandleMixin
from dataclasses import dataclass
from enum import IntFlag
from typing import Any
from collections.abc import Callable

from .geo import Feature
from .json import JsonObject, JsonValue
from .query import (
    FeatureExtensionResult,
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


class OpenGLContextProvider(IntFlag):
    """OpenGL context provider support bits reported by the native library."""

    NONE = 0
    WGL = 1 << 0
    EGL = 1 << 1


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
    def null(cls) -> "NativePointer":
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


@dataclass(frozen=True, slots=True)
class MetalContextDescriptor:
    """Borrowed Metal context values shared by Metal render targets."""

    device: NativePointer = NativePointer(0)


@dataclass(frozen=True, slots=True)
class VulkanContextDescriptor:
    """Borrowed Vulkan context values shared by Vulkan render targets."""

    instance: NativePointer = NativePointer(0)
    physical_device: NativePointer = NativePointer(0)
    device: NativePointer = NativePointer(0)
    graphics_queue: NativePointer = NativePointer(0)
    graphics_queue_family_index: int = 0
    get_instance_proc_addr: NativePointer = NativePointer(0)
    get_device_proc_addr: NativePointer = NativePointer(0)


@dataclass(frozen=True, slots=True)
class WglContextDescriptor:
    """Borrowed WGL context values shared by OpenGL render targets."""

    device_context: NativePointer = NativePointer(0)
    share_context: NativePointer = NativePointer(0)
    get_proc_address: NativePointer = NativePointer(0)


@dataclass(frozen=True, slots=True)
class EglContextDescriptor:
    """Borrowed EGL context values shared by OpenGL render targets."""

    display: NativePointer = NativePointer(0)
    config: NativePointer = NativePointer(0)
    share_context: NativePointer = NativePointer(0)
    get_proc_address: NativePointer = NativePointer(0)


OpenGLContextDescriptor = WglContextDescriptor | EglContextDescriptor


@dataclass(frozen=True, slots=True)
class MetalSurfaceDescriptor:
    """Metal native surface attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: MetalContextDescriptor = MetalContextDescriptor()
    layer: NativePointer = NativePointer(0)


@dataclass(frozen=True, slots=True)
class VulkanSurfaceDescriptor:
    """Vulkan native surface attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: VulkanContextDescriptor = VulkanContextDescriptor()
    surface: NativePointer = NativePointer(0)


@dataclass(frozen=True, slots=True)
class OpenGLSurfaceDescriptor:
    """OpenGL native surface attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: OpenGLContextDescriptor = EglContextDescriptor()
    surface: NativePointer = NativePointer(0)


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureDescriptor:
    """Metal session-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: MetalContextDescriptor = MetalContextDescriptor()


@dataclass(frozen=True, slots=True)
class MetalBorrowedTextureDescriptor:
    """Metal caller-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    texture: NativePointer = NativePointer(0)


@dataclass(frozen=True, slots=True)
class VulkanOwnedTextureDescriptor:
    """Vulkan session-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: VulkanContextDescriptor = VulkanContextDescriptor()


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTextureDescriptor:
    """Vulkan caller-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: VulkanContextDescriptor = VulkanContextDescriptor()
    image: NativePointer = NativePointer(0)
    image_view: NativePointer = NativePointer(0)
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

    extent: RenderTargetExtent = RenderTargetExtent()
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
    def _from_native(cls, raw: dict[str, Any]) -> "TextureImageInfo":
        """Build metadata from private native values."""
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
    def _from_native(cls, raw: dict[str, Any]) -> "PremultipliedRgba8Image":
        """Build an image from private native values."""
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
    def _from_native(cls, raw: dict[str, Any]) -> "MetalOwnedTextureFrame":
        """Build frame metadata from private native values."""
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
    def _from_native(cls, raw: dict[str, Any]) -> "VulkanOwnedTextureFrame":
        """Build frame metadata from private native values."""
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
    def _from_native(cls, raw: dict[str, Any]) -> "OpenGLOwnedTextureFrame":
        """Build frame metadata from private native values."""
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
    """Close-only render session handle after backend resources detach."""

    _handle_name = "DetachedRenderSessionHandle"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _DETACHED_RENDER_SESSION_HANDLE_CREATE_KEY:
            msg = "DetachedRenderSessionHandle instances are created by RenderSessionHandle"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> "DetachedRenderSessionHandle":
        return cls(native, _create_key=_DETACHED_RENDER_SESSION_HANDLE_CREATE_KEY)


class RenderSessionHandle(NativeHandleMixin):
    """Owner-thread render session handle bound to a retained map wrapper."""

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
        self._map = map_handle

    @classmethod
    def _from_native(cls, native: Any, map_handle: MapHandle) -> "RenderSessionHandle":
        return cls(native, map_handle, _create_key=_RENDER_SESSION_HANDLE_CREATE_KEY)

    @property
    def detached(self) -> bool:
        """Return whether backend resources have been detached."""
        return bool(self._native.detached)

    def resize(self, width: int, height: int, scale_factor: float) -> None:
        """Resize this attached render session."""
        self._native.resize(width, height, scale_factor)

    def render_update(self) -> None:
        """Process the latest map render update for this target."""
        self._native.render_update()

    def detach(self) -> DetachedRenderSessionHandle:
        """Detach backend resources and return a close-only handle."""
        return DetachedRenderSessionHandle._from_native(self._native.detach())  # noqa: SLF001

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

    def acquire_metal_owned_texture_frame(self) -> "MetalOwnedTextureFrameHandle":
        """Acquire a borrowed Metal frame from a session-owned texture target."""
        return MetalOwnedTextureFrameHandle._from_native(  # noqa: SLF001
            self._native.acquire_metal_owned_texture_frame()
        )

    def acquire_vulkan_owned_texture_frame(self) -> "VulkanOwnedTextureFrameHandle":
        """Acquire a borrowed Vulkan frame from a session-owned texture target."""
        return VulkanOwnedTextureFrameHandle._from_native(  # noqa: SLF001
            self._native.acquire_vulkan_owned_texture_frame()
        )

    def acquire_opengl_owned_texture_frame(self) -> "OpenGLOwnedTextureFrameHandle":
        """Acquire a borrowed OpenGL frame from a session-owned texture target."""
        return OpenGLOwnedTextureFrameHandle._from_native(  # noqa: SLF001
            self._native.acquire_opengl_owned_texture_frame()
        )

    def query_rendered_features(
        self,
        geometry: RenderedQueryGeometry,
        options: RenderedFeatureQueryOptions | None = None,
    ) -> tuple[QueriedFeature, ...]:
        """Query rendered features from the latest render session state."""
        from .query import (
            QueriedFeature,
            _geometry_to_native_wire,
            _rendered_options_to_native_wire,
        )

        layer_ids, filter_ = _rendered_options_to_native_wire(options)
        raw = self._native.query_rendered_features(
            _geometry_to_native_wire(geometry),
            layer_ids,
            filter_,
        )
        return tuple(QueriedFeature._from_native(feature) for feature in raw)

    def query_source_features(
        self,
        source_id: str,
        options: SourceFeatureQueryOptions | None = None,
    ) -> tuple[QueriedFeature, ...]:
        """Query source features from the latest render session state."""
        from .query import (
            QueriedFeature,
            _source_options_to_native_wire,
        )

        source_layer_ids, filter_ = _source_options_to_native_wire(options)
        raw = self._native.query_source_features(source_id, source_layer_ids, filter_)
        return tuple(QueriedFeature._from_native(feature) for feature in raw)

    def query_feature_extensions(
        self,
        source_id: str,
        feature: Feature,
        extension: str,
        extension_field: str,
        arguments: JsonObject | None = None,
    ) -> FeatureExtensionResult:
        """Query a feature extension from the latest render session state."""
        from .query import FeatureExtensionResult

        raw = self._native.query_feature_extensions(
            source_id,
            feature,
            extension,
            extension_field,
            arguments,
        )
        return FeatureExtensionResult._from_native(raw)

    def set_feature_state(
        self,
        selector: FeatureStateSelector,
        state: JsonValue,
    ) -> None:
        """Set per-feature state on a render source for this render session."""
        self._native.set_feature_state(
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
            state,
        )

    def get_feature_state(self, selector: FeatureStateSelector) -> JsonValue:
        """Return copied per-feature state from a render source."""
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
    def _from_native(cls, native: Any) -> "MetalOwnedTextureFrameHandle":
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
    def _from_native(cls, native: Any) -> "VulkanOwnedTextureFrameHandle":
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
    def _from_native(cls, native: Any) -> "OpenGLOwnedTextureFrameHandle":
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
    "OpenGLContextDescriptor",
    "OpenGLContextProvider",
    "OpenGLOwnedTextureDescriptor",
    "OpenGLOwnedTextureFrame",
    "OpenGLOwnedTextureFrameHandle",
    "OpenGLSurfaceDescriptor",
    "PremultipliedRgba8Image",
    "RenderBackend",
    "RenderSessionHandle",
    "RenderTargetExtent",
    "TextureImageInfo",
    "VulkanBorrowedTextureDescriptor",
    "VulkanContextDescriptor",
    "VulkanOwnedTextureDescriptor",
    "VulkanOwnedTextureFrame",
    "VulkanOwnedTextureFrameHandle",
    "VulkanSurfaceDescriptor",
    "WglContextDescriptor",
]

from .map import MapHandle  # noqa: E402
