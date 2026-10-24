"""Generated from the C headers by tools/bindgen. Do not edit."""

from __future__ import annotations

from collections.abc import Buffer, Callable, Sequence
from dataclasses import dataclass, field
from enum import IntFlag
from typing import TYPE_CHECKING

from ._enum import UnknownIntEnum
from ._operation import _wrap_response

if TYPE_CHECKING:
    from ._generated_owners import (
        HttpHeaderTransformResponseScope,
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
class OpenglContextDescriptorWglVariant:
    value: WglContextDescriptor
    _tag = 1

    @classmethod
    def _from_native(cls, raw):
        return cls(WglContextDescriptor._from_native(raw))


@dataclass(frozen=True, slots=True)
class OpenglContextDescriptorEglVariant:
    value: EglContextDescriptor
    _tag = 2

    @classmethod
    def _from_native(cls, raw):
        return cls(EglContextDescriptor._from_native(raw))


@dataclass(frozen=True, slots=True)
class OpenglContextDescriptorWebglVariant:
    value: WebglContextDescriptor
    _tag = 3

    @classmethod
    def _from_native(cls, raw):
        return cls(WebglContextDescriptor._from_native(raw))


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
    _tag = 4

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventTileAction._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionStatusVariant:
    value: RuntimeEventOfflineRegionStatus
    _tag = 5

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionStatus._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionResponseErrorVariant:
    value: RuntimeEventOfflineRegionResponseError
    _tag = 6

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionResponseError._from_native(raw))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionTileCountLimitVariant:
    value: RuntimeEventOfflineRegionTileCountLimit
    _tag = 7

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventOfflineRegionTileCountLimit._from_native(raw))


class AmbientCacheOperation(UnknownIntEnum):
    RESET_DATABASE = 1
    PACK_DATABASE = 2
    INVALIDATE = 3
    CLEAR = 4


class CameraChangeMode(UnknownIntEnum):
    """Camera change kinds reported by camera will-change and did-change events.

    See `mln_camera_change_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    IMMEDIATE = 0
    ANIMATED = 1


class CameraTransitionOutcome(UnknownIntEnum):
    """How the transitions of one camera command ended.

    See `mln_camera_transition_outcome` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    COMPLETED = 0
    CANCELLED = 1
    CLOSED = 2


class CameraUpdateMode(UnknownIntEnum):
    """Camera transition behavior for `mln_camera_update`.

    See `mln_camera_update_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    JUMP = 0
    EASE = 1
    FLY = 2


class CommandDisposition(UnknownIntEnum):
    """Terminal dispositions reported by command completions.

    See `mln_command_disposition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/completion_8h.html).
    """

    COMMITTED = 0
    SUPERSEDED = 1
    FAILED = 2
    CANCELLED = 3


class ConstrainMode(UnknownIntEnum):
    """Map constraint modes used by `mln_map_viewport_options`.

    See `mln_constrain_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    NONE = 0
    HEIGHT_ONLY = 1
    WIDTH_AND_HEIGHT = 2
    SCREEN = 3


class FrameDemandFlag(IntFlag):
    """Frame-demand policy bits.

    See `mln_frame_demand_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    IF_NEEDED = 1
    PRESENT = 2
    WAIT_FOR_UPDATE = 4


class GesturePhase(UnknownIntEnum):
    """Gesture boundary carried atomically with a camera update or delta.

    See `mln_gesture_phase` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    NONE = 0
    BEGIN = 1
    UPDATE = 2
    END = 3
    CANCEL = 4


class GpuSyncKind(UnknownIntEnum):
    """Synchronization payload kind for acquired texture frames.

    See `mln_gpu_sync_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    CPU_COMPLETE = 0
    OPENGL_FENCE = 3


class LogEvent(UnknownIntEnum):
    """Log event categories emitted by MapLibre Native.

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
    """Log severity values emitted by MapLibre Native.

    See `mln_log_severity` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    INFO = 1
    WARNING = 2
    ERROR = 3


class LogSeverityMask(IntFlag):
    """Bitmask values for log severities dispatched asynchronously.

    See `mln_log_severity_mask` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    INFO = 2
    WARNING = 4
    ERROR = 8
    DEFAULT = 6
    ALL = 14


class MapDebugOption(IntFlag):
    """Debug overlay mask values for `mln_map_set_debug_options()`.

    See `mln_map_debug_option` in the
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
    """Map rendering modes used when creating a map.

    See `mln_map_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    CONTINUOUS = 0
    STATIC = 1
    TILE = 2


class NetworkStatus(UnknownIntEnum):
    ONLINE = 1
    OFFLINE = 2


class NorthOrientation(UnknownIntEnum):
    """Map north orientation values used by `mln_map_viewport_options`.

    See `mln_north_orientation` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    UP = 0
    RIGHT = 1
    DOWN = 2
    LEFT = 3


class OfflineRegionDownloadState(UnknownIntEnum):
    INACTIVE = 0
    ACTIVE = 1


class OpenglClientApi(UnknownIntEnum):
    """OpenGL client API a dedicated EGL session creates its context for.

    See `mln_opengl_client_api` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    UNSPECIFIED = 0
    GL = 1
    GLES = 2


class OpenglContextOwnership(UnknownIntEnum):
    """How a session's OpenGL context relates to its driver thread and host
    graphics state.

    See `mln_opengl_context_ownership` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    SHARED = 0
    DEDICATED = 1


class OpenglContextProviderFlag(IntFlag):
    """OpenGL context providers supported by this build.

    See `mln_opengl_context_provider_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    WGL = 1
    EGL = 2
    WEBGL = 4


class RenderAbandonDisposition(UnknownIntEnum):
    """What abandon did with a session's graphics resources.

    See `mln_render_abandon_disposition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    CLEAN = 0
    QUARANTINED = 1


class RenderBackendFlag(IntFlag):
    """Render backend support flags reported by this native library build.

    See `mln_render_backend_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    METAL = 1
    VULKAN = 2
    OPENGL = 4
    WEBGPU = 8


class RenderDriverKind(UnknownIntEnum):
    """Execution placement for one render session.

    See `mln_render_driver_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    CORE_WORKER = 1
    CALLER_GRAPHICS_THREAD = 2


class RenderMode(UnknownIntEnum):
    """Render modes reported by render observer events.

    See `mln_render_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    PARTIAL = 0
    FULL = 1


class RenderResult(UnknownIntEnum):
    """Terminal disposition of one accepted frame demand.

    See `mln_render_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    RENDERED = 0
    NO_UPDATE = 1
    TARGET_NOT_READY = 3
    SUPERSEDED = 4


class RenderSessionCapabilityFlag(IntFlag):
    """Optional render-session capabilities.

    See `mln_render_session_capability_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    FRAME_ACQUISITION = 1
    READBACK = 2
    CONSUMER_SYNC = 4
    PRESENTATION = 8


class RenderSessionState(UnknownIntEnum):
    """Render-session lifecycle visible in snapshots.

    See `mln_render_session_state` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    ATTACHING = 1
    ATTACHED = 2
    DETACHING = 3
    DETACHED = 4
    TARGET_LOST = 5
    ABANDONED = 6


class ResourceErrorReason(UnknownIntEnum):
    NONE = 0
    NOT_FOUND = 1
    SERVER = 2
    CONNECTION = 3
    RATE_LIMIT = 4
    OTHER = 5


class ResourceKind(UnknownIntEnum):
    UNKNOWN = 0
    STYLE = 1
    SOURCE = 2
    TILE = 3
    GLYPHS = 4
    SPRITE_IMAGE = 5
    SPRITE_JSON = 6
    IMAGE = 7


class ResourceKindMask(IntFlag):
    """The resource kinds that a route or rule matches, one bit per
    `mln_resource_kind` value. Registration requires at least one known bit.
    `MLN_RESOURCE_KIND_MASK_ALL` selects every resource kind.

    See `mln_resource_kind_mask` in the
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
    ALL = 0
    CACHE_ONLY = 1
    NETWORK_ONLY = 2


class ResourcePriority(UnknownIntEnum):
    REGULAR = 0
    LOW = 1


class ResourceResponseStatus(UnknownIntEnum):
    """How a resource provider answered a request.

    See `mln_resource_response_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    OK = 0
    ERROR = 1
    NO_CONTENT = 2
    NOT_MODIFIED = 3


class ResourceStoragePolicy(UnknownIntEnum):
    PERMANENT = 0
    VOLATILE = 1


class ResourceUsage(UnknownIntEnum):
    ONLINE = 0
    OFFLINE = 1


class RuntimeEventMask(IntFlag):
    """Bit values for the map and runtime event subscription masks.

    See `mln_runtime_event_mask` in the
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
    MAP_RENDER_ERROR = 1024
    MAP_RENDER_FRAME_STARTED = 8192
    MAP_RENDER_FRAME_FINISHED = 16384
    MAP_RENDER_MAP_STARTED = 32768
    MAP_RENDER_MAP_FINISHED = 65536
    MAP_STYLE_IMAGE_MISSING = 131072
    MAP_TILE_ACTION = 262144
    OFFLINE_REGION_STATUS_CHANGED = 524288
    OFFLINE_REGION_RESPONSE_ERROR = 1048576
    OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 2097152
    MAP_SOURCE_CHANGED = 8388608
    ALL_MAP_EVENTS = 8906238
    ALL_RUNTIME_EVENTS = 3670016
    ALL = 12576254


class RuntimeEventSourceType(UnknownIntEnum):
    """Source kinds used by `mln_runtime_event.source_type`.

    See `mln_runtime_event_source_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    RUNTIME = 0
    MAP = 1


class RuntimeEventType(UnknownIntEnum):
    """Runtime event types carried by `mln_runtime_event.type`.

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
    MAP_RENDER_ERROR = 10
    MAP_RENDER_FRAME_STARTED = 13
    MAP_RENDER_FRAME_FINISHED = 14
    MAP_RENDER_MAP_STARTED = 15
    MAP_RENDER_MAP_FINISHED = 16
    MAP_STYLE_IMAGE_MISSING = 17
    MAP_TILE_ACTION = 18
    OFFLINE_REGION_STATUS_CHANGED = 19
    OFFLINE_REGION_RESPONSE_ERROR = 20
    OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21
    MAP_SOURCE_CHANGED = 23


class Status(UnknownIntEnum):
    """Status values returned by status-returning functions.

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
    """Style expression types that a parsed expression may be required to have.

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
    """How a stretchable image fits text along one axis.

    See `mln_style_image_text_fit` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    STRETCH_OR_SHRINK = 0
    STRETCH_ONLY = 1
    PROPORTIONAL = 2


class StyleLayerVisibility(UnknownIntEnum):
    """Layer visibility values used by the visibility setter and layer info.

    See `mln_style_layer_visibility` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    VISIBLE = 0
    NONE = 1


