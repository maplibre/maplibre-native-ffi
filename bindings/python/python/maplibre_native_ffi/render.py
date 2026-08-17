"""Render target values and backend interop helpers."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass, field
from enum import IntFlag
from typing import Any, TypeVar, cast, overload

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

_RENDER_SESSION_HANDLE_CREATE_KEY = object()
_METAL_FRAME_HANDLE_CREATE_KEY = object()
_VULKAN_FRAME_HANDLE_CREATE_KEY = object()
_OPENGL_FRAME_HANDLE_CREATE_KEY = object()
_WEBGPU_FRAME_HANDLE_CREATE_KEY = object()
_T = TypeVar("_T")


def _identity(value: object) -> object:
    return value


def _cast_bytes(value: object) -> bytes:
    return cast(bytes, value)


def _cast_queried_features(value: object) -> list[QueriedFeature]:
    return [
        QueriedFeature._from_native(raw) for raw in cast("list[dict[str, Any]]", value)
    ]


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
    """Terminal disposition of an accepted frame demand."""

    RENDERED = 0
    NO_UPDATE = 1
    SIZE_PENDING = 2
    TARGET_NOT_READY = 3
    SUPERSEDED = 4
    DEADLINE_MISSED = 5


class RenderDriver(UnknownIntEnum):
    """Execution placement for a render session."""

    CORE_WORKER = 1
    CALLER_GRAPHICS_THREAD = 2


class RenderSessionState(UnknownIntEnum):
    """Lifecycle state reported by a render-session snapshot."""

    ATTACHING = 1
    ATTACHED = 2
    DETACHING = 3
    DETACHED = 4
    TARGET_LOST = 5
    ABANDONED = 6


class FrameDemandFlag(IntFlag):
    """Frame-demand policy bits."""

    IF_NEEDED = 1 << 0
    PRESENT = 1 << 1


class RenderSessionCapability(IntFlag):
    """Optional render-session capabilities."""

    FRAME_ACQUISITION = 1 << 0
    READBACK = 1 << 1
    CONSUMER_SYNC = 1 << 2
    PRESENTATION = 1 << 3


class GpuSyncKind(UnknownIntEnum):
    """Synchronization payload kind for an acquired texture frame."""

    CPU_COMPLETE = 0
    METAL_SHARED_EVENT = 1
    VULKAN_TIMELINE_SEMAPHORE = 2
    OPENGL_FENCE = 3
    WEBGPU_TOKEN = 4


class RenderAbandonDisposition(UnknownIntEnum):
    """Resource disposition after irreversible target abandonment."""

    CLEAN = 0
    QUARANTINED = 1


class OpenGLContextOwnership(UnknownIntEnum):
    """How a session's OpenGL context relates to its driver thread and host
    graphics state.

    A shared session leaves the thread as it found it: every render makes the
    session context current and restores whatever was current before. The
    session context joins the share group named by the descriptor, so a host may
    hand the session a texture and sample it from its own context.

    A dedicated session owns its driver thread's context. It keeps the context
    current between renders and joins no share group. The driver may be a native
    core worker or a dedicated host thread.
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
class RenderSessionAttachOptions:
    """Driver placement and texture-ring policy for attachment."""

    driver: RenderDriver
    requested_texture_ring_depth: int = 0


@dataclass(frozen=True, slots=True)
class FrameDemand:
    """One nonblocking frame request."""

    flags: FrameDemandFlag = FrameDemandFlag.IF_NEEDED
    token: int = 0
    coalescing_boundary: int = 0
    presentation_time_ns: int = 0
    deadline_ns: int = 0


