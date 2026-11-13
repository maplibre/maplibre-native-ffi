"""Generated from the C headers by tools/bindgen. Do not edit."""

from __future__ import annotations

from collections.abc import Buffer, Sequence
from dataclasses import dataclass, field
from enum import IntFlag
from typing import TYPE_CHECKING, Any, Final, Protocol

from ._enum import UnknownIntEnum
from ._operation import _adopt_value

if TYPE_CHECKING:
    from ._generated_owners import (
        MapHandle,
        ResourceRequestHandle,
        ResourceTransformRequestHandle,
    )


@dataclass(frozen=True, slots=True)
class UnknownVariant:
    tag: int

    @property
    def _tag(self):
        return self.tag


def _copy_variant(raw, variants, empty=None):
    if raw["kind"] is None:
        return None if raw["tag"] == empty else UnknownVariant(raw["tag"])
    return variants[raw["kind"]]._from_native(raw["value"])


def _maybe(convert, raw):
    return None if raw is None else convert(raw)


def _sequence(value):
    if value is None or type(value) is tuple:
        return value
    if isinstance(value, str):
        raise TypeError("expected a sequence, not str")
    return tuple(value)


def _bytes(value):
    return (
        value if value is None or type(value) is bytes else memoryview(value).tobytes()
    )


@dataclass(frozen=True, slots=True)
class OpenGlContextDescriptorWglVariant:
    value: WglContextDescriptor
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(WglContextDescriptor._from_native(raw))


@dataclass(frozen=True, slots=True)
class OpenGlContextDescriptorEglVariant:
    value: EglContextDescriptor
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(EglContextDescriptor._from_native(raw))