class StyleRasterDemEncoding(UnknownIntEnum):
    """DEM raster encoding values.

    See `mln_style_raster_dem_encoding` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MAPBOX = 0
    TERRARIUM = 1


class StyleSourceType(UnknownIntEnum):
    """Style source type values returned by source metadata queries.

    See `mln_style_source_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    UNKNOWN = 0
    VECTOR = 1
    RASTER = 2
    RASTER_DEM = 3
    GEOJSON = 4
    IMAGE = 5
    VIDEO = 6
    ANNOTATIONS = 7
    CUSTOM_VECTOR = 8
    CUSTOM_MVT_VECTOR = 9


class StyleTileScheme(UnknownIntEnum):
    """Tile URL coordinate scheme values.

    See `mln_style_tile_scheme` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    XYZ = 0
    TMS = 1


class StyleVectorTileEncoding(UnknownIntEnum):
    """Vector tile encoding values.

    See `mln_style_vector_tile_encoding` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MVT = 0
    MLT = 1


class TileLodMode(UnknownIntEnum):
    """Tile LOD algorithms used by `mln_map_tile_options`.

    See `mln_tile_lod_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    DEFAULT = 0
    DISTANCE = 1


class TileOperation(UnknownIntEnum):
    """Tile operations reported by tile observer events.

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


class UrlMatchFlags(IntFlag):
    """How a rule or route compares its url against a request URL.

    See `mln_url_match_flags` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    EXACT = 0
    GLOB = 1


class ViewportMode(UnknownIntEnum):
    """Viewport orientation modes used by `mln_map_viewport_options`.

    See `mln_viewport_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    DEFAULT = 0
    FLIPPED_Y = 1


class WebglContextKind(UnknownIntEnum):
    """WebGL context placement.

    See `mln_webgl_context_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    EXISTING = 0
    TRANSFERRED_CANVAS = 1


@dataclass(frozen=True, slots=True)
class AnimationOptions:
    """Optional animation controls for camera transitions.

    See `mln_animation_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    duration_ms: float | None = None
    """Duration in milliseconds. Must be finite, non-negative, and short enough
    to represent as a duration.
    """

    velocity: float | None = None
    """Average fly velocity in screenfuls per second. Must be positive and
    finite; defaults to 1.2 when omitted. A flight whose derived duration is
    too long to represent fails its completion with
    `MLN_STATUS_NATIVE_ERROR`. An explicit duration overrides velocity.
    """

    min_zoom: float | None = None
    """Peak zoom for flyTo transitions."""

    easing: UnitBezier | None = None

    transition_id: int | None = None
    """Caller-chosen identity of the command that these options animate, which
    `mln_map_cancel_camera_transition()` matches.
    """

    end_handler: CameraTransitionHandler = field(
        default_factory=lambda: AnimationOptions.default().end_handler
    )
    """Reports the end of the command's transitions. Disabled by default."""

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_animation_options())


@dataclass(frozen=True, slots=True)
class BoundOptions:
    """Optional map camera constraint fields.

    See `mln_bound_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    bounds: LatLngBounds | None = None
    """Read when fields contains `MLN_BOUND_OPTION_BOUNDS`."""

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_bound_options())


@dataclass(frozen=True, slots=True)
class CameraDelta:
    """One atomic relative camera update.

    See `mln_camera_delta` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    offset: ScreenPoint | None = None
    """Pan in logical map pixels; the content moves by this offset."""

    scale: float | None = None
    """Positive zoom factor; 2 zooms in one level."""

    bearing: float | None = None
    """Degrees added to the bearing."""

    pitch: float | None = None
    """Degrees added to the pitch; positive tilts further from straight down."""

    anchor: ScreenPoint | None = None
    """Screen point in logical map pixels that scale, bearing, and pitch keep
    fixed.
    """

    animation: AnimationOptions = field(
        default_factory=lambda: CameraDelta.default().animation
    )

    gesture_phase: GesturePhase = field(
        default_factory=lambda: CameraDelta.default().gesture_phase
    )

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_camera_delta())


@dataclass(frozen=True, slots=True)
class CameraFitOptions:
    """Optional fitting controls for camera-for-viewport queries.

    See `mln_camera_fit_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    padding: EdgeInsets | None = None

    bearing: float | None = None

    pitch: float | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            padding=_maybe(EdgeInsets._from_native, raw["padding"]),
            bearing=raw["bearing"],
            pitch=raw["pitch"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_camera_fit_options())


@dataclass(frozen=True, slots=True)
class CameraOptions:
    """Camera fields used by snapshots and camera updates.

    See `mln_camera_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    center: LatLng | None = None

    center_altitude: float | None = None

    padding: EdgeInsets | None = None

    anchor: ScreenPoint | None = None
    """Optional screen-space focal point in logical map pixels."""

    zoom: float | None = None

    bearing: float | None = None

    pitch: float | None = None

    roll: float | None = None

    field_of_view: float | None = None

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_camera_options())


@dataclass(frozen=True, slots=True)
class CameraQueryResult:
    """Camera result borrowed for an ordered camera-query completion.

    See `mln_camera_query_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    generation: int

    camera: CameraOptions

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            camera=CameraOptions._from_native(raw["camera"]),
        )


@dataclass(frozen=True, slots=True)
class CameraTransitionEnd:
    """The end of one camera command's transitions, borrowed for the callback.

    See `mln_camera_transition_end` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    outcome: CameraTransitionOutcome
    """One of `mln_camera_transition_outcome`."""

    generation: int
    """Generation of the published snapshot that shows the camera where the
    transitions left it. The map events of the change that ended them carry
    this generation and are queued before the callback runs. Zero for
    `MLN_CAMERA_TRANSITION_OUTCOME_CLOSED`.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            outcome=CameraTransitionOutcome(raw["outcome"]),
            generation=raw["generation"],
        )


