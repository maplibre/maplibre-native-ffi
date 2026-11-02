"""Generated from the C headers by tools/bindgen. Do not edit."""

from __future__ import annotations

from collections.abc import Buffer, Sequence
from dataclasses import dataclass, field
from enum import IntFlag
from typing import TYPE_CHECKING, Any, Protocol

from ._enum import UnknownIntEnum
from ._operation import _adopt_value

if TYPE_CHECKING:
    from ._generated_owners import (
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
class RuntimeEventRenderFrameVariant:
    value: RuntimeEventRenderFrame
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventRenderFrame._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventRenderMapVariant:
    value: RuntimeEventRenderMap
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventRenderMap._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventTileActionVariant:
    value: RuntimeEventTileAction
    _tag = 3

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventTileAction._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionStatusVariant:
    value: RuntimeEventOfflineRegionStatus
    _tag = 4

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionStatus._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionResponseErrorVariant:
    value: RuntimeEventOfflineRegionResponseError
    _tag = 5

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionResponseError._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionTileCountLimitVariant:
    value: RuntimeEventOfflineRegionTileCountLimit
    _tag = 6

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionTileCountLimit._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventGlyphsVariant:
    value: RuntimeEventGlyphs
    _tag = 7

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventGlyphs._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventSpriteVariant:
    value: RuntimeEventSprite
    _tag = 8

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventSprite._from_native(raw))


class AmbientCacheOperation(UnknownIntEnum):
    """An action on a runtime's ambient cache.

    The ambient cache holds the resources that maps load, apart from the ones
    that offline regions hold.

    See `mln_ambient_cache_operation` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    RESET_DATABASE = 1
    PACK_DATABASE = 2
    INVALIDATE = 3
    CLEAR = 4


class CameraChangeMode(UnknownIntEnum):
    """Whether a camera change jumps or animates.

    See `mln_camera_change_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    IMMEDIATE = 0
    ANIMATED = 1


class CameraTransitionOutcome(UnknownIntEnum):
    """How a camera change ended.

    See `mln_camera_transition_outcome` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    COMPLETED = 0
    CANCELLED = 1
    CLOSED = 2


class CameraUpdateMode(UnknownIntEnum):
    """How `MapHandle.update_camera()` moves the camera.

    See `mln_camera_update_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    JUMP = 0
    EASE = 1
    FLY = 2


class ConstrainMode(UnknownIntEnum):
    """How the map keeps the camera within the world or within its bounds.

    See `mln_constrain_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    NONE = 0
    HEIGHT_ONLY = 1
    WIDTH_AND_HEIGHT = 2
    SCREEN = 3


class FrameDemandFlag(IntFlag):
    """How a frame request behaves.

    See `mln_frame_demand_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    FORCE = 1
    PRESENT = 2
    WAIT_FOR_UPDATE = 4


class GesturePhase(UnknownIntEnum):
    """Where a camera change falls in a gesture that the user makes.

    See `mln_gesture_phase` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    NONE = 0
    BEGIN = 1
    UPDATE = 2
    END = 3
    CANCEL = 4


class GpuSyncKind(UnknownIntEnum):
    """How one side waits for the other's GPU work on a shared texture.

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


class MapMode(UnknownIntEnum):
    """When a map renders.

    See `mln_map_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    CONTINUOUS = 0
    STATIC = 1
    TILE = 2


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
    """The OpenGL API that a dedicated EGL context uses.

    See `mln_open_gl_client_api` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    UNSPECIFIED = 0
    GL = 1
    GLES = 2


class OpenGlContextOwnership(UnknownIntEnum):
    """Whether a render session's OpenGL context shares objects with yours.

    See `mln_open_gl_context_ownership` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    SHARED = 0
    DEDICATED = 1