@dataclass(frozen=True, slots=True)
class OpenGlContextDescriptorWebGlVariant:
    value: WebGlContextDescriptor
    _tag = 3

    @classmethod
    def _from_native(cls, raw):
        return cls(WebGlContextDescriptor._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetOffscreenVariant:
    value: OffscreenTarget
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(OffscreenTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetMetalLayerVariant:
    value: MetalLayerTarget
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(MetalLayerTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetWin32WindowVariant:
    value: Win32WindowTarget
    _tag = 3

    @classmethod
    def _from_native(cls, raw):
        return cls(Win32WindowTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetXlibWindowVariant:
    value: XlibWindowTarget
    _tag = 4

    @classmethod
    def _from_native(cls, raw):
        return cls(XlibWindowTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetWaylandSurfaceVariant:
    value: WaylandSurfaceTarget
    _tag = 5

    @classmethod
    def _from_native(cls, raw):
        return cls(WaylandSurfaceTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetAndroidWindowVariant:
    value: AndroidWindowTarget
    _tag = 6

    @classmethod
    def _from_native(cls, raw):
        return cls(AndroidWindowTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetOhosWindowVariant:
    value: OhosWindowTarget
    _tag = 7

    @classmethod
    def _from_native(cls, raw):
        return cls(OhosWindowTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetVulkanSurfaceVariant:
    value: VulkanSurfaceTarget
    _tag = 10

    @classmethod
    def _from_native(cls, raw):
        return cls(VulkanSurfaceTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetOpenGlSurfaceVariant:
    value: OpenGlSurfaceTarget
    _tag = 11

    @classmethod
    def _from_native(cls, raw):
        return cls(OpenGlSurfaceTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetMetalBorrowedTexturesVariant:
    value: MetalBorrowedTexturesTarget
    _tag = 12

    @classmethod
    def _from_native(cls, raw):
        return cls(MetalBorrowedTexturesTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetVulkanBorrowedTexturesVariant:
    value: VulkanBorrowedTexturesTarget
    _tag = 13

    @classmethod
    def _from_native(cls, raw):
        return cls(VulkanBorrowedTexturesTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetOpenGlBorrowedTexturesVariant:
    value: OpenGlBorrowedTexturesTarget
    _tag = 14

    @classmethod
    def _from_native(cls, raw):
        return cls(OpenGlBorrowedTexturesTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetWebGpuBorrowedTexturesVariant:
    value: WebGpuBorrowedTexturesTarget
    _tag = 15

    @classmethod
    def _from_native(cls, raw):
        return cls(WebGpuBorrowedTexturesTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetExportedRingVariant:
    value: ExportedRingTarget
    _tag = 16

    @classmethod
    def _from_native(cls, raw):
        return cls(ExportedRingTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderTargetWebCanvasVariant:
    value: WebCanvasTarget
    _tag = 17

    @classmethod
    def _from_native(cls, raw):
        return cls(WebCanvasTarget._from_native(raw))


@dataclass(frozen=True, slots=True)
class FeatureIdStringValueVariant:
    value: str
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(raw)


@dataclass(frozen=True, slots=True)
class FeatureIdUintValueVariant:
    value: int
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(raw)


@dataclass(frozen=True, slots=True)
class FeatureIdIntValueVariant:
    value: int
    _tag = 3

    @classmethod
    def _from_native(cls, raw):
        return cls(raw)


@dataclass(frozen=True, slots=True)
class FeatureIdDoubleValueVariant:
    """A finite number."""

    value: float
    _tag = 4

    @classmethod
    def _from_native(cls, raw):
        return cls(raw)


@dataclass(frozen=True, slots=True)
class RenderedQueryGeometryPointVariant:
    value: ScreenPoint
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(ScreenPoint._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderedQueryGeometryBoxVariant:
    value: ScreenBox
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(ScreenBox._from_native(raw))


@dataclass(frozen=True, slots=True)
class RenderedQueryGeometryLineStringVariant:
    value: ScreenLineString
    _tag = 3

    @classmethod
    def _from_native(cls, raw):
        return cls(ScreenLineString._from_native(raw))


@dataclass(frozen=True, slots=True)
class OfflineRegionDefinitionTilePyramidVariant:
    value: OfflineTilePyramidRegionDefinition
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(OfflineTilePyramidRegionDefinition._from_native(raw))


@dataclass(frozen=True, slots=True)
class OfflineRegionDefinitionGeometryVariant:
    value: OfflineGeometryRegionDefinition
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(OfflineGeometryRegionDefinition._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventCameraChangeVariant:
    value: RuntimeEventCameraChange
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventCameraChange._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventLoadingFailedVariant:
    value: RuntimeEventLoadingFailed
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventLoadingFailed._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventRenderFrameVariant:
    value: RuntimeEventRenderFrame
    _tag = 3

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventRenderFrame._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventStyleImageVariant:
    value: RuntimeEventStyleImage
    _tag = 4

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventStyleImage._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventTileActionVariant:
    value: RuntimeEventTileAction
    _tag = 5

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventTileAction._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventSourceChangedVariant:
    value: RuntimeEventSourceChanged
    _tag = 6

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventSourceChanged._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionStatusVariant:
    value: RuntimeEventOfflineRegionStatus
    _tag = 7

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionStatus._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionResponseErrorVariant:
    value: RuntimeEventOfflineRegionResponseError
    _tag = 8

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionResponseError._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventGlyphsVariant:
    value: RuntimeEventGlyphs
    _tag = 9

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventGlyphs._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventSpriteVariant:
    value: RuntimeEventSprite
    _tag = 10

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventSprite._from_native(raw))


class CacheDatabaseOperation(UnknownIntEnum):
    """A maintenance action on a runtime's cache database.

    The cache database is the file that `RuntimeOptions.cache_path` names. It
    holds the ambient cache, meaning the resources that maps loaded, and the
    offline regions.

    See `mln_cache_database_operation` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    CLEAR = 1
    INVALIDATE = 2
    PACK = 3
    RESET = 4


class CameraChangeMode(UnknownIntEnum):
    """Whether a camera change jumps or animates.

    See `mln_camera_change_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    IMMEDIATE = 0
    ANIMATED = 1


class CameraChangeSource(UnknownIntEnum):
    """What moved the camera.

    See `mln_camera_change_source` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    COMMAND = 0
    GESTURE = 1
    MOMENTUM = 2


class CameraTransitionOutcome(UnknownIntEnum):
    """How a camera change ended.

    A change that ends in more than one way reports
    `CameraTransitionOutcome.CANCELLED` if any of it was cancelled, and
    otherwise `CameraTransitionOutcome.REPLACED` if any of it was replaced.

    See `mln_camera_transition_outcome` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    COMPLETED = 0
    CANCELLED = 1
    CLOSED = 2
    REPLACED = 3


class CameraUpdateMode(UnknownIntEnum):
    """How a camera command moves the camera.

    See `mln_camera_update_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    JUMP = 0
    EASE = 1
    FLY = 2


class ClipDepthRange(UnknownIntEnum):
    """How a graphics API's clip space holds depth.

    See `mln_clip_depth_range` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    ZERO_TO_ONE = 1
    NEGATIVE_ONE_TO_ONE = 2


class ConstrainMode(UnknownIntEnum):
    """How the map keeps the camera within the world or within its bounds.

    See `mln_constrain_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    NONE = 0
    HEIGHT_ONLY = 1
    WIDTH_AND_HEIGHT = 2
    SCREEN = 3


class DepthStencilFormatFlag(IntFlag):
    """A format of depth and stencil textures, as a bit in a set.

    See `mln_depth_stencil_format_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    DEPTH24_STENCIL8 = 1
    DEPTH32_FLOAT_STENCIL8 = 2


class FrameDemandFlag(IntFlag):
    """How a frame demand behaves.

    Without flags, a demand draws a frame when the map changed since the last
    one, and otherwise finishes with `RenderResult.NO_UPDATE`. A session with
    a surface target shows each frame that it draws.

    See `mln_frame_demand_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    FORCE = 1
    WAIT_FOR_UPDATE = 4


class GesturePhase(UnknownIntEnum):
    """Where a camera change falls in a gesture that the user makes.

    `GesturePhase.BEGIN` and `GesturePhase.UPDATE` mark a gesture in
    progress, and `GesturePhase.END` and `GesturePhase.CANCEL` end it.

    See `mln_gesture_phase` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    NONE = 0
    BEGIN = 1
    UPDATE = 2
    END = 3
    CANCEL = 4


class GpuExportKind(UnknownIntEnum):
    """A way to share a texture's memory with another API or process.

    See `mln_gpu_export_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    NONE = 0
    IO_SURFACE = 1
    DMA_BUF = 2
    ANDROID_HARDWARE_BUFFER = 3
    DXGI_SHARED_HANDLE = 4
    OPAQUE_FD = 5


class GpuExportKindFlag(IntFlag):
    """A way to share a texture's memory, as a bit in a set.

    See `mln_gpu_export_kind_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    IO_SURFACE = 1
    DMA_BUF = 2
    ANDROID_HARDWARE_BUFFER = 4
    DXGI_SHARED_HANDLE = 8
    OPAQUE_FD = 16


class GpuSyncKind(UnknownIntEnum):
    """How one side waits for the other's GPU work on a shared texture.

    A sync file descriptor or an OpenGL fence signals once, so when
    `AcquiredFrameHandle.release()` succeeds, the session takes the one you
    pass. A Vulkan timeline semaphore, Metal shared event, or D3D12 fence
    counts up with each frame, so the one you pass stays yours. Keep it valid
    until a detach or release of the session completes.

    See `mln_gpu_sync_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    CPU_COMPLETE = 0
    OPEN_GL_FENCE = 1
    QUEUE_ORDER = 2
    VULKAN_TIMELINE_SEMAPHORE = 3
    SYNC_FD = 4
    METAL_SHARED_EVENT = 5
    D3D12_FENCE = 6


class GpuSyncKindFlag(IntFlag):
    """A GPU sync kind, as a bit in a set.

    See `mln_gpu_sync_kind_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    CPU_COMPLETE = 1
    OPEN_GL_FENCE = 2
    QUEUE_ORDER = 4
    VULKAN_TIMELINE_SEMAPHORE = 8
    SYNC_FD = 16
    METAL_SHARED_EVENT = 32
    D3D12_FENCE = 64


class LogEvent(UnknownIntEnum):
    """The part of the library that logged a message.

    See `mln_log_event` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    GENERAL = 0
    SETUP = 1
    SHADER = 2
    PARSE_STYLE = 3
    PARSE_TILE = 4
    RENDER = 5
    STYLE = 6
    DATABASE = 7
    HTTP_REQUEST = 8
    SPRITE = 9
    IMAGE = 10
    GRAPHICS_BACKEND = 11
    JNI = 12
    ANDROID = 13
    CRASH = 14
    GLYPH = 15
    TIMING = 16


class LogSeverity(UnknownIntEnum):
    """The severity of a log message.

    See `mln_log_severity` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    INFO = 1
    WARNING = 2
    ERROR = 3


class LogSeverityFlag(IntFlag):
    """A log severity, as a bit in a set.

    See `mln_log_severity_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    INFO = 2
    WARNING = 4
    ERROR = 8
    DEFAULT = 6
    ALL = 14


class MapDebugFlag(IntFlag):
    """A debug overlay that the map draws.

    See `mln_map_debug_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    TILE_BORDERS = 2
    PARSE_STATUS = 4
    TIMESTAMPS = 8
    COLLISION = 16
    OVERDRAW = 32
    STENCIL_CLIP = 64
    DEPTH_BUFFER = 128


class MapLoadError(UnknownIntEnum):
    """Why a map failed to load a style.

    See `mln_map_load_error` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    UNKNOWN = 0
    STYLE_PARSE = 1
    STYLE_LOAD = 2


class MapMode(UnknownIntEnum):
    """When a map renders.

    See `mln_map_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    CONTINUOUS = 0
    STATIC = 1
    TILE = 2


class MomentumDecayModel(UnknownIntEnum):
    """How a part of a gesture's momentum slows from its release speed.

    See `mln_momentum_decay_model` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    FRICTION = 0
    EASE = 1


class NetworkStatus(UnknownIntEnum):
    """Whether the library may use the network.

    See `mln_network_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    ONLINE = 1
    OFFLINE = 2


class NorthOrientation(UnknownIntEnum):
    """The screen direction that north points to when the bearing is 0.

    See `mln_north_orientation` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    UP = 0
    RIGHT = 1
    DOWN = 2
    LEFT = 3


class OfflineRegionDownloadState(UnknownIntEnum):
    """Whether an offline region is downloading.

    See `mln_offline_region_download_state` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    INACTIVE = 0
    ACTIVE = 1


class OpenGlClientApi(UnknownIntEnum):
    """The OpenGL API that a context renders with.

    See `mln_open_gl_client_api` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    UNSPECIFIED = 0
    GL = 1
    GLES = 2


class OpenGlContextProviderFlag(IntFlag):
    """An OpenGL context platform, as a bit in a set.

    See `mln_open_gl_context_provider_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    WGL = 1
    EGL = 2
    WEB_GL = 4


class ProjectionType(UnknownIntEnum):
    """A projection that a style's
    \\[projection\\](https://maplibre.org/maplibre-style-spec/projection/)
    names.

    See `mln_projection_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    MERCATOR = 0
    VERTICAL_PERSPECTIVE = 1
    GLOBE = 2


class RenderAbandonDisposition(UnknownIntEnum):
    """What `RenderSessionHandle.abandon()` did with a session's graphics
    objects.

    See `mln_render_abandon_disposition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    CLEAN = 0
    QUARANTINED = 1


class RenderBackend(UnknownIntEnum):
    """A render backend.

    See `mln_render_backend` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    METAL = 1
    VULKAN = 2
    OPEN_GL = 3
    WEB_GPU = 4


class RenderBackendFlag(IntFlag):
    """A render backend, as a bit in a set.

    See `mln_render_backend_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    METAL = 1
    VULKAN = 2
    OPEN_GL = 4
    WEB_GPU = 8


class RenderDriverKind(UnknownIntEnum):
    """The thread that runs a render session's graphics work.

    See `mln_render_driver_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    CORE_WORKER = 1
    CALLER_GRAPHICS_THREAD = 2


class RenderMode(UnknownIntEnum):
    """Whether a frame drew the map complete, or lacked resources that are still
    loading.

    See `mln_render_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    PARTIAL = 0
    FULL = 1


class RenderResult(UnknownIntEnum):
    """How a frame demand finished.

    See `mln_render_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    RENDERED = 0
    NO_UPDATE = 1
    TARGET_NOT_READY = 2
    SUPERSEDED = 3
    FAILED = 4


class RenderSessionCapabilityFlag(IntFlag):
    """Something that a render session can do.

    See `mln_render_session_capability_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    FRAME_ACQUISITION = 1
    READBACK = 2
    PRESENTATION = 4


class RenderSessionState(UnknownIntEnum):
    """Where a render session is in its life.

    See `mln_render_session_state` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    ATTACHING = 1
    ATTACHED = 2
    DETACHING = 3
    DETACHED = 4
    FAILED = 5
    ABANDONED = 6


class RenderTargetKindFlag(IntFlag):
    """A render target kind, as a bit in a set.

    See `mln_render_target_kind_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    OFFSCREEN = 1
    METAL_LAYER = 2
    WIN32_WINDOW = 4
    XLIB_WINDOW = 8
    WAYLAND_SURFACE = 16
    ANDROID_WINDOW = 32
    OHOS_WINDOW = 64
    VULKAN_SURFACE = 512
    OPEN_GL_SURFACE = 1024
    METAL_BORROWED_TEXTURES = 2048
    VULKAN_BORROWED_TEXTURES = 4096
    OPEN_GL_BORROWED_TEXTURES = 8192
    WEB_GPU_BORROWED_TEXTURES = 16384
    EXPORTED_RING = 32768
    WEB_CANVAS = 65536


class ResourceErrorReason(UnknownIntEnum):
    """Why a resource request failed.

    See `mln_resource_error_reason` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    OTHER = 0
    NOT_FOUND = 1
    SERVER = 2
    CONNECTION = 3
    RATE_LIMIT = 4


class ResourceKind(UnknownIntEnum):
    """The kind of a resource that the library loads.

    See `mln_resource_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    UNKNOWN = 0
    STYLE = 1
    SOURCE = 2
    TILE = 3
    GLYPHS = 4
    SPRITE_IMAGE = 5
    SPRITE_JSON = 6
    IMAGE = 7


class ResourceKindFlag(IntFlag):
    """A resource kind, as a bit in a set.

    See `mln_resource_kind_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    UNKNOWN = 1
    STYLE = 2
    SOURCE = 4
    TILE = 8
    GLYPHS = 16
    SPRITE_IMAGE = 32
    SPRITE_JSON = 64
    IMAGE = 128
    ALL = 255


class ResourceLoadingMethod(UnknownIntEnum):
    """Where a request may load its resource from.

    See `mln_resource_loading_method` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    ALL = 0
    CACHE_ONLY = 1
    NETWORK_ONLY = 2


class ResourcePriority(UnknownIntEnum):
    """The priority of a resource request.

    See `mln_resource_priority` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    REGULAR = 0
    LOW = 1


class ResourceResponseStatus(UnknownIntEnum):
    """The outcome of a resource request.

    See `mln_resource_response_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    OK = 0
    ERROR = 1
    NO_CONTENT = 2
    NOT_MODIFIED = 3


class ResourceStoragePolicy(UnknownIntEnum):
    """Whether the library keeps a loaded resource in its cache.

    See `mln_resource_storage_policy` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    PERMANENT = 0
    VOLATILE = 1


class ResourceUsage(UnknownIntEnum):
    """Whether a request loads a resource for a map or an offline region.

    See `mln_resource_usage` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    ONLINE = 0
    OFFLINE = 1


class RuntimeEventFlag(IntFlag):
    """An event type, as a bit in a set.

    See `mln_runtime_event_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    NONE = 0
    MAP_CAMERA_WILL_CHANGE = 2
    MAP_CAMERA_IS_CHANGING = 4
    MAP_CAMERA_DID_CHANGE = 8
    MAP_STYLE_LOADED = 16
    MAP_LOADING_STARTED = 32
    MAP_LOADING_FINISHED = 64
    MAP_LOADING_FAILED = 128
    MAP_IDLE = 256
    MAP_RENDER_ERROR = 512
    MAP_RENDER_FRAME_STARTED = 1024
    MAP_RENDER_FRAME_FINISHED = 2048
    MAP_RENDER_MAP_STARTED = 4096
    MAP_RENDER_MAP_FINISHED = 8192
    MAP_STYLE_IMAGE_MISSING = 16384
    MAP_TILE_ACTION = 32768
    MAP_SOURCE_CHANGED = 65536
    OFFLINE_REGION_STATUS_CHANGED = 131072
    OFFLINE_REGION_RESPONSE_ERROR = 262144
    MAP_GLYPHS_REQUESTED = 524288
    MAP_GLYPHS_LOADED = 1048576
    MAP_GLYPHS_ERROR = 2097152
    MAP_SPRITE_REQUESTED = 4194304
    MAP_SPRITE_LOADED = 8388608
    MAP_SPRITE_ERROR = 16777216
    ALL_MAP_EVENTS = 33161214
    ALL_RUNTIME_EVENTS = 393216
    ALL = 33554430


class RuntimeEventSourceType(UnknownIntEnum):
    """The kind of handle that raised an event.

    See `mln_runtime_event_source_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    RUNTIME = 0
    MAP = 1


class RuntimeEventType(UnknownIntEnum):
    """The kind of an event.

    Only maps in `MapMode.CONTINUOUS` raise
    `RuntimeEventType.MAP_LOADING_FINISHED`, `RuntimeEventType.MAP_IDLE`, and
    the render frame and render map events. Offline region events come only
    for the regions and types that
    `RuntimeHandle.set_offline_region_observed()` selects.

    See `mln_runtime_event_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    MAP_CAMERA_WILL_CHANGE = 1
    MAP_CAMERA_IS_CHANGING = 2
    MAP_CAMERA_DID_CHANGE = 3
    MAP_STYLE_LOADED = 4
    MAP_LOADING_STARTED = 5
    MAP_LOADING_FINISHED = 6
    MAP_LOADING_FAILED = 7
    MAP_IDLE = 8
    MAP_RENDER_ERROR = 9
    MAP_RENDER_FRAME_STARTED = 10
    MAP_RENDER_FRAME_FINISHED = 11
    MAP_RENDER_MAP_STARTED = 12
    MAP_RENDER_MAP_FINISHED = 13
    MAP_STYLE_IMAGE_MISSING = 14
    MAP_TILE_ACTION = 15
    MAP_SOURCE_CHANGED = 16
    OFFLINE_REGION_STATUS_CHANGED = 17
    OFFLINE_REGION_RESPONSE_ERROR = 18
    MAP_GLYPHS_REQUESTED = 19
    MAP_GLYPHS_LOADED = 20
    MAP_GLYPHS_ERROR = 21
    MAP_SPRITE_REQUESTED = 22
    MAP_SPRITE_LOADED = 23
    MAP_SPRITE_ERROR = 24


class Status(UnknownIntEnum):
    """The result of a call or of asynchronous work.

    See `mln_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    OK = 0
    INVALID_ARGUMENT = -1
    INVALID_STATE = -2
    WRONG_THREAD = -3
    UNSUPPORTED = -4
    NATIVE_ERROR = -5
    CANCELLED = -6
    BUSY = -7
    TARGET_LOST = -8
    NOT_FOUND = -10
    ALREADY_EXISTS = -11


class StyleExpressionTypeKind(UnknownIntEnum):
    """The type of a style expression's result, or of the items of an array
    result.

    See `mln_style_expression_type_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    VALUE = 0
    NUMBER = 1
    STRING = 2
    BOOLEAN = 3
    COLOR = 4
    OBJECT = 5
    FORMATTED = 6
    IMAGE = 7
    PADDING = 8
    VARIABLE_ANCHOR_OFFSET_COLLECTION = 9
    ARRAY = 10


class StyleImageTextFit(UnknownIntEnum):
    """How an image stretches to fit text along one axis.

    See `mln_style_image_text_fit` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    STRETCH_OR_SHRINK = 0
    STRETCH_ONLY = 1
    PROPORTIONAL = 2


class StyleLayerVisibility(UnknownIntEnum):
    """Whether a layer shows.

    See `mln_style_layer_visibility` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    VISIBLE = 0
    NONE = 1


class StyleRasterDemEncoding(UnknownIntEnum):
    """How a raster DEM tile encodes elevation in its colors.

    See `mln_style_raster_dem_encoding` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MAPBOX = 0
    TERRARIUM = 1


class StyleSourceType(UnknownIntEnum):
    """The type of a style source.

    See `mln_style_source_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    UNKNOWN = 0
    VECTOR = 1
    RASTER = 2
    RASTER_DEM = 3
    GEO_JSON = 4
    IMAGE = 5
    VIDEO = 6
    CUSTOM_GEOMETRY = 7
    CUSTOM_VECTOR = 8


class StyleTileScheme(UnknownIntEnum):
    """Whether a tileset counts tile rows from the north (XYZ) or the south
    (TMS).

    See `mln_style_tile_scheme` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    XYZ = 0
    TMS = 1


class StyleVectorTileEncoding(UnknownIntEnum):
    """The format of vector tiles.

    See `mln_style_vector_tile_encoding` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MVT = 0
    MLT = 1


class TileLodMode(UnknownIntEnum):
    """How the map picks the level of detail of its tiles.

    See `mln_tile_lod_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    DEFAULT = 0
    DISTANCE = 1


class TileOperation(UnknownIntEnum):
    """A step in loading a tile.

    See `mln_tile_operation` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    REQUESTED_FROM_CACHE = 0
    REQUESTED_FROM_NETWORK = 1
    LOAD_FROM_NETWORK = 2
    LOAD_FROM_CACHE = 3
    START_PARSE = 4
    END_PARSE = 5
    ERROR = 6
    CANCELLED = 7
    NULL = 8


class UrlMatchFlag(IntFlag):
    """How a URL pattern matches a request's URL.

    A pattern matches the whole URL, query included. Without flags, it
    matches only the URL that equals it exactly.

    See `mln_url_match_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    GLOB = 1


class ViewportMode(UnknownIntEnum):
    """The vertical direction of the image that the map renders.

    See `mln_viewport_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    DEFAULT = 0
    FLIPPED_Y = 1


class VulkanImageOwner(UnknownIntEnum):
    """The queue family that a borrowed image goes to after each frame.

    With `VulkanImageOwner.EXTERNAL` or `VulkanImageOwner.FOREIGN`, acquire
    the image on your side before you read it. You don't need to release it
    back.

    See `mln_vulkan_image_owner` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    GRAPHICS_QUEUE = 0
    EXTERNAL = 1
    FOREIGN = 2


@dataclass(frozen=True, slots=True)
class AnchoredMomentum:
    """How a gesture's zoom, bearing, or pitch keeps changing after release.

    See `mln_anchored_momentum` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    velocity: float | None = None
    """How fast the value changed at release: zoom levels per second for zoom,
    and degrees per second for bearing and pitch. The part moves only when
    this is set.
    """

    anchor: ScreenPoint | None = None
    """The screen point that the change keeps in place. Without one, the change
    keeps the center of the view in place.
    """

    decay: MomentumDecay = field(default_factory=lambda: MomentumDecay())

    snap: MomentumSnap | None = None
    """Where the part settles. Without this, it stays where the gesture and its
    glide leave it.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            velocity=raw["velocity"],
            anchor=_maybe(ScreenPoint._from_native, raw["anchor"]),
            decay=MomentumDecay._from_native(raw["decay"]),
            snap=_maybe(MomentumSnap._from_native, raw["snap"]),
        )


@dataclass(frozen=True, slots=True)
class AndroidHardwareBufferFrame:
    """The Android hardware buffer of an exported ring's acquired frame.

    See `mln_android_hardware_buffer_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The buffer's index in the ring."""

    buffer: int
    """The `AHardwareBuffer*`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            buffer=raw["buffer"],
        )


@dataclass(frozen=True, slots=True)
class AndroidWindowTarget:
    """An Android window to present to.

    The session takes its own reference to the window, so you can release
    yours once the call returns. When the platform destroys the window, such
    as in `SurfaceHolder.Callback.surfaceDestroyed`, release the target with
    `RenderSessionHandle.release_target()`, or give the session another
    window with `RenderSessionHandle.set_target()`, without waiting for that
    to complete. A frame to the destroyed window finishes with
    `RenderResult.TARGET_NOT_READY`, or with `RenderResult.RENDERED` if the
    window went away while the frame was presenting.

    See `mln_android_window_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    window: int
    """Your `ANativeWindow*`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            window=raw["window"],
        )


@dataclass(frozen=True, slots=True)
class AnimationOptions:
    """How a camera change animates.

    See `mln_animation_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    duration_ms: float | None = None

    velocity: float | None = None
    """The average speed of a fly animation, in screens per second.
    `duration_ms` overrides it.
    """

    min_zoom: float | None = None
    """The zoom level at the peak of a fly animation."""

    max_duration_ms: float | None = None
    """The longest a fly animation may take. A fly animation whose duration, set
    or derived from `velocity`, is longer moves the camera at once instead,
    and so does one too slow for the library to time, without this field.
    Other animations ignore it.
    """

    easing: UnitBezier | None = None

    transition_id: int | None = None
    """An ID that `MapHandle.cancel_camera_transitions_with_id()` cancels the
    change by. Several changes can share one.
    """

    end_handler: CameraTransitionHandler = field(
        default_factory=lambda: CameraTransitionHandler()
    )

    @classmethod
    def _from_native(cls, raw):
        return cls(
            duration_ms=raw["duration_ms"],
            velocity=raw["velocity"],
            min_zoom=raw["min_zoom"],
            max_duration_ms=raw["max_duration_ms"],
            easing=_maybe(UnitBezier._from_native, raw["easing"]),
            transition_id=raw["transition_id"],
            end_handler=CameraTransitionHandler(),
        )


@dataclass(frozen=True, slots=True)
class BoundOptions:
    """Limits on the camera, for `MapHandle.set_bounds()`.

    See `mln_bound_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    bounds: LatLngBounds | None = None
    """The area that the camera's center stays within."""

    min_zoom: float | None = None

    max_zoom: float | None = None

    min_pitch: float | None = None
    """The lowest pitch, in degrees."""

    max_pitch: float | None = None
    """The highest pitch, in degrees."""

    unbounded: bool = False

    @classmethod
    def _from_native(cls, raw):
        return cls(
            bounds=_maybe(LatLngBounds._from_native, raw["bounds"]),
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            min_pitch=raw["min_pitch"],
            max_pitch=raw["max_pitch"],
            unbounded=raw["unbounded"],
        )


@dataclass(frozen=True, slots=True)
class Camera:
    """A map's whole camera.

    See `mln_camera` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    center: LatLng = field(default_factory=lambda: LatLng())

    center_altitude: float = 0.0
    """The altitude of the center, in meters above sea level."""

    padding: EdgeInsets = field(default_factory=lambda: EdgeInsets())
    """Insets from the map's edges. The camera centers within the rest."""

    zoom: float = 0.0

    bearing: float = 0.0
    """The rotation from north, in degrees."""

    pitch: float = 0.0
    """The tilt toward the horizon, in degrees. 0 looks straight down."""

    roll: float = 0.0
    """The roll, in degrees."""

    field_of_view: float = 0.0
    """The vertical field of view, in degrees."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            center=LatLng._from_native(raw["center"]),
            center_altitude=raw["center_altitude"],
            padding=EdgeInsets._from_native(raw["padding"]),
            zoom=raw["zoom"],
            bearing=raw["bearing"],
            pitch=raw["pitch"],
            roll=raw["roll"],
            field_of_view=raw["field_of_view"],
        )

    def to_options(self) -> CameraOptions:
        """Converts this `Camera` to `CameraOptions` that set each of its
        members.
        """
        return CameraOptions(
            center=self.center,
            center_altitude=self.center_altitude,
            padding=self.padding,
            zoom=self.zoom,
            bearing=self.bearing,
            pitch=self.pitch,
            roll=self.roll,
            field_of_view=self.field_of_view,
        )


@dataclass(frozen=True, slots=True)
class CameraBounds:
    """All the limits on a map's camera.

    Its members mean the same as those of `BoundOptions`.

    See `mln_camera_bounds` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    bounds: LatLngBounds
    """The area that the camera's center stays within. When `unbounded` is true,
    it covers the whole world.
    """

    unbounded: bool
    """Whether no area limits the camera's center."""

    min_zoom: float

    max_zoom: float

    min_pitch: float
    """The lowest pitch, in degrees."""

    max_pitch: float
    """The highest pitch, in degrees."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            bounds=LatLngBounds._from_native(raw["bounds"]),
            unbounded=raw["unbounded"],
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            min_pitch=raw["min_pitch"],
            max_pitch=raw["max_pitch"],
        )


@dataclass(frozen=True, slots=True)
class CameraDelta:
    """A change to the camera from where it is now, for gestures.

    See `mln_camera_delta` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    offset: ScreenPoint | None = None
    """Moves the map's content this far on the screen, as a drag does."""

    scale: float | None = None
    """Multiplies the map's scale. 2 zooms in by one level."""

    bearing: float | None = None
    """Degrees to add to the bearing."""

    pitch: float | None = None
    """Degrees to add to the pitch."""

    anchor: ScreenPoint | None = None
    """The screen point that `scale`, `bearing`, and `pitch` keep in place. It
    needs one of them, and it combines with `offset` only in a delta without
    a duration.
    """

    momentum: GestureMomentum | None = None
    """How the gesture keeps moving after this delta. Only a delta in the
    `GesturePhase.END` phase carries it.
    """

    animation: AnimationOptions = field(default_factory=lambda: AnimationOptions())

    gesture_phase: GesturePhase = GesturePhase.NONE

    @classmethod
    def _from_native(cls, raw):
        return cls(
            offset=_maybe(ScreenPoint._from_native, raw["offset"]),
            scale=raw["scale"],
            bearing=raw["bearing"],
            pitch=raw["pitch"],
            anchor=_maybe(ScreenPoint._from_native, raw["anchor"]),
            momentum=_maybe(GestureMomentum._from_native, raw["momentum"]),
            animation=AnimationOptions._from_native(raw["animation"]),
            gesture_phase=GesturePhase(raw["gesture_phase"]),
        )


@dataclass(frozen=True, slots=True)
class CameraFitOptions:
    """Options for finding the camera that fits an area.

    See `mln_camera_fit_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    padding: EdgeInsets | None = None
    """Space to leave between the area and each edge of the map. The left and
    right insets must add up to less than the map's width, and the top and
    bottom to less than its height.
    """

    bearing: float | None = None
    """The rotation from north, in degrees."""

    pitch: float | None = None
    """The tilt toward the horizon, in degrees."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            padding=_maybe(EdgeInsets._from_native, raw["padding"]),
            bearing=raw["bearing"],
            pitch=raw["pitch"],
        )


@dataclass(frozen=True, slots=True)
class CameraFitUpdate:
    """A camera change that fits an area, for
    `MapHandle.fit_camera_to_lat_lng_bounds()` and the other fit commands.

    A jump uses only the end handler of `animation`, as in `CameraUpdate`.

    See `mln_camera_fit_update` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    mode: CameraUpdateMode = CameraUpdateMode.JUMP

    fit: CameraFitOptions = field(default_factory=lambda: CameraFitOptions())

    animation: AnimationOptions = field(default_factory=lambda: AnimationOptions())

    @classmethod
    def _from_native(cls, raw):
        return cls(
            mode=CameraUpdateMode(raw["mode"]),
            fit=CameraFitOptions._from_native(raw["fit"]),
            animation=AnimationOptions._from_native(raw["animation"]),
        )


@dataclass(frozen=True, slots=True)
class CameraOptions:
    """A camera, with only the members that `fields` selects.

    See `mln_camera_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    center: LatLng | None = None

    center_altitude: float | None = None
    """The altitude of the center, in meters above sea level."""

    padding: EdgeInsets | None = None
    """Insets from the map's edges. The camera centers within the rest."""

    anchor: ScreenPoint | None = None
    """The screen point that a change to the zoom, bearing, or pitch keeps in
    place. A `center` overrides it.
    """

    zoom: float | None = None

    bearing: float | None = None
    """The rotation from north, in degrees."""

    pitch: float | None = None
    """The tilt toward the horizon, in degrees. 0 looks straight down."""

    roll: float | None = None
    """The roll, in degrees."""

    field_of_view: float | None = None
    """The vertical field of view, in degrees."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            center=_maybe(LatLng._from_native, raw["center"]),
            center_altitude=raw["center_altitude"],
            padding=_maybe(EdgeInsets._from_native, raw["padding"]),
            anchor=_maybe(ScreenPoint._from_native, raw["anchor"]),
            zoom=raw["zoom"],
            bearing=raw["bearing"],
            pitch=raw["pitch"],
            roll=raw["roll"],
            field_of_view=raw["field_of_view"],
        )


@dataclass(frozen=True, slots=True)
class CameraQueryResult:
    """A map's camera, with the snapshot generation that it matches.

    See `mln_camera_query_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    generation: int

    camera: Camera

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"], camera=Camera._from_native(raw["camera"])
        )


@dataclass(frozen=True, slots=True)
class CameraTransitionEnd:
    """The end of a camera change.

    See `mln_camera_transition_end` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    outcome: CameraTransitionOutcome

    generation: int
    """The generation of the first map snapshot that shows where the change left
    the camera, or 0 when the map closed first.
    """

    velocity: CameraVelocity
    """How fast a gesture's momentum still moved when it was cancelled, which
    the next gesture can add to its own release velocities. Zero for every
    other end.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            outcome=CameraTransitionOutcome(raw["outcome"]),
            generation=raw["generation"],
            velocity=CameraVelocity._from_native(raw["velocity"]),
        )


@dataclass(frozen=True, slots=True)
class CameraTransitionHandler:
    """A callback for the end of a camera change, for `AnimationOptions`.

    The callback runs once for each camera command, after every part of the
    change that the command started has ended.

    See `mln_camera_transition_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    callback: CameraTransitionEndCallback | None = None

    def _invoke_callback(self, end):
        callback = self.callback
        assert callback is not None
        return callback(CameraTransitionEnd._from_native(end))


@dataclass(frozen=True, slots=True)
class CameraUpdate:
    """A camera change for `MapHandle.update_camera()`.

    A jump uses only the end handler of `animation`, and the handler runs as
    soon as the camera moves.

    See `mln_camera_update` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    mode: CameraUpdateMode = CameraUpdateMode.JUMP

    camera: CameraOptions = field(default_factory=lambda: CameraOptions())

    animation: AnimationOptions = field(default_factory=lambda: AnimationOptions())

    gesture_phase: GesturePhase = GesturePhase.NONE

    @classmethod
    def _from_native(cls, raw):
        return cls(
            mode=CameraUpdateMode(raw["mode"]),
            camera=CameraOptions._from_native(raw["camera"]),
            animation=AnimationOptions._from_native(raw["animation"]),
            gesture_phase=GesturePhase(raw["gesture_phase"]),
        )


@dataclass(frozen=True, slots=True)
class CameraVelocity:
    """How fast each part of a camera change moves.

    See `mln_camera_velocity` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    pan: ScreenPoint
    """How fast the map's content moves on the screen, in logical pixels per
    second.
    """

    zoom: float
    """In zoom levels per second."""

    bearing: float
    """In degrees per second."""

    pitch: float
    """In degrees per second."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            pan=ScreenPoint._from_native(raw["pan"]),
            zoom=raw["zoom"],
            bearing=raw["bearing"],
            pitch=raw["pitch"],
        )


@dataclass(frozen=True, slots=True)
class CanonicalTileId:
    """A tile's zoom level `z`, column `x`, and row `y`, with row 0 at the
    north.

    See `mln_canonical_tile_id` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    z: int = 0

    x: int = 0

    y: int = 0

    @classmethod
    def _from_native(cls, raw):
        return cls(z=raw["z"], x=raw["x"], y=raw["y"])


@dataclass(frozen=True, slots=True)
class CustomGeometrySourceOptions:
    """The callbacks and options of a custom geometry source.

    Zoom levels are whole numbers from 0 to 32.

    See `mln_custom_geometry_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    fetch_tile: CustomSourceFetchTileCallback
    cancel_tile: CustomSourceCancelTileCallback | None = None
    min_zoom: float | None = None

    max_zoom: float | None = None

    tolerance: float | None = None
    """How far, in pixels, simplified geometry can stray from the original."""

    tile_size: int | None = None
    """The width of a tile in pixels, a power of two no larger than 8192."""

    buffer: int | None = None
    """The width in pixels of the margin around a tile that holds geometry from
    beyond its edges. `buffer` times 8192 / `tile_size` must be at most
    65535.
    """

    clip: bool | None = None
    """Whether to cut geometry at the outer edge of a tile's margin."""

    wrap: bool | None = None
    """Whether to wrap geometry that crosses the antimeridian."""

    def _invoke_fetch_tile(self, map, source_id, tile_id):
        callback = self.fetch_tile
        map = _adopt_value(map, "MapHandle")
        return callback(map, source_id, CanonicalTileId._from_native(tile_id))

    def _invoke_cancel_tile(self, map, source_id, tile_id):
        callback = self.cancel_tile
        assert callback is not None
        map = _adopt_value(map, "MapHandle")
        return callback(map, source_id, CanonicalTileId._from_native(tile_id))


@dataclass(frozen=True, slots=True)
class CustomLayerHost:
    """The callbacks of a custom layer, which draws with your own GPU commands
    in the map's frame.

    Nor does a browser WebGPU session yet: the library's WebGPU (wgpu)
    exposes no command encoder or render pass to hand you. Leave unset the
    callbacks that you don't need.

    Runs on the thread that runs the session's graphics work: a library
    thread, or yours with `RenderDriverKind.CALLER_GRAPHICS_THREAD`. It can't
    call the library.

    It can't call the library.

    See `mln_custom_layer_host` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    @classmethod
    def native(
        cls,
        prerender: int,
        render: int,
        deinitialize: int,
        context_lost: int,
        user_data: int = 0,
        release_user_data: int = 0,
    ) -> NativeCustomLayerHost:
        """A native registration, whose function pointers, context, and release
        the host's native code supplies as addresses. Zero is null. The
        binding copies each address unchanged, and native code validates
        them.
        """
        from . import _native

        return NativeCustomLayerHost._from_registration(
            _native._NativeRegistration(
                "mln_custom_layer_host",
                (prerender, render, deinitialize, context_lost),
                user_data,
                release_user_data,
            )
        )


class NativeCustomLayerHost:
    """A native `CustomLayerHost` from `CustomLayerHost.native`, accepted
    wherever a `CustomLayerHost` is.

    The first call that carries it consumes it. Native code releases the
    context of a call that it accepts, and the binding releases the context
    of a call that native code does not accept. A second call raises an
    invalid-state error.
    """

    __slots__ = ("_registration",)

    def __init__(self):
        raise TypeError("native registrations come from CustomLayerHost.native")

    @classmethod
    def _from_registration(cls, registration):
        value = cls.__new__(cls)
        value._registration = registration
        return value


@dataclass(frozen=True, slots=True)
class CustomVectorSourceOptions:
    """The callbacks and options of a custom vector source.

    Zoom levels are whole numbers from 0 to 32.

    See `mln_custom_vector_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    fetch_tile: CustomSourceFetchTileCallback
    cancel_tile: CustomSourceCancelTileCallback | None = None
    min_zoom: float | None = None

    max_zoom: float | None = None

    def _invoke_fetch_tile(self, map, source_id, tile_id):
        callback = self.fetch_tile
        map = _adopt_value(map, "MapHandle")
        return callback(map, source_id, CanonicalTileId._from_native(tile_id))

    def _invoke_cancel_tile(self, map, source_id, tile_id):
        callback = self.cancel_tile
        assert callback is not None
        map = _adopt_value(map, "MapHandle")
        return callback(map, source_id, CanonicalTileId._from_native(tile_id))


@dataclass(frozen=True, slots=True)
class DmaBufFormat:
    """A DRM format and modifier that you can import.

    See `mln_dma_buf_format` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    fourcc: int = 0
    """A DRM fourcc code, such as DRM_FORMAT_ABGR8888."""

    modifier: int = 0
    """A DRM format modifier, such as DRM_FORMAT_MOD_LINEAR."""

    @classmethod
    def _from_native(cls, raw):
        return cls(fourcc=raw["fourcc"], modifier=raw["modifier"])


@dataclass(frozen=True, slots=True)
class DmaBufFrame:
    """The dmabuf of an exported ring's acquired frame.

    See `mln_dma_buf_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The buffer's index in the ring."""

    fourcc: int

    modifier: int

    planes: tuple[DmaBufPlane, ...]
    """The buffer's planes, in the order that its fourcc and modifier define.
    Import all of them, as EGL_EXT_image_dma_buf_import_modifiers planes 0
    and up, or with one `zwp_linux_buffer_params_v1.add` call each.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            fourcc=raw["fourcc"],
            modifier=raw["modifier"],
            planes=tuple(DmaBufPlane._from_native(item) for item in raw["planes"]),
        )


@dataclass(frozen=True, slots=True)
class DmaBufPlane:
    """Where one plane of a dmabuf keeps its data.

    See `mln_dma_buf_plane` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    fd: int
    """The file descriptor of the dmabuf that holds the plane. The planes of a
    buffer often share one.
    """

    offset: int
    """The bytes from the start of the dmabuf to the plane's first byte."""

    stride: int
    """The bytes from the start of one row to the start of the next."""

    @classmethod
    def _from_native(cls, raw):
        return cls(fd=raw["fd"], offset=raw["offset"], stride=raw["stride"])


@dataclass(frozen=True, slots=True)
class DxgiSharedTextureFrame:
    """The DXGI shared texture of an exported ring's acquired frame.

    See `mln_dxgi_shared_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The texture's index in the ring."""

    format: int
    """The texture's `DXGI_FORMAT`."""

    handle: int
    """The texture's shared NT `HANDLE`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            format=raw["format"],
            handle=raw["handle"],
        )


@dataclass(frozen=True, slots=True)
class EdgeInsets:
    """Distances in logical pixels from each edge of the map.

    See `mln_edge_insets` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    top: float = 0.0

    left: float = 0.0

    bottom: float = 0.0

    right: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(
            top=raw["top"], left=raw["left"], bottom=raw["bottom"], right=raw["right"]
        )


@dataclass(frozen=True, slots=True)
class EglContextDescriptor:
    """Your EGL context, which each session's context shares objects with.

    Each session makes its context on `display` with `config` and the client
    API and version of `share_context`.

    See `mln_egl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    display: int
    """An `EGLDisplay`, which you initialized."""

    config: int
    """The `EGLConfig` of `share_context`."""

    share_context: int
    """Your `EGLContext`."""

    get_proc_address: int = 0
    """Your `eglGetProcAddress`, or null to use the system's."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            display=raw["display"],
            config=raw["config"],
            share_context=raw["share_context"],
            get_proc_address=raw["get_proc_address"],
        )


@dataclass(frozen=True, slots=True)
class ExportedRingTarget:
    """A ring of textures that the session allocates on its device and shares
    with you, in one of the ways that you can import.

    Each texture has the size in pixels of `logical_size` at the map's scale
    factor. `RenderSessionHandle.resize()` gives the ring textures of the new
    size, and a frame that you hold keeps its texture of the old size until
    you release or discard it.

    The ring shares its textures in the first kind of `export_kinds` that the
    device exports, in this order, and
    `RenderSessionCapabilities.export_kind` reports it:

    - `GpuExportKind.IO_SURFACE`: IOSurfaces of the pixel format
      kCVPixelFormatType_32BGRA, which Metal names MTLPixelFormatBGRA8Unorm.
      Get each frame's surface with `AcquiredFrameHandle.io_surface()`.
    - `GpuExportKind.ANDROID_HARDWARE_BUFFER`: hardware buffers of the format
      AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, with the usage
      AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
      AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of
      `android_hardware_buffer_usage`. Get each frame's buffer with
      `AcquiredFrameHandle.android_hardware_buffer()`.
    - `GpuExportKind.DXGI_SHARED_HANDLE`: D3D11 textures of the format
      DXGI_FORMAT_B8G8R8A8_UNORM on the device's GPU, which bind as shader
      resources and render targets. Open them on a D3D11 or D3D12 device of
      the same adapter, which you pick with `GpuDeviceOptions.adapter_luid`.
      Get each frame's texture with
      `AcquiredFrameHandle.dxgi_shared_texture()`.
    - `GpuExportKind.DMA_BUF`: dmabufs of a format and modifier from
      `dma_buf_formats`. Get each frame's buffer with
      `AcquiredFrameHandle.dma_buf()`.
    - `GpuExportKind.OPAQUE_FD`: the memory of Vulkan images, which only a
      Vulkan device or an OpenGL context with GL_EXT_memory_object_fd on the
      same GPU and driver imports. To pick the GPU, create the device with
      `GpuDeviceOptions.adapter_uuid` set to your context's
      GL_DEVICE_UUID_EXT. Get each frame's memory with
      `AcquiredFrameHandle.opaque_fd()`.

    `GpuDeviceCapabilities.export_kinds` reports the kinds that a device
    exports. Attach with `RenderDriverKind.CORE_WORKER`. Without a device,
    the session renders on the first of the platform's backends whose device
    exports one of `export_kinds`.

    Each frame hands over with `GpuSyncKind.CPU_COMPLETE` or with the sync
    kind that crosses APIs on its platform: `GpuSyncKind.METAL_SHARED_EVENT`
    for an IOSurface, and `GpuSyncKind.SYNC_FD` for the others, where the
    device has them. For a reader that waits on neither, such as a Wayland
    compositor without explicit synchronization or Flutter's
    `copyPixelBuffer`, leave them out of
    `RenderSessionAttachOptions.producer_sync_kinds`, so that each frame
    finishes before you acquire it, and release each frame only once the
    reader is done with its texture. Give the ring 3 textures, so the session
    can render while you hold two frames.

    See `mln_exported_ring_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    texture_count: int
    """How many textures the ring has, from 1 to `TEXTURE_RING_MAX_DEPTH`."""

    export_kinds: GpuExportKindFlag
    """The ways of sharing a texture that you can import."""

    dma_buf_formats: Sequence[DmaBufFormat] = ()
    """The dmabuf formats and modifiers that you import, most preferred first,
    such as those that eglQueryDmaBufModifiersEXT() reports. A ring exports
    dmabufs only of one of them. The library renders only DRM_FORMAT_ABGR8888
    and DRM_FORMAT_ARGB8888. A modifier can give a buffer more planes than
    its format has, such as a plane of compression metadata, and the library
    takes any such modifier that its GPU can render into and share. The
    library takes no implicit modifier: it skips DRM_FORMAT_MOD_INVALID, so a
    list of only that one gets no dmabuf.
    """

    android_hardware_buffer_usage: int = 0
    """More `AHARDWAREBUFFER_USAGE_*` bits that your reads of a hardware buffer
    need, such as AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
    """

    def __post_init__(self):
        object.__setattr__(self, "dma_buf_formats", _sequence(self.dma_buf_formats))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            texture_count=raw["texture_count"],
            export_kinds=GpuExportKindFlag(raw["export_kinds"]),
            dma_buf_formats=tuple(
                DmaBufFormat._from_native(item) for item in raw["dma_buf_formats"]
            ),
            android_hardware_buffer_usage=raw["android_hardware_buffer_usage"],
        )


type FeatureId = (
    FeatureIdStringValueVariant
    | FeatureIdUintValueVariant
    | FeatureIdIntValueVariant
    | FeatureIdDoubleValueVariant
)
"""The ID of a feature in a source, as the source's data gives it.

The map compares feature IDs as text. A number names the same feature as the
string of its shortest decimal form, so 7, 7.0, and "7" name one feature.

See `mln_feature_id` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
"""


@dataclass(frozen=True, slots=True)
class FeatureStateSelector:
    """The features whose state to set, read, or remove.

    See `mln_feature_state_selector` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    source_id: str

    source_layer_id: str = ""
    """The source layer, for a vector source. Empty for other sources."""

    feature_id: FeatureId | None = None
    """The feature's ID. Setting or reading state needs it. Without it,
    `MapHandle.remove_feature_state()` removes the state of every feature in
    the source layer.
    """

    state_key: str = ""
    """The one key that `MapHandle.remove_feature_state()` removes. It needs
    `feature_id`. Empty removes every key.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            source_id=raw["source_id"],
            source_layer_id=raw["source_layer_id"],
            feature_id=None
            if raw["feature_id"] is None
            else (
                _copy_variant(
                    raw["feature_id"],
                    {
                        "string_value": FeatureIdStringValueVariant,
                        "uint_value": FeatureIdUintValueVariant,
                        "int_value": FeatureIdIntValueVariant,
                        "double_value": FeatureIdDoubleValueVariant,
                    },
                    None,
                )
            ),
            state_key=raw["state_key"],
        )


@dataclass(frozen=True, slots=True)
class FrameDemand:
    """A demand for one frame.

    See `mln_frame_demand` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    flags: FrameDemandFlag = field(default_factory=lambda: FrameDemandFlag(0))

    token: int = 0
    """A value of your choice that the demand's frame result carries."""

    @classmethod
    def _from_native(cls, raw):
        return cls(flags=FrameDemandFlag(raw["flags"]), token=raw["token"])


@dataclass(frozen=True, slots=True)
class FramePacerOptions:
    """Options for creating a frame pacer, or for
    `FramePacerHandle.set_options()`.

    See `mln_frame_pacer_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    maximum_frames_per_second: int | None = None
    """The most frames per second that the pacer demands, greater than zero. The
    pacer skips display refreshes to stay within it. By default, it can
    demand frames at every refresh.
    """

    unlimited_frames_per_second: bool = False

    @classmethod
    def _from_native(cls, raw):
        return cls(
            maximum_frames_per_second=raw["maximum_frames_per_second"],
            unlimited_frames_per_second=raw["unlimited_frames_per_second"],
        )


@dataclass(frozen=True, slots=True)
class FrameProjection:
    """Where a frame drew the map, and the depth that it wrote there.

    World coordinates are Web Mercator's. x runs from 0 at 180° west to 1 at
    180° east, and on past both into the copies of the world beside the
    first. y runs from 0 at the north edge of the Mercator world, about
    85.05° north, to 1 at its south edge. z is the altitude in meters above
    sea level, at the scale of the heights of the map's 3D layers, and the
    map draws its ground at 0.

    `matrix` takes a world point to the clip coordinates where the frame drew
    it, with the camera's padding, roll, field of view, and center altitude,
    and the map's viewport and projection mode. Clip y points up, as the map
    draws with OpenGL, Metal, and WebGPU. The map negates clip y with Vulkan,
    so negate it there too. At high zooms, single precision can't tell nearby
    world points apart: subtract an origin near your geometry from its
    coordinates in double precision, and fold the origin into the matrix,
    before you convert to float.

    Clip z follows `clip_depth_range`, the convention of the device's
    graphics API. With the API's default depth range of 0 to 1, a fragment
    that you draw through `matrix` gets the depth that the map's 3D layers,
    such as fill extrusions, wrote at the same place: 0 at `near_distance`
    from the camera, up to `far_depth` at `far_distance`. Depth-tested
    against the frame's depth, your fragments hide behind the map's 3D layers
    where those stand in front of them.

    The frame's depth starts at 1. The map's 3D layers write the depth of
    each of their fragments. A 2D layer below every 3D layer of the style
    that draws opaque fragments, such as an opaque fill, writes one depth
    over all of them, wherever they are: the higher the layer, the lower its
    depth, and every one between `far_depth` and 1. Backgrounds, translucent
    fragments, and the 2D layers above a 3D layer, such as labels, write no
    depth. So what you draw through `matrix`, even on the ground, lies in
    front of every 2D layer.

    While `globe` is true, the frame drew the map onto a globe, and its 2D
    layers wrote no depth. `matrix` then is the map's Mercator one, which
    doesn't place points on the globe.

    See `mln_frame_projection` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    matrix: Mat4

    clip_depth_range: ClipDepthRange

    near_distance: float
    """The distance from the camera to the plane where the depth that you get
    through `matrix` is 0, along the camera's view. Like `far_distance`, it
    is in pixels of the map at the camera's zoom, where the world is 512 ×
    2^zoom pixels wide.
    """

    far_distance: float
    """The distance to the plane where the depth is `far_depth`."""

    far_depth: float
    """The depth at `far_distance`, below 1."""

    globe: bool
    """Whether the frame drew the map as a globe, alone or blending into
    Mercator.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            matrix=Mat4._from_native(raw["matrix"]),
            clip_depth_range=ClipDepthRange(raw["clip_depth_range"]),
            near_distance=raw["near_distance"],
            far_distance=raw["far_distance"],
            far_depth=raw["far_depth"],
            globe=raw["globe"],
        )


@dataclass(frozen=True, slots=True)
class FreeCamera:
    """A map camera's whole position and orientation in three dimensions.

    Its members mean the same as those of `FreeCameraOptions`.

    See `mln_free_camera` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    position: Vec3

    orientation: Quaternion

    @classmethod
    def _from_native(cls, raw):
        return cls(
            position=Vec3._from_native(raw["position"]),
            orientation=Quaternion._from_native(raw["orientation"]),
        )


@dataclass(frozen=True, slots=True)
class FreeCameraOptions:
    """The camera's position and orientation in three dimensions.

    See `mln_free_camera_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    position: Vec3 | None = None
    """The camera's position in Web Mercator units, where the world is 1 wide,
    (0, 0) is its northwest corner, and (1, 1) is its southeast corner. `z`
    is the altitude in the same units.
    """

    orientation: Quaternion | None = None
    """The camera's rotation from looking straight down with north at the top of
    the screen.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            position=_maybe(Vec3._from_native, raw["position"]),
            orientation=_maybe(Quaternion._from_native, raw["orientation"]),
        )


@dataclass(frozen=True, slots=True)
class GeoJsonSourceOptions:
    """Options for a GeoJSON
    \\[source\\](https://maplibre.org/maplibre-style-spec/sources/#geojson).

    Zoom levels are whole numbers.

    See `mln_geo_json_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    min_zoom: float | None = None

    max_zoom: float | None = None

    tolerance: float | None = None

    cluster_max_zoom: float | None = None
    """The last zoom level that has clusters, at most 30."""

    cluster_properties: Any = None
    """The `clusterProperties` object, or empty for none."""

    buffer: int | None = None

    cluster_radius: int | None = None

    cluster_min_points: int | None = None
    """At most 4294967295 on a 32-bit target."""

    line_metrics: bool | None = None

    cluster: bool | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            tolerance=raw["tolerance"],
            cluster_max_zoom=raw["cluster_max_zoom"],
            cluster_properties=raw["cluster_properties"],
            buffer=raw["buffer"],
            cluster_radius=raw["cluster_radius"],
            cluster_min_points=raw["cluster_min_points"],
            line_metrics=raw["line_metrics"],
            cluster=raw["cluster"],
        )


@dataclass(frozen=True, slots=True)
class GestureMomentum:
    """How a gesture keeps moving the camera after the user lets go.

    A part moves only when its velocity is set, and each part slows on its
    own. Every part defaults to `MomentumDecayModel.FRICTION` with a
    half-life of 140 ms, and these other defaults:

    - Pan stops at 10 logical pixels per second, needs a release of at least
      200, and counts a release faster than 10000 as 10000.
    - Zoom stops at 0.01 levels per second and moves at most 2.5 levels. Its
      snap settles on the nearest whole level.
    - Bearing stops at 1 degree per second and needs a release of at least
      60. Its snap settles on north within 7 degrees.
    - Pitch stops at 1 degree per second and needs a release of at least 5.
      Its snap settles flat within 7 degrees.

    See `mln_gesture_momentum` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    pan: PanMomentum = field(default_factory=lambda: PanMomentum())

    zoom: AnchoredMomentum = field(default_factory=lambda: AnchoredMomentum())

    bearing: AnchoredMomentum = field(default_factory=lambda: AnchoredMomentum())

    pitch: AnchoredMomentum = field(default_factory=lambda: AnchoredMomentum())

    @classmethod
    def _from_native(cls, raw):
        return cls(
            pan=PanMomentum._from_native(raw["pan"]),
            zoom=AnchoredMomentum._from_native(raw["zoom"]),
            bearing=AnchoredMomentum._from_native(raw["bearing"]),
            pitch=AnchoredMomentum._from_native(raw["pitch"]),
        )


@dataclass(frozen=True, slots=True)
class GpuDeviceCapabilities:
    """What a GPU device can do.

    See `mln_gpu_device_capabilities` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
    """

    backend: RenderBackend

    target_kinds: RenderTargetKindFlag
    """The kinds of `RenderTarget` that sessions on the device render to."""

    export_kinds: GpuExportKindFlag
    """The ways that the device can share the textures of an exported ring. A
    kind is here only once the device has exported a texture of it, so a
    driver that offers an extension but can't allocate the memory reports no
    kind of it.
    """

    sync_kinds: GpuSyncKindFlag
    """The kinds of `GpuSync` that the device can signal and wait on."""

    depth_stencil_formats: DepthStencilFormatFlag
    """The formats of the depth textures that a borrowed ring takes."""

    depth_export_kinds: GpuExportKindFlag
    """The kinds of `export_kinds` whose rings also share each frame's depth
    texture, with `RenderSessionAttachOptions.keep_depth`. An IOSurface or a
    dmabuf holds no depth format, so only `GpuExportKind.OPAQUE_FD` can be
    here.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            backend=RenderBackend(raw["backend"]),
            target_kinds=RenderTargetKindFlag(raw["target_kinds"]),
            export_kinds=GpuExportKindFlag(raw["export_kinds"]),
            sync_kinds=GpuSyncKindFlag(raw["sync_kinds"]),
            depth_stencil_formats=DepthStencilFormatFlag(raw["depth_stencil_formats"]),
            depth_export_kinds=GpuExportKindFlag(raw["depth_export_kinds"]),
        )


@dataclass(frozen=True, slots=True)
class GpuDeviceOptions:
    """Options for `GpuDeviceHandle.create()`.

    Name the GPU by at most one of `adapter_index`, `adapter_uuid`, and
    `adapter_luid`. Without one, the device prefers a discrete GPU.

    See `mln_gpu_device_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
    """

    backend: RenderBackend | None = None
    """The backend that the device renders with. By default, the platform's own:
    Metal on Apple platforms, Vulkan elsewhere, or OpenGL where the system
    has no Vulkan driver, and in a browser the build's one backend: WebGPU or
    OpenGL (WebGL2).
    """

    adapter_index: int | None = None
    """The GPU to use, as an index into the backend's list of GPUs."""

    adapter_uuid: Buffer | None = None
    """The 16-byte UUID of the GPU to use: Vulkan's
    `VkPhysicalDeviceIDProperties.deviceUUID`, or OpenGL's
    `GL_DEVICE_UUID_EXT`.
    """

    adapter_luid: Buffer | None = None
    """The 8-byte LUID of the GPU to use: on Windows, its DXGI adapter LUID,
    which Vulkan reports as `VkPhysicalDeviceIDProperties.deviceLUID`, and on
    Apple platforms, its `MTLDevice.registryID`, in the platform's byte
    order.
    """

    open_gl_client_api: OpenGlClientApi | None = None
    """The OpenGL API that an OpenGL device renders with. An EGL device defaults
    to `OpenGlClientApi.GLES`, and takes `OpenGlClientApi.GL` too on Linux
    and FreeBSD. A WGL device takes only `OpenGlClientApi.GL`, its default,
    and a WebGL device only `OpenGlClientApi.GLES`, its default.
    """

    open_gl_single_context: bool | None = None
    """Whether the sessions of an EGL device take turns in one OpenGL context
    instead of having one each.
    """

    queue_lock: QueueLock | NativeQueueLock = field(default_factory=lambda: QueueLock())
    """Your lock on a Vulkan device's queue, when you submit to the queue that
    `GpuDeviceHandle.get_vulkan_context()` reports. Only a Vulkan device
    takes one.
    """

    def __post_init__(self):
        object.__setattr__(self, "adapter_uuid", _bytes(self.adapter_uuid))
        object.__setattr__(self, "adapter_luid", _bytes(self.adapter_luid))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            backend=_maybe(RenderBackend, raw["backend"]),
            adapter_index=raw["adapter_index"],
            adapter_uuid=raw["adapter_uuid"],
            adapter_luid=raw["adapter_luid"],
            open_gl_client_api=_maybe(OpenGlClientApi, raw["open_gl_client_api"]),
            open_gl_single_context=raw["open_gl_single_context"],
            queue_lock=QueueLock(),
        )


@dataclass(frozen=True, slots=True)
class GpuDeviceVulkanContextResult:
    """The Vulkan objects of a device, as `GpuDeviceHandle.get_vulkan_context()`
    reports them.

    See `mln_gpu_device_vulkan_context_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
    """

    context: VulkanContextDescriptor

    @classmethod
    def _from_native(cls, raw):
        return cls(context=VulkanContextDescriptor._from_native(raw["context"]))


@dataclass(frozen=True, slots=True)
class GpuSync:
    """A point in GPU work that the other side waits for before using a texture.

    See `mln_gpu_sync` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    kind: GpuSyncKind = GpuSyncKind.CPU_COMPLETE

    handle: int = 0
    """The object to wait on, for the kinds that name one."""

    value: int = 0

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=GpuSyncKind(raw["kind"]), handle=raw["handle"], value=raw["value"]
        )


@dataclass(frozen=True, slots=True)
class HttpHeader:
    """An HTTP header.

    See `mln_http_header` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    name: str

    value: str = ""

    @classmethod
    def _from_native(cls, raw):
        return cls(name=raw["name"], value=raw["value"])


@dataclass(frozen=True, slots=True)
class HttpHeaderRule:
    """Headers to add to the HTTP requests whose URL matches a pattern.

    See `mln_http_header_rule` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    kinds: ResourceKindFlag = field(default_factory=lambda: ResourceKindFlag(0))
    """The kinds of resource that the rule covers. Zero covers every kind."""

    flags: UrlMatchFlag = field(default_factory=lambda: UrlMatchFlag(0))

    url: str = ""

    headers: Sequence[HttpHeader] = ()

    def __post_init__(self):
        object.__setattr__(self, "headers", _sequence(self.headers))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kinds=ResourceKindFlag(raw["kinds"]),
            flags=UrlMatchFlag(raw["flags"]),
            url=raw["url"],
            headers=tuple(HttpHeader._from_native(item) for item in raw["headers"]),
        )


@dataclass(frozen=True, slots=True)
class HttpHeaderTransform:
    """Headers to add to HTTP requests, for
    `RuntimeHandle.set_http_header_transform()`.

    See `mln_http_header_transform` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    rules: Sequence[HttpHeaderRule] = ()
    """The first rule that matches a request adds its headers, and no other."""

    def __post_init__(self):
        object.__setattr__(self, "rules", _sequence(self.rules))


@dataclass(frozen=True, slots=True)
class ImageContent:
    """The area of an image, in image pixels, that text fits inside.

    See `mln_image_content` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    left: float = 0.0

    top: float = 0.0

    right: float = 0.0

    bottom: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(
            left=raw["left"], top=raw["top"], right=raw["right"], bottom=raw["bottom"]
        )


@dataclass(frozen=True, slots=True)
class ImageStretch:
    """A range of image pixels, from `from` to `to`, that stretches to fit text.

    See `mln_image_stretch` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    from_: float = 0.0

    to: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(from_=raw["from_"], to=raw["to"])


@dataclass(frozen=True, slots=True)
class IoSurfaceFrame:
    """The IOSurface of an exported ring's acquired frame.

    See `mln_io_surface_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The surface's index in the ring."""

    surface: int
    """The `IOSurfaceRef`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            surface=raw["surface"],
        )


@dataclass(frozen=True, slots=True)
class LatLng:
    """A geographic coordinate, in degrees.

    See `mln_lat_lng` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    latitude: float = 0.0

    longitude: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(latitude=raw["latitude"], longitude=raw["longitude"])


@dataclass(frozen=True, slots=True)
class LatLngBounds:
    """A rectangle of coordinates, from its southwest to its northeast corner.

    See `mln_lat_lng_bounds` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    southwest: LatLng = field(default_factory=lambda: LatLng())

    northeast: LatLng = field(default_factory=lambda: LatLng())

    @classmethod
    def _from_native(cls, raw):
        return cls(
            southwest=LatLng._from_native(raw["southwest"]),
            northeast=LatLng._from_native(raw["northeast"]),
        )


@dataclass(frozen=True, slots=True)
class LogHandler:
    """A log callback, for `set_log_callback()`.

    The callback runs on the binding's dispatcher thread, after native queues
    the call.

    See `mln_log_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    callback: LogCallback
    platform_severities: LogSeverityFlag = field(
        default_factory=lambda: LogSeverityFlag(0)
    )
    """The severities that also go to the platform's log, as they do without a
    callback. 0 sends none. A message of these severities goes to the
    platform's log even when the handler's queue can't accept it.
    """

    def _invoke_callback(self, severity, event, code, message):
        callback = self.callback
        return callback(LogSeverity(severity), LogEvent(event), code, message)


@dataclass(frozen=True, slots=True)
class LogicalExtent:
    """A size in logical pixels, and the physical pixels in each logical pixel.

    See `mln_logical_extent` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    width: int = 0

    height: int = 0

    scale_factor: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"], height=raw["height"], scale_factor=raw["scale_factor"]
        )


@dataclass(frozen=True, slots=True)
class LogicalSize:
    """A size in logical pixels, which the map's scale factor turns into pixels.

    See `mln_logical_size` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    width: int = 0

    height: int = 0

    @classmethod
    def _from_native(cls, raw):
        return cls(width=raw["width"], height=raw["height"])


@dataclass(frozen=True, slots=True)
class MapOptions:
    """Options for `RuntimeHandle.create_map()`.

    See `mln_map_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    initial_extent: LogicalExtent
    """The map's first size, and its scale factor. Render sessions of the map
    render at its scale factor. `MapHandle.resize()` changes both, or
    `RenderSessionHandle.resize()` while a render session is attached.
    """

    map_mode: MapMode = MapMode.CONTINUOUS

    fast_pfor_enabled: bool = False
    """Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
    encoding. Without it, such tiles fail to load.
    """

    event_mask: RuntimeEventFlag | None = None
    """The events that the map reports. Without it, the map reports them all."""

    tile_cache_enabled: bool | None = None
    """Whether the map keeps tiles in memory after they leave the view, so they
    show again without loading. Without it, the map keeps them.
    """

    fallback_font_faces: str = ""
    """Fonts for the text that a style's own fonts don't draw, such as scripts
    that need complex shaping, like Myanmar or Devanagari. A JSON array of
    font files, each an object with a `url` and an optional `unicode-range`,
    as in a font face of a style's `font-faces`, which every font stack uses
    after the style's own font faces. Empty for none.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            initial_extent=LogicalExtent._from_native(raw["initial_extent"]),
            map_mode=MapMode(raw["map_mode"]),
            fast_pfor_enabled=raw["fast_pfor_enabled"],
            event_mask=_maybe(RuntimeEventFlag, raw["event_mask"]),
            tile_cache_enabled=raw["tile_cache_enabled"],
            fallback_font_faces=raw["fallback_font_faces"],
        )


@dataclass(frozen=True, slots=True)
class MapProjectionCameraResult:
    """A projection's camera, as `MapProjectionHandle.get_camera()` reports it.

    See `mln_map_projection_camera_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    """

    camera: Camera

    @classmethod
    def _from_native(cls, raw):
        return cls(camera=Camera._from_native(raw["camera"]))


@dataclass(frozen=True, slots=True)
class MapSnapshot:
    """A map's state, as `MapHandle.get_snapshot()` reads it.

    See `mln_map_snapshot` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    debug_flags: MapDebugFlag

    generation: int
    """Grows with each change to the map's state."""

    fully_loaded: bool
    """Whether the style and every resource that the view needs have loaded."""

    rendering_stats_view_enabled: bool

    gesture_in_progress: bool

    transition_in_progress: bool

    event_mask: RuntimeEventFlag

    latest_render_update_generation: int
    """Counts the render updates that the map has made for its render session."""

    tile: MapTileSettings

    bounds: CameraBounds

    free_camera: FreeCamera

    view: MapView

    globe_rendering: bool
    """Whether the map draws as a globe, alone or blending into Mercator."""

    globe_free_bearing_enabled: bool
    """The setting from `MapHandle.set_globe_free_bearing_enabled()`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            debug_flags=MapDebugFlag(raw["debug_flags"]),
            generation=raw["generation"],
            fully_loaded=raw["fully_loaded"],
            rendering_stats_view_enabled=raw["rendering_stats_view_enabled"],
            gesture_in_progress=raw["gesture_in_progress"],
            transition_in_progress=raw["transition_in_progress"],
            event_mask=RuntimeEventFlag(raw["event_mask"]),
            latest_render_update_generation=raw["latest_render_update_generation"],
            tile=MapTileSettings._from_native(raw["tile"]),
            bounds=CameraBounds._from_native(raw["bounds"]),
            free_camera=FreeCamera._from_native(raw["free_camera"]),
            view=MapView._from_native(raw["view"]),
            globe_rendering=raw["globe_rendering"],
            globe_free_bearing_enabled=raw["globe_free_bearing_enabled"],
        )


@dataclass(frozen=True, slots=True)
class MapTileOptions:
    """How the map picks the tiles to load.

    The `lod_` members lower the level of detail away from the camera, so the
    map loads fewer tiles when the pitch is high.

    See `mln_map_tile_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    prefetch_zoom_delta: int | None = None
    """How many zoom levels lower the map also loads tiles for, so it can show a
    coarse map while the current tiles load. 0 loads only the current tiles.
    """

    lod_min_radius: float | None = None
    """The radius around the view point, in tiles, that keeps full detail."""

    lod_scale: float | None = None
    """Scales the distance to the view point. Above 1 lowers the detail."""

    lod_pitch_threshold: float | None = None
    """The pitch, in degrees, above which the map lowers the detail."""

    lod_zoom_shift: float | None = None
    """Zoom levels to add for picking the detail. Below 0 loads fewer tiles."""

    lod_mode: TileLodMode | None = None
    """`TileLodMode.DISTANCE` ignores `lod_min_radius`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            prefetch_zoom_delta=raw["prefetch_zoom_delta"],
            lod_min_radius=raw["lod_min_radius"],
            lod_scale=raw["lod_scale"],
            lod_pitch_threshold=raw["lod_pitch_threshold"],
            lod_zoom_shift=raw["lod_zoom_shift"],
            lod_mode=_maybe(TileLodMode, raw["lod_mode"]),
        )


@dataclass(frozen=True, slots=True)
class MapTileSettings:
    """All the settings for how a map picks the tiles to load.

    Its members mean the same as those of `MapTileOptions`.

    See `mln_map_tile_settings` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    prefetch_zoom_delta: int

    lod_min_radius: float

    lod_scale: float

    lod_pitch_threshold: float
    """The pitch, in degrees, above which the map lowers the detail."""

    lod_zoom_shift: float

    lod_mode: TileLodMode

    @classmethod
    def _from_native(cls, raw):
        return cls(
            prefetch_zoom_delta=raw["prefetch_zoom_delta"],
            lod_min_radius=raw["lod_min_radius"],
            lod_scale=raw["lod_scale"],
            lod_pitch_threshold=raw["lod_pitch_threshold"],
            lod_zoom_shift=raw["lod_zoom_shift"],
            lod_mode=TileLodMode(raw["lod_mode"]),
        )


@dataclass(frozen=True, slots=True)
class MapView:
    """One view of a map: its camera, size, and projection.

    See `mln_map_view` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    camera: Camera = field(default_factory=lambda: Camera())

    extent: LogicalExtent = field(default_factory=lambda: LogicalExtent())

    projection_mode: ProjectionMode = field(default_factory=lambda: ProjectionMode())

    viewport: MapViewport = field(default_factory=lambda: MapViewport())

    projection: ProjectionDefinition = field(
        default_factory=lambda: ProjectionDefinition()
    )
    """The style's projection at the camera's zoom."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            camera=Camera._from_native(raw["camera"]),
            extent=LogicalExtent._from_native(raw["extent"]),
            projection_mode=ProjectionMode._from_native(raw["projection_mode"]),
            viewport=MapViewport._from_native(raw["viewport"]),
            projection=ProjectionDefinition._from_native(raw["projection"]),
        )


@dataclass(frozen=True, slots=True)
class MapViewport:
    """A map's whole viewport.

    Its members mean the same as those of `MapViewportOptions`.

    See `mln_map_viewport` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    north_orientation: NorthOrientation = NorthOrientation.UP

    constrain_mode: ConstrainMode = ConstrainMode.NONE

    viewport_mode: ViewportMode = ViewportMode.DEFAULT

    frustum_offset: EdgeInsets = field(default_factory=lambda: EdgeInsets())

    @classmethod
    def _from_native(cls, raw):
        return cls(
            north_orientation=NorthOrientation(raw["north_orientation"]),
            constrain_mode=ConstrainMode(raw["constrain_mode"]),
            viewport_mode=ViewportMode(raw["viewport_mode"]),
            frustum_offset=EdgeInsets._from_native(raw["frustum_offset"]),
        )


@dataclass(frozen=True, slots=True)
class MapViewportOptions:
    """How the map orients and constrains its view.

    See `mln_map_viewport_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    north_orientation: NorthOrientation | None = None

    constrain_mode: ConstrainMode | None = None

    viewport_mode: ViewportMode | None = None

    frustum_offset: EdgeInsets | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            north_orientation=_maybe(NorthOrientation, raw["north_orientation"]),
            constrain_mode=_maybe(ConstrainMode, raw["constrain_mode"]),
            viewport_mode=_maybe(ViewportMode, raw["viewport_mode"]),
            frustum_offset=_maybe(EdgeInsets._from_native, raw["frustum_offset"]),
        )


@dataclass(frozen=True, slots=True)
class Mat4:
    """A 4×4 matrix, by its columns: it takes a vector to `x` times the vector's
    x, plus `y` times its y, `z` times its z, and `w` times its w. In memory
    it is 16 doubles in column-major order, as OpenGL, Metal, Vulkan, and
    WebGPU shaders take a matrix.

    See `mln_mat4` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: Vec4

    y: Vec4

    z: Vec4

    w: Vec4

    @classmethod
    def _from_native(cls, raw):
        return cls(
            x=Vec4._from_native(raw["x"]),
            y=Vec4._from_native(raw["y"]),
            z=Vec4._from_native(raw["z"]),
            w=Vec4._from_native(raw["w"]),
        )


@dataclass(frozen=True, slots=True)
class MetalBorrowedTexture:
    """A Metal texture of a borrowed ring.

    See `mln_metal_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    texture: int
    """Your `MTLTexture`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"])


@dataclass(frozen=True, slots=True)
class MetalBorrowedTexturesTarget:
    """A ring of your Metal textures to render into, on the Metal device that
    you attach on.

    `physical_width` and `physical_height` give the textures' size in pixels,
    which is usually `logical_size` times the map's scale factor. The
    textures must all have that size, the device's `MTLDevice`, one pixel
    format, one sample, and `MTLTextureUsageRenderTarget`. The ring holds up
    to `TEXTURE_RING_MAX_DEPTH` distinct textures.

    Give the ring `depth_textures` to draw into each frame depth-tested
    against the map, as `FrameProjection` says: one for each of `textures`,
    at the same index, or none. Each frame clears the one at its slot,
    renders the map's depth and stencil into it, and stores them. They must
    have the size of `textures`, one pixel format of
    `GpuDeviceCapabilities.depth_stencil_formats`, one sample,
    `MTLTextureUsageRenderTarget`, and a storage mode other than
    `MTLStorageModeMemoryless`.

    See `mln_metal_borrowed_textures_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    physical_width: int

    physical_height: int

    textures: Sequence[MetalBorrowedTexture]

    depth_textures: Sequence[MetalBorrowedTexture] = ()

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))
        object.__setattr__(self, "depth_textures", _sequence(self.depth_textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            textures=tuple(
                MetalBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            depth_textures=tuple(
                MetalBorrowedTexture._from_native(item)
                for item in raw["depth_textures"]
            ),
        )


@dataclass(frozen=True, slots=True)
class MetalDeviceDescriptor:
    """Your Metal device and command queue, for `GpuDeviceHandle.wrap_metal()`.

    See `mln_metal_device_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
    """

    device: int
    """Your `MTLDevice`."""

    command_queue: int = 0
    """Your `MTLCommandQueue` on `device`, or null for a queue of the library's.
    With your queue, borrowed rings can use `GpuSyncKind.QUEUE_ORDER`.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(device=raw["device"], command_queue=raw["command_queue"])


@dataclass(frozen=True, slots=True)
class MetalLayerTarget:
    """A `CAMetalLayer` to present to.

    The session sets the layer's device, pixel format, and drawable size.

    See `mln_metal_layer_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    layer: int
    """Your `CAMetalLayer*`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            layer=raw["layer"],
        )


@dataclass(frozen=True, slots=True)
class MetalTextureFrame:
    """The Metal texture of an acquired frame.

    The depth members are set when the frame has a depth texture: one of a
    borrowed ring's `depth_textures`, or an exported ring's with
    `RenderSessionAttachOptions.keep_depth`. Otherwise they are zero.

    See `mln_metal_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The texture's index in the descriptor's `textures`."""

    texture: int
    """The MTLTexture."""

    device: int
    """The texture's MTLDevice."""

    pixel_format: int
    """The texture's MTLPixelFormat."""

    depth_texture: int
    """The MTLTexture that holds the frame's depth and stencil."""

    depth_pixel_format: int
    """The depth texture's MTLPixelFormat."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            texture=raw["texture"],
            device=raw["device"],
            pixel_format=raw["pixel_format"],
            depth_texture=raw["depth_texture"],
            depth_pixel_format=raw["depth_pixel_format"],
        )


@dataclass(frozen=True, slots=True)
class MomentumDecay:
    """How one part of a gesture's momentum slows from its release speed.

    Speeds and distances are in the part's unit: logical pixels for pan, zoom
    levels for zoom, and degrees for bearing and pitch. A setting whose bit
    isn't set takes the part's default, which `GestureMomentum` lists.
    `min_speed` is finite and not negative, and every other number is finite
    and positive. A friction decay can serve every release, as a preset that
    matches a platform's scrolling. An eased glide takes its direction from
    the part's velocity, and its distance and duration from the caller's
    plan.

    See `mln_momentum_decay` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    model: MomentumDecayModel | None = None

    half_life_ms: float | None = None
    """For `MomentumDecayModel.FRICTION`, how long the speed takes to halve."""

    stop_speed: float | None = None
    """For `MomentumDecayModel.FRICTION`, the speed at which it ends."""

    distance: float | None = None
    """Required for `MomentumDecayModel.EASE`."""

    duration_ms: float | None = None
    """Required for `MomentumDecayModel.EASE`."""

    easing: UnitBezier | None = None
    """For `MomentumDecayModel.EASE`, the share of `distance` covered over the
    share of `duration_ms`. Its control points are between 0 and 1, so the
    glide never passes its distance. Defaults to (0, 0, 0.25, 1), the curve
    that camera animations ease along by default. Android's scroll fling
    follows (0.175, 0.5, 0.35, 1).
    """

    min_speed: float | None = None
    """A slower release starts no momentum."""

    max_speed: float | None = None
    """For `MomentumDecayModel.FRICTION`, a faster release counts as this."""

    max_distance: float | None = None
    """The farthest the part moves. A friction release that would move farther
    starts slower, so it ends sooner, and a planned glide moves at most this
    far over its duration.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            model=_maybe(MomentumDecayModel, raw["model"]),
            half_life_ms=raw["half_life_ms"],
            stop_speed=raw["stop_speed"],
            distance=raw["distance"],
            duration_ms=raw["duration_ms"],
            easing=_maybe(UnitBezier._from_native, raw["easing"]),
            min_speed=raw["min_speed"],
            max_speed=raw["max_speed"],
            max_distance=raw["max_distance"],
        )


@dataclass(frozen=True, slots=True)
class MomentumSnap:
    """Where a part settles after a gesture that changed it.

    The targets are every `spacing` from `offset`, in the part's unit.
    Bearing targets wrap around the circle. Once the part's glide ends, or
    right away when it doesn't move, the part eases to the nearest target if
    it is within `tolerance` of it, and otherwise stays where it is. A
    setting whose bit isn't set takes the part's default, which
    `GestureMomentum` lists.

    See `mln_momentum_snap` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    spacing: float | None = None
    """Positive."""

    offset: float | None = None
    """Defaults to 0."""

    tolerance: float | None = None
    """Not negative."""

    duration_ms: float | None = None
    """Zero settles without easing. Defaults to 300."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            spacing=raw["spacing"],
            offset=raw["offset"],
            tolerance=raw["tolerance"],
            duration_ms=raw["duration_ms"],
        )


@dataclass(frozen=True, slots=True)
class OfflineGeometryRegionDefinition:
    """An offline region that covers a GeoJSON geometry.

    The members other than `geometry` mean the same as those of
    `OfflineTilePyramidRegionDefinition`.

    See `mln_offline_geometry_region_definition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    geometry: Any

    style_url: str = ""

    min_zoom: float = 0.0

    max_zoom: float = 0.0

    pixel_ratio: float = 0.0

    include_ideographs: bool = False

    @classmethod
    def _from_native(cls, raw):
        return cls(
            style_url=raw["style_url"],
            geometry=raw["geometry"],
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            pixel_ratio=raw["pixel_ratio"],
            include_ideographs=raw["include_ideographs"],
        )


type OfflineRegionDefinition = (
    OfflineRegionDefinitionTilePyramidVariant
    | OfflineRegionDefinitionGeometryVariant
    | UnknownVariant
)
"""The area, zoom range, and style that an offline region covers.

See `mln_offline_region_definition` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
"""


@dataclass(frozen=True, slots=True)
class OfflineRegionInfo:
    """An offline region in the runtime's cache file.

    See `mln_offline_region_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    id: int

    definition: OfflineRegionDefinition

    metadata: bytes

    @classmethod
    def _from_native(cls, raw):
        return cls(
            id=raw["id"],
            definition=_copy_variant(
                raw["definition"],
                {
                    "tile_pyramid": OfflineRegionDefinitionTilePyramidVariant,
                    "geometry": OfflineRegionDefinitionGeometryVariant,
                },
                None,
            ),
            metadata=raw["metadata"],
        )


@dataclass(frozen=True, slots=True)
class OfflineRegionStatus:
    """The download progress of an offline region.

    See `mln_offline_region_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    download_state: OfflineRegionDownloadState

    completed_resource_count: int
    """The resources downloaded so far, tiles included."""

    completed_resource_size: int
    """The size of the downloaded resources in bytes, tiles included."""

    completed_tile_count: int

    required_tile_count: int

    completed_tile_size: int
    """The size of the downloaded tiles in bytes."""

    required_resource_count: int
    """The resources that the region needs, tiles included. Until
    `required_resource_count_is_precise` is true, this is only a lower bound.
    """

    required_resource_count_is_precise: bool

    complete: bool
    """Whether the region has downloaded every resource it needs."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            download_state=OfflineRegionDownloadState(raw["download_state"]),
            completed_resource_count=raw["completed_resource_count"],
            completed_resource_size=raw["completed_resource_size"],
            completed_tile_count=raw["completed_tile_count"],
            required_tile_count=raw["required_tile_count"],
            completed_tile_size=raw["completed_tile_size"],
            required_resource_count=raw["required_resource_count"],
            required_resource_count_is_precise=raw[
                "required_resource_count_is_precise"
            ],
            complete=raw["complete"],
        )


@dataclass(frozen=True, slots=True)
class OfflineTilePyramidRegionDefinition:
    """An offline region that covers a rectangle of the map.

    See `mln_offline_tile_pyramid_region_definition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    style_url: str = ""
    """The style whose resources the region holds. Maps use the region when they
    load the same style URL.
    """

    bounds: LatLngBounds = field(default_factory=lambda: LatLngBounds())

    min_zoom: float = 0.0

    max_zoom: float = 0.0
    """INFINITY covers every zoom level that the style's sources have."""

    pixel_ratio: float = 0.0
    """The pixel ratio of the images to download, such as 2 for dense screens."""

    include_ideographs: bool = False
    """Whether the region holds the glyphs of CJK ideographs."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            style_url=raw["style_url"],
            bounds=LatLngBounds._from_native(raw["bounds"]),
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            pixel_ratio=raw["pixel_ratio"],
            include_ideographs=raw["include_ideographs"],
        )


@dataclass(frozen=True, slots=True)
class OffscreenTarget:
    """An image that the session allocates, renders at the map's scale factor,
    and reads back with `RenderSessionHandle.read_texture()`.

    See `mln_offscreen_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    @classmethod
    def _from_native(cls, raw):
        return cls(logical_size=LogicalSize._from_native(raw["logical_size"]))


@dataclass(frozen=True, slots=True)
class OhosWindowTarget:
    """An OpenHarmony window to present to.

    The session takes its own reference to the window, so you can release
    yours once the call returns. When the platform destroys the window, such
    as in an `XComponent`'s `onSurfaceDestroyed` callback, release the target
    or give the session another window without waiting, as for an Android
    window.

    See `mln_ohos_window_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    window: int
    """Your `OHNativeWindow*`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            window=raw["window"],
        )


@dataclass(frozen=True, slots=True)
class OpaqueFdFrame:
    """The memory of an exported ring's acquired frame, as an opaque file
    descriptor.

    The memory holds a 2D Vulkan image of `format`, with one mip level, one
    layer, optimal tiling, and the usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_SAMPLED_BIT, and VK_IMAGE_USAGE_TRANSFER_SRC_BIT, which a
    Vulkan image that imports it has too. In OpenGL, import it with
    glImportMemoryFdEXT() and GL_HANDLE_TYPE_OPAQUE_FD_EXT, and make the
    texture with GL_TEXTURE_TILING_EXT set to GL_OPTIMAL_TILING_EXT and
    glTexStorageMem2DEXT() at offset 0. Each frame leaves the image in
    VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL.

    With `RenderSessionAttachOptions.keep_depth`, on a device whose
    `GpuDeviceCapabilities.depth_export_kinds` has `GpuExportKind.OPAQUE_FD`,
    the depth members share the memory of the frame's depth image in the same
    way. Otherwise `depth_fd` is -1 and the others are zero. The depth image
    is 2D, of `depth_format`, with one mip level, one layer, optimal tiling,
    and the usage VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_SAMPLED_BIT, and VK_IMAGE_USAGE_TRANSFER_SRC_BIT. Each
    frame leaves it in VK_IMAGE_LAYOUT_GENERAL, released to
    VK_QUEUE_FAMILY_EXTERNAL. In OpenGL, make its texture with the internal
    format that `DepthStencilFormatFlag` names for `depth_format`.

    See `mln_opaque_fd_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The texture's index in the ring."""

    fd: int
    """The file descriptor of the image's memory."""

    allocation_size: int
    """The size of the memory in bytes."""

    dedicated: bool
    """Whether the memory is the image's own, which an import says, such as with
    GL_DEDICATED_MEMORY_OBJECT_EXT.
    """

    format: int
    """The image's `VkFormat`: VK_FORMAT_R8G8B8A8_UNORM, OpenGL's GL_RGBA8."""

    depth_fd: int
    """The file descriptor of the depth image's memory."""

    depth_allocation_size: int
    """The size of the depth image's memory in bytes."""

    depth_dedicated: bool
    """Whether the memory is the depth image's own."""

    depth_format: int
    """The depth image's `VkFormat`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            fd=raw["fd"],
            allocation_size=raw["allocation_size"],
            dedicated=raw["dedicated"],
            format=raw["format"],
            depth_fd=raw["depth_fd"],
            depth_allocation_size=raw["depth_allocation_size"],
            depth_dedicated=raw["depth_dedicated"],
            depth_format=raw["depth_format"],
        )


@dataclass(frozen=True, slots=True)
class OpenGlBorrowedTexture:
    """An OpenGL texture of a borrowed ring.

    See `mln_open_gl_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    texture: int
    """Your texture name."""

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"])


@dataclass(frozen=True, slots=True)
class OpenGlBorrowedTexturesTarget:
    """A ring of your OpenGL textures to render into, on an OpenGL device that
    `GpuDeviceHandle.wrap_open_gl()` made from your context.

    `physical_width` and `physical_height` give the textures' size in pixels,
    which is usually `logical_size` times the map's scale factor. Each
    texture must be in your context's share group, with RGBA8 storage of that
    size. The ring holds up to `TEXTURE_RING_MAX_DEPTH` distinct textures.

    Give the ring `depth_textures` to draw into each frame depth-tested
    against the map, as `FrameProjection` says: one for each of `textures`,
    at the same index, or none. Each frame clears the one at its slot, and
    renders the map's depth and stencil into it. They must be `GL_TEXTURE_2D`
    textures in the share group, with storage of the size of `textures` in a
    format of `GpuDeviceCapabilities.depth_stencil_formats`.

    See `mln_open_gl_borrowed_textures_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    physical_width: int

    physical_height: int

    textures: Sequence[OpenGlBorrowedTexture]

    target: int
    """The textures' target. It must be `GL_TEXTURE_2D`."""

    depth_textures: Sequence[OpenGlBorrowedTexture] = ()

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))
        object.__setattr__(self, "depth_textures", _sequence(self.depth_textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            textures=tuple(
                OpenGlBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            target=raw["target"],
            depth_textures=tuple(
                OpenGlBorrowedTexture._from_native(item)
                for item in raw["depth_textures"]
            ),
        )


type OpenGlContextDescriptor = (
    OpenGlContextDescriptorWglVariant
    | OpenGlContextDescriptorEglVariant
    | OpenGlContextDescriptorWebGlVariant
)
"""Your OpenGL context, for `GpuDeviceHandle.wrap_open_gl()`: the root of the
share group that the device's sessions join.

See `mln_open_gl_context_descriptor` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
"""


@dataclass(frozen=True, slots=True)
class OpenGlSurfaceTarget:
    """Your OpenGL surface to present to, with the context that
    `GpuDeviceHandle.wrap_open_gl()` wrapped.

    The session makes its context current on the surface for each frame, and
    swaps the surface's buffers to present.

    See `mln_open_gl_surface_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    surface: int
    """Your `EGLSurface` on the context's display, with its config, or your
    `HDC` with the context's pixel format.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            surface=raw["surface"],
        )


@dataclass(frozen=True, slots=True)
class OpenGlTextureFrame:
    """The OpenGL texture of an acquired frame.

    The depth members are set when the frame has a depth texture, as for
    `MetalTextureFrame`. Otherwise they are zero.

    See `mln_open_gl_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The texture's index in the descriptor's `textures`."""

    texture: int

    target: int

    depth_texture: int
    """The `GL_TEXTURE_2D` texture that holds the frame's depth and stencil."""

    depth_internal_format: int
    """The depth texture's internal format, such as GL_DEPTH24_STENCIL8, or zero
    for a borrowed ring's.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            texture=raw["texture"],
            target=raw["target"],
            depth_texture=raw["depth_texture"],
            depth_internal_format=raw["depth_internal_format"],
        )


@dataclass(frozen=True, slots=True)
class PanMomentum:
    """How a gesture's pan keeps moving the map's content after release.

    See `mln_pan_momentum` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    velocity: ScreenPoint | None = None
    """How fast the map's content moved on the screen at release, in logical
    pixels per second. The pan moves only when this is set.
    """

    decay: MomentumDecay = field(default_factory=lambda: MomentumDecay())

    @classmethod
    def _from_native(cls, raw):
        return cls(
            velocity=_maybe(ScreenPoint._from_native, raw["velocity"]),
            decay=MomentumDecay._from_native(raw["decay"]),
        )


@dataclass(frozen=True, slots=True)
class PremultipliedRgba8Image:
    """An image of 8-bit RGBA pixels whose colors are multiplied by their alpha.

    See `mln_premultiplied_rgba8_image` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    width: int

    height: int

    stride: int
    """The number of bytes from the start of one row to the start of the next,
    at least `width` \\* 4.
    """

    pixels: Buffer
    """The rows of pixels, `stride` bytes apart, from the top row down."""

    def __post_init__(self):
        object.__setattr__(self, "pixels", _bytes(self.pixels))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            stride=raw["stride"],
            pixels=raw["pixels"],
        )


@dataclass(frozen=True, slots=True)
class ProjectedMeters:
    """A point in the Web Mercator projection, in meters.

    See `mln_projected_meters` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    northing: float = 0.0

    easting: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(northing=raw["northing"], easting=raw["easting"])


@dataclass(frozen=True, slots=True)
class ProjectionDefinition:
    """A style's projection at one zoom: `from` turning into `to`.

    A zeroed definition is Mercator.

    See `mln_projection_definition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    from_: ProjectionType = ProjectionType.MERCATOR

    to: ProjectionType = ProjectionType.MERCATOR

    transition: float = 0.0
    """How far `from` has turned into `to`, from 0 to 1. Ignored when `from`
    equals `to`. A value outside that range holds at the nearer end.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            from_=ProjectionType(raw["from_"]),
            to=ProjectionType(raw["to"]),
            transition=raw["transition"],
        )


@dataclass(frozen=True, slots=True)
class ProjectionMode:
    """A map's whole projection mode.

    Its members mean the same as those of `ProjectionModeOptions`.

    See `mln_projection_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    axonometric: bool = False

    x_skew: float = 0.0

    y_skew: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(
            axonometric=raw["axonometric"], x_skew=raw["x_skew"], y_skew=raw["y_skew"]
        )


@dataclass(frozen=True, slots=True)
class ProjectionModeOptions:
    """Settings that skew the map or draw it without perspective.

    See `mln_projection_mode_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    axonometric: bool | None = None
    """Whether the map draws without perspective."""

    x_skew: float | None = None

    y_skew: float | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            axonometric=raw["axonometric"], x_skew=raw["x_skew"], y_skew=raw["y_skew"]
        )


@dataclass(frozen=True, slots=True)
class Quaternion:
    """A rotation, as a quaternion.

    See `mln_quaternion` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: float = 0.0

    y: float = 0.0

    z: float = 0.0

    w: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"], z=raw["z"], w=raw["w"])


@dataclass(frozen=True, slots=True)
class QueueLock:
    """Your lock on a Vulkan queue that you and the render sessions of a GPU
    device all submit to.

    A session holds the lock only while it submits or presents to the queue,
    or submits the empty batch that drains its work from the queue. It never
    holds the lock while it waits. Set both `lock` and `unlock`, or neither
    when you don't submit to the queue yourself.

    Don't hold the lock while you call the library.

    It can run on a library thread, and it can't call the library.

    See `mln_queue_lock` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    @classmethod
    def native(
        cls, lock: int, unlock: int, user_data: int = 0, release_user_data: int = 0
    ) -> NativeQueueLock:
        """A native registration, whose function pointers, context, and release
        the host's native code supplies as addresses. Zero is null. The
        binding copies each address unchanged, and native code validates
        them.
        """
        from . import _native

        return NativeQueueLock._from_registration(
            _native._NativeRegistration(
                "mln_queue_lock", (lock, unlock), user_data, release_user_data
            )
        )


class NativeQueueLock:
    """A native `QueueLock` from `QueueLock.native`, accepted wherever a
    `QueueLock` is.

    The first call that carries it consumes it. Native code releases the
    context of a call that it accepts, and the binding releases the context
    of a call that native code does not accept. A second call raises an
    invalid-state error.
    """

    __slots__ = ("_registration",)

    def __init__(self):
        raise TypeError("native registrations come from QueueLock.native")

    @classmethod
    def _from_registration(cls, registration):
        value = cls.__new__(cls)
        value._registration = registration
        return value


@dataclass(frozen=True, slots=True)
class RenderAbandonResult:
    """The outcome of `RenderSessionHandle.abandon()`.

    See `mln_render_abandon_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    disposition: RenderAbandonDisposition

    quarantined_resource_count: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            disposition=RenderAbandonDisposition(raw["disposition"]),
            quarantined_resource_count=raw["quarantined_resource_count"],
        )


@dataclass(frozen=True, slots=True)
class RenderFrameResult:
    """How one frame demand finished.

    See `mln_render_frame_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    disposition: RenderResult

    token: int

    map_update_generation: int
    """The map update that a rendered frame shows. The frame includes a
    command's change when this is at least the command's
    `CommandCompletion.render_update_generation`.
    """

    generation: int

    frame_generation: int

    needs_repaint: bool

    view: MapView
    """The camera and extent that a rendered frame shows."""

    frame_projection: FrameProjection
    """Where a rendered frame drew the map, for drawing into its frame with
    depth. All zero for a demand that rendered nothing.
    """

    mode: RenderMode

    @classmethod
    def _from_native(cls, raw):
        return cls(
            disposition=RenderResult(raw["disposition"]),
            token=raw["token"],
            map_update_generation=raw["map_update_generation"],
            generation=raw["generation"],
            frame_generation=raw["frame_generation"],
            needs_repaint=raw["needs_repaint"],
            view=MapView._from_native(raw["view"]),
            frame_projection=FrameProjection._from_native(raw["frame_projection"]),
            mode=RenderMode(raw["mode"]),
        )


@dataclass(frozen=True, slots=True)
class RenderSessionAttachOptions:
    """Options for attaching a render session to a map.

    See `mln_render_session_attach_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    driver: RenderDriverKind
    """The thread that runs the session's graphics work, which depends on the
    target:

    - `RenderDriverKind.CALLER_GRAPHICS_THREAD` for a borrowed ring of WebGPU
      textures.
    - `RenderDriverKind.CORE_WORKER` for an offscreen target, a window, or an
      exported ring.
    - Either one for a web canvas. In a browser both run on the thread that
      loaded the library: `RenderDriverKind.CORE_WORKER` in the library's own
      tasks, and `RenderDriverKind.CALLER_GRAPHICS_THREAD` when you service
      the session.
    - Either one for a Metal layer, a Vulkan surface, an OpenGL surface, or a
      borrowed ring of Metal, Vulkan, or OpenGL textures. A build may drive
      these on `RenderDriverKind.CORE_WORKER` only, and fails an attach with
      `RenderDriverKind.CALLER_GRAPHICS_THREAD` then.

    An attach with another driver fails with `Status.UNSUPPORTED`.
    """

    producer_sync_kinds: GpuSyncKindFlag = field(
        default_factory=lambda: GpuSyncKindFlag(0)
    )
    """The kinds of `GpuSync` that you can wait on before you read an acquired
    frame. The session uses one of them that its target supports. Zero lets
    the session pick a kind that names no object to wait on.
    """

    frame_wake: Wake | NativeWake = field(default_factory=lambda: Wake())
    """Called when frame results are waiting for
    `RenderSessionHandle.drain_frame_results()`. It isn't called again until
    a drain takes them.
    """

    redraw_wake: Wake | NativeWake = field(default_factory=lambda: Wake())
    """Called when the session has something new to draw. Demand a frame at your
    next display refresh. It isn't called again until you demand a frame, or
    while the session has no target.
    """

    driver_work_wake: Wake | NativeWake = field(default_factory=lambda: Wake())
    """Called when a session driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD`
    has work for `RenderSessionHandle.service_driver_work()`. It isn't called
    again until a service runs.
    """

    keep_depth: bool = False
    """Whether an exported ring keeps the depth and stencil of each frame for
    you, so that you can draw into the frame depth-tested against the map, as
    `FrameProjection` says. The getter of the device's API, such as
    `AcquiredFrameHandle.vulkan_texture()`, gets a frame's depth texture on
    the device, and `AcquiredFrameHandle.opaque_fd()` shares it across APIs
    when `GpuDeviceCapabilities.depth_export_kinds` has the ring's kind.
    Keeping depth costs memory bandwidth, most on tile-based GPUs. A borrowed
    ring keeps depth in the textures that you give it instead, and the other
    targets keep none, so an attach to them with `keep_depth` fails with
    `Status.UNSUPPORTED`.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            producer_sync_kinds=GpuSyncKindFlag(raw["producer_sync_kinds"]),
            frame_wake=Wake(),
            redraw_wake=Wake(),
            driver_work_wake=Wake(),
            keep_depth=raw["keep_depth"],
        )


@dataclass(frozen=True, slots=True)
class RenderSessionCapabilities:
    """What an attached render session supports.

    See `mln_render_session_capabilities` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    driver: RenderDriverKind

    texture_ring_depth: int
    """How many textures the session's texture ring has."""

    flags: RenderSessionCapabilityFlag

    producer_sync_kind: GpuSyncKind
    """The kind of `GpuSync` that `AcquiredFrameHandle.producer_sync()` reports."""

    consumer_sync_kinds: GpuSyncKindFlag
    """The kinds of `GpuSync` that `AcquiredFrameHandle.release()` accepts. They
    depend on the target and on what the device supports.
    """

    export_kind: GpuExportKind
    """How an exported ring shares its textures, which says which getter gets an
    acquired frame's texture, or `GpuExportKind.NONE` for another target.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            texture_ring_depth=raw["texture_ring_depth"],
            flags=RenderSessionCapabilityFlag(raw["flags"]),
            producer_sync_kind=GpuSyncKind(raw["producer_sync_kind"]),
            consumer_sync_kinds=GpuSyncKindFlag(raw["consumer_sync_kinds"]),
            export_kind=GpuExportKind(raw["export_kind"]),
        )


@dataclass(frozen=True, slots=True)
class RenderSessionSnapshot:
    """The state of a render session at one moment.

    See `mln_render_session_snapshot` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    state: RenderSessionState

    extent: LogicalExtent
    """The target's logical size, and the map's scale factor."""

    generation: int

    frame_generation: int

    acquired_frame_count: int

    failure_status: Status
    """`Status.OK` until the session fails. Then `Status.TARGET_LOST` when it
    failed because it lost its target, or `Status.NATIVE_ERROR` when it
    failed for another reason.
    """

    has_target: bool
    """Whether the session is attached and has a target to render to. It's false
    from when `RenderSessionHandle.release_target()` completes until you give
    the session a new target.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            state=RenderSessionState(raw["state"]),
            extent=LogicalExtent._from_native(raw["extent"]),
            generation=raw["generation"],
            frame_generation=raw["frame_generation"],
            acquired_frame_count=raw["acquired_frame_count"],
            failure_status=Status(raw["failure_status"]),
            has_target=raw["has_target"],
        )


type RenderTarget = (
    RenderTargetOffscreenVariant
    | RenderTargetMetalLayerVariant
    | RenderTargetWin32WindowVariant
    | RenderTargetXlibWindowVariant
    | RenderTargetWaylandSurfaceVariant
    | RenderTargetAndroidWindowVariant
    | RenderTargetOhosWindowVariant
    | RenderTargetVulkanSurfaceVariant
    | RenderTargetOpenGlSurfaceVariant
    | RenderTargetMetalBorrowedTexturesVariant
    | RenderTargetVulkanBorrowedTexturesVariant
    | RenderTargetOpenGlBorrowedTexturesVariant
    | RenderTargetWebGpuBorrowedTexturesVariant
    | RenderTargetExportedRingVariant
    | RenderTargetWebCanvasVariant
)
"""What a render session renders into: one kind of target, and its descriptor.

Every target has a size in logical pixels, which sets the map's size. The
session renders it at the map's scale factor. A borrowed or exported ring
hands each frame to you: acquire it with
`RenderSessionHandle.acquire_frame()`, and release it with your GPU's
synchronization once you have read it.

The session uses the objects that the descriptor names until a replacement or
release of the target completes, a detach or release of the session
completes, or an abandon of it returns, except as a kind's descriptor says. A
window target presents without waiting for the display, so demand at most one
frame per display refresh.

See `mln_render_target` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
"""


@dataclass(frozen=True, slots=True)
class RenderedFeature:
    """A feature that a rendered feature query found.

    See `mln_rendered_feature` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    feature: Any
    """The feature, as a GeoJSON feature."""

    layer_id: str
    """The layer that drew the feature."""

    source_id: str

    source_layer_id: str
    """The feature's source layer, or empty when its source has none."""

    state: Any
    """The feature's state, as a JSON object."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            feature=raw["feature"],
            layer_id=raw["layer_id"],
            source_id=raw["source_id"],
            source_layer_id=raw["source_layer_id"],
            state=raw["state"],
        )


@dataclass(frozen=True, slots=True)
class RenderedFeatureQueryOptions:
    """Options for a rendered feature query.

    See `mln_rendered_feature_query_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    layer_ids: Sequence[str] | None = None
    """The layers to query. Without them, the query covers every layer."""

    filter: Any = None
    """A \\[filter\\](https://maplibre.org/maplibre-style-spec/expressions/) that
    each feature must match. Empty matches every feature.
    """

    def __post_init__(self):
        object.__setattr__(self, "layer_ids", _sequence(self.layer_ids))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            layer_ids=None
            if raw["layer_ids"] is None
            else (tuple(item for item in raw["layer_ids"])),
            filter=raw["filter"],
        )


type RenderedQueryGeometry = (
    RenderedQueryGeometryPointVariant
    | RenderedQueryGeometryBoxVariant
    | RenderedQueryGeometryLineStringVariant
)
"""The screen area that a rendered feature query covers.

See `mln_rendered_query_geometry` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
"""


@dataclass(frozen=True, slots=True)
class RenderingStats:
    """Statistics about the frames that a map renders.

    See `mln_rendering_stats` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    encoding_time_ms: float
    """The time that the frame took to encode, in milliseconds."""

    rendering_time_ms: float
    """The time that the frame took to render, in milliseconds."""

    frame_count: int
    """The frames rendered so far."""

    draw_call_count: int
    """The draw calls in the frame."""

    total_draw_call_count: int
    """The draw calls in every frame so far."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            encoding_time_ms=raw["encoding_time_ms"],
            rendering_time_ms=raw["rendering_time_ms"],
            frame_count=raw["frame_count"],
            draw_call_count=raw["draw_call_count"],
            total_draw_call_count=raw["total_draw_call_count"],
        )


@dataclass(frozen=True, slots=True)
class ResourceProvider:
    """A resource provider, for `RuntimeHandle.set_resource_provider()`.

    See `mln_resource_provider` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: ResourceProviderCallback
    routes: Sequence[ResourceRoute] = ()
    """The requests that the provider serves. Without routes, it serves every
    network request.
    """

    def __post_init__(self):
        object.__setattr__(self, "routes", _sequence(self.routes))

    def _invoke_callback(self, request, handle):
        callback = self.callback
        handle = _adopt_value(handle, "ResourceRequestHandle")
        try:
            return callback(ResourceRequestInfo._from_native(request), handle)
        except BaseException:
            handle.release()
            raise


@dataclass(frozen=True, slots=True)
class ResourceRange:
    """The bytes from `start` to `end` of a resource, both included.

    See `mln_resource_range` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    start: int

    end: int

    @classmethod
    def _from_native(cls, raw):
        return cls(start=raw["start"], end=raw["end"])


@dataclass(frozen=True, slots=True)
class ResourceRequestCancelHandler:
    """A cancel callback, for `ResourceRequestHandle.set_cancel_callback()`.

    See `mln_resource_request_cancel_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: ResourceRequestCancelCallback

    def _invoke_callback(self):
        callback = self.callback
        return callback()


@dataclass(frozen=True, slots=True)
class ResourceRequestInfo:
    """A request that a resource provider serves.

    When the cache holds an earlier copy of the resource, the `prior_` fields
    describe it.

    See `mln_resource_request_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    requested_url: str
    """The URL as the style names it."""

    resolved_url: str
    """The URL to load, after the library expands any shorthand in
    `requested_url` and before any resource transform. Routes match this URL.
    """

    kind: ResourceKind

    loading_method: ResourceLoadingMethod

    priority: ResourcePriority

    usage: ResourceUsage

    storage_policy: ResourceStoragePolicy

    range: ResourceRange | None
    """The only bytes of the resource that the request wants."""

    prior_modified_unix_ms: int | None

    prior_expires_unix_ms: int | None

    prior_etag: str
    """Empty when the earlier copy has no ETag."""

    prior_data: bytes

    @classmethod
    def _from_native(cls, raw):
        return cls(
            requested_url=raw["requested_url"],
            resolved_url=raw["resolved_url"],
            kind=ResourceKind(raw["kind"]),
            loading_method=ResourceLoadingMethod(raw["loading_method"]),
            priority=ResourcePriority(raw["priority"]),
            usage=ResourceUsage(raw["usage"]),
            storage_policy=ResourceStoragePolicy(raw["storage_policy"]),
            range=_maybe(ResourceRange._from_native, raw["range"]),
            prior_modified_unix_ms=raw["prior_modified_unix_ms"],
            prior_expires_unix_ms=raw["prior_expires_unix_ms"],
            prior_etag=raw["prior_etag"],
            prior_data=raw["prior_data"],
        )


@dataclass(frozen=True, slots=True)
class ResourceResponse:
    """The answer to a resource request.

    See `mln_resource_response` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    status: ResourceResponseStatus = ResourceResponseStatus.OK

    error_reason: ResourceErrorReason = ResourceErrorReason.OTHER
    """Why the request failed, for `ResourceResponseStatus.ERROR`."""

    bytes: Buffer = b""
    """The resource, for `ResourceResponseStatus.OK`."""

    error_message: str = ""
    """A description of the error, for `ResourceResponseStatus.ERROR`."""

    must_revalidate: bool = False
    """Whether the library must request the resource again before it reuses this
    copy.
    """

    modified_unix_ms: int | None = None

    expires_unix_ms: int | None = None

    etag: str = ""
    """Empty when the resource has no ETag."""

    retry_after_unix_ms: int | None = None
    """When to retry, for `ResourceResponseStatus.ERROR`."""

    def __post_init__(self):
        object.__setattr__(self, "bytes", _bytes(self.bytes))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            status=ResourceResponseStatus(raw["status"]),
            error_reason=ResourceErrorReason(raw["error_reason"]),
            bytes=raw["bytes"],
            error_message=raw["error_message"],
            must_revalidate=raw["must_revalidate"],
            modified_unix_ms=raw["modified_unix_ms"],
            expires_unix_ms=raw["expires_unix_ms"],
            etag=raw["etag"],
            retry_after_unix_ms=raw["retry_after_unix_ms"],
        )