@dataclass(frozen=True, slots=True)
class CameraTransitionHandler:
    """Callback or queue state that one camera command copies to report the end
    of its transitions.

    See `mln_camera_transition_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    callback: Callable[[CameraTransitionEnd], None] | None = None

    def _invoke_callback(self, end):
        callback = self.callback
        assert callback is not None
        return callback(CameraTransitionEnd._from_native(end))


@dataclass(frozen=True, slots=True)
class CameraUpdate:
    """One atomic absolute camera update.

    See `mln_camera_update` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    mode: CameraUpdateMode = field(default_factory=lambda: CameraUpdate.default().mode)

    camera: CameraOptions = field(default_factory=lambda: CameraUpdate.default().camera)

    animation: AnimationOptions = field(
        default_factory=lambda: CameraUpdate.default().animation
    )

    gesture_phase: GesturePhase = field(
        default_factory=lambda: CameraUpdate.default().gesture_phase
    )

    @classmethod
    def _from_native(cls, raw):
        return cls(
            mode=CameraUpdateMode(raw["mode"]),
            camera=CameraOptions._from_native(raw["camera"]),
            animation=AnimationOptions._from_native(raw["animation"]),
            gesture_phase=GesturePhase(raw["gesture_phase"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_camera_update())


@dataclass(frozen=True, slots=True)
class CanonicalTileId:
    """Canonical tile identity used by custom geometry and custom MVT vector
    source callbacks.

    See `mln_canonical_tile_id` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    z: int

    x: int

    y: int

    @classmethod
    def _from_native(cls, raw):
        return cls(z=raw["z"], x=raw["x"], y=raw["y"])


@dataclass(frozen=True, slots=True)
class CustomGeometrySourceOptions:
    """Options for custom geometry sources.

    See `mln_custom_geometry_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    fetch_tile: Callable[[CanonicalTileId], None] | None = None
    cancel_tile: Callable[[CanonicalTileId], None] | None = None
    min_zoom: float | None = None

    max_zoom: float | None = None

    tolerance: float | None = None

    tile_size: int | None = None

    buffer: int | None = None

    clip: bool | None = None

    wrap: bool | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            tolerance=raw["tolerance"],
            tile_size=raw["tile_size"],
            buffer=raw["buffer"],
            clip=raw["clip"],
            wrap=raw["wrap"],
        )

    def _invoke_fetch_tile(self, tile_id):
        callback = self.fetch_tile
        assert callback is not None
        return callback(CanonicalTileId._from_native(tile_id))

    def _invoke_cancel_tile(self, tile_id):
        callback = self.cancel_tile
        assert callback is not None
        return callback(CanonicalTileId._from_native(tile_id))

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_custom_geometry_source_options())


@dataclass(frozen=True, slots=True)
class CustomMvtVectorSourceOptions:
    """Options for custom MVT vector sources.

    See `mln_custom_mvt_vector_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    fetch_tile: Callable[[CanonicalTileId], None] | None = None
    cancel_tile: Callable[[CanonicalTileId], None] | None = None
    min_zoom: float | None = None

    max_zoom: float | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(min_zoom=raw["min_zoom"], max_zoom=raw["max_zoom"])

    def _invoke_fetch_tile(self, tile_id):
        callback = self.fetch_tile
        assert callback is not None
        return callback(CanonicalTileId._from_native(tile_id))

    def _invoke_cancel_tile(self, tile_id):
        callback = self.cancel_tile
        assert callback is not None
        return callback(CanonicalTileId._from_native(tile_id))

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_custom_mvt_vector_source_options())


@dataclass(frozen=True, slots=True)
class EdgeInsets:
    """Screen-space inset in logical map pixels.

    See `mln_edge_insets` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    top: float

    left: float

    bottom: float

    right: float

    @classmethod
    def _from_native(cls, raw):
        return cls(
            top=raw["top"], left=raw["left"], bottom=raw["bottom"], right=raw["right"]
        )


@dataclass(frozen=True, slots=True)
class EglContextDescriptor:
    """EGL context fields shared by OpenGL render targets.

    See `mln_egl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    display: int
    """Borrowed EGLDisplay. Required and kept initialized through teardown.
    eglGetDisplay(EGL_DEFAULT_DISPLAY) returns the same display for the whole
    process, so a shared display must stay initialized until every session
    using it has detached or been destroyed. The host must not call
    eglTerminate per session.
    """

    config: int
    """Borrowed EGLConfig used to create the session context. Required. OpenGL
    texture targets require EGL_SURFACE_TYPE to include EGL_PBUFFER_BIT.
    """

    share_context: int
    """Borrowed EGLContext whose share group the session context joins. Required
    under shared ownership, where the session also takes its client API from
    this context. A dedicated session joins no share group, so it must be
    null there and names client_api instead.
    """

    client_api: OpenglClientApi
    """Client API the session creates its context for. Required under dedicated
    ownership. A shared session queries share_context for it, so this is
    ignored there.
    """

    get_proc_address: int
    """Optional eglGetProcAddress-compatible function for the host loader."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            display=raw["display"],
            config=raw["config"],
            share_context=raw["share_context"],
            client_api=OpenglClientApi(raw["client_api"]),
            get_proc_address=raw["get_proc_address"],
        )


@dataclass(frozen=True, slots=True)
class EventBatchView:
    """A borrowed span of runtime events in one delivery.

    See `mln_event_batch_view` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    events: tuple[RuntimeEvent, ...]
    """Borrowed array of event_count events in queue order."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            events=tuple(RuntimeEvent._from_native(item) for item in raw["events"])
        )


@dataclass(frozen=True, slots=True)
class FeatureStateSelector:
    """Feature-state source, feature, and key selector.

    See `mln_feature_state_selector` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    source_id: str
    """Source ID. Required and borrowed for the duration of the call."""

    source_layer_id: str | None = None
    """Optional source layer ID. Required for vector-source disambiguation."""

    feature_id: str | None = None
    """Optional feature ID. Required by set and get, optional for remove. A
    numeric feature ID matches its decimal text, such as "7" for the GeoJSON
    id 7.
    """

    state_key: str | None = None
    """Optional state key. Used only by remove and requires feature_id."""

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
    """One nonblocking request for a frame.

    See `mln_frame_demand` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    flags: FrameDemandFlag = field(default_factory=lambda: FrameDemand.default().flags)
    """A bitwise OR of `mln_frame_demand_flag` values. Defaults to
    `MLN_FRAME_DEMAND_IF_NEEDED`.
    """

    token: int = field(default_factory=lambda: FrameDemand.default().token)
    """Host identity returned with the terminal frame result."""

    @classmethod
    def _from_native(cls, raw):
        return cls(flags=FrameDemandFlag(raw["flags"]), token=raw["token"])

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_frame_demand())


@dataclass(frozen=True, slots=True)
class FreeCameraOptions:
    """Free camera position and orientation in MapLibre Native camera space.

    See `mln_free_camera_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    position: Vec3 | None = None

    orientation: Quaternion | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            position=_maybe(Vec3._from_native, raw["position"]),
            orientation=_maybe(Quaternion._from_native, raw["orientation"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_free_camera_options())


@dataclass(frozen=True, slots=True)
class GeojsonSourceOptions:
    """Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel
    tile size.

    See `mln_geojson_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    min_zoom: float | None = None
    """Minimum tiling zoom. Defaults to 0."""

    max_zoom: float | None = None
    """Maximum tiling zoom, an integer within \\[0, 24\\]. Defaults to 18."""

    tolerance: float | None = None
    """Douglas-Peucker simplification tolerance. Defaults to 0.375."""

    cluster_max_zoom: float | None = None
    """Highest zoom that clusters points. Defaults to 17."""

    cluster_properties: bytes | None = None
    """Cluster aggregation expressions keyed by property name, as a JSON object
    whose members follow the MapLibre Style Spec clusterProperties form. The
    UTF-8 bytes are borrowed for the call.
    """

    buffer: int | None = None
    """Tile buffer in pixels within \\[0, 512\\]. Defaults to 128."""

    cluster_radius: int | None = None
    """Cluster radius in pixels within \\[0, 4095\\]. Defaults to 50."""

    cluster_min_points: int | None = None
    """Points required to form a cluster. Defaults to 2."""

    line_metrics: bool | None = None
    """Adds line distance metrics to line features. Defaults to false."""

    cluster: bool | None = None
    """Clusters point features. Defaults to false."""

    synchronous_tiling: bool | None = None
    """Tiles the data for the next render update instead of in the background.
    Defaults to false.
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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_geojson_source_options())


@dataclass(frozen=True, slots=True)
class GpuSync:
    """Backend synchronization copied by frame access and release calls.

    See `mln_gpu_sync` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    kind: GpuSyncKind = field(default_factory=lambda: GpuSync.default().kind)
    """One `mln_gpu_sync_kind` value."""

    object: int = field(default_factory=lambda: GpuSync.default().object)
    """Bit pattern of the GLsync pointer."""

    @classmethod
    def _from_native(cls, raw):
        return cls(kind=GpuSyncKind(raw["kind"]), object=raw["object"])

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_gpu_sync())


@dataclass(frozen=True, slots=True)
class HttpHeader:
    """One HTTP header that a header rule supplies.

    See `mln_http_header` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    name: str

    value: str

    @classmethod
    def _from_native(cls, raw):
        return cls(name=raw["name"], value=raw["value"])


@dataclass(frozen=True, slots=True)
class HttpHeaderRule:
    """One rule of a native HTTP header transform.

    See `mln_http_header_rule` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    kinds: ResourceKindMask = field(
        default_factory=lambda: HttpHeaderRule.default().kinds
    )
    """The resource kinds to match."""

    flags: UrlMatchFlags = field(default_factory=lambda: HttpHeaderRule.default().flags)
    """A bitwise OR of `mln_url_match_flags` values."""

    url: str = field(default_factory=lambda: HttpHeaderRule.default().url)
    """The URL or glob pattern to compare."""

    headers: Sequence[HttpHeader] = field(
        default_factory=lambda: HttpHeaderRule.default().headers
    )
    """The headers to set. Registration validates each one as
    `mln_http_header_transform_response_set_header()` does.
    """

    def __post_init__(self):
        object.__setattr__(self, "headers", _sequence(self.headers))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kinds=ResourceKindMask(raw["kinds"]),
            flags=UrlMatchFlags(raw["flags"]),
            url=raw["url"],
            headers=tuple(HttpHeader._from_native(item) for item in raw["headers"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_http_header_rule())


@dataclass(frozen=True, slots=True)
class HttpHeaderTransform:
    """HTTP header transform state: a callback, or native header rules.

    See `mln_http_header_transform` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: (
        Callable[[ResourceKind, str, HttpHeaderTransformResponseScope], Status | None]
        | None
    ) = None
    rules: Sequence[HttpHeaderRule] = ()
    """Header rules for a transform without a callback; empty with one."""

    def __post_init__(self):
        object.__setattr__(self, "rules", _sequence(self.rules))

    def _invoke_callback(self, kind, url, out_response):
        callback = self.callback
        assert callback is not None
        return callback(
            ResourceKind(kind),
            url,
            _wrap_response(out_response, "HttpHeaderTransformResponseScope"),
        )


@dataclass(frozen=True, slots=True)
class ImageContent:
    """Content-box insets in image pixels, measured from the image's top-left.

    See `mln_image_content` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    left: float

    top: float

    right: float

    bottom: float

    @classmethod
    def _from_native(cls, raw):
        return cls(
            left=raw["left"], top=raw["top"], right=raw["right"], bottom=raw["bottom"]
        )


@dataclass(frozen=True, slots=True)
class ImageStretch:
    """One stretchable interval along an image axis, in image pixels.

    See `mln_image_stretch` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    from_: float

    to: float

    @classmethod
    def _from_native(cls, raw):
        return cls(from_=raw["from_"], to=raw["to"])


@dataclass(frozen=True, slots=True)
class LatLng:
    """Geographic coordinate in degrees used by map and projection APIs.

    See `mln_lat_lng` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    latitude: float
    """Latitude in degrees, finite and within \\[-90, 90\\] on input."""

    longitude: float
    """Longitude in degrees. Input longitude must be finite."""

    @classmethod
    def _from_native(cls, raw):
        return cls(latitude=raw["latitude"], longitude=raw["longitude"])


@dataclass(frozen=True, slots=True)
class LatLngBounds:
    """Geographic bounds in degrees.

    See `mln_lat_lng_bounds` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    southwest: LatLng

    northeast: LatLng

    @classmethod
    def _from_native(cls, raw):
        return cls(
            southwest=LatLng._from_native(raw["southwest"]),
            northeast=LatLng._from_native(raw["northeast"]),
        )


@dataclass(frozen=True, slots=True)
class LogHandler:
    """Process-global log callback or queue state.

    See `mln_log_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
    """

    callback: Callable[[LogSeverity, LogEvent, int, str], int | None] | None = None

    def _invoke_callback(self, severity, event, code, message):
        callback = self.callback
        assert callback is not None
        return callback(LogSeverity(severity), LogEvent(event), code, message)


@dataclass(frozen=True, slots=True)
class LogicalExtent:
    """Logical extent in UI pixels and the device-pixel scale.

    See `mln_logical_extent` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    width: int = field(default_factory=lambda: LogicalExtent.default().width)
    """Width in UI pixels. Defaults to 256."""

    height: int = field(default_factory=lambda: LogicalExtent.default().height)
    """Height in UI pixels. Defaults to 256."""

    scale_factor: float = field(
        default_factory=lambda: LogicalExtent.default().scale_factor
    )
    """Device pixels per UI pixel. Defaults to 1.0."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"], height=raw["height"], scale_factor=raw["scale_factor"]
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_logical_extent())


@dataclass(frozen=True, slots=True)
class MapOptions:
    """Options used when creating a map.

    See `mln_map_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    initial_extent: LogicalExtent = field(
        default_factory=lambda: MapOptions.default().initial_extent
    )
    """Initial logical extent. Width and height must be nonzero, and
    scale_factor must be finite and positive. scale_factor is fixed for the
    map's lifetime.
    """

    map_mode: MapMode = field(default_factory=lambda: MapOptions.default().map_mode)
    """One of `mln_map_mode`. Defaults to `MLN_MAP_MODE_CONTINUOUS`."""

    fast_pfor_enabled: bool = field(
        default_factory=lambda: MapOptions.default().fast_pfor_enabled
    )
    """Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
    encodings. Defaults to false.
    """

    event_mask: RuntimeEventMask = field(
        default_factory=lambda: MapOptions.default().event_mask
    )
    """Map-originated event types this map queues, as a bitwise OR of
    `mln_runtime_event_mask` values.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            initial_extent=LogicalExtent._from_native(raw["initial_extent"]),
            map_mode=MapMode(raw["map_mode"]),
            fast_pfor_enabled=raw["fast_pfor_enabled"],
            event_mask=RuntimeEventMask(raw["event_mask"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_map_options())


@dataclass(frozen=True, slots=True)
class MapSnapshot:
    """Immutable map state copied from the latest published generation.

    See `mln_map_snapshot` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    debug_options: MapDebugOption
    """Debug overlay mask of `mln_map_debug_option` values."""

    generation: int

    fully_loaded: bool
    """True once every requested style and tile resource finished loading."""

    rendering_stats_view_enabled: bool

    gesture_in_progress: bool
    """True while the map is inside a gesture."""

    transition_in_progress: bool
    """True while a camera command with a duration is still animating."""

    event_mask: RuntimeEventMask

    latest_render_update_generation: int
    """Generation of the latest render update the map published. A rendered
    frame at or past it draws map state that includes every command this
    snapshot observes, though animations and resource loads finish in later
    frames.
    """

    tile: MapTileOptions

    bounds: BoundOptions

    free_camera: FreeCameraOptions

    view: MapView
    """The map's camera, extent, projection mode, and viewport options.
    `mln_map_projection_create()` converts coordinates for this view.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            debug_options=MapDebugOption(raw["debug_options"]),
            generation=raw["generation"],
            fully_loaded=raw["fully_loaded"],
            rendering_stats_view_enabled=raw["rendering_stats_view_enabled"],
            gesture_in_progress=raw["gesture_in_progress"],
            transition_in_progress=raw["transition_in_progress"],
            event_mask=RuntimeEventMask(raw["event_mask"]),
            latest_render_update_generation=raw["latest_render_update_generation"],
            tile=MapTileOptions._from_native(raw["tile"]),
            bounds=BoundOptions._from_native(raw["bounds"]),
            free_camera=FreeCameraOptions._from_native(raw["free_camera"]),
            view=MapView._from_native(raw["view"]),
        )