@dataclass(frozen=True, slots=True)
class RenderFrameResult:
    """Owned terminal result for one accepted frame demand.

    ``needs_repaint`` reports whether the map asked for another frame while it
    rendered this one, as during an ongoing camera transition. It is set only
    when ``disposition`` is ``RENDERED``, and reads false for every other
    outcome.
    """

    disposition: RenderResult
    token: int
    map_update_generation: int
    extent_generation: int
    frame_generation: int
    presentation_time_ns: int
    needs_repaint: bool

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> RenderFrameResult:
        return cls(
            disposition=RenderResult(raw["disposition"]),
            token=raw["token"],
            map_update_generation=raw["map_update_generation"],
            extent_generation=raw["extent_generation"],
            frame_generation=raw["frame_generation"],
            presentation_time_ns=raw["presentation_time_ns"],
            needs_repaint=raw["needs_repaint"],
        )


@dataclass(frozen=True, slots=True)
class RenderSessionCapabilities:
    """Immutable capabilities negotiated during attachment."""

    driver: RenderDriver
    texture_ring_depth: int
    flags: RenderSessionCapability

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> RenderSessionCapabilities:
        return cls(
            driver=RenderDriver(raw["driver"]),
            texture_ring_depth=raw["texture_ring_depth"],
            flags=RenderSessionCapability(raw["flags"]),
        )


@dataclass(frozen=True, slots=True)
class RenderSessionSnapshot:
    """Any-thread snapshot of render-session state and generations."""

    state: RenderSessionState
    driver: RenderDriver
    latest_result: RenderResult
    extent: RenderTargetExtent
    generation: int
    map_update_generation: int
    rendered_update_generation: int
    extent_generation: int
    frame_generation: int
    latest_demand_token: int
    pending_demand_count: int
    acquired_frame_count: int
    target_ready: bool
    pending_changes: bool

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> RenderSessionSnapshot:
        width, height, scale_factor = raw["extent"]
        return cls(
            state=RenderSessionState(raw["state"]),
            driver=RenderDriver(raw["driver"]),
            latest_result=RenderResult(raw["latest_result"]),
            extent=RenderTargetExtent(width, height, scale_factor),
            generation=raw["generation"],
            map_update_generation=raw["map_update_generation"],
            rendered_update_generation=raw["rendered_update_generation"],
            extent_generation=raw["extent_generation"],
            frame_generation=raw["frame_generation"],
            latest_demand_token=raw["latest_demand_token"],
            pending_demand_count=raw["pending_demand_count"],
            acquired_frame_count=raw["acquired_frame_count"],
            target_ready=raw["target_ready"],
            pending_changes=raw["pending_changes"],
        )


@dataclass(frozen=True, slots=True)
class GpuSync:
    """Backend synchronization copied across a frame lease boundary."""

    kind: GpuSyncKind = GpuSyncKind.CPU_COMPLETE
    object: NativePointer = field(default_factory=NativePointer.null)
    value: int = 0

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> GpuSync:
        return cls(
            kind=GpuSyncKind(raw["kind"]),
            object=NativePointer(raw["object_address"]),
            value=raw["value"],
        )


_DEFAULT_FRAME_DEMAND = FrameDemand()
_DEFAULT_GPU_SYNC = GpuSync()