@dataclass(frozen=True, slots=True)
class ResourceRoute:
    """The requests that a resource provider serves.

    See `mln_resource_route` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    kinds: ResourceKindFlag = field(default_factory=lambda: ResourceKindFlag(0))
    """The kinds of resource that the route covers. Zero covers every kind."""

    flags: UrlMatchFlag = field(default_factory=lambda: UrlMatchFlag(0))

    url: str = ""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kinds=ResourceKindFlag(raw["kinds"]),
            flags=UrlMatchFlag(raw["flags"]),
            url=raw["url"],
        )


@dataclass(frozen=True, slots=True)
class ResourceTransform:
    """How to rewrite request URLs, for
    `RuntimeHandle.set_resource_transform()`.

    Set either `callback` or `rules`, not both.

    See `mln_resource_transform` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: ResourceTransformCallback | None = None
    rules: Sequence[ResourceTransformRule] = ()
    """The first rule that matches a request adds its parameters, and no other.
    A request that no rule matches keeps its URL.
    """

    def __post_init__(self):
        object.__setattr__(self, "rules", _sequence(self.rules))

    def _invoke_callback(self, kind, url, request):
        callback = self.callback
        assert callback is not None
        request = _adopt_value(request, "ResourceTransformRequestHandle")
        try:
            return callback(ResourceKind(kind), url, request)
        except BaseException:
            request.release()
            raise