@dataclass(frozen=True, slots=True)
class MapTileOptions:
    """Tile prefetch and LOD tuning controls.

    See `mln_map_tile_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    prefetch_zoom_delta: int | None = None
    """Prefetch zoom delta, at most 255."""

    lod_min_radius: float | None = None

    lod_scale: float | None = None

    lod_pitch_threshold: float | None = None

    lod_zoom_shift: float | None = None

    lod_mode: TileLodMode | None = None
    """One of `mln_tile_lod_mode`."""

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_map_tile_options())


@dataclass(frozen=True, slots=True)
class MapView:
    """Everything that a projection depends on: the transform that a published
    snapshot or a rendered frame describes.

    See `mln_map_view` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    camera: CameraOptions
    """The camera. Its center is unwrapped: the longitude keeps the visible
    world copy and may fall outside -180 to 180. Every field except
    `MLN_CAMERA_OPTION_ANCHOR` is set.
    """

    extent: LogicalExtent
    """Logical size and device-pixel scale. A snapshot carries the map's scale
    factor, and a frame carries the scale factor of the target that drew it.
    """

    projection_mode: ProjectionMode
    """Axonometric mode and skew, with every field set."""

    viewport: MapViewportOptions
    """North orientation, constrain mode, viewport mode, and frustum offset,
    with every field set.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            camera=CameraOptions._from_native(raw["camera"]),
            extent=LogicalExtent._from_native(raw["extent"]),
            projection_mode=ProjectionMode._from_native(raw["projection_mode"]),
            viewport=MapViewportOptions._from_native(raw["viewport"]),
        )


@dataclass(frozen=True, slots=True)
class MapViewportOptions:
    """Live map viewport and render-transform controls.

    See `mln_map_viewport_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    north_orientation: NorthOrientation | None = None
    """One of `mln_north_orientation`."""

    constrain_mode: ConstrainMode | None = None
    """One of `mln_constrain_mode`."""

    viewport_mode: ViewportMode | None = None
    """One of `mln_viewport_mode`."""

    frustum_offset: EdgeInsets | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            north_orientation=_maybe(NorthOrientation, raw["north_orientation"]),
            constrain_mode=_maybe(ConstrainMode, raw["constrain_mode"]),
            viewport_mode=_maybe(ViewportMode, raw["viewport_mode"]),
            frustum_offset=_maybe(EdgeInsets._from_native, raw["frustum_offset"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_map_viewport_options())


@dataclass(frozen=True, slots=True)
class MetalBorrowedTexture:
    """One caller-owned Metal texture of a borrowed texture ring.

    See `mln_metal_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    texture: int
    """Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required."""

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"])


@dataclass(frozen=True, slots=True)
class MetalBorrowedTextureDescriptor:
    """Metal attachment options for a borrowed texture target.

    See `mln_metal_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: MetalBorrowedTextureDescriptor.default().extent
    )
    """Logical texture extent; physical_width and physical_height give the
    texture's size. A scale_factor that differs from the map's is accepted
    and logged as a warning.
    """

    physical_width: int = field(
        default_factory=lambda: MetalBorrowedTextureDescriptor.default().physical_width
    )
    """Physical texture width in device pixels. Must be positive. Defaults to
    256.
    """

    physical_height: int = field(
        default_factory=lambda: MetalBorrowedTextureDescriptor.default().physical_height
    )
    """Physical texture height in device pixels. Must be positive. Defaults to
    256.
    """

    textures: Sequence[MetalBorrowedTexture] = field(
        default_factory=lambda: MetalBorrowedTextureDescriptor.default().textures
    )
    """The ring's textures, one per slot, in slot order. Required."""

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
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_metal_borrowed_texture_descriptor())


@dataclass(frozen=True, slots=True)
class MetalContextDescriptor:
    """Metal backend context fields shared by Metal render targets.

    See `mln_metal_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    device: int
    """`id<MTLDevice>` / `MTL::Device*`. Retained when the target requires it."""

    @classmethod
    def _from_native(cls, raw):
        return cls(device=raw["device"])


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureDescriptor:
    """Metal attachment options for an owned texture target.

    See `mln_metal_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: MetalOwnedTextureDescriptor.default().extent
    )
    """Logical texture extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: MetalContextDescriptor = field(
        default_factory=lambda: MetalOwnedTextureDescriptor.default().context
    )
    """Metal backend context. device is required."""

    pixel_format: int = field(
        default_factory=lambda: MetalOwnedTextureDescriptor.default().pixel_format
    )
    """Backend-native MTLPixelFormat value of the ring's textures: either
    MTLPixelFormatRGBA8Unorm or MTLPixelFormatBGRA8Unorm. Defaults to
    MTLPixelFormatRGBA8Unorm. Readback returns RGBA8 for either format.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=MetalContextDescriptor._from_native(raw["context"]),
            pixel_format=raw["pixel_format"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_metal_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class MetalSurfaceDescriptor:
    """Metal attachment options for a native surface.

    See `mln_metal_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: MetalSurfaceDescriptor.default().extent
    )
    """Logical surface extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: MetalContextDescriptor = field(
        default_factory=lambda: MetalSurfaceDescriptor.default().context
    )
    """Metal backend context. device is optional for Metal surfaces."""

    layer: int = field(default_factory=lambda: MetalSurfaceDescriptor.default().layer)
    """`CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=MetalContextDescriptor._from_native(raw["context"]),
            layer=raw["layer"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_metal_surface_descriptor())


@dataclass(frozen=True, slots=True)
class MetalTextureFrame:
    """Metal frame acquired from a texture ring.

    See `mln_metal_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    """Session generation that produced this frame."""

    width: int
    """Physical Metal texture width in device pixels."""

    height: int
    """Physical Metal texture height in device pixels."""

    scale_factor: float
    """UI-to-device pixel scale used for this frame."""

    frame_id: int
    """Opaque identity of this frame."""

    slot: int
    """Ring slot that holds this frame. For a borrowed target, the index of its
    texture in the descriptor's textures array.
    """

    texture: int
    """Borrowed `id<MTLTexture>` / `MTL::Texture*`. Valid until frame release."""

    device: int
    """Borrowed `id<MTLDevice>` / `MTL::Device*`. Valid until frame release."""

    pixel_format: int
    """Backend-native pixel format value. Metal uses MTLPixelFormat."""

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
    """Geometry offline region definition.

    See `mln_offline_geometry_region_definition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    style_url: str
    """Style URL. Copied during region creation."""

    geometry: bytes
    """UTF-8 GeoJSON Geometry bytes. Borrowed during region creation."""

    min_zoom: float

    max_zoom: float
    """Maximum zoom. Positive infinity lets each tile source use its own maximum
    zoom.
    """

    pixel_ratio: float

    include_ideographs: bool

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


@dataclass(frozen=True, slots=True)
class OfflineRegionDefinition:
    """Tagged offline region definition.

    See `mln_offline_region_definition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    data: (
        OfflineRegionDefinitionTilePyramidVariant
        | OfflineRegionDefinitionGeometryVariant
        | UnknownVariant
    )

    @classmethod
    def _from_native(cls, raw):
        return cls(
            data=_copy_variant(
                raw["data"],
                {
                    "tile_pyramid": OfflineRegionDefinitionTilePyramidVariant,
                    "geometry": OfflineRegionDefinitionGeometryVariant,
                },
                None,
            )
        )


@dataclass(frozen=True, slots=True)
class OfflineRegionInfo:
    """Region data delivered by an offline completion.

    See `mln_offline_region_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    id: int

    definition: OfflineRegionDefinition

    metadata: bytes
    """Metadata bytes."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            id=raw["id"],
            definition=OfflineRegionDefinition._from_native(raw["definition"]),
            metadata=raw["metadata"],
        )


@dataclass(frozen=True, slots=True)
class OfflineRegionStatus:
    """Offline region status snapshot.

    See `mln_offline_region_status` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    download_state: OfflineRegionDownloadState
    """One of `mln_offline_region_download_state`."""

    completed_resource_count: int

    completed_resource_size: int

    completed_tile_count: int

    required_tile_count: int

    completed_tile_size: int

    required_resource_count: int

    required_resource_count_is_precise: bool

    complete: bool

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
    """Tile-pyramid offline region definition.

    See `mln_offline_tile_pyramid_region_definition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    style_url: str
    """Style URL. Copied during region creation."""

    bounds: LatLngBounds

    min_zoom: float

    max_zoom: float
    """Maximum zoom. Positive infinity lets each tile source use its own maximum
    zoom.
    """

    pixel_ratio: float

    include_ideographs: bool

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
class OpenglBorrowedTexture:
    """One caller-owned OpenGL texture of a borrowed texture ring.

    See `mln_opengl_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    texture: int
    """Borrowed OpenGL texture object name. Required."""

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"])


@dataclass(frozen=True, slots=True)
class OpenglBorrowedTextureDescriptor:
    """OpenGL attachment options for a borrowed texture target.

    See `mln_opengl_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: OpenglBorrowedTextureDescriptor.default().extent
    )
    """Logical texture extent; physical_width and physical_height give the
    texture's size. A scale_factor that differs from the map's is accepted
    and logged as a warning.
    """

    physical_width: int = field(
        default_factory=lambda: OpenglBorrowedTextureDescriptor.default().physical_width
    )
    """Physical texture width in device pixels. Must be positive. Defaults to
    256.
    """

    physical_height: int = field(
        default_factory=lambda: (
            OpenglBorrowedTextureDescriptor.default().physical_height
        )
    )
    """Physical texture height in device pixels. Must be positive. Defaults to
    256.
    """

    context: OpenglContextDescriptor = field(
        default_factory=lambda: OpenglBorrowedTextureDescriptor.default().context
    )
    """Borrowed OpenGL context provider data. The textures must belong to this
    context or a context in the same share group.
    """

    textures: Sequence[OpenglBorrowedTexture] = field(
        default_factory=lambda: OpenglBorrowedTextureDescriptor.default().textures
    )
    """The ring's textures, one per slot, in slot order. Required."""

    target: int = field(
        default_factory=lambda: OpenglBorrowedTextureDescriptor.default().target
    )
    """OpenGL texture target of every texture. Must be GL_TEXTURE_2D."""

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            context=OpenglContextDescriptor._from_native(raw["context"]),
            textures=tuple(
                OpenglBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            target=raw["target"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_opengl_borrowed_texture_descriptor())


@dataclass(frozen=True, slots=True)
class OpenglContextDescriptor:
    """OpenGL backend context fields shared by OpenGL render targets.

    See `mln_opengl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    ownership: OpenglContextOwnership
    """Whether the session shares its driver thread and graphics objects with
    the host. A private EGL or WGL owned texture and a transferred WebGL
    canvas are dedicated to their core worker.
    """

    data: (
        OpenglContextDescriptorWglVariant
        | OpenglContextDescriptorEglVariant
        | OpenglContextDescriptorWebglVariant
        | UnknownVariant
    )

    @classmethod
    def _from_native(cls, raw):
        return cls(
            ownership=OpenglContextOwnership(raw["ownership"]),
            data=_copy_variant(
                raw["data"],
                {
                    "wgl": OpenglContextDescriptorWglVariant,
                    "egl": OpenglContextDescriptorEglVariant,
                    "webgl": OpenglContextDescriptorWebglVariant,
                },
                None,
            ),
        )


@dataclass(frozen=True, slots=True)
class OpenglOwnedTextureDescriptor:
    """OpenGL attachment options for an owned texture target.

    See `mln_opengl_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: OpenglOwnedTextureDescriptor.default().extent
    )
    """Logical texture extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: OpenglContextDescriptor = field(
        default_factory=lambda: OpenglOwnedTextureDescriptor.default().context
    )
    """Borrowed OpenGL context provider data. Shared ownership creates a context
    whose texture frames the host can acquire. Dedicated EGL, WGL, or
    transferred WebGL ownership creates a private core-worker context for CPU
    readback.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=OpenglContextDescriptor._from_native(raw["context"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_opengl_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class OpenglSurfaceDescriptor:
    """OpenGL attachment options for a native surface.

    See `mln_opengl_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: OpenglSurfaceDescriptor.default().extent
    )
    """Logical surface extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: OpenglContextDescriptor = field(
        default_factory=lambda: OpenglSurfaceDescriptor.default().context
    )
    """Borrowed OpenGL context provider data."""

    surface: int = field(
        default_factory=lambda: OpenglSurfaceDescriptor.default().surface
    )
    """Borrowed platform surface handle: an HDC for WGL and an EGLSurface for
    EGL, both required. Null for WebGL, whose context carries its canvas
    binding.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=OpenglContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_opengl_surface_descriptor())