class OpenGlContextProviderFlag(IntFlag):
    """An OpenGL context platform, as a bit in a set.

    See `mln_open_gl_context_provider_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    WGL = 1
    EGL = 2
    WEB_GL = 4


class RenderAbandonDisposition(UnknownIntEnum):
    """What `RenderSessionHandle.abandon()` did with a session's graphics
    objects.

    See `mln_render_abandon_disposition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    CLEAN = 0
    QUARANTINED = 1


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
    """How a frame request finished.

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


class ResourceErrorReason(UnknownIntEnum):
    """Why a resource request failed.

    See `mln_resource_error_reason` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    NONE = 0
    NOT_FOUND = 1
    SERVER = 2
    CONNECTION = 3
    RATE_LIMIT = 4
    OTHER = 5


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
    OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 524288
    MAP_GLYPHS_REQUESTED = 1048576
    MAP_GLYPHS_LOADED = 2097152
    MAP_GLYPHS_ERROR = 4194304
    MAP_SPRITE_REQUESTED = 8388608
    MAP_SPRITE_LOADED = 16777216
    MAP_SPRITE_ERROR = 33554432
    ALL_MAP_EVENTS = 66191358
    ALL_RUNTIME_EVENTS = 917504
    ALL = 67108862


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
    for the regions that `RuntimeHandle.set_offline_region_observed()`
    observes.

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
    OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 19
    MAP_GLYPHS_REQUESTED = 20
    MAP_GLYPHS_LOADED = 21
    MAP_GLYPHS_ERROR = 22
    MAP_SPRITE_REQUESTED = 23
    MAP_SPRITE_LOADED = 24
    MAP_SPRITE_ERROR = 25


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
    NOT_READY = -9
    NOT_FOUND = -10


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
    ANNOTATIONS = 7
    CUSTOM_GEOMETRY = 8
    CUSTOM_VECTOR = 9


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

    A pattern matches the whole URL, query included.

    See `mln_url_match_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    EXACT = 0
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
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    GRAPHICS_QUEUE = 0
    EXTERNAL = 1
    FOREIGN = 2


class WebGlContextKind(UnknownIntEnum):
    """How a render session gets its WebGL context.

    See `mln_web_gl_context_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    EXISTING = 0
    TRANSFERRED_CANVAS = 1


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

    easing: UnitBezier | None = None

    transition_id: int | None = None
    """An ID that `MapHandle.cancel_camera_transition()` cancels the change by.
    Several changes can share one.
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

    max_pitch: float | None = None

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

    Its members mean the same as those of `CameraOptions`.

    See `mln_camera` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    center: LatLng = field(default_factory=lambda: LatLng())

    center_altitude: float = 0.0

    padding: EdgeInsets = field(default_factory=lambda: EdgeInsets())

    zoom: float = 0.0

    bearing: float = 0.0

    pitch: float = 0.0

    roll: float = 0.0

    field_of_view: float = 0.0

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
    """Space to leave between the area and each edge of the map."""

    bearing: float | None = None

    pitch: float | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            padding=_maybe(EdgeInsets._from_native, raw["padding"]),
            bearing=raw["bearing"],
            pitch=raw["pitch"],
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

    @classmethod
    def _from_native(cls, raw):
        return cls(
            outcome=CameraTransitionOutcome(raw["outcome"]),
            generation=raw["generation"],
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
    beyond its edges.
    """

    clip: bool | None = None
    """Whether to cut geometry at the outer edge of a tile's margin."""

    wrap: bool | None = None
    """Whether to wrap geometry that crosses the antimeridian."""

    def _invoke_fetch_tile(self, tile_id):
        callback = self.fetch_tile
        return callback(CanonicalTileId._from_native(tile_id))

    def _invoke_cancel_tile(self, tile_id):
        callback = self.cancel_tile
        assert callback is not None
        return callback(CanonicalTileId._from_native(tile_id))


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

    def _invoke_fetch_tile(self, tile_id):
        callback = self.fetch_tile
        return callback(CanonicalTileId._from_native(tile_id))

    def _invoke_cancel_tile(self, tile_id):
        callback = self.cancel_tile
        assert callback is not None
        return callback(CanonicalTileId._from_native(tile_id))


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
    """The EGL objects that a render session makes its OpenGL context from.

    See `mln_egl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    display: int
    """An `EGLDisplay`."""

    config: int
    """An `EGLConfig`."""

    share_context: int = 0
    """Your `EGLContext`, for a shared context."""

    client_api: OpenGlClientApi = OpenGlClientApi.UNSPECIFIED
    """The API of a dedicated context. A shared context uses the API of yours."""

    get_proc_address: int = 0
    """Your `eglGetProcAddress`, or null to use the system's."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            display=raw["display"],
            config=raw["config"],
            share_context=raw["share_context"],
            client_api=OpenGlClientApi(raw["client_api"]),
            get_proc_address=raw["get_proc_address"],
        )