@dataclass(frozen=True, slots=True)
class ResourceTransformError:
    """Why a resource transform fails a request.

    See `mln_resource_transform_error` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    reason: ResourceErrorReason = ResourceErrorReason.OTHER

    message: str = ""

    retry_after_unix_ms: int | None = None
    """When to retry, for `ResourceErrorReason.RATE_LIMIT`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            reason=ResourceErrorReason(raw["reason"]),
            message=raw["message"],
            retry_after_unix_ms=raw["retry_after_unix_ms"],
        )


@dataclass(frozen=True, slots=True)
class ResourceTransformRule:
    """Query parameters to add to the URLs of requests that match a pattern.

    The library adds the parameters, in order, at the end of the URL's query
    and before any fragment. It keeps the parameters that the URL already
    has, even one with the same name.

    See `mln_resource_transform_rule` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    kinds: ResourceKindFlag = field(default_factory=lambda: ResourceKindFlag(0))
    """The kinds of resource that the rule covers. Zero covers every kind."""

    flags: UrlMatchFlag = field(default_factory=lambda: UrlMatchFlag(0))

    url: str = ""

    parameters: Sequence[UrlQueryParameter] = ()

    def __post_init__(self):
        object.__setattr__(self, "parameters", _sequence(self.parameters))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kinds=ResourceKindFlag(raw["kinds"]),
            flags=UrlMatchFlag(raw["flags"]),
            url=raw["url"],
            parameters=tuple(
                UrlQueryParameter._from_native(item) for item in raw["parameters"]
            ),
        )


@dataclass(frozen=True, slots=True)
class RuntimeEvent:
    """An event that a runtime or one of its maps raised.

    See `mln_runtime_event` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    type: RuntimeEventType

    source_type: RuntimeEventSourceType

    source: int
    """The runtime or map that raised the event, as `source_type` says."""

    generation: int
    """For a map event, a map snapshot with this generation or a later one
    includes what the event reports. Zero for a runtime event.
    """

    message: str
    """The error's text for an event that reports a failure, or empty."""

    payload: (
        RuntimeEventCameraChangeVariant
        | RuntimeEventLoadingFailedVariant
        | RuntimeEventRenderFrameVariant
        | RuntimeEventStyleImageVariant
        | RuntimeEventTileActionVariant
        | RuntimeEventSourceChangedVariant
        | RuntimeEventOfflineRegionStatusVariant
        | RuntimeEventOfflineRegionResponseErrorVariant
        | RuntimeEventGlyphsVariant
        | RuntimeEventSpriteVariant
        | UnknownVariant
        | None
    )

    @classmethod
    def _from_native(cls, raw):
        return cls(
            type=RuntimeEventType(raw["type"]),
            source_type=RuntimeEventSourceType(raw["source_type"]),
            source=raw["source"],
            generation=raw["generation"],
            message=raw["message"],
            payload=_copy_variant(
                raw["payload"],
                {
                    "camera_change": RuntimeEventCameraChangeVariant,
                    "loading_failed": RuntimeEventLoadingFailedVariant,
                    "render_frame": RuntimeEventRenderFrameVariant,
                    "style_image": RuntimeEventStyleImageVariant,
                    "tile_action": RuntimeEventTileActionVariant,
                    "source_changed": RuntimeEventSourceChangedVariant,
                    "offline_region_status": RuntimeEventOfflineRegionStatusVariant,
                    "offline_region_response_error": RuntimeEventOfflineRegionResponseErrorVariant,
                    "glyphs": RuntimeEventGlyphsVariant,
                    "sprite": RuntimeEventSpriteVariant,
                },
                0,
            ),
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventCameraChange:
    """The details of `RuntimeEventType.MAP_CAMERA_WILL_CHANGE` and
    `RuntimeEventType.MAP_CAMERA_DID_CHANGE`.

    See `mln_runtime_event_camera_change` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    mode: CameraChangeMode

    source: CameraChangeSource

    @classmethod
    def _from_native(cls, raw):
        return cls(
            mode=CameraChangeMode(raw["mode"]), source=CameraChangeSource(raw["source"])
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventGlyphs:
    """The details of a glyph event.

    The event covers the glyphs from `first` to `last`, both included.

    See `mln_runtime_event_glyphs` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    font_stack: tuple[str, ...]
    """The fonts that the glyphs come from, in order of preference."""

    first: int

    last: int

    font_face: bool

    @classmethod
    def _from_native(cls, raw):
        return cls(
            font_stack=tuple(item for item in raw["font_stack"]),
            first=raw["first"],
            last=raw["last"],
            font_face=raw["font_face"],
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventHandler:
    """An event callback, for `RuntimeHandle.set_event_handler()`.

    See `mln_runtime_event_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: RuntimeEventCallback

    def _invoke_callback(self, events):
        callback = self.callback
        return callback(tuple(RuntimeEvent._from_native(item) for item in events))


@dataclass(frozen=True, slots=True)
class RuntimeEventLoadingFailed:
    """The details of `RuntimeEventType.MAP_LOADING_FAILED`.

    See `mln_runtime_event_loading_failed` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    error: MapLoadError

    @classmethod
    def _from_native(cls, raw):
        return cls(error=MapLoadError(raw["error"]))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionResponseError:
    """The details of `RuntimeEventType.OFFLINE_REGION_RESPONSE_ERROR`.

    See `mln_runtime_event_offline_region_response_error` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    region_id: int

    reason: ResourceErrorReason

    @classmethod
    def _from_native(cls, raw):
        return cls(
            region_id=raw["region_id"], reason=ResourceErrorReason(raw["reason"])
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionStatus:
    """The details of `RuntimeEventType.OFFLINE_REGION_STATUS_CHANGED`.

    See `mln_runtime_event_offline_region_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    region_id: int

    status: OfflineRegionStatus

    @classmethod
    def _from_native(cls, raw):
        return cls(
            region_id=raw["region_id"],
            status=OfflineRegionStatus._from_native(raw["status"]),
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventRenderFrame:
    """The details of `RuntimeEventType.MAP_RENDER_FRAME_FINISHED`.

    See `mln_runtime_event_render_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    mode: RenderMode

    needs_repaint: bool

    placement_changed: bool

    stats: RenderingStats

    @classmethod
    def _from_native(cls, raw):
        return cls(
            mode=RenderMode(raw["mode"]),
            needs_repaint=raw["needs_repaint"],
            placement_changed=raw["placement_changed"],
            stats=RenderingStats._from_native(raw["stats"]),
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventSourceChanged:
    """The details of `RuntimeEventType.MAP_SOURCE_CHANGED`.

    See `mln_runtime_event_source_changed` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    source_id: str

    @classmethod
    def _from_native(cls, raw):
        return cls(source_id=raw["source_id"])


@dataclass(frozen=True, slots=True)
class RuntimeEventSprite:
    """The details of a sprite event.

    `id` and `url` are both empty when the style has no sprite.

    See `mln_runtime_event_sprite` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    id: str

    url: str

    @classmethod
    def _from_native(cls, raw):
        return cls(id=raw["id"], url=raw["url"])


@dataclass(frozen=True, slots=True)
class RuntimeEventStyleImage:
    """The details of `RuntimeEventType.MAP_STYLE_IMAGE_MISSING`.

    See `mln_runtime_event_style_image` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    image_id: str

    @classmethod
    def _from_native(cls, raw):
        return cls(image_id=raw["image_id"])


@dataclass(frozen=True, slots=True)
class RuntimeEventTileAction:
    """The details of `RuntimeEventType.MAP_TILE_ACTION`.

    See `mln_runtime_event_tile_action` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    source_id: str
    """The source that the tile belongs to."""

    operation: TileOperation

    tile_id: TileId

    @classmethod
    def _from_native(cls, raw):
        return cls(
            source_id=raw["source_id"],
            operation=TileOperation(raw["operation"]),
            tile_id=TileId._from_native(raw["tile_id"]),
        )


@dataclass(frozen=True, slots=True)
class RuntimeOptions:
    """Options for `RuntimeHandle.create()`.

    See `mln_runtime_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    asset_path: str = ""
    """The directory that `asset://` URLs load from. Empty uses the working
    directory, or the app's assets on Android.
    """

    cache_path: str = ""
    """The path of the cache database file, which holds the ambient cache and
    offline regions. Its directory must be writable. Empty keeps the cache in
    memory.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(asset_path=raw["asset_path"], cache_path=raw["cache_path"])


@dataclass(frozen=True, slots=True)
class ScreenBox:
    """A rectangle of the screen, from its `min` corner to its `max` corner.

    See `mln_screen_box` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    min: ScreenPoint = field(default_factory=lambda: ScreenPoint())

    max: ScreenPoint = field(default_factory=lambda: ScreenPoint())

    @classmethod
    def _from_native(cls, raw):
        return cls(
            min=ScreenPoint._from_native(raw["min"]),
            max=ScreenPoint._from_native(raw["max"]),
        )


@dataclass(frozen=True, slots=True)
class ScreenLineString:
    """A line through points on the screen.

    See `mln_screen_line_string` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    points: Sequence[ScreenPoint] = ()

    def __post_init__(self):
        object.__setattr__(self, "points", _sequence(self.points))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            points=tuple(ScreenPoint._from_native(item) for item in raw["points"])
        )


@dataclass(frozen=True, slots=True)
class ScreenPoint:
    """A point on the screen, in logical pixels from the map's top-left corner.

    See `mln_screen_point` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: float = 0.0

    y: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"])


@dataclass(frozen=True, slots=True)
class SourceFeature:
    """A feature that a source feature query found.

    See `mln_source_feature` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    feature: Any
    """The feature, as a GeoJSON feature."""

    source_layer_id: str
    """The feature's source layer, or empty when its source has none."""

    @classmethod
    def _from_native(cls, raw):
        return cls(feature=raw["feature"], source_layer_id=raw["source_layer_id"])


@dataclass(frozen=True, slots=True)
class SourceFeatureQueryOptions:
    """Options for a source feature query.

    See `mln_source_feature_query_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    source_layer_ids: Sequence[str] | None = None
    """The source layers to query. A vector source finds features only in the
    source layers named here.
    """

    filter: Any = None
    """A \\[filter\\](https://maplibre.org/maplibre-style-spec/expressions/) that
    each feature must match. Empty matches every feature.
    """

    def __post_init__(self):
        object.__setattr__(self, "source_layer_ids", _sequence(self.source_layer_ids))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            source_layer_ids=None
            if raw["source_layer_ids"] is None
            else (tuple(item for item in raw["source_layer_ids"])),
            filter=raw["filter"],
        )


@dataclass(frozen=True, slots=True)
class StyleExpressionInfo:
    """What a style expression's result depends on.

    See `mln_style_expression_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    is_feature_constant: bool
    """True when the result doesn't depend on the feature or its state."""

    is_zoom_constant: bool
    """True when the result doesn't depend on the zoom level."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            is_feature_constant=raw["is_feature_constant"],
            is_zoom_constant=raw["is_zoom_constant"],
        )


@dataclass(frozen=True, slots=True)
class StyleExpressionInputs:
    """The inputs that a style expression reads while it evaluates.

    See `mln_style_expression_inputs` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    zoom: float | None = None

    color_ramp_parameter: float | None = None
    """The value of `heatmap-density`, `line-progress`, and `elevation`."""

    feature: Any = None
    """The feature to evaluate against, as a GeoJSON feature, or empty for none."""

    feature_state: Any = None
    """The feature's state, as a JSON object, or empty for none."""

    global_state: Any = None
    """The values that `global-state` reads, as a JSON object, or empty for
    none.
    """

    canonical_tile_id: CanonicalTileId | None = None
    """The tile that holds the feature. `within` and `distance` need it to read
    the feature's geometry.
    """

    available_images: Sequence[str] = ()
    """The IDs of the images that `image` treats as available."""

    def __post_init__(self):
        object.__setattr__(self, "available_images", _sequence(self.available_images))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            zoom=raw["zoom"],
            color_ramp_parameter=raw["color_ramp_parameter"],
            feature=raw["feature"],
            feature_state=raw["feature_state"],
            global_state=raw["global_state"],
            canonical_tile_id=_maybe(
                CanonicalTileId._from_native, raw["canonical_tile_id"]
            ),
            available_images=tuple(item for item in raw["available_images"]),
        )


@dataclass(frozen=True, slots=True)
class StyleExpressionOptions:
    """Options for parsing a style expression.

    See `mln_style_expression_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    expected_type: StyleExpressionType | None = None
    """The type that the result must have."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            expected_type=_maybe(StyleExpressionType._from_native, raw["expected_type"])
        )


@dataclass(frozen=True, slots=True)
class StyleExpressionType:
    """The type of a style expression's result.

    See `mln_style_expression_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    kind: StyleExpressionTypeKind = StyleExpressionTypeKind.VALUE

    item_kind: StyleExpressionTypeKind = StyleExpressionTypeKind.VALUE
    """The type of each item when `kind` is `StyleExpressionTypeKind.ARRAY`. It
    can't be an array itself.
    """

    item_count: int = 0
    """The length of an array, or 0 for any length."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=StyleExpressionTypeKind(raw["kind"]),
            item_kind=StyleExpressionTypeKind(raw["item_kind"]),
            item_count=raw["item_count"],
        )


@dataclass(frozen=True, slots=True)
class StyleImageInfo:
    """An image in the map's style.

    See `mln_style_image_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    width: int

    height: int

    pixels: bytes
    """The premultiplied RGBA pixels, in rows of `width` \\* 4 bytes, from the
    top row down.
    """

    stretch_x: tuple[ImageStretch, ...]

    stretch_y: tuple[ImageStretch, ...]

    content: ImageContent | None

    text_fit_width: StyleImageTextFit | None

    text_fit_height: StyleImageTextFit | None

    pixel_ratio: float

    sdf: bool

    keep_unused: bool

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            pixels=raw["pixels"],
            stretch_x=tuple(
                ImageStretch._from_native(item) for item in raw["stretch_x"]
            ),
            stretch_y=tuple(
                ImageStretch._from_native(item) for item in raw["stretch_y"]
            ),
            content=_maybe(ImageContent._from_native, raw["content"]),
            text_fit_width=_maybe(StyleImageTextFit, raw["text_fit_width"]),
            text_fit_height=_maybe(StyleImageTextFit, raw["text_fit_height"]),
            pixel_ratio=raw["pixel_ratio"],
            sdf=raw["sdf"],
            keep_unused=raw["keep_unused"],
        )


@dataclass(frozen=True, slots=True)
class StyleImageOptions:
    """Options for a style image, as the
    \\[sprite\\](https://maplibre.org/maplibre-style-spec/sprite/) spec
    describes them.

    See `mln_style_image_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    stretch_x: Sequence[ImageStretch] | None = None
    """The ranges of columns that stretch, in increasing order without overlap."""

    stretch_y: Sequence[ImageStretch] | None = None
    """The ranges of rows that stretch, in increasing order without overlap."""

    content: ImageContent | None = None

    text_fit_width: StyleImageTextFit | None = None

    text_fit_height: StyleImageTextFit | None = None

    pixel_ratio: float | None = None

    sdf: bool | None = None

    keep_unused: bool | None = None
    """Keeps the image after no layer uses it. Without this, the map can remove
    an image that it asked for through
    `RuntimeEventType.MAP_STYLE_IMAGE_MISSING`.
    """

    def __post_init__(self):
        object.__setattr__(self, "stretch_x", _sequence(self.stretch_x))
        object.__setattr__(self, "stretch_y", _sequence(self.stretch_y))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            stretch_x=None
            if raw["stretch_x"] is None
            else (tuple(ImageStretch._from_native(item) for item in raw["stretch_x"])),
            stretch_y=None
            if raw["stretch_y"] is None
            else (tuple(ImageStretch._from_native(item) for item in raw["stretch_y"])),
            content=_maybe(ImageContent._from_native, raw["content"]),
            text_fit_width=_maybe(StyleImageTextFit, raw["text_fit_width"]),
            text_fit_height=_maybe(StyleImageTextFit, raw["text_fit_height"]),
            pixel_ratio=raw["pixel_ratio"],
            sdf=raw["sdf"],
            keep_unused=raw["keep_unused"],
        )


@dataclass(frozen=True, slots=True)
class StyleLayerInfo:
    """A layer in the map's style.

    See `mln_style_layer_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    id: str

    type: str
    """The layer's
    \\[type\\](https://maplibre.org/maplibre-style-spec/layers/#type), such as
    `fill`.
    """

    source_id: str

    source_layer: str

    min_zoom: float
    """The zoom level where the layer starts to show, or negative infinity when
    the layer doesn't set one.
    """

    max_zoom: float
    """The zoom level where the layer stops showing, or infinity when the layer
    doesn't set one.
    """

    visibility: StyleLayerVisibility

    @classmethod
    def _from_native(cls, raw):
        return cls(
            id=raw["id"],
            type=raw["type"],
            source_id=raw["source_id"],
            source_layer=raw["source_layer"],
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            visibility=StyleLayerVisibility(raw["visibility"]),
        )


@dataclass(frozen=True, slots=True)
class StyleSourceInfo:
    """A source in the map's style.

    See `mln_style_source_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    id: str

    type: StyleSourceType

    is_volatile: bool

    attribution: str

    url: str

    tile_json: StyleSourceTileInfo | None
    """The tiles that the style lists in the source itself, instead of at a URL."""

    bounds: LatLngBounds | None

    tile_size: int | None

    vector_encoding: StyleVectorTileEncoding | None

    raster_encoding: StyleRasterDemEncoding | None

    geo_json: GeoJsonSourceOptions | None
    """A GeoJSON source's options, with every field set."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            id=raw["id"],
            type=StyleSourceType(raw["type"]),
            is_volatile=raw["is_volatile"],
            attribution=raw["attribution"],
            url=raw["url"],
            tile_json=_maybe(StyleSourceTileInfo._from_native, raw["tile_json"]),
            bounds=_maybe(LatLngBounds._from_native, raw["bounds"]),
            tile_size=raw["tile_size"],
            vector_encoding=_maybe(StyleVectorTileEncoding, raw["vector_encoding"]),
            raster_encoding=_maybe(StyleRasterDemEncoding, raw["raster_encoding"]),
            geo_json=_maybe(GeoJsonSourceOptions._from_native, raw["geo_json"]),
        )


@dataclass(frozen=True, slots=True)
class StyleSourceTileInfo:
    """The tiles of a source, as the style lists them.

    See `mln_style_source_tile_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    tile_urls: tuple[str, ...]

    min_zoom: float

    max_zoom: float

    scheme: StyleTileScheme

    @classmethod
    def _from_native(cls, raw):
        return cls(
            tile_urls=tuple(item for item in raw["tile_urls"]),
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            scheme=StyleTileScheme(raw["scheme"]),
        )


@dataclass(frozen=True, slots=True)
class StyleTransitionOptions:
    """The default
    \\[transition\\](https://maplibre.org/maplibre-style-spec/transition/) for
    changes to a style's paint properties.

    See `mln_style_transition_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    duration_ms: float | None = None

    delay_ms: float | None = None

    enable_placement_transitions: bool | None = None
    """Whether symbols fade in and out as the map places them."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            duration_ms=raw["duration_ms"],
            delay_ms=raw["delay_ms"],
            enable_placement_transitions=raw["enable_placement_transitions"],
        )


@dataclass(frozen=True, slots=True)
class TextureImageInfo:
    """The layout of the pixels that `RenderSessionHandle.read_texture()` reads.

    See `mln_texture_image_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    stride: int
    """The bytes from the start of one row to the start of the next."""

    byte_length: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            stride=raw["stride"],
            byte_length=raw["byte_length"],
        )


@dataclass(frozen=True, slots=True)
class TextureReadbackResult:
    """The pixels that `RenderSessionHandle.read_texture()` reads.

    See `mln_texture_readback_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    data: bytes
    """Premultiplied RGBA pixels with 8 bits per channel, in rows from top to
    bottom.
    """

    info: TextureImageInfo

    @classmethod
    def _from_native(cls, raw):
        return cls(data=raw["data"], info=TextureImageInfo._from_native(raw["info"]))


@dataclass(frozen=True, slots=True)
class TileId:
    """The ID of a tile.

    `canonical_x` and `canonical_y` place the tile in the source's grid at
    zoom `canonical_z`. `overscaled_z` is the zoom that the map draws the
    tile at, and is greater when the source has no tiles that deep. `wrap`
    says which copy of the world holds the tile: zero for the main one,
    negative to the west, and positive to the east.

    See `mln_tile_id` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    overscaled_z: int

    wrap: int

    canonical_z: int

    canonical_x: int

    canonical_y: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            overscaled_z=raw["overscaled_z"],
            wrap=raw["wrap"],
            canonical_z=raw["canonical_z"],
            canonical_x=raw["canonical_x"],
            canonical_y=raw["canonical_y"],
        )


@dataclass(frozen=True, slots=True)
class UnitBezier:
    """A timing curve from (0, 0) to (1, 1), with the control points (`x1`,
    `y1`) and (`x2`, `y2`).

    `x1` and `x2` are between 0 and 1.

    See `mln_unit_bezier` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x1: float = 0.0

    y1: float = 0.0

    x2: float = 0.0

    y2: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(x1=raw["x1"], y1=raw["y1"], x2=raw["x2"], y2=raw["y2"])


@dataclass(frozen=True, slots=True)
class UrlQueryParameter:
    """A query parameter that a resource transform rule adds to a URL.

    Pass `name` and `value` unencoded. The library percent-encodes every byte
    of them except the letters, digits, `-`, `.`, `_`, and `~`, and adds them
    as `name=value`.

    See `mln_url_query_parameter` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    name: str

    value: str = ""

    @classmethod
    def _from_native(cls, raw):
        return cls(name=raw["name"], value=raw["value"])


@dataclass(frozen=True, slots=True)
class Vec3:
    """A vector in three dimensions.

    See `mln_vec3` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: float = 0.0

    y: float = 0.0

    z: float = 0.0

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"], z=raw["z"])


@dataclass(frozen=True, slots=True)
class Vec4:
    """A vector in four dimensions, such as a point in homogeneous coordinates.

    See `mln_vec4` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: float

    y: float

    z: float

    w: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"], z=raw["z"], w=raw["w"])


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTexture:
    """A Vulkan image of a borrowed ring.

    See `mln_vulkan_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    image: int
    """Your `VkImage`."""

    image_view: int
    """A 2D `VkImageView` of `image` with one mip level and one layer."""

    @classmethod
    def _from_native(cls, raw):
        return cls(image=raw["image"], image_view=raw["image_view"])


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTexturesTarget:
    """A ring of your Vulkan images to render into, on the Vulkan device that
    you attach on.

    `physical_width` and `physical_height` give the images' size in pixels,
    which is usually `logical_size` times the map's scale factor. Each image
    must have that size, `format`, and `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`,
    and belong to the device's `VkDevice`. The ring holds up to
    `TEXTURE_RING_MAX_DEPTH` distinct images.

    Give the ring `depth_textures` to draw into each frame depth-tested
    against the map, as `FrameProjection` says: one for each of `textures`,
    at the same index, or none. Each frame clears the one at its slot,
    renders the map's depth and stencil into it, stores them, and leaves it
    in `depth_final_layout`, with `owner` as for the color image. They must
    have the size of `textures`, `depth_format`, one sample, and
    `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT`, and their views both the
    depth and the stencil aspect.

    See `mln_vulkan_borrowed_textures_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    physical_width: int

    physical_height: int

    textures: Sequence[VulkanBorrowedTexture]

    format: int
    """The images' `VkFormat`."""

    final_layout: int
    """The `VkImageLayout` that each frame leaves its image in."""

    owner: VulkanImageOwner = VulkanImageOwner.GRAPHICS_QUEUE

    depth_textures: Sequence[VulkanBorrowedTexture] = ()

    depth_format: int = 0
    """The depth images' `VkFormat`, of
    `GpuDeviceCapabilities.depth_stencil_formats`.
    """

    depth_final_layout: int = 0
    """The `VkImageLayout` that each frame leaves its depth image in."""

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))
        object.__setattr__(self, "depth_textures", _sequence(self.depth_textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            textures=tuple(
                VulkanBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            format=raw["format"],
            final_layout=raw["final_layout"],
            owner=VulkanImageOwner(raw["owner"]),
            depth_textures=tuple(
                VulkanBorrowedTexture._from_native(item)
                for item in raw["depth_textures"]
            ),
            depth_format=raw["depth_format"],
            depth_final_layout=raw["depth_final_layout"],
        )


@dataclass(frozen=True, slots=True)
class VulkanContextDescriptor:
    """The Vulkan device and queue that a render session uses.

    Keep these objects alive until a detach or release of the session
    completes, or an abandon of it returns.

    See `mln_vulkan_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    instance: int
    """A `VkInstance`."""

    physical_device: int
    """A `VkPhysicalDevice`."""

    device: int
    """A `VkDevice`."""

    graphics_queue: int
    """A `VkQueue` from `graphics_queue_family_index` that supports graphics. If
    you submit to it too, give the device your `QueueLock`.
    """

    graphics_queue_family_index: int = 0

    get_instance_proc_addr: int = 0
    """Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's."""

    get_device_proc_addr: int = 0
    """Your `vkGetDeviceProcAddr`, or null to look it up from the instance."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            instance=raw["instance"],
            physical_device=raw["physical_device"],
            device=raw["device"],
            graphics_queue=raw["graphics_queue"],
            graphics_queue_family_index=raw["graphics_queue_family_index"],
            get_instance_proc_addr=raw["get_instance_proc_addr"],
            get_device_proc_addr=raw["get_device_proc_addr"],
        )


@dataclass(frozen=True, slots=True)
class VulkanDeviceDescriptor:
    """Your Vulkan device and queue, for `GpuDeviceHandle.wrap_vulkan()`.

    See `mln_vulkan_device_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
    """

    context: VulkanContextDescriptor

    queue_lock: QueueLock | NativeQueueLock = field(default_factory=lambda: QueueLock())
    """Your lock on `context.graphics_queue`, when you submit to it yourself.
    Give every device that wraps the queue the same lock.
    """

    enabled_extensions: Sequence[str] = ()
    """The names of the device extensions that you enabled on `context.device`,
    such as "VK_KHR_external_semaphore_fd". Vulkan offers no way to ask a
    device which extensions it has, so the device offers the export kinds and
    `GpuSyncKind.SYNC_FD` only when the extensions that they need are listed
    here, and none of them when the list is empty.
    """

    def __post_init__(self):
        object.__setattr__(
            self, "enabled_extensions", _sequence(self.enabled_extensions)
        )

    @classmethod
    def _from_native(cls, raw):
        return cls(
            context=VulkanContextDescriptor._from_native(raw["context"]),
            queue_lock=QueueLock(),
            enabled_extensions=tuple(item for item in raw["enabled_extensions"]),
        )


@dataclass(frozen=True, slots=True)
class VulkanSurfaceTarget:
    """Your Vulkan surface to present to.

    Make it with the instance of the Vulkan device that you attach on. The
    device's queue family must support presenting to the surface. The session
    creates the swapchain, and destroys it but not the surface.

    See `mln_vulkan_surface_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    surface: int
    """Your `VkSurfaceKHR`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            surface=raw["surface"],
        )


@dataclass(frozen=True, slots=True)
class VulkanTextureFrame:
    """The Vulkan image of an acquired frame.

    The depth members are set when the frame has a depth image, as for
    `MetalTextureFrame`. Otherwise they are zero.

    See `mln_vulkan_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int

    height: int

    scale_factor: float

    slot: int
    """The image's index in the descriptor's `textures`."""

    image: int
    """The VkImage."""

    image_view: int
    """The VkImageView."""

    device: int
    """The image's VkDevice."""

    format: int
    """The image's VkFormat."""

    layout: int
    """The VkImageLayout that the frame left the image in."""

    depth_image: int
    """The VkImage that holds the frame's depth and stencil."""

    depth_image_view: int
    """A VkImageView of the depth image's depth and stencil aspects."""

    depth_format: int
    """The depth image's VkFormat."""

    depth_layout: int
    """The VkImageLayout that the frame left the depth image in."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            slot=raw["slot"],
            image=raw["image"],
            image_view=raw["image_view"],
            device=raw["device"],
            format=raw["format"],
            layout=raw["layout"],
            depth_image=raw["depth_image"],
            depth_image_view=raw["depth_image_view"],
            depth_format=raw["depth_format"],
            depth_layout=raw["depth_layout"],
        )


@dataclass(frozen=True, slots=True)
class Wake:
    """A callback that tells the caller when work is ready.

    The callback runs on the binding's dispatcher thread, after native queues
    the call.

    See `mln_wake` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
    """

    callback: WakeCallback | None = None

    def _invoke_callback(self):
        callback = self.callback
        assert callback is not None
        return callback()

    @classmethod
    def native(
        cls, callback: int, user_data: int = 0, release_user_data: int = 0
    ) -> NativeWake:
        """A native registration, whose function pointers, context, and release
        the host's native code supplies as addresses. Zero is null. The
        binding copies each address unchanged, and native code validates
        them.
        """
        from . import _native

        return NativeWake._from_registration(
            _native._NativeRegistration(
                "mln_wake", (callback,), user_data, release_user_data
            )
        )


class NativeWake:
    """A native `Wake` from `Wake.native`, accepted wherever a `Wake` is.

    The first call that carries it consumes it. Native code releases the
    context of a call that it accepts, and the binding releases the context
    of a call that native code does not accept. A second call raises an
    invalid-state error.
    """

    __slots__ = ("_registration",)

    def __init__(self):
        raise TypeError("native registrations come from Wake.native")

    @classmethod
    def _from_registration(cls, registration):
        value = cls.__new__(cls)
        value._registration = registration
        return value


@dataclass(frozen=True, slots=True)
class WaylandSurfaceTarget:
    """A Wayland surface to present to.

    Keep the display connected until the session's detach completes. On an
    OpenGL device, if you also use EGL on the display, initialize it before
    you attach.

    See `mln_wayland_surface_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    display: int
    """Your `wl_display*`."""

    surface: int
    """Your `wl_surface*`, on `display`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            display=raw["display"],
            surface=raw["surface"],
        )


@dataclass(frozen=True, slots=True)
class WebCanvasTarget:
    """A canvas to present to.

    The session makes the canvas's "webgpu" or "webgl2" context with the
    device's backend, and sets the canvas's size to `logical_size` at the
    map's scale factor. Attached without a device, it renders with the
    build's backend: a WebGPU build asks the browser for a device, and
    attaches once it has one. For an OpenGL device that wraps your WebGL2
    context, the canvas must be the context's own.

    See `mln_web_canvas_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    canvas: int
    """An `OffscreenCanvas`, such as one from
    `HTMLCanvasElement.transferControlToOffscreen()` that you posted to the
    library's Web Worker, or an `HTMLCanvasElement` when the library runs on
    the page's thread.
    """

    format: str = ""
    """For a WebGPU device, the `GPUTextureFormat` to configure the canvas with,
    such as `bgra8unorm`. Empty for a format that the device's adapter
    prefers.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            canvas=raw["canvas"],
            format=raw["format"],
        )


@dataclass(frozen=True, slots=True)
class WebGlContextDescriptor:
    """The WebGL2 context that an OpenGL device wraps.

    See `mln_web_gl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    context: int
    """Your `WebGL2RenderingContext`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(context=raw["context"])


@dataclass(frozen=True, slots=True)
class WebGpuBorrowedTexture:
    """A WebGPU texture of a borrowed ring.

    See `mln_web_gpu_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    texture: int
    """Your `GPUTexture`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"])


@dataclass(frozen=True, slots=True)
class WebGpuBorrowedTexturesTarget:
    """A ring of your WebGPU textures to render into, on a WebGPU device that
    wraps yours.

    `physical_width` and `physical_height` give the textures' size in pixels,
    which is usually `logical_size` times the map's scale factor. Each
    texture must be 2D with that size, `format`, one layer, one mip level,
    one sample, and `GPUTextureUsage.RENDER_ATTACHMENT`. The ring holds up to
    `TEXTURE_RING_MAX_DEPTH` distinct textures.

    Give the ring `depth_textures` to draw into each frame depth-tested
    against the map, as `FrameProjection` says: one for each of `textures`,
    at the same index, or none. Each frame clears the one at its slot,
    renders the map's depth and stencil into it, and stores them. They must
    be 2D with the size of `textures`, `depth_format`, one layer, one mip
    level, one sample, and `GPUTextureUsage.RENDER_ATTACHMENT`.

    See `mln_web_gpu_borrowed_textures_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    physical_width: int

    physical_height: int

    textures: Sequence[WebGpuBorrowedTexture]

    format: str
    """The textures' `GPUTextureFormat`, such as `rgba8unorm`."""

    depth_textures: Sequence[WebGpuBorrowedTexture] = ()

    depth_format: str = ""
    """The depth textures' `GPUTextureFormat`, of
    `GpuDeviceCapabilities.depth_stencil_formats`.
    """

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))
        object.__setattr__(self, "depth_textures", _sequence(self.depth_textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            textures=tuple(
                WebGpuBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            format=raw["format"],
            depth_textures=tuple(
                WebGpuBorrowedTexture._from_native(item)
                for item in raw["depth_textures"]
            ),
            depth_format=raw["depth_format"],
        )


@dataclass(frozen=True, slots=True)
class WglContextDescriptor:
    """Your WGL context, which each session's context shares objects with.

    See `mln_wgl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    device_context: int
    """An `HDC` with the pixel format of `share_context`."""

    share_context: int
    """Your `HGLRC`."""

    get_proc_address: int = 0
    """Your `wglGetProcAddress`, or null to use the system's."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            device_context=raw["device_context"],
            share_context=raw["share_context"],
            get_proc_address=raw["get_proc_address"],
        )


@dataclass(frozen=True, slots=True)
class Win32WindowTarget:
    """A Win32 window to present to.

    On an OpenGL device, the session sets the window's pixel format, so don't
    set one yourself. While the window is minimized, frame demands finish
    with `RenderResult.TARGET_NOT_READY`.

    See `mln_win32_window_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    window: int
    """Your `HWND`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            window=raw["window"],
        )