@dataclass(frozen=True, slots=True)
class OpenglTextureFrame:
    """OpenGL frame acquired from a texture ring.

    See `mln_opengl_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    """Session generation that produced this frame."""

    width: int
    """Physical OpenGL texture width in device pixels."""

    height: int
    """Physical OpenGL texture height in device pixels."""

    scale_factor: float
    """UI-to-device pixel scale used for this frame."""

    frame_id: int
    """Opaque identity of this frame."""

    slot: int
    """Ring slot that holds this frame. For a borrowed target, the index of its
    texture in the descriptor's textures array.
    """

    texture: int
    """Borrowed OpenGL texture object name. Valid until frame release."""

    target: int
    """OpenGL texture target. GL_TEXTURE_2D is the expected target."""

    internal_format: int
    """OpenGL internal format, such as GL_RGBA8. Zero for a borrowed texture,
    whose format the host chose.
    """

    format: int
    """OpenGL pixel format, such as GL_RGBA. Zero for a borrowed texture."""

    type: int
    """OpenGL pixel type, such as GL_UNSIGNED_BYTE. Zero for a borrowed texture."""

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
    """Caller-owned premultiplied RGBA8 image pixels.

    See `mln_premultiplied_rgba8_image` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    width: int = field(default_factory=lambda: PremultipliedRgba8Image.default().width)

    height: int = field(
        default_factory=lambda: PremultipliedRgba8Image.default().height
    )

    stride: int = field(
        default_factory=lambda: PremultipliedRgba8Image.default().stride
    )
    """Bytes per image row. Must be at least width \\* 4."""

    pixels: Buffer = field(
        default_factory=lambda: PremultipliedRgba8Image.default().pixels
    )
    """Premultiplied RGBA8 pixels. Must not be null for a non-empty image."""

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_premultiplied_rgba8_image())


@dataclass(frozen=True, slots=True)
class ProjectedMeters:
    """Lower-level Spherical Mercator projected-meter coordinate.

    See `mln_projected_meters` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    northing: float
    """Distance measured northward from the equator, in meters."""

    easting: float
    """Distance measured eastward from the prime meridian, in meters."""

    @classmethod
    def _from_native(cls, raw):
        return cls(northing=raw["northing"], easting=raw["easting"])


@dataclass(frozen=True, slots=True)
class ProjectionMode:
    """MapLibre axonometric rendering options used for snapshots and commands.

    See `mln_projection_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    axonometric: bool | None = None
    """Enables a non-perspective axonometric render transform."""

    x_skew: float | None = None
    """X-skew factor of the axonometric transform."""

    y_skew: float | None = None
    """Y-skew factor of the axonometric transform."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            axonometric=raw["axonometric"], x_skew=raw["x_skew"], y_skew=raw["y_skew"]
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_projection_mode())


@dataclass(frozen=True, slots=True)
class Quaternion:
    """Quaternion stored as x, y, z, w components.

    See `mln_quaternion` in the
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
class QueueLock:
    """Host lock on the graphics queue that a session shares with its host,
    copied by a successful attach.

    See `mln_queue_lock` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    lock: Callable[[], None] | None = None
    unlock: Callable[[], None] | None = None

    def _invoke_lock(self):
        callback = self.lock
        assert callback is not None
        return callback()

    def _invoke_unlock(self):
        callback = self.unlock
        assert callback is not None
        return callback()

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
    disposition: RenderAbandonDisposition
    """One `mln_render_abandon_disposition` value."""

    quarantined_resource_count: int
    """Backend resource groups intentionally retained until process exit."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            disposition=RenderAbandonDisposition(raw["disposition"]),
            quarantined_resource_count=raw["quarantined_resource_count"],
        )


@dataclass(frozen=True, slots=True)
class RenderFrameBatchView:
    """A borrowed view of one owned frame-result batch.

    See `mln_render_frame_batch_view` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    results: tuple[RenderFrameResult, ...]
    """Borrowed array of result_count terminal frame results in completion
    order.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            results=tuple(
                RenderFrameResult._from_native(item) for item in raw["results"]
            )
        )


@dataclass(frozen=True, slots=True)
class RenderFrameResult:
    """Terminal result of one frame demand, held by an owned frame-result batch
    and copied by `mln_acquired_frame_get_result()`.

    See `mln_render_frame_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    disposition: RenderResult
    """One `mln_render_result` value."""

    token: int

    map_update_generation: int
    """Generation of the map render update the demand evaluated. When
    disposition is `MLN_RENDER_RESULT_RENDERED`, the frame drew that update;
    compare it with `mln_completion_result.render_update_generation` to find
    the first frame that includes a command.
    """

    generation: int
    """The session's target generation when the demand reached its result. See
    `mln_render_session_snapshot.generation`.
    """

    frame_generation: int
    """Position of the frame among the frames that the session rendered,
    counting from one. Zero unless disposition is
    `MLN_RENDER_RESULT_RENDERED`.
    """

    needs_repaint: bool
    """Whether the map asked for another frame while it rendered this one, as
    during an ongoing paint transition. Set only when disposition is
    `MLN_RENDER_RESULT_RENDERED`, and false for every other outcome. It
    matches the needs_repaint field of
    `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`. A camera transition does
    not set it: the map publishes an update for each of the transition's
    frames instead, which a render-if-needed demand renders.
    """

    view: MapView
    """The view that this frame drew, from the transform of the map update that
    it rendered. A host that displays the frame builds its projection with
    `mln_map_projection_create()` to place overlays where the frame drew
    their coordinates. Zero, with a size of zero, unless disposition is
    `MLN_RENDER_RESULT_RENDERED`.
    """

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
        )


@dataclass(frozen=True, slots=True)
class RenderSessionAttachOptions:
    """Common attachment policy copied before an attach call returns.

    See `mln_render_session_attach_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    driver: RenderDriverKind = field(
        default_factory=lambda: RenderSessionAttachOptions.default().driver
    )
    """One `mln_render_driver_kind` value. Defaults to
    `MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD`.
    """

    requested_texture_ring_depth: int = field(
        default_factory=lambda: (
            RenderSessionAttachOptions.default().requested_texture_ring_depth
        )
    )
    """Requested slot count of a session-owned texture ring, from one to
    `MLN_TEXTURE_RING_MAX_DEPTH`. Private targets grant one slot regardless
    of this value. A borrowed texture ring's depth is its texture count, so
    borrowed and other targets ignore this value. Defaults to 2, so the host
    can retain one frame while the session renders its replacement.
    """

    producer_sync_kind: GpuSyncKind = field(
        default_factory=lambda: RenderSessionAttachOptions.default().producer_sync_kind
    )
    """Producer synchronization for acquired texture frames. Defaults to
    `MLN_GPU_SYNC_CPU_COMPLETE`. `MLN_GPU_SYNC_OPENGL_FENCE` requires a
    shared EGL or WGL texture target and a caller-graphics-thread driver.
    Other targets support only CPU completion and fail an unsupported
    request.
    """

    frame_wake: Wake | NativeWake = field(
        default_factory=lambda: RenderSessionAttachOptions.default().frame_wake
    )
    """Wakes the receiver when the frame-result queue becomes nonempty."""

    driver_work_wake: Wake | NativeWake = field(
        default_factory=lambda: RenderSessionAttachOptions.default().driver_work_wake
    )
    """Wakes the graphics receiver when caller-driver work is available,
    including work that a finished driver result releases.
    """

    queue_lock: QueueLock | NativeQueueLock = field(
        default_factory=lambda: RenderSessionAttachOptions.default().queue_lock
    )
    """Host lock on the graphics queue, disabled by default. Only Vulkan targets
    accept an enabled lock; other backends fail the attach with
    `MLN_STATUS_UNSUPPORTED`.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            requested_texture_ring_depth=raw["requested_texture_ring_depth"],
            producer_sync_kind=GpuSyncKind(raw["producer_sync_kind"]),
            frame_wake=Wake(),
            driver_work_wake=Wake(),
            queue_lock=QueueLock(),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_render_session_attach_options())


@dataclass(frozen=True, slots=True)
class RenderSessionCapabilities:
    """Driver and target capabilities fixed for one attached render session.

    See `mln_render_session_capabilities` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    driver: RenderDriverKind
    """One `mln_render_driver_kind` value."""

    texture_ring_depth: int
    """Granted texture ring depth: the slot count of a session-owned ring, or
    the texture count of a borrowed one. Zero for a surface.
    """

    flags: RenderSessionCapabilityFlag
    """A bitwise OR of `mln_render_session_capability_flag` values."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            texture_ring_depth=raw["texture_ring_depth"],
            flags=RenderSessionCapabilityFlag(raw["flags"]),
        )