@dataclass(frozen=True, slots=True)
class FeatureStateSelector:
    """The features whose state to set, read, or remove.

    See `mln_feature_state_selector` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    source_id: str

    source_layer_id: str = ""
    """The source layer, for a vector source. Empty for other sources."""

    feature_id: str | None = None
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
            feature_id=raw["feature_id"],
            state_key=raw["state_key"],
        )


@dataclass(frozen=True, slots=True)
class FrameDemand:
    """A request for one frame.

    See `mln_frame_demand` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    flags: FrameDemandFlag = field(default_factory=lambda: FrameDemandFlag(0))

    token: int = 0
    """A value of your choice that the request's frame result carries."""

    @classmethod
    def _from_native(cls, raw):
        return cls(flags=FrameDemandFlag(raw["flags"]), token=raw["token"])


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

    cluster_properties: Any = None
    """The `clusterProperties` object, or empty for none."""

    buffer: int | None = None

    cluster_radius: int | None = None

    cluster_min_points: int | None = None

    line_metrics: bool | None = None

    cluster: bool | None = None

    synchronous_tiling: bool | None = None
    """Builds tiles while the map renders instead of in the background, so that
    new data shows in the next frame.
    """

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
            synchronous_tiling=raw["synchronous_tiling"],
        )


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
class GraphicsDeviceOptions:
    """Options for `GraphicsDeviceHandle.create()`.

    See `mln_graphics_device_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
    """

    adapter_index: int | None = None
    """The GPU to use, as an index into the platform's list of GPUs."""

    single_context: bool | None = None
    """Whether the device's texture sessions take turns in one OpenGL context
    instead of having one each. Only an EGL device reads it.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            adapter_index=raw["adapter_index"], single_context=raw["single_context"]
        )


@dataclass(frozen=True, slots=True)
class GraphicsDeviceTextureDescriptor:
    """The texture that `MapHandle.attach_graphics_device_texture()` draws into.

    See `mln_graphics_device_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
    """

    extent: LogicalExtent

    @classmethod
    def _from_native(cls, raw):
        return cls(extent=LogicalExtent._from_native(raw["extent"]))


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

    flags: UrlMatchFlag = UrlMatchFlag.EXACT

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

    Set either `callback` or `rules`, not both.

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
class MapOptions:
    """Options for `RuntimeHandle.create_map()`.

    See `mln_map_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    initial_extent: LogicalExtent
    """The map's first size. Its scale factor stays fixed for the map's life."""

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

    @classmethod
    def _from_native(cls, raw):
        return cls(
            initial_extent=LogicalExtent._from_native(raw["initial_extent"]),
            map_mode=MapMode(raw["map_mode"]),
            fast_pfor_enabled=raw["fast_pfor_enabled"],
            event_mask=_maybe(RuntimeEventFlag, raw["event_mask"]),
            tile_cache_enabled=raw["tile_cache_enabled"],
        )