@dataclass(frozen=True, slots=True)
class RenderAbandonResult:
    """Result of irreversible CPU-side target abandonment."""

    disposition: RenderAbandonDisposition
    quarantined_resource_count: int

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> RenderAbandonResult:
        return cls(
            disposition=RenderAbandonDisposition(raw["disposition"]),
            quarantined_resource_count=raw["quarantined_resource_count"],
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
class WebGPUContextDescriptor:
    """Borrowed WebGPU context values shared by WebGPU render targets."""

    instance: NativePointer = field(default_factory=NativePointer.null)
    device: NativePointer = field(default_factory=NativePointer.null)
    queue: NativePointer = field(default_factory=NativePointer.null)


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
class WebGPUSurfaceDescriptor:
    """WebGPU native surface attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: WebGPUContextDescriptor = WebGPUContextDescriptor()
    surface: NativePointer = field(default_factory=NativePointer.null)
    format: int = 0


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
class WebGPUOwnedTextureDescriptor:
    """WebGPU session-owned texture attachment descriptor."""

    extent: RenderTargetExtent = RenderTargetExtent()
    context: WebGPUContextDescriptor = WebGPUContextDescriptor()


@dataclass(frozen=True, slots=True)
class WebGPUBorrowedTextureDescriptor:
    """WebGPU caller-owned texture attachment descriptor."""

    extent: RenderTargetExtent
    physical_width: int
    physical_height: int
    context: WebGPUContextDescriptor = WebGPUContextDescriptor()
    texture: NativePointer = field(default_factory=NativePointer.null)
    texture_view: NativePointer = field(default_factory=NativePointer.null)
    format: int = 0


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
class WebGPUOwnedTextureFrame:
    """Copied metadata for an acquired WebGPU session-owned texture frame."""

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    format: int

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> WebGPUOwnedTextureFrame:
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            format=raw["format"],
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


class RenderSessionHandle(NativeHandleMixin):
    """Any-thread render-session control handle.

    Caller-graphics-thread sessions additionally require
    :meth:`service_driver_work` on the graphics thread. The first successful
    service call fixes that thread identity for the session.
    """

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
        self._runtime = map_handle._runtime

    @classmethod
    def _from_native(cls, native: Any, map_handle: MapHandle) -> RenderSessionHandle:
        return cls(native, map_handle, _create_key=_RENDER_SESSION_HANDLE_CREATE_KEY)

    def close(self) -> None:
        """Destroy detached or abandoned CPU-side session state."""
        self._native.close()
        self._map = None

    @overload
    def _operation(
        self,
        start: Callable[..., int],
        take_result: None = None,
        adapt_result: None = None,
        *args: object,
    ) -> OperationHandle[None]: ...

    @overload
    def _operation(
        self,
        start: Callable[..., int],
        take_result: Callable[[int], object],
        adapt_result: Callable[[Any], _T],
        *args: object,
    ) -> OperationHandle[_T]: ...

    def _operation(
        self,
        start: Callable[..., int],
        take_result: Callable[[int], object] | None = None,
        adapt_result: Callable[[Any], Any] | None = None,
        *args: object,
    ) -> OperationHandle[Any]:
        if take_result is not None and adapt_result is None:
            adapt_result = _identity
        return self._runtime._operation(start, take_result, adapt_result, *args)

    @property
    def capabilities(self) -> RenderSessionCapabilities:
        """Return capabilities fixed during attachment."""
        return RenderSessionCapabilities._from_native(self._native.capabilities())

    @property
    def snapshot(self) -> RenderSessionSnapshot:
        """Return the latest state and generation snapshot."""
        return RenderSessionSnapshot._from_native(self._native.snapshot())

    def request_frame(self, demand: FrameDemand = _DEFAULT_FRAME_DEMAND) -> None:
        """Submit one nonblocking frame demand."""
        self._native.request_frame(
            int(demand.flags),
            demand.token,
            demand.coalescing_boundary,
            demand.presentation_time_ns,
            demand.deadline_ns,
        )

    def drain_frame_results(self) -> list[RenderFrameResult]:
        """Drain an owned batch of terminal frame-demand results."""
        return [
            RenderFrameResult._from_native(raw)
            for raw in self._native.drain_frame_results()
        ]

    def service_driver_work(self, max_work: int = 64) -> int:
        """Service caller-driver work on this session's graphics thread."""
        return self._native.service_driver_work(max_work)

    def resize(self, extent: RenderTargetExtent) -> OperationHandle[None]:
        """Start an ordered logical resize."""
        return self._operation(
            self._native.resize_start,
            None,
            None,
            extent.width,
            extent.height,
            extent.scale_factor,
        )

    def barrier(self, min_update_generation: int = 0) -> OperationHandle[None]:
        """Start a render barrier for accepted work and a map generation."""
        return self._operation(
            self._native.barrier_start, None, None, min_update_generation
        )

    def reduce_memory_use(self) -> OperationHandle[None]:
        """Start best-effort renderer cache release."""
        return self._operation(self._native.reduce_memory_use_start)

    def clear_data(self) -> OperationHandle[None]:
        """Start clearing renderer data."""
        return self._operation(self._native.clear_data_start)

    def dump_debug_logs(self) -> OperationHandle[None]:
        """Start renderer diagnostic-log emission."""
        return self._operation(self._native.dump_debug_logs_start)

    def detach(self) -> OperationHandle[None]:
        """Start normal graphics-owner teardown and map detachment."""
        return self._operation(self._native.detach_start)

    def abandon(self) -> RenderAbandonResult:
        """Irreversibly abandon a lost target without graphics calls."""
        result = RenderAbandonResult._from_native(self._native.abandon())
        self._map = None
        return result

    def _set_target(
        self,
        start: Callable[..., int],
        descriptor: Any,
        *args: object,
    ) -> OperationHandle[None]:
        extent = descriptor.extent
        return self._operation(
            start,
            None,
            None,
            extent.width,
            extent.height,
            extent.scale_factor,
            *args,
        )

    def set_metal_surface_target(
        self, descriptor: MetalSurfaceDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered Metal surface replacement."""
        return self._set_target(
            self._native.set_metal_surface_target,
            descriptor,
            descriptor.context.device.address,
            descriptor.layer.address,
        )

    def set_vulkan_surface_target(
        self, descriptor: VulkanSurfaceDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered Vulkan surface replacement."""
        context = descriptor.context
        return self._set_target(
            self._native.set_vulkan_surface_target,
            descriptor,
            context.instance.address,
            context.physical_device.address,
            context.device.address,
            context.graphics_queue.address,
            context.graphics_queue_family_index,
            context.get_instance_proc_addr.address,
            context.get_device_proc_addr.address,
            descriptor.surface.address,
        )

    def set_webgpu_surface_target(
        self, descriptor: WebGPUSurfaceDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered WebGPU surface replacement."""
        context = descriptor.context
        return self._set_target(
            self._native.set_webgpu_surface_target,
            descriptor,
            context.instance.address,
            context.device.address,
            context.queue.address,
            descriptor.surface.address,
            descriptor.format,
        )

    def set_opengl_surface_target(
        self, descriptor: OpenGLSurfaceDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered OpenGL surface replacement."""
        return self._set_target(
            self._native.set_opengl_surface_target,
            descriptor,
            *_opengl_context_parts(descriptor.context),
            descriptor.surface.address,
        )

    def set_metal_borrowed_texture_target(
        self, descriptor: MetalBorrowedTextureDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered caller-owned Metal texture replacement."""
        return self._set_target(
            self._native.set_metal_borrowed_texture_target,
            descriptor,
            descriptor.physical_width,
            descriptor.physical_height,
            descriptor.texture.address,
        )

    def set_vulkan_borrowed_texture_target(
        self, descriptor: VulkanBorrowedTextureDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered caller-owned Vulkan texture replacement."""
        context = descriptor.context
        return self._set_target(
            self._native.set_vulkan_borrowed_texture_target,
            descriptor,
            descriptor.physical_width,
            descriptor.physical_height,
            context.instance.address,
            context.physical_device.address,
            context.device.address,
            context.graphics_queue.address,
            context.graphics_queue_family_index,
            context.get_instance_proc_addr.address,
            context.get_device_proc_addr.address,
            descriptor.image.address,
            descriptor.image_view.address,
            descriptor.format,
            descriptor.initial_layout,
            descriptor.final_layout,
        )

    def set_webgpu_borrowed_texture_target(
        self, descriptor: WebGPUBorrowedTextureDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered caller-owned WebGPU texture replacement."""
        context = descriptor.context
        return self._set_target(
            self._native.set_webgpu_borrowed_texture_target,
            descriptor,
            descriptor.physical_width,
            descriptor.physical_height,
            context.instance.address,
            context.device.address,
            context.queue.address,
            descriptor.texture.address,
            descriptor.texture_view.address,
            descriptor.format,
        )

    def set_opengl_borrowed_texture_target(
        self, descriptor: OpenGLBorrowedTextureDescriptor
    ) -> OperationHandle[None]:
        """Start an ordered caller-owned OpenGL texture replacement."""
        return self._set_target(
            self._native.set_opengl_borrowed_texture_target,
            descriptor,
            descriptor.physical_width,
            descriptor.physical_height,
            *_opengl_context_parts(descriptor.context),
            descriptor.texture,
            descriptor.target,
        )

    def read_premultiplied_rgba8(
        self,
    ) -> OperationHandle[PremultipliedRgba8Image]:
        """Start readback of the latest rendered texture frame."""
        return self._operation(
            self._native.read_premultiplied_rgba8_start,
            self._native.read_premultiplied_rgba8_take_result,
            PremultipliedRgba8Image._from_native,
        )

    def query_rendered_features(
        self,
        geometry: RenderedQueryGeometry,
        options: RenderedFeatureQueryOptions | None = None,
    ) -> OperationHandle[list[QueriedFeature]]:
        """Start a rendered-feature query."""
        from .query import _geometry_to_native_wire

        return self._operation(
            self._native.query_rendered_features_start,
            self._native.render_query_features_take_result,
            _cast_queried_features,
            _geometry_to_native_wire(geometry),
            options.layer_ids if options is not None else None,
            options.filter if options is not None else None,
        )

    def query_source_features(
        self,
        source_id: str,
        options: SourceFeatureQueryOptions | None = None,
    ) -> OperationHandle[list[QueriedFeature]]:
        """Start a source-feature query."""
        return self._operation(
            self._native.query_source_features_start,
            self._native.render_query_features_take_result,
            _cast_queried_features,
            source_id,
            options.source_layer_ids if options is not None else None,
            options.filter if options is not None else None,
        )

    def query_feature_extensions(
        self,
        source_id: str,
        feature: bytes,
        extension: str,
        extension_field: str,
        arguments: bytes | None = None,
    ) -> OperationHandle[bytes]:
        """Start a feature-extension query."""
        return self._operation(
            self._native.query_feature_extensions_start,
            self._native.render_query_take_result,
            _cast_bytes,
            source_id,
            feature,
            extension,
            extension_field,
            arguments,
        )

    def set_feature_state(
        self, selector: FeatureStateSelector, state: bytes
    ) -> OperationHandle[None]:
        """Start setting per-feature render state."""
        if selector.state_key is not None:
            msg = "state_key is valid only when removing feature state"
            raise ValueError(msg)
        return self._operation(
            self._native.set_feature_state_start,
            None,
            None,
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            state,
        )

    def get_feature_state(
        self, selector: FeatureStateSelector
    ) -> OperationHandle[bytes]:
        """Start reading copied per-feature render state."""
        if selector.state_key is not None:
            msg = "state_key is valid only when removing feature state"
            raise ValueError(msg)
        return self._operation(
            self._native.get_feature_state_start,
            self._native.get_feature_state_take_result,
            _cast_bytes,
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
        )

    def remove_feature_state(
        self, selector: FeatureStateSelector
    ) -> OperationHandle[None]:
        """Start removing selected per-feature render state."""
        return self._operation(
            self._native.remove_feature_state_start,
            None,
            None,
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
        )

    def acquire_metal_owned_texture_frame(self) -> MetalOwnedTextureFrameHandle:
        """Acquire a Metal texture-slot lease without waiting."""
        return MetalOwnedTextureFrameHandle._from_native(
            self._native.acquire_metal_owned_texture_frame(), self._runtime
        )

    def acquire_vulkan_owned_texture_frame(self) -> VulkanOwnedTextureFrameHandle:
        """Acquire a Vulkan texture-slot lease without waiting."""
        return VulkanOwnedTextureFrameHandle._from_native(
            self._native.acquire_vulkan_owned_texture_frame(), self._runtime
        )

    def acquire_webgpu_owned_texture_frame(self) -> WebGPUOwnedTextureFrameHandle:
        """Acquire a WebGPU texture-slot lease without waiting."""
        return WebGPUOwnedTextureFrameHandle._from_native(
            self._native.acquire_webgpu_owned_texture_frame(), self._runtime
        )

    def acquire_opengl_owned_texture_frame(self) -> OpenGLOwnedTextureFrameHandle:
        """Acquire an OpenGL texture-slot lease without waiting."""
        return OpenGLOwnedTextureFrameHandle._from_native(
            self._native.acquire_opengl_owned_texture_frame(), self._runtime
        )


class _AcquiredFrameHandle:
    def __init__(
        self,
        native: Any,
        runtime: Any,
        create_key: object,
        expected_key: object,
    ) -> None:
        if create_key is not expected_key:
            msg = "acquired frame handles are created by RenderSessionHandle"
            raise TypeError(msg)
        self._native = native
        self._runtime = runtime

    @property
    def closed(self) -> bool:
        """Return whether this lease was released."""
        return bool(self._native.closed)

    @property
    def producer_sync(self) -> GpuSync:
        """Return synchronization that guards producer completion."""
        return GpuSync._from_native(self._native.producer_sync())

    @property
    def result(self) -> RenderFrameResult:
        """Return common frame-demand metadata for this lease."""
        return RenderFrameResult._from_native(self._native.result())

    def release(
        self, consumer_completion: GpuSync = _DEFAULT_GPU_SYNC
    ) -> OperationHandle[None]:
        """Release this lease after optional consumer GPU work."""
        return self._runtime._operation(
            self._native.release_start,
            None,
            None,
            consumer_completion.kind.native_code,
            consumer_completion.object.address,
            consumer_completion.value,
        )


class MetalOwnedTextureFrameHandle(_AcquiredFrameHandle):
    """Acquired Metal texture-slot lease."""

    def __init__(
        self,
        native: Any,
        runtime: Any,
        *,
        _create_key: object | None = None,
    ) -> None:
        super().__init__(native, runtime, _create_key, _METAL_FRAME_HANDLE_CREATE_KEY)

    @classmethod
    def _from_native(cls, native: Any, runtime: Any) -> MetalOwnedTextureFrameHandle:
        return cls(native, runtime, _create_key=_METAL_FRAME_HANDLE_CREATE_KEY)

    @property
    def frame(self) -> MetalOwnedTextureFrame:
        """Return copied frame metadata."""
        return MetalOwnedTextureFrame._from_native(self._native.frame())

    @property
    def texture(self) -> NativePointer:
        """Return the borrowed Metal texture while this lease is live."""
        return NativePointer(
            self._native.texture_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Metal texture",
        )

    @property
    def device(self) -> NativePointer:
        """Return the borrowed Metal device while this lease is live."""
        return NativePointer(
            self._native.device_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Metal device",
        )


class VulkanOwnedTextureFrameHandle(_AcquiredFrameHandle):
    """Acquired Vulkan texture-slot lease."""

    def __init__(
        self,
        native: Any,
        runtime: Any,
        *,
        _create_key: object | None = None,
    ) -> None:
        super().__init__(native, runtime, _create_key, _VULKAN_FRAME_HANDLE_CREATE_KEY)

    @classmethod
    def _from_native(cls, native: Any, runtime: Any) -> VulkanOwnedTextureFrameHandle:
        return cls(native, runtime, _create_key=_VULKAN_FRAME_HANDLE_CREATE_KEY)

    @property
    def frame(self) -> VulkanOwnedTextureFrame:
        """Return copied frame metadata."""
        return VulkanOwnedTextureFrame._from_native(self._native.frame())

    @property
    def image(self) -> NativePointer:
        """Return the borrowed Vulkan image while this lease is live."""
        return NativePointer(
            self._native.image_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Vulkan image",
        )

    @property
    def image_view(self) -> NativePointer:
        """Return the borrowed Vulkan image view while this lease is live."""
        return NativePointer(
            self._native.image_view_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Vulkan image view",
        )

    @property
    def device(self) -> NativePointer:
        """Return the borrowed Vulkan device while this lease is live."""
        return NativePointer(
            self._native.device_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="Vulkan device",
        )


class WebGPUOwnedTextureFrameHandle(_AcquiredFrameHandle):
    """Acquired WebGPU texture-slot lease."""

    def __init__(
        self,
        native: Any,
        runtime: Any,
        *,
        _create_key: object | None = None,
    ) -> None:
        super().__init__(native, runtime, _create_key, _WEBGPU_FRAME_HANDLE_CREATE_KEY)

    @classmethod
    def _from_native(cls, native: Any, runtime: Any) -> WebGPUOwnedTextureFrameHandle:
        return cls(native, runtime, _create_key=_WEBGPU_FRAME_HANDLE_CREATE_KEY)

    @property
    def frame(self) -> WebGPUOwnedTextureFrame:
        """Return copied frame metadata."""
        return WebGPUOwnedTextureFrame._from_native(self._native.frame())

    @property
    def texture(self) -> NativePointer:
        """Return the borrowed WebGPU texture while this lease is live."""
        return NativePointer(
            self._native.texture_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="WebGPU texture",
        )

    @property
    def texture_view(self) -> NativePointer:
        """Return the borrowed WebGPU texture view while this lease is live."""
        return NativePointer(
            self._native.texture_view_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="WebGPU texture view",
        )

    @property
    def device(self) -> NativePointer:
        """Return the borrowed WebGPU device while this lease is live."""
        return NativePointer(
            self._native.device_address(),
            _is_live=lambda: not self.closed,
            _diagnostic_name="WebGPU device",
        )


class OpenGLOwnedTextureFrameHandle(_AcquiredFrameHandle):
    """Acquired OpenGL texture-slot lease."""

    def __init__(
        self,
        native: Any,
        runtime: Any,
        *,
        _create_key: object | None = None,
    ) -> None:
        super().__init__(native, runtime, _create_key, _OPENGL_FRAME_HANDLE_CREATE_KEY)

    @classmethod
    def _from_native(cls, native: Any, runtime: Any) -> OpenGLOwnedTextureFrameHandle:
        return cls(native, runtime, _create_key=_OPENGL_FRAME_HANDLE_CREATE_KEY)

    @property
    def frame(self) -> OpenGLOwnedTextureFrame:
        """Return copied frame metadata."""
        return OpenGLOwnedTextureFrame._from_native(self._native.frame())

    @property
    def texture(self) -> FrameOpenGLTextureName:
        """Return the borrowed OpenGL texture while this lease is live."""
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


from .offline import OperationHandle

__all__ = [
    "EglContextDescriptor",
    "FrameDemand",
    "FrameDemandFlag",
    "FrameOpenGLTextureName",
    "GpuSync",
    "GpuSyncKind",
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
    "RenderAbandonDisposition",
    "RenderAbandonResult",
    "RenderBackend",
    "RenderDriver",
    "RenderFrameResult",
    "RenderResult",
    "RenderSessionAttachOptions",
    "RenderSessionCapabilities",
    "RenderSessionCapability",
    "RenderSessionHandle",
    "RenderSessionSnapshot",
    "RenderSessionState",
    "RenderTargetExtent",
    "TextureImageInfo",
    "VulkanBorrowedTextureDescriptor",
    "VulkanContextDescriptor",
    "VulkanOwnedTextureDescriptor",
    "VulkanOwnedTextureFrame",
    "VulkanOwnedTextureFrameHandle",
    "VulkanSurfaceDescriptor",
    "WebGPUBorrowedTextureDescriptor",
    "WebGPUContextDescriptor",
    "WebGPUOwnedTextureDescriptor",
    "WebGPUOwnedTextureFrame",
    "WebGPUOwnedTextureFrameHandle",
    "WebGPUSurfaceDescriptor",
    "WglContextDescriptor",
]

from .map import MapHandle