@dataclass(frozen=True, slots=True)
class RenderSessionSnapshot:
    """Any-thread render-session snapshot.

    See `mln_render_session_snapshot` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    state: RenderSessionState
    """One `mln_render_session_state` value."""

    extent: LogicalExtent
    """Logical extent applied by the driver."""

    generation: int
    """Increases when the session's lifecycle state, target, or applied extent
    changes. An acquired texture frame reports the value it rendered under.
    """

    map_update_generation: int
    """Generation of the latest map render update that the session has seen."""

    rendered_update_generation: int
    """Map render update generation that the latest rendered frame drew. Zero
    before the first frame after attachment or a target replacement.
    """

    frame_generation: int
    """Frame generation of the latest rendered frame, zero before the first."""

    acquired_frame_count: int
    """Acquired frames that the host has not released or discarded."""

    pending_changes: bool
    """True when a map update or an accepted resize arrived after the latest
    rendered frame.
    """

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
            pending_changes=raw["pending_changes"],
        )


@dataclass(frozen=True, slots=True)
class RenderedFeature:
    """One rendered-feature query hit borrowed for a completion callback.

    See `mln_rendered_feature` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    feature: bytes

    layer_id: str

    source_id: str | None = None

    source_layer_id: str | None = None

    state: bytes | None = None

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
    """Options for rendered feature queries.

    See `mln_rendered_feature_query_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    layer_ids: Sequence[str] | None = None
    """Optional style layer IDs. When absent, all rendered layers are queried."""

    filter: bytes | None = None
    """Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter."""

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_rendered_feature_query_options())


@dataclass(frozen=True, slots=True)
class RenderedQueryGeometry:
    """Rendered feature query geometry descriptor.

    See `mln_rendered_query_geometry` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    data: (
        RenderedQueryGeometryPointVariant
        | RenderedQueryGeometryBoxVariant
        | RenderedQueryGeometryLineStringVariant
        | UnknownVariant
    )

    @classmethod
    def _from_native(cls, raw):
        return cls(
            data=_copy_variant(
                raw["data"],
                {
                    "point": RenderedQueryGeometryPointVariant,
                    "box": RenderedQueryGeometryBoxVariant,
                    "line_string": RenderedQueryGeometryLineStringVariant,
                },
                None,
            )
        )


@dataclass(frozen=True, slots=True)
class RenderingStats:
    """Rendering statistics reported in
    `MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME`.

    See `mln_rendering_stats` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    encoding_time: float
    """Frame CPU encoding time in seconds."""

    rendering_time: float
    """Frame CPU rendering time in seconds."""

    frame_count: int
    """Number of frames rendered by the native renderer."""

    draw_call_count: int
    """Draw calls executed during the most recent frame."""

    total_draw_call_count: int
    """Total draw calls executed by the native renderer."""

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
    """Resource provider state.

    See `mln_resource_provider` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: Callable[[ResourceRequestInfo, ResourceRequestHandle], None] | None = None
    routes: Sequence[ResourceRoute] = ()
    """The requests that reach the provider; empty to claim every request."""

    def __post_init__(self):
        object.__setattr__(self, "routes", _sequence(self.routes))

    def _invoke_callback(self, request, handle):
        callback = self.callback
        assert callback is not None
        handle = _wrap_response(handle, "ResourceRequestHandle")
        try:
            return callback(ResourceRequestInfo._from_native(request), handle)
        except BaseException:
            handle.close()
            raise


@dataclass(frozen=True, slots=True)
class ResourceRange:
    """Inclusive byte range of a resource request.

    See `mln_resource_range` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    start: int
    """First byte offset of the requested range."""

    end: int
    """Last byte offset of the requested range, inclusive."""

    @classmethod
    def _from_native(cls, raw):
        return cls(start=raw["start"], end=raw["end"])


@dataclass(frozen=True, slots=True)
class ResourceRequestCancelHandler:
    """Cancel callback or queue state for one handled resource request.

    See `mln_resource_request_cancel_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: Callable[[], None] | None = None

    def _invoke_callback(self):
        callback = self.callback
        assert callback is not None
        return callback()


@dataclass(frozen=True, slots=True)
class ResourceRequestInfo:
    kind: ResourceKind

    loading_method: ResourceLoadingMethod

    priority: ResourcePriority

    usage: ResourceUsage

    storage_policy: ResourceStoragePolicy

    prior_data: bytes

    requested_url: str | None = None
    """URL entering the network layer, before tile server normalization."""

    resolved_url: str | None = None
    """URL to fetch, after resource-kind normalization against the runtime's
    tile server options and API key.
    """

    range: ResourceRange | None = None

    prior_modified_unix_ms: int | None = None

    prior_expires_unix_ms: int | None = None

    prior_etag: str | None = None

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
    """A resource provider's answer to one request.

    See `mln_resource_response` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    status: ResourceResponseStatus

    error_reason: ResourceErrorReason

    bytes: Buffer
    """Response bytes. May be null only when byte_count is 0."""

    must_revalidate: bool

    error_message: str | None = None

    modified_unix_ms: int | None = None

    expires_unix_ms: int | None = None

    etag: str | None = None

    retry_after_unix_ms: int | None = None

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
    """One request pattern that a resource provider claims.

    See `mln_resource_route` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    kinds: ResourceKindMask = field(
        default_factory=lambda: ResourceRoute.default().kinds
    )
    """The resource kinds to match."""

    flags: UrlMatchFlags = field(default_factory=lambda: ResourceRoute.default().flags)
    """A bitwise OR of `mln_url_match_flags` values."""

    url: str = field(default_factory=lambda: ResourceRoute.default().url)
    """The URL or glob pattern to compare."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kinds=ResourceKindMask(raw["kinds"]),
            flags=UrlMatchFlags(raw["flags"]),
            url=raw["url"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_resource_route())


@dataclass(frozen=True, slots=True)
class ResourceTransform:
    """Resource transform state.

    See `mln_resource_transform` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: (
        Callable[[ResourceKind, str, ResourceTransformRequestHandle], None] | None
    ) = None

    def _invoke_callback(self, kind, url, request):
        callback = self.callback
        assert callback is not None
        request = _wrap_response(request, "ResourceTransformRequestHandle")
        try:
            return callback(ResourceKind(kind), url, request)
        except BaseException:
            request.close()
            raise


@dataclass(frozen=True, slots=True)
class RuntimeEvent:
    """One delivered runtime event.

    See `mln_runtime_event` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    type: RuntimeEventType
    """One of `mln_runtime_event_type`."""

    source_type: RuntimeEventSourceType
    """One of `mln_runtime_event_source_type`."""

    source: int
    """Source handle selected by source_type: an `mln_runtime` or an `mln_map`."""

    generation: int
    """Map snapshot generation that the event reports, or zero when source_type
    is `MLN_RUNTIME_EVENT_SOURCE_RUNTIME`.
    """

    code: int
    """Secondary event detail whose meaning type selects; see
    `mln_runtime_event_type`.
    """

    message: str
    """Borrowed UTF-8 message bytes, empty when this event carries no message."""

    payload: (
        RuntimeEventRenderFrameVariant
        | RuntimeEventRenderMapVariant
        | RuntimeEventTileActionVariant
        | RuntimeEventOfflineRegionStatusVariant
        | RuntimeEventOfflineRegionResponseErrorVariant
        | RuntimeEventOfflineRegionTileCountLimitVariant
        | UnknownVariant
        | None
    )
    """Typed payload selected by payload_type."""

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
                },
                0,
            ),
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventHandler:
    """Runtime and map event delivery. Set either callback or queue.

    See `mln_runtime_event_handler` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    callback: Callable[[EventBatchView], None] | None = None

    def _invoke_callback(self, events):
        callback = self.callback
        assert callback is not None
        return callback(EventBatchView._from_native(events))


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionResponseError:
    """Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR`.

    See `mln_runtime_event_offline_region_response_error` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    region_id: int

    reason: ResourceErrorReason
    """One of `mln_resource_error_reason`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            region_id=raw["region_id"], reason=ResourceErrorReason(raw["reason"])
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionStatus:
    """Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED`.

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
    """Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`.

    See `mln_runtime_event_offline_region_tile_count_limit` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    region_id: int

    limit: int

    @classmethod
    def _from_native(cls, raw):
        return cls(region_id=raw["region_id"], limit=raw["limit"])


@dataclass(frozen=True, slots=True)
class RuntimeEventRenderFrame:
    """Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`.

    See `mln_runtime_event_render_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    mode: RenderMode
    """One of `mln_render_mode`."""

    needs_repaint: bool
    """Whether MapLibre needs another frame after this one."""

    placement_changed: bool
    """Whether symbol placement changed during this frame."""

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
    """Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED`.

    See `mln_runtime_event_render_map` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    mode: RenderMode
    """One of `mln_render_mode`."""

    @classmethod
    def _from_native(cls, raw):
        return cls(mode=RenderMode(raw["mode"]))


@dataclass(frozen=True, slots=True)
class RuntimeEventTileAction:
    """Payload for `MLN_RUNTIME_EVENT_MAP_TILE_ACTION`.

    See `mln_runtime_event_tile_action` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    operation: TileOperation
    """One of `mln_tile_operation`."""

    tile_id: TileId

    @classmethod
    def _from_native(cls, raw):
        return cls(
            operation=TileOperation(raw["operation"]),
            tile_id=TileId._from_native(raw["tile_id"]),
        )


@dataclass(frozen=True, slots=True)
class RuntimeOptions:
    """Options used when creating a runtime.

    See `mln_runtime_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    asset_path: str | None = None
    """Directory root for asset:// URLs. Copied during runtime creation. Empty
    selects `/android_asset` on Android and `.` elsewhere.
    """

    cache_path: str | None = None
    """Cache database path. Copied during runtime creation."""

    @classmethod
    def _from_native(cls, raw):
        return cls(asset_path=raw["asset_path"], cache_path=raw["cache_path"])

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_runtime_options())


@dataclass(frozen=True, slots=True)
class ScreenBox:
    """Screen-space box in logical map pixels.

    See `mln_screen_box` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    min: ScreenPoint

    max: ScreenPoint

    @classmethod
    def _from_native(cls, raw):
        return cls(
            min=ScreenPoint._from_native(raw["min"]),
            max=ScreenPoint._from_native(raw["max"]),
        )


@dataclass(frozen=True, slots=True)
class ScreenLineString:
    """Screen-space line string in logical map pixels.

    See `mln_screen_line_string` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    points: Sequence[ScreenPoint]
    """Points. Null only when point_count is 0."""

    def __post_init__(self):
        object.__setattr__(self, "points", _sequence(self.points))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            points=tuple(ScreenPoint._from_native(item) for item in raw["points"])
        )