@dataclass(frozen=True, slots=True)
class MapSnapshot:
    """A map's state, as `MapHandle.get_snapshot()` reads it.

    See `mln_map_snapshot` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    debug_options: MapDebugFlag

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

    tile: MapTileOptions

    bounds: BoundOptions

    free_camera: FreeCameraOptions

    view: MapView

    @classmethod
    def _from_native(cls, raw):
        return cls(
            debug_options=MapDebugFlag(raw["debug_options"]),
            generation=raw["generation"],
            fully_loaded=raw["fully_loaded"],
            rendering_stats_view_enabled=raw["rendering_stats_view_enabled"],
            gesture_in_progress=raw["gesture_in_progress"],
            transition_in_progress=raw["transition_in_progress"],
            event_mask=RuntimeEventFlag(raw["event_mask"]),
            latest_render_update_generation=raw["latest_render_update_generation"],
            tile=MapTileOptions._from_native(raw["tile"]),
            bounds=BoundOptions._from_native(raw["bounds"]),
            free_camera=FreeCameraOptions._from_native(raw["free_camera"]),
            view=MapView._from_native(raw["view"]),
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
    """The pitch, in radians, above which the map lowers the detail."""

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
class MapView:
    """One view of a map: its camera, size, and projection.

    See `mln_map_view` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    camera: Camera = field(default_factory=lambda: Camera())

    extent: LogicalExtent = field(default_factory=lambda: LogicalExtent())

    projection_mode: ProjectionMode = field(default_factory=lambda: ProjectionMode())

    viewport: MapViewportOptions = field(default_factory=lambda: MapViewportOptions())

    @classmethod
    def _from_native(cls, raw):
        return cls(
            camera=Camera._from_native(raw["camera"]),
            extent=LogicalExtent._from_native(raw["extent"]),
            projection_mode=ProjectionMode._from_native(raw["projection_mode"]),
            viewport=MapViewportOptions._from_native(raw["viewport"]),
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
class MetalBorrowedTexture:
    """A Metal texture of a borrowed ring.

    See `mln_metal_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    texture: int
    """Your MTLTexture."""

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"])


@dataclass(frozen=True, slots=True)
class MetalBorrowedTextureDescriptor:
    """A ring of your Metal textures, for
    `MapHandle.attach_metal_borrowed_texture()` and
    `RenderSessionHandle.set_metal_borrowed_texture_target()`.

    `extent` sets the map's size and the session's scale factor, so give it
    the map's scale factor. `physical_width` and `physical_height` give the
    textures' size in pixels. The textures must all have that size, one
    device, one pixel format, one sample, and MTLTextureUsageRenderTarget.

    The session uses the textures until a replacement or release of the
    target completes, a detach or disposal of the session completes, or an
    abandon of it returns.

    See `mln_metal_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent

    physical_width: int

    physical_height: int

    textures: Sequence[MetalBorrowedTexture]

    command_queue: int = 0
    """Your MTLCommandQueue on the textures' device, or null for a queue of the
    session's own. With your queue, frames can use `GpuSyncKind.QUEUE_ORDER`.
    """

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            textures=tuple(
                MetalBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            command_queue=raw["command_queue"],
        )


@dataclass(frozen=True, slots=True)
class MetalContextDescriptor:
    """The Metal device that a render session uses.

    See `mln_metal_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    device: int = 0
    """An `MTLDevice`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(device=raw["device"])


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureDescriptor:
    """A ring of Metal textures that the session creates, for
    `MapHandle.attach_metal_owned_texture()`.

    See `mln_metal_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent

    pixel_format: int
    """The textures' MTLPixelFormat: MTLPixelFormatRGBA8Unorm or
    MTLPixelFormatBGRA8Unorm.
    """

    context: MetalContextDescriptor = field(
        default_factory=lambda: MetalContextDescriptor()
    )
    """Names the MTLDevice that creates the textures."""

    command_queue: int = 0
    """Your MTLCommandQueue on the same device, or null for a queue of the
    session's own. With your queue, frames can use `GpuSyncKind.QUEUE_ORDER`.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=MetalContextDescriptor._from_native(raw["context"]),
            pixel_format=raw["pixel_format"],
            command_queue=raw["command_queue"],
        )


@dataclass(frozen=True, slots=True)
class MetalSurfaceDescriptor:
    """A Metal layer to render to, for `MapHandle.attach_metal_surface()` and
    `RenderSessionHandle.set_metal_surface_target()`.

    The session sets the layer's device, pixel format, and drawable size. It
    uses the layer until a replacement or release of the target completes, a
    detach or disposal of the session completes, or an abandon of it returns.

    See `mln_metal_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: LogicalExtent

    layer: int
    """Your CAMetalLayer."""

    context: MetalContextDescriptor = field(
        default_factory=lambda: MetalContextDescriptor()
    )
    """The MTLDevice to render with. A null device picks the system's default."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=MetalContextDescriptor._from_native(raw["context"]),
            layer=raw["layer"],
        )


@dataclass(frozen=True, slots=True)
class MetalTextureFrame:
    """The Metal texture of an acquired frame.

    See `mln_metal_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int

    width: int

    height: int

    scale_factor: float

    frame_id: int

    slot: int
    """The texture's index in the ring. For a borrowed ring, it indexes the
    descriptor's `textures`.
    """

    texture: int
    """The MTLTexture."""

    device: int
    """The texture's MTLDevice."""

    pixel_format: int
    """The texture's MTLPixelFormat."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            slot=raw["slot"],
            texture=raw["texture"],
            device=raw["device"],
            pixel_format=raw["pixel_format"],
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
class OpenGlBorrowedTexture:
    """An OpenGL texture of a borrowed ring.

    See `mln_open_gl_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    texture: int
    """Your texture name."""

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"])


@dataclass(frozen=True, slots=True)
class OpenGlBorrowedTextureDescriptor:
    """A ring of your OpenGL textures, for
    `MapHandle.attach_open_gl_borrowed_texture()` and
    `RenderSessionHandle.set_open_gl_borrowed_texture_target()`.

    `extent` sets the map's size and the session's scale factor, so give it
    the map's scale factor. `physical_width` and `physical_height` give the
    textures' size in pixels. The context must use
    `OpenGlContextOwnership.SHARED`. Each texture must be in your context's
    share group, with storage of that size.

    The session uses the textures until a replacement or release of the
    target completes, a detach or disposal of the session completes, or an
    abandon of it returns.

    See `mln_open_gl_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent

    physical_width: int

    physical_height: int

    context: OpenGlContextDescriptor

    textures: Sequence[OpenGlBorrowedTexture]

    target: int
    """The textures' target. It must be GL_TEXTURE_2D."""

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            context=OpenGlContextDescriptor._from_native(raw["context"]),
            textures=tuple(
                OpenGlBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            target=raw["target"],
        )


@dataclass(frozen=True, slots=True)
class OpenGlContextDescriptor:
    """How a render session gets the OpenGL context that it renders with.

    Keep the objects that it names alive until a detach or disposal of the
    session completes, or an abandon of it returns.

    See `mln_open_gl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    data: (
        OpenGlContextDescriptorWglVariant
        | OpenGlContextDescriptorEglVariant
        | OpenGlContextDescriptorWebGlVariant
    )

    ownership: OpenGlContextOwnership = OpenGlContextOwnership.SHARED

    @classmethod
    def _from_native(cls, raw):
        return cls(
            ownership=OpenGlContextOwnership(raw["ownership"]),
            data=_copy_variant(
                raw["data"],
                {
                    "wgl": OpenGlContextDescriptorWglVariant,
                    "egl": OpenGlContextDescriptorEglVariant,
                    "web_gl": OpenGlContextDescriptorWebGlVariant,
                },
                None,
            ),
        )


@dataclass(frozen=True, slots=True)
class OpenGlOwnedTextureDescriptor:
    """A ring of OpenGL textures that the session creates, for
    `MapHandle.attach_open_gl_owned_texture()`.

    See `mln_open_gl_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent

    context: OpenGlContextDescriptor

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=OpenGlContextDescriptor._from_native(raw["context"]),
        )


@dataclass(frozen=True, slots=True)
class OpenGlSurfaceDescriptor:
    """An OpenGL surface to present to, for `MapHandle.attach_open_gl_surface()`
    and `RenderSessionHandle.set_open_gl_surface_target()`.

    The session uses the surface until a replacement or release of the target
    completes, a detach or disposal of the session completes, or an abandon
    of it returns.

    See `mln_open_gl_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: LogicalExtent

    context: OpenGlContextDescriptor

    surface: int = 0
    """Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=OpenGlContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
        )


@dataclass(frozen=True, slots=True)
class OpenGlTextureFrame:
    """The OpenGL texture of an acquired frame.

    See `mln_open_gl_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int

    width: int

    height: int

    scale_factor: float

    frame_id: int

    slot: int
    """The texture's index in the ring. For a borrowed ring, it indexes the
    descriptor's `textures`.
    """

    texture: int

    target: int

    internal_format: int
    """The texture's internal format, or 0 for a borrowed ring."""

    format: int
    """The texture's pixel format, or 0 for a borrowed ring."""

    type: int
    """The texture's pixel type, or 0 for a borrowed ring."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            slot=raw["slot"],
            texture=raw["texture"],
            target=raw["target"],
            internal_format=raw["internal_format"],
            format=raw["format"],
            type=raw["type"],
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
class ProjectionMode:
    """Settings that skew the map or draw it without perspective.

    See `mln_projection_mode` in the
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
    """Your lock on a Vulkan queue that you and a render session both submit to.

    The session holds the lock only while it submits or presents to the
    queue. Set both `lock` and `unlock`, or neither when you don't submit to
    the queue yourself.

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
    """How one frame request finished.

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
            mode=RenderMode(raw["mode"]),
        )


@dataclass(frozen=True, slots=True)
class RenderSessionAttachOptions:
    """Options for attaching a render session to a map.

    See `mln_render_session_attach_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    driver: RenderDriverKind

    requested_texture_ring_depth: int = 0
    """How many textures a ring that the session creates has, up to 8. Zero
    picks a default.
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

    driver_work_wake: Wake | NativeWake = field(default_factory=lambda: Wake())
    """Called when a session driven by `RenderDriverKind.CALLER_GRAPHICS_THREAD`
    has work for `RenderSessionHandle.service_driver_work()`. It isn't called
    again until a service runs.
    """

    queue_lock: QueueLock | NativeQueueLock = field(default_factory=lambda: QueueLock())
    """Your lock on the Vulkan queue, when you submit to it yourself."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            requested_texture_ring_depth=raw["requested_texture_ring_depth"],
            producer_sync_kinds=GpuSyncKindFlag(raw["producer_sync_kinds"]),
            frame_wake=Wake(),
            driver_work_wake=Wake(),
            queue_lock=QueueLock(),
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
    """The kinds of `GpuSync` that `AcquiredFrameHandle.close()` accepts."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            texture_ring_depth=raw["texture_ring_depth"],
            flags=RenderSessionCapabilityFlag(raw["flags"]),
            producer_sync_kind=GpuSyncKind(raw["producer_sync_kind"]),
            consumer_sync_kinds=GpuSyncKindFlag(raw["consumer_sync_kinds"]),
        )


@dataclass(frozen=True, slots=True)
class RenderSessionSnapshot:
    """The state of a render session at one moment.

    See `mln_render_session_snapshot` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    state: RenderSessionState

    extent: LogicalExtent

    generation: int

    map_update_generation: int

    rendered_update_generation: int

    frame_generation: int

    acquired_frame_count: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            state=RenderSessionState(raw["state"]),
            extent=LogicalExtent._from_native(raw["extent"]),
            generation=raw["generation"],
            map_update_generation=raw["map_update_generation"],
            rendered_update_generation=raw["rendered_update_generation"],
            frame_generation=raw["frame_generation"],
            acquired_frame_count=raw["acquired_frame_count"],
        )


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

    encoding_time: float
    """The time that the frame took to encode, in seconds."""

    rendering_time: float
    """The time that the frame took to render, in seconds."""

    frame_count: int
    """The frames rendered so far."""

    draw_call_count: int
    """The draw calls in the frame."""

    total_draw_call_count: int
    """The draw calls in every frame so far."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            encoding_time=raw["encoding_time"],
            rendering_time=raw["rendering_time"],
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
            handle.close()
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

    error_reason: ResourceErrorReason = ResourceErrorReason.NONE
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

    flags: UrlMatchFlag = UrlMatchFlag.EXACT

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
    """A resource transform callback, for
    `RuntimeHandle.set_resource_transform()`.

    See `mln_resource_transform` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: ResourceTransformCallback

    def _invoke_callback(self, kind, url, request):
        callback = self.callback
        request = _adopt_value(request, "ResourceTransformRequestHandle")
        try:
            return callback(ResourceKind(kind), url, request)
        except BaseException:
            request.close()
            raise


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

    code: int
    """The `CameraChangeMode` for `RuntimeEventType.MAP_CAMERA_WILL_CHANGE` and
    `RuntimeEventType.MAP_CAMERA_DID_CHANGE`.
    """

    message: str
    """The error for an event that reports a failure, the image ID for
    `RuntimeEventType.MAP_STYLE_IMAGE_MISSING`, or the source ID for
    `RuntimeEventType.MAP_SOURCE_CHANGED` and
    `RuntimeEventType.MAP_TILE_ACTION`.
    """

    payload: (
        RuntimeEventRenderFrameVariant
        | RuntimeEventRenderMapVariant
        | RuntimeEventTileActionVariant
        | RuntimeEventOfflineRegionStatusVariant
        | RuntimeEventOfflineRegionResponseErrorVariant
        | RuntimeEventOfflineRegionTileCountLimitVariant
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
            code=raw["code"],
            message=raw["message"],
            payload=_copy_variant(
                raw["payload"],
                {
                    "render_frame": RuntimeEventRenderFrameVariant,
                    "render_map": RuntimeEventRenderMapVariant,
                    "tile_action": RuntimeEventTileActionVariant,
                    "offline_region_status": RuntimeEventOfflineRegionStatusVariant,
                    "offline_region_response_error": RuntimeEventOfflineRegionResponseErrorVariant,
                    "offline_region_tile_count_limit": RuntimeEventOfflineRegionTileCountLimitVariant,
                    "glyphs": RuntimeEventGlyphsVariant,
                    "sprite": RuntimeEventSpriteVariant,
                },
                0,
            ),
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
class RuntimeEventOfflineRegionTileCountLimit:
    """The details of
    `RuntimeEventType.OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`.

    See `mln_runtime_event_offline_region_tile_count_limit` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    region_id: int

    limit: int
    """The number of Mapbox tiles that offline regions may hold."""

    @classmethod
    def _from_native(cls, raw):
        return cls(region_id=raw["region_id"], limit=raw["limit"])


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
class RuntimeEventRenderMap:
    """The details of `RuntimeEventType.MAP_RENDER_MAP_FINISHED`.

    See `mln_runtime_event_render_map` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    mode: RenderMode

    @classmethod
    def _from_native(cls, raw):
        return cls(mode=RenderMode(raw["mode"]))


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
class RuntimeEventTileAction:
    """The details of `RuntimeEventType.MAP_TILE_ACTION`.

    See `mln_runtime_event_tile_action` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    operation: TileOperation

    tile_id: TileId

    @classmethod
    def _from_native(cls, raw):
        return cls(
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
    """The database file that holds the ambient cache and offline regions. Empty
    keeps them in memory.
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
    """The value that `heatmap-density` and `line-progress` read."""

    elevation: float | None = None
    """The value that `elevation` reads when `color_ramp_parameter` isn't set."""

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
            elevation=raw["elevation"],
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
class VulkanBorrowedTexture:
    """A Vulkan image of a borrowed ring.

    See `mln_vulkan_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    image: int
    """Your VkImage."""

    image_view: int
    """A 2D VkImageView of `image` with one mip level and one layer."""

    @classmethod
    def _from_native(cls, raw):
        return cls(image=raw["image"], image_view=raw["image_view"])


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTextureDescriptor:
    """A ring of your Vulkan images, for
    `MapHandle.attach_vulkan_borrowed_texture()` and
    `RenderSessionHandle.set_vulkan_borrowed_texture_target()`.

    `extent` sets the map's size and the session's scale factor, so give it
    the map's scale factor. `physical_width` and `physical_height` give the
    images' size in pixels. Each image must have that size, `format`, and
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, and belong to the context's device.

    The session uses the images until a replacement or release of the target
    completes, a detach or disposal of the session completes, or an abandon
    of it returns.

    See `mln_vulkan_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent

    physical_width: int

    physical_height: int

    context: VulkanContextDescriptor

    textures: Sequence[VulkanBorrowedTexture]

    format: int
    """The images' VkFormat."""

    final_layout: int
    """The VkImageLayout that each frame leaves its image in."""

    owner: VulkanImageOwner = VulkanImageOwner.GRAPHICS_QUEUE

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            context=VulkanContextDescriptor._from_native(raw["context"]),
            textures=tuple(
                VulkanBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            format=raw["format"],
            final_layout=raw["final_layout"],
            owner=VulkanImageOwner(raw["owner"]),
        )


@dataclass(frozen=True, slots=True)
class VulkanContextDescriptor:
    """The Vulkan device and queue that a render session uses.

    Keep these objects alive until a detach or disposal of the session
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
    you submit to it too, set `RenderSessionAttachOptions.queue_lock`.
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
class VulkanOwnedTextureDescriptor:
    """A ring of Vulkan images that the session creates, for
    `MapHandle.attach_vulkan_owned_texture()`.

    Each frame leaves its image in the layout that
    `VulkanTextureFrame.layout` reports, owned by the queue family of
    `graphics_queue_family_index`.

    See `mln_vulkan_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent

    context: VulkanContextDescriptor

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=VulkanContextDescriptor._from_native(raw["context"]),
        )