@dataclass(frozen=True, slots=True)
class XlibWindowTarget:
    """An X11 window to present to, through Xlib.

    Keep the display connected until the session's detach completes.

    See `mln_xlib_window_target` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    logical_size: LogicalSize

    display: int
    """Your `Display*`."""

    window: int
    """Your `Window`, on `display`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            logical_size=LogicalSize._from_native(raw["logical_size"]),
            display=raw["display"],
            window=raw["window"],
        )


class CameraTransitionEndCallback(Protocol):
    """Receives the end of a camera change.

    See `mln_camera_transition_end_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    def __call__(self, end: CameraTransitionEnd, /) -> None: ...


class CustomSourceCancelTileCallback(Protocol):
    """Reports that a custom source no longer needs a tile.

    `map` and `source_id` name the source.

    See `mln_custom_source_cancel_tile_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    def __call__(
        self, map: MapHandle, source_id: str, tile_id: CanonicalTileId, /
    ) -> None: ...


class CustomSourceFetchTileCallback(Protocol):
    """Asks for the data of a tile in a custom source.

    `map` and `source_id` name the source that asks. Return quickly. Once the
    data is ready, pass it to
    `MapHandle.set_custom_geometry_source_tile_data()` or
    `MapHandle.set_custom_vector_source_tile_data()`. If it fails to load,
    report that with `MapHandle.set_custom_geometry_source_tile_error()` or
    `MapHandle.set_custom_vector_source_tile_error()`.

    See `mln_custom_source_fetch_tile_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    def __call__(
        self, map: MapHandle, source_id: str, tile_id: CanonicalTileId, /
    ) -> None: ...