@dataclass(frozen=True, slots=True)
class ScreenPoint:
    """Screen-space point in logical map pixels, from the top-left of the
    viewport with y growing downward.

    See `mln_screen_point` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: float

    y: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"])


@dataclass(frozen=True, slots=True)
class SourceFeature:
    """One source-feature query hit borrowed for a completion callback.

    See `mln_source_feature` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    feature: bytes

    source_layer_id: str | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(feature=raw["feature"], source_layer_id=raw["source_layer_id"])


@dataclass(frozen=True, slots=True)
class SourceFeatureQueryOptions:
    """Options for source feature queries.

    See `mln_source_feature_query_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    source_layer_ids: Sequence[str] | None = None
    """Optional source-layer IDs. Required by vector sources; ignored by
    GeoJSON.
    """

    filter: bytes | None = None
    """Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter."""

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_source_feature_query_options())


@dataclass(frozen=True, slots=True)
class StyleExpressionInfo:
    """What a parsed expression depends on.

    See `mln_style_expression_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    is_feature_constant: bool
    """Whether the result is the same for every feature and feature state."""

    is_zoom_constant: bool
    """Whether the result is the same at every zoom."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            is_feature_constant=raw["is_feature_constant"],
            is_zoom_constant=raw["is_zoom_constant"],
        )


@dataclass(frozen=True, slots=True)
class StyleExpressionInputs:
    """What one evaluation reads.

    See `mln_style_expression_inputs` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    zoom: float | None = None
    """The value of the zoom expression."""

    color_ramp_parameter: float | None = None
    """The value of the heatmap-density and line-progress expressions, and of
    the elevation expression in a color-relief layer.
    """

    elevation: float | None = None
    """The value of the elevation expression outside a color-relief layer."""

    feature: bytes | None = None
    """One UTF-8 GeoJSON Feature that feature data expressions read."""

    feature_state: bytes | None = None
    """A UTF-8 JSON object that the feature-state expression reads."""

    global_state: bytes | None = None
    """A UTF-8 JSON object that the global-state expression reads."""

    canonical_tile_id: CanonicalTileId | None = None
    """The tile that holds the feature, which the within and distance
    expressions need to place its geometry.
    """

    available_images: Sequence[str] = field(
        default_factory=lambda: StyleExpressionInputs.default().available_images
    )
    """The image IDs that the image expression reports as available. The
    expression reports any other ID as unavailable.
    """

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_style_expression_inputs())


@dataclass(frozen=True, slots=True)
class StyleExpressionOptions:
    """Options for parsing a style expression.

    See `mln_style_expression_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    expected_type: StyleExpressionType | None = None
    """The type that the expression must produce. A string type coerces the
    result to a string, as a string layer property does; every other type
    asserts it. Without it, the expression keeps the type it infers.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            expected_type=_maybe(StyleExpressionType._from_native, raw["expected_type"])
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_style_expression_options())


@dataclass(frozen=True, slots=True)
class StyleExpressionType:
    """A style expression type.

    See `mln_style_expression_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    """

    kind: StyleExpressionTypeKind
    """One of `mln_style_expression_type_kind`."""

    item_kind: StyleExpressionTypeKind
    """For an array, its items' kind; ignored for any other kind."""

    item_count: int
    """For an array, its fixed length, or zero for any length."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=StyleExpressionTypeKind(raw["kind"]),
            item_kind=StyleExpressionTypeKind(raw["item_kind"]),
            item_count=raw["item_count"],
        )


@dataclass(frozen=True, slots=True)
class StyleImageInfo:
    """One complete runtime style image, borrowed for a completion callback.

    See `mln_style_image_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    width: int
    """Image width in pixels."""

    height: int
    """Image height in pixels."""

    pixels: bytes
    """Premultiplied RGBA8 pixels in tightly packed rows of width \\* 4 bytes,
    top row first.
    """

    stretch_x: tuple[ImageStretch, ...]
    """Horizontally stretchable intervals."""

    stretch_y: tuple[ImageStretch, ...]
    """Vertically stretchable intervals."""

    pixel_ratio: float
    """Sprite pixel ratio."""

    sdf: bool
    """Whether the image is a signed distance field icon."""

    content: ImageContent | None = None
    """Content box, when the image sets one."""

    text_fit_width: StyleImageTextFit | None = None
    """How the image fits text horizontally, when it sets this."""

    text_fit_height: StyleImageTextFit | None = None
    """How the image fits text vertically, when it sets this."""

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
        )


@dataclass(frozen=True, slots=True)
class StyleImageOptions:
    """Options for runtime style images.

    See `mln_style_image_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    stretch_x: Sequence[ImageStretch] | None = None
    """Horizontally stretchable intervals. Borrowed for the call and copied
    before return. May be null only when stretch_x_count is 0.
    """

    stretch_y: Sequence[ImageStretch] | None = None
    """Vertically stretchable intervals. Borrowed for the call and copied before
    return. May be null only when stretch_y_count is 0.
    """

    content: ImageContent | None = None
    """Content box used when icon-text-fit applies."""

    text_fit_width: StyleImageTextFit | None = None
    """One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK."""

    text_fit_height: StyleImageTextFit | None = None
    """One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK."""

    pixel_ratio: float | None = None
    """Sprite pixel ratio. Defaults to 1."""

    sdf: bool | None = None
    """Whether the image is a signed distance field icon. Defaults to false."""

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
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_style_image_options())


@dataclass(frozen=True, slots=True)
class StyleLayerInfo:
    """One style layer, borrowed for a completion callback.

    See `mln_style_layer_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    id: str
    """Layer ID."""

    type: str
    """The style-spec layer type string."""

    min_zoom: float
    """Lowest zoom at which the layer draws; -INFINITY with no lower bound."""

    max_zoom: float
    """Highest zoom at which the layer draws; INFINITY with no upper bound."""

    visibility: StyleLayerVisibility
    """One of `mln_style_layer_visibility`."""

    source_id: str | None = None
    """Source ID. Empty for a layer type that takes no source."""

    source_layer: str | None = None
    """Source-layer ID. Empty when the layer sets none."""

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
    """Complete metadata of one style source, borrowed for a completion
    callback.

    See `mln_style_source_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    id: str
    """Source ID."""

    type: StyleSourceType
    """One of `mln_style_source_type`."""

    is_volatile: bool
    """Whether the source is marked volatile."""

    attribution: str | None = None
    """Attribution string, when the source sets one. It may be empty."""

    url: str | None = None
    """URL that the source loads from, when it has one."""

    tilejson: StyleSourceTileInfo | None = None
    """Inline TileJSON metadata, when the source was defined with it."""

    bounds: LatLngBounds | None = None
    """Geographic bounds, when inline TileJSON sets them."""

    tile_size: int | None = None
    """Tile size in pixels, for a tile source."""

    vector_encoding: StyleVectorTileEncoding | None = None
    """Vector tile encoding, for a vector source."""

    raster_encoding: StyleRasterDemEncoding | None = None
    """DEM raster encoding, when inline TileJSON sets one."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            id=raw["id"],
            type=StyleSourceType(raw["type"]),
            is_volatile=raw["is_volatile"],
            attribution=raw["attribution"],
            url=raw["url"],
            tilejson=_maybe(StyleSourceTileInfo._from_native, raw["tilejson"]),
            bounds=_maybe(LatLngBounds._from_native, raw["bounds"]),
            tile_size=raw["tile_size"],
            vector_encoding=_maybe(StyleVectorTileEncoding, raw["vector_encoding"]),
            raster_encoding=_maybe(StyleRasterDemEncoding, raw["raster_encoding"]),
        )


@dataclass(frozen=True, slots=True)
class StyleSourceTileInfo:
    """Inline TileJSON metadata of a tile source.

    See `mln_style_source_tile_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    tile_urls: tuple[str, ...]
    """Tile URL templates in TileJSON order."""

    min_zoom: float
    """Lowest zoom level the TileJSON provides tiles for."""

    max_zoom: float
    """Highest zoom level the TileJSON provides tiles for."""

    scheme: StyleTileScheme
    """One of `mln_style_tile_scheme`."""

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
    """Global style transition options.

    See `mln_style_transition_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    duration_ms: float | None = None
    """Transition duration in milliseconds. Must be finite, non-negative, and
    short enough to represent as a duration.
    """

    delay_ms: float | None = None
    """Transition delay in milliseconds. Must be finite, non-negative, and short
    enough to represent as a duration.
    """

    enable_placement_transitions: bool | None = None
    """Whether symbol placement changes cross-fade."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            duration_ms=raw["duration_ms"],
            delay_ms=raw["delay_ms"],
            enable_placement_transitions=raw["enable_placement_transitions"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_style_transition_options())


@dataclass(frozen=True, slots=True)
class TextureImageInfo:
    """CPU image readback metadata for a texture target frame.

    See `mln_texture_image_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    width: int
    """Physical image width in device pixels."""

    height: int
    """Physical image height in device pixels."""

    stride: int
    """Bytes per image row."""

    byte_length: int
    """Required output buffer byte length."""

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
    """Texture readback borrowed for a completion callback.

    See `mln_texture_readback_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    data: bytes
    """Borrowed pixel bytes, valid only during the callback."""

    info: TextureImageInfo

    @classmethod
    def _from_native(cls, raw):
        return cls(data=raw["data"], info=TextureImageInfo._from_native(raw["info"]))


@dataclass(frozen=True, slots=True)
class TileId:
    """Overscaled tile identity reported in tile observer events.

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
    """Cubic easing curve for animated camera transitions.

    See `mln_unit_bezier` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x1: float

    y1: float

    x2: float

    y2: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x1=raw["x1"], y1=raw["y1"], x2=raw["x2"], y2=raw["y2"])


@dataclass(frozen=True, slots=True)
class Vec3:
    """Three-component vector used by free camera options.

    See `mln_vec3` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: float

    y: float

    z: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"], z=raw["z"])


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTexture:
    """One caller-owned Vulkan image of a borrowed texture ring.

    See `mln_vulkan_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    image: int
    """Borrowed VkImage. Required."""

    image_view: int
    """Borrowed VkImageView for image. Required. The view must be a 2D color
    view that matches image and the descriptor's format.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(image=raw["image"], image_view=raw["image_view"])


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTextureDescriptor:
    """Vulkan attachment options for a borrowed texture target.

    See `mln_vulkan_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: VulkanBorrowedTextureDescriptor.default().extent
    )
    """Logical texture extent; physical_width and physical_height give the
    texture's size. A scale_factor that differs from the map's is accepted
    and logged as a warning.
    """

    physical_width: int = field(
        default_factory=lambda: VulkanBorrowedTextureDescriptor.default().physical_width
    )
    """Physical image width in device pixels. Must be positive. Defaults to 256."""

    physical_height: int = field(
        default_factory=lambda: (
            VulkanBorrowedTextureDescriptor.default().physical_height
        )
    )
    """Physical image height in device pixels. Must be positive. Defaults to
    256.
    """

    context: VulkanContextDescriptor = field(
        default_factory=lambda: VulkanBorrowedTextureDescriptor.default().context
    )
    """Borrowed Vulkan context. All handles are required."""

    textures: Sequence[VulkanBorrowedTexture] = field(
        default_factory=lambda: VulkanBorrowedTextureDescriptor.default().textures
    )
    """The ring's images, one per slot, in slot order. Required."""

    format: int = field(
        default_factory=lambda: VulkanBorrowedTextureDescriptor.default().format
    )
    """Backend-native VkFormat value of every image. VK_FORMAT_UNDEFINED is
    invalid.
    """

    initial_layout: int = field(
        default_factory=lambda: VulkanBorrowedTextureDescriptor.default().initial_layout
    )
    """Backend-native VkImageLayout value expected at render-pass begin."""

    final_layout: int = field(
        default_factory=lambda: VulkanBorrowedTextureDescriptor.default().final_layout
    )
    """Backend-native VkImageLayout value left after rendering succeeds.
    Defaults to 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    """

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
            initial_layout=raw["initial_layout"],
            final_layout=raw["final_layout"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_vulkan_borrowed_texture_descriptor())