@dataclass(frozen=True, slots=True)
class VulkanSurfaceDescriptor:
    """A Vulkan surface to present to, for `MapHandle.attach_vulkan_surface()`
    and `RenderSessionHandle.set_vulkan_surface_target()`.

    The device must have VK_KHR_swapchain enabled, and the queue family of
    `graphics_queue_family_index` must support presenting to the surface. The
    session creates the swapchain. It uses the surface until a replacement or
    release of the target completes, a detach or disposal of the session
    completes, or an abandon of it returns.

    See `mln_vulkan_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: LogicalExtent

    context: VulkanContextDescriptor

    surface: int
    """Your VkSurfaceKHR."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=VulkanContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
        )


@dataclass(frozen=True, slots=True)
class VulkanTextureFrame:
    """The Vulkan image of an acquired frame.

    See `mln_vulkan_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int

    width: int

    height: int

    scale_factor: float

    frame_id: int

    slot: int
    """The image's index in the ring. For a borrowed ring, it indexes the
    descriptor's `textures`.
    """

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

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            slot=raw["slot"],
            image=raw["image"],
            image_view=raw["image_view"],
            device=raw["device"],
            format=raw["format"],
            layout=raw["layout"],
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
class WebGlContextDescriptor:
    """The WebGL context or canvas that a render session uses.

    See `mln_web_gl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    kind: WebGlContextKind = WebGlContextKind.EXISTING

    context: int = 0
    """Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context."""

    canvas_selector: str = ""
    """The canvas's CSS selector, for a transferred canvas."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=WebGlContextKind(raw["kind"]),
            context=raw["context"],
            canvas_selector=raw["canvas_selector"],
        )


@dataclass(frozen=True, slots=True)
class WglContextDescriptor:
    """The WGL objects that a render session makes its OpenGL context from.

    See `mln_wgl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    device_context: int
    """An `HDC`."""

    share_context: int = 0
    """Your `HGLRC`, for a shared context."""

    get_proc_address: int = 0
    """Your `wglGetProcAddress`, or null to use the system's."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            device_context=raw["device_context"],
            share_context=raw["share_context"],
            get_proc_address=raw["get_proc_address"],
        )


class CameraTransitionEndCallback(Protocol):
    """Receives the end of a camera change.

    See `mln_camera_transition_end_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    def __call__(self, end: CameraTransitionEnd, /) -> None: ...


class CustomSourceCancelTileCallback(Protocol):
    """Reports that a custom source no longer needs a tile.

    See `mln_custom_source_cancel_tile_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    def __call__(self, tile_id: CanonicalTileId, /) -> None: ...


class CustomSourceFetchTileCallback(Protocol):
    """Asks for the data of a tile in a custom source.

    Return quickly. Once the data is ready, pass it to
    `MapHandle.set_custom_geometry_source_tile_data()` or
    `MapHandle.set_custom_vector_source_tile_data()`. If it fails to load,
    report that with `MapHandle.set_custom_geometry_source_tile_error()` or
    `MapHandle.set_custom_vector_source_tile_error()`.

    See `mln_custom_source_fetch_tile_callback` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    def __call__(self, tile_id: CanonicalTileId, /) -> None: ...


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
    `ResourceTransformRequestHandle.complete()` or release `request` to keep
    `url`. You can answer now or later, from any thread.

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