class LogCallback(Protocol):
    """Receives a log message.

    The callback runs on the binding's dispatcher thread, after native queues
    the call.

    See `mln_log_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    def __call__(
        self, severity: LogSeverity, event: LogEvent, code: int, message: str, /
    ) -> None: ...


class ResourceProviderCallback(Protocol):
    """Serves a network request in place of the library.

    Answer with `ResourceRequestHandle.complete()`, or hand the request back
    to the library with `ResourceRequestHandle.pass_through()`. Either can
    come now or later, from any thread. A request released without an answer
    fails.

    See `mln_resource_provider_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    def __call__(
        self, request: ResourceRequestInfo, handle: ResourceRequestHandle, /
    ) -> None: ...


class ResourceRequestCancelCallback(Protocol):
    """Reports that the library no longer needs a resource request.

    The request no longer accepts an answer, so stop working on it and
    release it.

    See `mln_resource_request_cancel_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    def __call__(self, /) -> None: ...


class ResourceTransformCallback(Protocol):
    """Rewrites the URL of a network request before the library sends it.

    The request waits until you answer with
    `ResourceTransformRequestHandle.complete()` or
    `ResourceTransformRequestHandle.fail()`. To keep `url`, complete with it.
    Releasing `request` without an answer fails the request. You can answer
    now or later, from any thread.

    See `mln_resource_transform_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    def __call__(
        self, kind: ResourceKind, url: str, request: ResourceTransformRequestHandle, /
    ) -> None: ...


class RuntimeEventCallback(Protocol):
    """Receives a batch of events.

    See `mln_runtime_event_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    def __call__(self, events: tuple[RuntimeEvent, ...], /) -> None: ...


class WakeCallback(Protocol):
    """Tells the caller that work is ready for it.

    One call can stand for several pieces of ready work, so take all of it
    each time.

    The callback runs on the binding's dispatcher thread, after native queues
    the call.

    See `mln_wake_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
    """

    def __call__(self, /) -> None: ...


DIAGNOSTIC_MESSAGE_CAPACITY: Final[int] = 4096
"""The size of a diagnostic message in bytes, with its terminator.

See `MLN_DIAGNOSTIC_MESSAGE_CAPACITY` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
"""

FRAME_DEMAND_TOKEN_STILL_IMAGE: Final[int] = 18446744073709551615
"""The token of the frame results of the demand that a session holds for an
image of `MapHandle.request_still_image()`, which says how that demand runs.
Your own demands can't use it.

See `MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
"""

TEXTURE_RING_MAX_DEPTH: Final[int] = 8
"""The most textures that a render session's texture ring can have.

See `MLN_TEXTURE_RING_MAX_DEPTH` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
"""

VULKAN_NON_DISPATCHABLE_HANDLE_NULL: Final[int] = 0
"""The null Vulkan non-dispatchable handle.

See `MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL` in the
[C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
"""