@dataclass(frozen=True, slots=True)
class VulkanContextDescriptor:
    """Vulkan backend context fields shared by Vulkan render targets.

    See `mln_vulkan_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    instance: int
    """Borrowed VkInstance. Required."""

    physical_device: int
    """Borrowed VkPhysicalDevice. Required."""

    device: int
    """Borrowed VkDevice. Required."""

    graphics_queue: int
    """Borrowed graphics VkQueue. Required. The session's driver submits to it
    from its own thread, so a host that uses the same queue passes
    `mln_render_session_attach_options.queue_lock` at attach.
    """

    graphics_queue_family_index: int
    """Queue family index for graphics_queue. Must support graphics commands."""

    get_instance_proc_addr: int
    """PFN_vkGetInstanceProcAddr for the loader that created the instance."""

    get_device_proc_addr: int
    """PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device."""

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
    """Vulkan attachment options for an owned texture target.

    See `mln_vulkan_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: VulkanOwnedTextureDescriptor.default().extent
    )
    """Logical texture extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: VulkanContextDescriptor = field(
        default_factory=lambda: VulkanOwnedTextureDescriptor.default().context
    )
    """Borrowed Vulkan context. All handles are required."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=VulkanContextDescriptor._from_native(raw["context"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_vulkan_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class VulkanSurfaceDescriptor:
    """Vulkan attachment options for a native surface.

    See `mln_vulkan_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: VulkanSurfaceDescriptor.default().extent
    )
    """Logical surface extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: VulkanContextDescriptor = field(
        default_factory=lambda: VulkanSurfaceDescriptor.default().context
    )
    """Borrowed Vulkan context. All handles are required. The device must
    support VK_KHR_swapchain, and the queue family must support graphics and
    presentation to this descriptor's surface.
    """

    surface: int = field(
        default_factory=lambda: VulkanSurfaceDescriptor.default().surface
    )
    """Borrowed VkSurfaceKHR bit pattern. Required."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=VulkanContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_vulkan_surface_descriptor())


@dataclass(frozen=True, slots=True)
class VulkanTextureFrame:
    """Vulkan frame acquired from a texture ring.

    See `mln_vulkan_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    """Session generation that produced this frame."""

    width: int
    """Physical Vulkan image width in device pixels."""

    height: int
    """Physical Vulkan image height in device pixels."""

    scale_factor: float
    """UI-to-device pixel scale used for this frame."""

    frame_id: int
    """Opaque identity of this frame."""

    slot: int
    """Ring slot that holds this frame. For a borrowed target, the index of its
    image in the descriptor's textures array.
    """

    image: int
    """Borrowed VkImage bit pattern. Valid until frame release."""

    image_view: int
    """Borrowed VkImageView bit pattern. Valid until frame release."""

    device: int
    """Borrowed VkDevice. Valid until frame release."""

    format: int
    """Backend-native VkFormat value."""

    layout: int
    """Backend-native VkImageLayout value that the image is in:
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for a session-owned ring, and
    the descriptor's final_layout for a borrowed one.
    """

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
    """Receiver wake callback copied by a successful owning call. The callback
    schedules service by the receiver and returns without waiting for another
    thread.

    See `mln_wake` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
    """

    callback: Callable[[], None] | None = None

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
class WebglContextDescriptor:
    """WebGL context fields shared by OpenGL render targets in the browser.

    See `mln_webgl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    kind: WebglContextKind
    """One `mln_webgl_context_kind` value."""

    context: int
    """Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive."""

    canvas_selector: str
    """Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
    canvas must still be transferable when attachment starts.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=WebglContextKind(raw["kind"]),
            context=raw["context"],
            canvas_selector=raw["canvas_selector"],
        )


@dataclass(frozen=True, slots=True)
class WebgpuBorrowedTexture:
    """One caller-owned WebGPU texture of a borrowed texture ring.

    See `mln_webgpu_borrowed_texture` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    texture: int
    """Borrowed WGPUTexture. Required."""

    texture_view: int
    """Borrowed WGPUTextureView for texture. Required. The view must be a 2D
    color view compatible with texture and the descriptor's format.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(texture=raw["texture"], texture_view=raw["texture_view"])


@dataclass(frozen=True, slots=True)
class WebgpuBorrowedTextureDescriptor:
    """WebGPU attachment options for a borrowed texture target.

    See `mln_webgpu_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: WebgpuBorrowedTextureDescriptor.default().extent
    )
    """Logical texture extent; physical_width and physical_height give the
    texture's size. A scale_factor that differs from the map's is accepted
    and logged as a warning.
    """

    physical_width: int = field(
        default_factory=lambda: WebgpuBorrowedTextureDescriptor.default().physical_width
    )
    """Physical texture width in device pixels. Defaults to 256."""

    physical_height: int = field(
        default_factory=lambda: (
            WebgpuBorrowedTextureDescriptor.default().physical_height
        )
    )
    """Physical texture height in device pixels. Defaults to 256."""

    context: WebgpuContextDescriptor = field(
        default_factory=lambda: WebgpuBorrowedTextureDescriptor.default().context
    )
    """Borrowed WebGPU context. device is required. Rendering is submitted
    through context.queue or that device's default queue.
    """

    textures: Sequence[WebgpuBorrowedTexture] = field(
        default_factory=lambda: WebgpuBorrowedTextureDescriptor.default().textures
    )
    """The ring's textures, one per slot, in slot order. Required."""

    format: int = field(
        default_factory=lambda: WebgpuBorrowedTextureDescriptor.default().format
    )
    """Backend-native WGPUTextureFormat value of every texture. Undefined is
    invalid.
    """

    def __post_init__(self):
        object.__setattr__(self, "textures", _sequence(self.textures))

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            context=WebgpuContextDescriptor._from_native(raw["context"]),
            textures=tuple(
                WebgpuBorrowedTexture._from_native(item) for item in raw["textures"]
            ),
            format=raw["format"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_webgpu_borrowed_texture_descriptor())


@dataclass(frozen=True, slots=True)
class WebgpuContextDescriptor:
    """WebGPU backend context fields shared by WebGPU render targets.

    See `mln_webgpu_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    instance: int
    """Borrowed WGPUInstance. Optional for texture targets."""

    device: int
    """Borrowed WGPUDevice. Required."""

    queue: int
    """Borrowed WGPUQueue. Optional; null uses the device default queue. A
    non-null queue must belong to device.
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(instance=raw["instance"], device=raw["device"], queue=raw["queue"])


@dataclass(frozen=True, slots=True)
class WebgpuOwnedTextureDescriptor:
    """WebGPU attachment options for an owned texture target.

    See `mln_webgpu_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: WebgpuOwnedTextureDescriptor.default().extent
    )
    """Logical texture extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: WebgpuContextDescriptor = field(
        default_factory=lambda: WebgpuOwnedTextureDescriptor.default().context
    )
    """Borrowed WebGPU context. device is required."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=WebgpuContextDescriptor._from_native(raw["context"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_webgpu_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class WebgpuSurfaceDescriptor:
    """WebGPU attachment options for a native surface.

    See `mln_webgpu_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: LogicalExtent = field(
        default_factory=lambda: WebgpuSurfaceDescriptor.default().extent
    )
    """Logical surface extent. A scale_factor that differs from the map's is
    accepted and logged as a warning.
    """

    context: WebgpuContextDescriptor = field(
        default_factory=lambda: WebgpuSurfaceDescriptor.default().context
    )
    """Borrowed WebGPU context. device is required."""

    surface: int = field(
        default_factory=lambda: WebgpuSurfaceDescriptor.default().surface
    )
    """Borrowed WGPUSurface. Required, and must stay alive for the session. The
    session configures it for this device and extent, and unconfigures it
    when the session ends.
    """

    format: int = field(
        default_factory=lambda: WebgpuSurfaceDescriptor.default().format
    )
    """WGPUTextureFormat to configure the surface with. Required. A browser host
    takes it from navigator.gpu.getPreferredCanvasFormat().
    """

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=LogicalExtent._from_native(raw["extent"]),
            context=WebgpuContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
            format=raw["format"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_webgpu_surface_descriptor())


@dataclass(frozen=True, slots=True)
class WebgpuTextureFrame:
    """WebGPU frame acquired from a texture ring.

    See `mln_webgpu_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    """Session generation that produced this frame."""

    width: int
    """Physical WebGPU texture width in device pixels."""

    height: int
    """Physical WebGPU texture height in device pixels."""

    scale_factor: float
    """UI-to-device pixel scale used for this frame."""

    frame_id: int
    """Opaque identity of this frame."""

    slot: int
    """Ring slot that holds this frame. For a borrowed target, the index of its
    texture in the descriptor's textures array.
    """

    texture: int
    """Borrowed WGPUTexture. Valid until frame release."""

    texture_view: int
    """Borrowed WGPUTextureView. Valid until frame release."""

    device: int
    """Borrowed WGPUDevice. Valid until frame release."""

    format: int
    """Backend-native WGPUTextureFormat value."""

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
            texture_view=raw["texture_view"],
            device=raw["device"],
            format=raw["format"],
        )


@dataclass(frozen=True, slots=True)
class WglContextDescriptor:
    """WGL context fields shared by OpenGL render targets on Windows.

    See `mln_wgl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    device_context: int
    """Borrowed HDC used to create the session context. Required. A dedicated
    owned texture makes its context current on this HDC from its core worker.
    """

    share_context: int
    """Borrowed HGLRC whose share group the session context joins. Required
    under shared ownership. A dedicated session joins no share group, so it
    must be null there.
    """

    get_proc_address: int
    """Optional wglGetProcAddress-compatible function for the host loader."""

    @classmethod
    def _from_native(cls, raw):
        return cls(
            device_context=raw["device_context"],
            share_context=raw["share_context"],
            get_proc_address=raw["get_proc_address"],
        )
