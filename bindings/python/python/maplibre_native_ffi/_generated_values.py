"""Generated from the C headers by tools/bindgen. Do not edit."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from enum import IntFlag
from typing import TYPE_CHECKING

from ._enum import UnknownIntEnum
from ._operation import _wrap_response

if TYPE_CHECKING:
    from ._generated_owners import (
        HttpHeaderTransformResponseScope,
        ResourceRequestHandle,
        ResourceTransformResponseScope,
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


@dataclass(frozen=True, slots=True)
class RuntimeEventCameraTransitionFinishedVariant:
    value: RuntimeEventCameraTransitionFinished
    _tag = 9

    @classmethod
    def _from_native(cls, raw):
        return cls(RuntimeEventCameraTransitionFinished._from_native(raw))


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


class AmbientCacheOperation(UnknownIntEnum):
    RESET_DATABASE = 1
    PACK_DATABASE = 2
    INVALIDATE = 3
    CLEAR = 4


class AnimationOptionField(IntFlag):
    """Field mask values for `mln_animation_options`.

    See `mln_animation_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    DURATION = 1
    VELOCITY = 2
    MIN_ZOOM = 4
    EASING = 8
    TRANSITION_ID = 16


class BoundOptionField(IntFlag):
    """Field mask values for `mln_bound_options`.

    See `mln_bound_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    BOUNDS = 1
    MIN_ZOOM = 2
    MAX_ZOOM = 4
    MIN_PITCH = 8
    MAX_PITCH = 16
    UNBOUNDED = 32


class CameraChangeMode(UnknownIntEnum):
    """Camera change kinds reported by camera will-change and did-change events.

    See `mln_camera_change_mode` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    IMMEDIATE = 0
    ANIMATED = 1


class CameraDeltaKind(UnknownIntEnum):
    """Relative camera operation carried by `mln_camera_delta`.

    See `mln_camera_delta_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    MOVE = 0
    SCALE = 1
    BEARING = 2
    PITCH = 3


class CameraFitOptionField(IntFlag):
    """Field mask values for `mln_camera_fit_options`.

    See `mln_camera_fit_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    PADDING = 1
    BEARING = 2
    PITCH = 4


class CameraOptionField(IntFlag):
    """Field mask values for `mln_camera_options`.

    See `mln_camera_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    CENTER = 1
    ZOOM = 2
    BEARING = 4
    PITCH = 8
    CENTER_ALTITUDE = 16
    PADDING = 32
    ANCHOR = 64
    ROLL = 128
    FOV = 256


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


class CustomGeometrySourceOptionField(IntFlag):
    """Field mask values for `mln_custom_geometry_source_options`.

    See `mln_custom_geometry_source_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MIN_ZOOM = 1
    MAX_ZOOM = 2
    TOLERANCE = 4
    TILE_SIZE = 8
    BUFFER = 16
    CLIP = 32
    WRAP = 64


class CustomMvtVectorSourceOptionField(IntFlag):
    """Field mask values for `mln_custom_mvt_vector_source_options`.

    See `mln_custom_mvt_vector_source_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MIN_ZOOM = 1
    MAX_ZOOM = 2


class FeatureStateSelectorField(IntFlag):
    """Optional fields for `mln_feature_state_selector`.

    See `mln_feature_state_selector_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    SOURCE_LAYER_ID = 1
    FEATURE_ID = 2
    STATE_KEY = 4


class FrameDemandFlag(IntFlag):
    """Frame-demand policy bits.

    See `mln_frame_demand_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    """

    IF_NEEDED = 1
    PRESENT = 2


class FreeCameraOptionField(IntFlag):
    """Field mask values for `mln_free_camera_options`.

    See `mln_free_camera_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    POSITION = 1
    ORIENTATION = 2


class GeojsonSourceOptionField(IntFlag):
    """Field mask values for `mln_geojson_source_options`.

    See `mln_geojson_source_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MIN_ZOOM = 1
    MAX_ZOOM = 2
    TOLERANCE = 4
    CLUSTER_MAX_ZOOM = 8
    CLUSTER_PROPERTIES = 16
    TILE_SIZE = 32
    BUFFER = 64
    CLUSTER_RADIUS = 128
    CLUSTER_MIN_POINTS = 256
    LINE_METRICS = 512
    CLUSTER = 1024
    SYNCHRONOUS_TILING = 2048


class GesturePhase(UnknownIntEnum):
    """Gesture boundary carried atomically with a camera update.

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
    METAL_SHARED_EVENT = 1
    VULKAN_TIMELINE_SEMAPHORE = 2
    OPENGL_FENCE = 3
    WEBGPU_TOKEN = 4


class LocationIndicatorImageKind(UnknownIntEnum):
    """Location indicator image-name properties.

    See `mln_location_indicator_image_kind` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    TOP = 0
    BEARING = 1
    SHADOW = 2


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


class MapTileOptionField(IntFlag):
    """Field mask values for `mln_map_tile_options`.

    See `mln_map_tile_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    PREFETCH_ZOOM_DELTA = 1
    LOD_MIN_RADIUS = 2
    LOD_SCALE = 4
    LOD_PITCH_THRESHOLD = 8
    LOD_ZOOM_SHIFT = 16
    LOD_MODE = 32


class MapViewportOptionField(IntFlag):
    """Field mask values for `mln_map_viewport_options`.

    See `mln_map_viewport_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    NORTH_ORIENTATION = 1
    CONSTRAIN_MODE = 2
    VIEWPORT_MODE = 4
    FRUSTUM_OFFSET = 8


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


class OfflineRegionDefinitionType(UnknownIntEnum):
    TILE_PYRAMID = 1
    GEOMETRY = 2


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


class OpenglContextPlatform(UnknownIntEnum):
    """OpenGL platform context provider used by a context descriptor.

    See `mln_opengl_context_platform` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    UNSPECIFIED = 0
    WGL = 1
    EGL = 2
    WEBGL = 3


class OpenglContextProviderFlag(IntFlag):
    """OpenGL context providers supported by this build.

    See `mln_opengl_context_provider_flag` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    WGL = 1
    EGL = 2
    WEBGL = 4


class ProjectionModeField(IntFlag):
    """Field mask values for MapLibre axonometric rendering options.

    See `mln_projection_mode_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    AXONOMETRIC = 1
    X_SKEW = 2
    Y_SKEW = 4


class QueriedFeatureField(IntFlag):
    """Optional fields for `mln_queried_feature`.

    See `mln_queried_feature_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    SOURCE_ID = 1
    SOURCE_LAYER_ID = 2
    STATE = 4


class RenderAbandonDisposition(UnknownIntEnum):
    """Result of irreversible CPU-side target abandonment.

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
    SIZE_PENDING = 2
    TARGET_NOT_READY = 3
    SUPERSEDED = 4
    DEADLINE_MISSED = 5


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


class RenderedFeatureQueryOptionField(IntFlag):
    """Optional fields for `mln_rendered_feature_query_options`.

    See `mln_rendered_feature_query_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    IDS = 1


class RenderedQueryGeometryType(UnknownIntEnum):
    """Rendered feature query geometry variants.

    See `mln_rendered_query_geometry_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    POINT = 1
    BOX = 2
    LINE_STRING = 3


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


class ResourceLoadingMethod(UnknownIntEnum):
    ALL = 0
    CACHE_ONLY = 1
    NETWORK_ONLY = 2


class ResourcePriority(UnknownIntEnum):
    REGULAR = 0
    LOW = 1


class ResourceProviderDecision(UnknownIntEnum):
    PASS_THROUGH = 0
    HANDLE = 1


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
    MAP_RENDER_UPDATE_AVAILABLE = 512
    MAP_RENDER_ERROR = 1024
    MAP_STILL_IMAGE_FINISHED = 2048
    MAP_STILL_IMAGE_FAILED = 4096
    MAP_RENDER_FRAME_STARTED = 8192
    MAP_RENDER_FRAME_FINISHED = 16384
    MAP_RENDER_MAP_STARTED = 32768
    MAP_RENDER_MAP_FINISHED = 65536
    MAP_STYLE_IMAGE_MISSING = 131072
    MAP_TILE_ACTION = 262144
    MAP_CAMERA_TRANSITION_FINISHED = 4194304
    OFFLINE_REGION_STATUS_CHANGED = 524288
    OFFLINE_REGION_RESPONSE_ERROR = 1048576
    OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 2097152
    ALL_MAP_EVENTS = 4718590
    ALL_RUNTIME_EVENTS = 3670016
    ALL = 8388606


class RuntimeEventPayloadType(UnknownIntEnum):
    """Payload kinds used by `mln_runtime_event.payload_type`.

    See `mln_runtime_event_payload_type` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    NONE = 0
    RENDER_FRAME = 1
    RENDER_MAP = 2
    TILE_ACTION = 4
    OFFLINE_REGION_STATUS = 5
    OFFLINE_REGION_RESPONSE_ERROR = 6
    OFFLINE_REGION_TILE_COUNT_LIMIT = 7
    CAMERA_TRANSITION_FINISHED = 9


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
    MAP_RENDER_UPDATE_AVAILABLE = 9
    MAP_RENDER_ERROR = 10
    MAP_STILL_IMAGE_FINISHED = 11
    MAP_STILL_IMAGE_FAILED = 12
    MAP_RENDER_FRAME_STARTED = 13
    MAP_RENDER_FRAME_FINISHED = 14
    MAP_RENDER_MAP_STARTED = 15
    MAP_RENDER_MAP_FINISHED = 16
    MAP_STYLE_IMAGE_MISSING = 17
    MAP_TILE_ACTION = 18
    OFFLINE_REGION_STATUS_CHANGED = 19
    OFFLINE_REGION_RESPONSE_ERROR = 20
    OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21
    MAP_CAMERA_TRANSITION_FINISHED = 22


class SourceFeatureQueryOptionField(IntFlag):
    """Optional fields for `mln_source_feature_query_options`.

    See `mln_source_feature_query_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    IDS = 1


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


class StyleImageOptionField(IntFlag):
    """Field mask values for `mln_style_image_options`.

    See `mln_style_image_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    PIXEL_RATIO = 1
    SDF = 2
    STRETCH_X = 4
    STRETCH_Y = 8
    CONTENT = 16
    TEXT_FIT_WIDTH = 32
    TEXT_FIT_HEIGHT = 64


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
    """DEM raster encoding values used by `mln_style_tile_source_options`.

    See `mln_style_raster_dem_encoding` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MAPBOX = 0
    TERRARIUM = 1


class StyleSourceInfoField(IntFlag):
    """Fields available in `mln_style_source_info`.

    See `mln_style_source_info_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    URL = 1
    TILEJSON = 2
    BOUNDS = 4
    TILE_SIZE = 8
    VECTOR_ENCODING = 16
    RASTER_ENCODING = 32


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
    """Tile URL coordinate scheme values used by
    `mln_style_tile_source_options`.

    See `mln_style_tile_scheme` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    XYZ = 0
    TMS = 1


class StyleTileSourceOptionField(IntFlag):
    """Field mask values for `mln_style_tile_source_options`.

    See `mln_style_tile_source_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    MIN_ZOOM = 1
    MAX_ZOOM = 2
    ATTRIBUTION = 4
    SCHEME = 8
    BOUNDS = 16
    TILE_SIZE = 32
    VECTOR_ENCODING = 64
    RASTER_ENCODING = 128


class StyleTransitionOptionField(IntFlag):
    """Field mask values for `mln_style_transition_options`.

    See `mln_style_transition_option_field` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    DURATION = 1
    DELAY = 2
    ENABLE_PLACEMENT_TRANSITIONS = 4


class StyleVectorTileEncoding(UnknownIntEnum):
    """Vector tile encoding values used by `mln_style_tile_source_options`.

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
    velocity: float | None = None
    min_zoom: float | None = None
    easing: UnitBezier | None = None
    transition_id: int | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            duration_ms=raw["duration_ms"],
            velocity=raw["velocity"],
            min_zoom=raw["min_zoom"],
            easing=_maybe(UnitBezier._from_native, raw["easing"]),
            transition_id=raw["transition_id"],
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
    """One relative camera operation.

    See `mln_camera_delta` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    kind: CameraDeltaKind
    offset: ScreenPoint
    amount: float
    animation: AnimationOptions
    anchor: ScreenPoint | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=CameraDeltaKind(raw["kind"]),
            offset=ScreenPoint._from_native(raw["offset"]),
            amount=raw["amount"],
            anchor=_maybe(ScreenPoint._from_native, raw["anchor"]),
            animation=AnimationOptions._from_native(raw["animation"]),
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
class CameraUpdate:
    """One atomic absolute camera update.

    See `mln_camera_update` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    mode: CameraUpdateMode
    camera: CameraOptions
    animation: AnimationOptions
    gesture_phase: GesturePhase

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
    config: int
    share_context: int
    client_api: OpenglClientApi
    get_proc_address: int

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
    """A borrowed view of one owned runtime-event batch.

    See `mln_event_batch_view` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    events: tuple[RuntimeEvent, ...]

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
    source_layer_id: str | None = None
    feature_id: str | None = None
    state_key: str | None = None

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

    flags: FrameDemandFlag
    token: int
    coalescing_boundary: int
    timeout_ns: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            flags=FrameDemandFlag(raw["flags"]),
            token=raw["token"],
            coalescing_boundary=raw["coalescing_boundary"],
            timeout_ns=raw["timeout_ns"],
        )

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
    """Options for GeoJSON sources.

    See `mln_geojson_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    min_zoom: float | None = None
    max_zoom: float | None = None
    tolerance: float | None = None
    cluster_max_zoom: float | None = None
    cluster_properties: bytes | None = None
    tile_size: int | None = None
    buffer: int | None = None
    cluster_radius: int | None = None
    cluster_min_points: int | None = None
    line_metrics: bool | None = None
    cluster: bool | None = None
    synchronous_tiling: bool | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            tolerance=raw["tolerance"],
            cluster_max_zoom=raw["cluster_max_zoom"],
            cluster_properties=raw["cluster_properties"],
            tile_size=raw["tile_size"],
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

    kind: GpuSyncKind
    object: int
    value: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=GpuSyncKind(raw["kind"]), object=raw["object"], value=raw["value"]
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_gpu_sync())


@dataclass(frozen=True, slots=True)
class HttpHeaderTransform:
    callback: (
        Callable[[ResourceKind, str, HttpHeaderTransformResponseScope], Status] | None
    ) = None

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
    longitude: float

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
class LogSetCallbackRegistration:
    callback: Callable[[LogSeverity, LogEvent, int, str], int] | None = None

    def _invoke_callback(self, severity, event, code, message):
        callback = self.callback
        assert callback is not None
        return callback(LogSeverity(severity), LogEvent(event), code, message)


@dataclass(frozen=True, slots=True)
class LogicalExtent:
    """Logical map extent in UI pixels and device-pixel scale.

    See `mln_logical_extent` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    width: int
    height: int
    scale_factor: float

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"], height=raw["height"], scale_factor=raw["scale_factor"]
        )


@dataclass(frozen=True, slots=True)
class MapOptions:
    """Options used when creating a map.

    See `mln_map_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    initial_extent: LogicalExtent
    map_mode: MapMode
    fast_pfor_enabled: bool
    event_mask: RuntimeEventMask

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
    generation: int
    camera: CameraOptions
    logical_extent: LogicalExtent
    projection_mode: ProjectionMode
    viewport: MapViewportOptions
    fully_loaded: bool
    rendering_stats_view_enabled: bool
    repaint_demand: bool
    gesture_in_progress: bool
    event_mask: RuntimeEventMask
    latest_render_update_generation: int
    tile: MapTileOptions
    bounds: BoundOptions
    free_camera: FreeCameraOptions

    @classmethod
    def _from_native(cls, raw):
        return cls(
            debug_options=MapDebugOption(raw["debug_options"]),
            generation=raw["generation"],
            camera=CameraOptions._from_native(raw["camera"]),
            logical_extent=LogicalExtent._from_native(raw["logical_extent"]),
            projection_mode=ProjectionMode._from_native(raw["projection_mode"]),
            viewport=MapViewportOptions._from_native(raw["viewport"]),
            fully_loaded=raw["fully_loaded"],
            rendering_stats_view_enabled=raw["rendering_stats_view_enabled"],
            repaint_demand=raw["repaint_demand"],
            gesture_in_progress=raw["gesture_in_progress"],
            event_mask=RuntimeEventMask(raw["event_mask"]),
            latest_render_update_generation=raw["latest_render_update_generation"],
            tile=MapTileOptions._from_native(raw["tile"]),
            bounds=BoundOptions._from_native(raw["bounds"]),
            free_camera=FreeCameraOptions._from_native(raw["free_camera"]),
        )


@dataclass(frozen=True, slots=True)
class MapTileOptions:
    """Tile prefetch and LOD tuning controls.

    See `mln_map_tile_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    prefetch_zoom_delta: int | None = None
    lod_min_radius: float | None = None
    lod_scale: float | None = None
    lod_pitch_threshold: float | None = None
    lod_zoom_shift: float | None = None
    lod_mode: TileLodMode | None = None

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
class MapViewportOptions:
    """Live map viewport and render-transform controls.

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

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_map_viewport_options())


@dataclass(frozen=True, slots=True)
class MetalBorrowedTextureDescriptor:
    """Metal attachment options for a borrowed texture target.

    See `mln_metal_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: RenderTargetExtent
    physical_width: int
    physical_height: int
    texture: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            texture=raw["texture"],
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

    @classmethod
    def _from_native(cls, raw):
        return cls(device=raw["device"])


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureDescriptor:
    """Metal attachment options for an owned texture target.

    See `mln_metal_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: RenderTargetExtent
    context: MetalContextDescriptor

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=MetalContextDescriptor._from_native(raw["context"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_metal_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureFrame:
    """Metal frame acquired from a session-owned texture target.

    See `mln_metal_owned_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    texture: int
    device: int
    pixel_format: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            texture=raw["texture"],
            device=raw["device"],
            pixel_format=raw["pixel_format"],
        )


@dataclass(frozen=True, slots=True)
class MetalSurfaceDescriptor:
    """Metal attachment options for a native surface.

    See `mln_metal_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: RenderTargetExtent
    context: MetalContextDescriptor
    layer: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=MetalContextDescriptor._from_native(raw["context"]),
            layer=raw["layer"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_metal_surface_descriptor())


@dataclass(frozen=True, slots=True)
class OfflineGeometryRegionDefinition:
    """Geometry offline region definition.

    See `mln_offline_geometry_region_definition` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    style_url: str
    geometry: bytes
    min_zoom: float
    max_zoom: float
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
    bounds: LatLngBounds
    min_zoom: float
    max_zoom: float
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
class OpenglBorrowedTextureDescriptor:
    """OpenGL attachment options for a borrowed texture target.

    See `mln_opengl_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: RenderTargetExtent
    physical_width: int
    physical_height: int
    context: OpenglContextDescriptor
    texture: int
    target: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            context=OpenglContextDescriptor._from_native(raw["context"]),
            texture=raw["texture"],
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

    extent: RenderTargetExtent
    context: OpenglContextDescriptor

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=OpenglContextDescriptor._from_native(raw["context"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_opengl_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class OpenglOwnedTextureFrame:
    """OpenGL frame acquired from a session-owned texture target.

    See `mln_opengl_owned_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    texture: int
    target: int
    internal_format: int
    format: int
    type: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            texture=raw["texture"],
            target=raw["target"],
            internal_format=raw["internal_format"],
            format=raw["format"],
            type=raw["type"],
        )


@dataclass(frozen=True, slots=True)
class OpenglSurfaceDescriptor:
    """OpenGL attachment options for a native surface.

    See `mln_opengl_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: RenderTargetExtent
    context: OpenglContextDescriptor
    surface: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=OpenglContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_opengl_surface_descriptor())


@dataclass(frozen=True, slots=True)
class PremultipliedRgba8Image:
    """Caller-owned premultiplied RGBA8 image pixels.

    See `mln_premultiplied_rgba8_image` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    width: int
    height: int
    stride: int
    pixels: bytes

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
    easting: float

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
    x_skew: float | None = None
    y_skew: float | None = None

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
class QueriedFeature:
    """One query hit borrowed for a completion callback.

    See `mln_queried_feature` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    feature: bytes
    source_id: str | None = None
    source_layer_id: str | None = None
    state: bytes | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            feature=raw["feature"],
            source_id=raw["source_id"],
            source_layer_id=raw["source_layer_id"],
            state=raw["state"],
        )


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


@dataclass(frozen=True, slots=True)
class RenderAbandonResult:
    disposition: RenderAbandonDisposition
    quarantined_resource_count: int

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
    token: int
    map_update_generation: int
    extent_generation: int
    frame_generation: int
    needs_repaint: bool

    @classmethod
    def _from_native(cls, raw):
        return cls(
            disposition=RenderResult(raw["disposition"]),
            token=raw["token"],
            map_update_generation=raw["map_update_generation"],
            extent_generation=raw["extent_generation"],
            frame_generation=raw["frame_generation"],
            needs_repaint=raw["needs_repaint"],
        )


@dataclass(frozen=True, slots=True)
class RenderSessionAttachOptions:
    """Common attachment policy copied before an attach call returns.

    See `mln_render_session_attach_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    driver: RenderDriverKind
    requested_texture_ring_depth: int
    frame_wake: Wake
    driver_work_wake: Wake
    queue_lock: QueueLock

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            requested_texture_ring_depth=raw["requested_texture_ring_depth"],
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
    texture_ring_depth: int
    flags: RenderSessionCapabilityFlag

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
    driver: RenderDriverKind
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
    def _from_native(cls, raw):
        return cls(
            state=RenderSessionState(raw["state"]),
            driver=RenderDriverKind(raw["driver"]),
            latest_result=RenderResult(raw["latest_result"]),
            extent=RenderTargetExtent._from_native(raw["extent"]),
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
class RenderTargetExtent:
    """Logical render target extent in UI pixels.

    See `mln_render_target_extent` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    width: int
    height: int
    scale_factor: float

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"], height=raw["height"], scale_factor=raw["scale_factor"]
        )


@dataclass(frozen=True, slots=True)
class RenderedFeatureQueryOptions:
    """Options for rendered feature queries.

    See `mln_rendered_feature_query_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    layer_ids: tuple[str, ...] | None = None
    filter: bytes | None = None

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
    rendering_time: float
    frame_count: int
    draw_call_count: int
    total_draw_call_count: int

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
    callback: (
        Callable[[ResourceRequest, ResourceRequestHandle], ResourceProviderDecision]
        | None
    ) = None

    def _invoke_callback(self, request, handle):
        callback = self.callback
        assert callback is not None
        return callback(
            ResourceRequest._from_native(request),
            _wrap_response(handle, "ResourceRequestHandle"),
        )


@dataclass(frozen=True, slots=True)
class ResourceRequest:
    kind: ResourceKind
    loading_method: ResourceLoadingMethod
    priority: ResourcePriority
    usage: ResourceUsage
    storage_policy: ResourceStoragePolicy
    prior_data: bytes
    requested_url: str | None = None
    resolved_url: str | None = None
    range: ResourceRequestRange | None = None
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
            range=_maybe(ResourceRequestRange._from_native, raw["range"]),
            prior_modified_unix_ms=raw["prior_modified_unix_ms"],
            prior_expires_unix_ms=raw["prior_expires_unix_ms"],
            prior_etag=raw["prior_etag"],
            prior_data=raw["prior_data"],
        )


@dataclass(frozen=True, slots=True)
class ResourceRequestRange:
    start: int
    end: int

    @classmethod
    def _from_native(cls, raw):
        return cls(start=raw["start"], end=raw["end"])


@dataclass(frozen=True, slots=True)
class ResourceResponse:
    status: ResourceResponseStatus
    error_reason: ResourceErrorReason
    bytes: bytes
    must_revalidate: bool
    error_message: str | None = None
    modified_unix_ms: int | None = None
    expires_unix_ms: int | None = None
    etag: str | None = None
    retry_after_unix_ms: int | None = None

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
class ResourceTransform:
    callback: (
        Callable[[ResourceKind, str, ResourceTransformResponseScope], Status] | None
    ) = None

    def _invoke_callback(self, kind, url, out_response):
        callback = self.callback
        assert callback is not None
        return callback(
            ResourceKind(kind),
            url,
            _wrap_response(out_response, "ResourceTransformResponseScope"),
        )


@dataclass(frozen=True, slots=True)
class RuntimeEvent:
    """One drained runtime event.

    See `mln_runtime_event` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    type: RuntimeEventType
    source_type: RuntimeEventSourceType
    source: int
    code: int
    payload: (
        RuntimeEventRenderFrameVariant
        | RuntimeEventRenderMapVariant
        | RuntimeEventTileActionVariant
        | RuntimeEventOfflineRegionStatusVariant
        | RuntimeEventOfflineRegionResponseErrorVariant
        | RuntimeEventOfflineRegionTileCountLimitVariant
        | RuntimeEventCameraTransitionFinishedVariant
        | UnknownVariant
        | None
    )
    message: str

    @classmethod
    def _from_native(cls, raw):
        return cls(
            type=RuntimeEventType(raw["type"]),
            source_type=RuntimeEventSourceType(raw["source_type"]),
            source=raw["source"],
            code=raw["code"],
            payload=_copy_variant(
                raw["payload"],
                {
                    "render_frame": RuntimeEventRenderFrameVariant,
                    "render_map": RuntimeEventRenderMapVariant,
                    "tile_action": RuntimeEventTileActionVariant,
                    "offline_region_status": RuntimeEventOfflineRegionStatusVariant,
                    "offline_region_response_error": RuntimeEventOfflineRegionResponseErrorVariant,
                    "offline_region_tile_count_limit": RuntimeEventOfflineRegionTileCountLimitVariant,
                    "camera_transition_finished": RuntimeEventCameraTransitionFinishedVariant,
                },
                0,
            ),
            message=raw["message"],
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventCameraTransitionFinished:
    """Payload for `MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED`.

    See `mln_runtime_event_camera_transition_finished` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    transition_id: int

    @classmethod
    def _from_native(cls, raw):
        return cls(transition_id=raw["transition_id"])


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionResponseError:
    """Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR`.

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
    """Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED`.

    See `mln_runtime_event_render_map` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    """

    mode: RenderMode

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

    flags: int
    event_mask: RuntimeEventMask
    event_wake: Wake
    asset_path: str | None = None
    cache_path: str | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            flags=raw["flags"],
            asset_path=raw["asset_path"],
            cache_path=raw["cache_path"],
            event_mask=RuntimeEventMask(raw["event_mask"]),
            event_wake=Wake(),
        )

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

    points: tuple[ScreenPoint, ...]

    @classmethod
    def _from_native(cls, raw):
        return cls(
            points=tuple(ScreenPoint._from_native(item) for item in raw["points"])
        )


@dataclass(frozen=True, slots=True)
class ScreenPoint:
    """Screen-space point in logical map pixels.

    See `mln_screen_point` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    """

    x: float
    y: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"])


@dataclass(frozen=True, slots=True)
class SourceFeatureQueryOptions:
    """Options for source feature queries.

    See `mln_source_feature_query_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    """

    source_layer_ids: tuple[str, ...] | None = None
    filter: bytes | None = None

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
class StyleImageInfo:
    """Fixed metadata for one runtime style image.

    See `mln_style_image_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    width: int
    height: int
    stride: int
    byte_length: int
    stretch_x_count: int
    stretch_y_count: int
    pixel_ratio: float
    sdf: bool
    content: ImageContent | None = None
    text_fit_width: StyleImageTextFit | None = None
    text_fit_height: StyleImageTextFit | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            stride=raw["stride"],
            byte_length=raw["byte_length"],
            stretch_x_count=raw["stretch_x_count"],
            stretch_y_count=raw["stretch_y_count"],
            content=_maybe(ImageContent._from_native, raw["content"]),
            text_fit_width=_maybe(StyleImageTextFit, raw["text_fit_width"]),
            text_fit_height=_maybe(StyleImageTextFit, raw["text_fit_height"]),
            pixel_ratio=raw["pixel_ratio"],
            sdf=raw["sdf"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_style_image_info())


@dataclass(frozen=True, slots=True)
class StyleImageOptions:
    """Options for runtime style images.

    See `mln_style_image_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    stretch_x: tuple[ImageStretch, ...] | None = None
    stretch_y: tuple[ImageStretch, ...] | None = None
    content: ImageContent | None = None
    text_fit_width: StyleImageTextFit | None = None
    text_fit_height: StyleImageTextFit | None = None
    pixel_ratio: float | None = None
    sdf: bool | None = None

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
class StyleImageResult:
    """Complete style image borrowed for a completion callback.

    See `mln_style_image_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    info: StyleImageInfo
    pixels: bytes
    stretch_x: tuple[ImageStretch, ...]
    stretch_y: tuple[ImageStretch, ...]

    @classmethod
    def _from_native(cls, raw):
        return cls(
            info=StyleImageInfo._from_native(raw["info"]),
            pixels=raw["pixels"],
            stretch_x=tuple(
                ImageStretch._from_native(item) for item in raw["stretch_x"]
            ),
            stretch_y=tuple(
                ImageStretch._from_native(item) for item in raw["stretch_y"]
            ),
        )


@dataclass(frozen=True, slots=True)
class StyleImageStretchesResult:
    """Borrowed image-stretch arrays available during a completion callback.

    See `mln_style_image_stretches_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    stretch_x: tuple[ImageStretch, ...]
    stretch_y: tuple[ImageStretch, ...]

    @classmethod
    def _from_native(cls, raw):
        return cls(
            stretch_x=tuple(
                ImageStretch._from_native(item) for item in raw["stretch_x"]
            ),
            stretch_y=tuple(
                ImageStretch._from_native(item) for item in raw["stretch_y"]
            ),
        )


@dataclass(frozen=True, slots=True)
class StyleLayerEntry:
    """One style layer borrowed for a list completion callback.

    See `mln_style_layer_entry` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    id: str
    type: str
    source_id: str | None = None
    source_layer: str | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            id=raw["id"],
            type=raw["type"],
            source_id=raw["source_id"],
            source_layer=raw["source_layer"],
        )


@dataclass(frozen=True, slots=True)
class StyleLayerInfo:
    """Fixed layer metadata included in `mln_style_layer_result`.

    See `mln_style_layer_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    type: str
    min_zoom: float
    max_zoom: float
    visibility: StyleLayerVisibility

    @classmethod
    def _from_native(cls, raw):
        return cls(
            type=raw["type"],
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            visibility=StyleLayerVisibility(raw["visibility"]),
        )


@dataclass(frozen=True, slots=True)
class StyleLayerResult:
    """Complete layer metadata borrowed for a completion callback.

    See `mln_style_layer_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    info: StyleLayerInfo
    source_id: str | None = None
    source_layer: str | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            info=StyleLayerInfo._from_native(raw["info"]),
            source_id=raw["source_id"],
            source_layer=raw["source_layer"],
        )


@dataclass(frozen=True, slots=True)
class StyleSourceInfo:
    """Fixed source metadata included in `mln_style_source_result`.

    See `mln_style_source_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    type: StyleSourceType
    id_size: int
    is_volatile: bool
    attribution_size: int | None = None
    url_size: int | None = None
    tilejson: StyleSourceTileInfo | None = None
    bounds: LatLngBounds | None = None
    tile_size: int | None = None
    vector_encoding: StyleVectorTileEncoding | None = None
    raster_encoding: StyleRasterDemEncoding | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            type=StyleSourceType(raw["type"]),
            id_size=raw["id_size"],
            is_volatile=raw["is_volatile"],
            attribution_size=raw["attribution_size"],
            url_size=raw["url_size"],
            tilejson=_maybe(StyleSourceTileInfo._from_native, raw["tilejson"]),
            bounds=_maybe(LatLngBounds._from_native, raw["bounds"]),
            tile_size=raw["tile_size"],
            vector_encoding=_maybe(StyleVectorTileEncoding, raw["vector_encoding"]),
            raster_encoding=_maybe(StyleRasterDemEncoding, raw["raster_encoding"]),
        )


@dataclass(frozen=True, slots=True)
class StyleSourceResult:
    """Complete source metadata borrowed for a completion callback.

    See `mln_style_source_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    info: StyleSourceInfo
    attribution: str | None = None
    url: str | None = None
    tile_urls: tuple[str, ...] | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            info=StyleSourceInfo._from_native(raw["info"]),
            attribution=raw["attribution"],
            url=raw["url"],
            tile_urls=None
            if raw["tile_urls"] is None
            else (tuple(item for item in raw["tile_urls"])),
        )


@dataclass(frozen=True, slots=True)
class StyleSourceTileInfo:
    """Inline tile metadata selected as one value by the source-info field mask.

    See `mln_style_source_tile_info` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    tile_count: int
    min_zoom: float
    max_zoom: float
    scheme: StyleTileScheme

    @classmethod
    def _from_native(cls, raw):
        return cls(
            tile_count=raw["tile_count"],
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            scheme=StyleTileScheme(raw["scheme"]),
        )


@dataclass(frozen=True, slots=True)
class StyleSourceTileUrlsResult:
    """Borrowed inline TileJSON tile URLs available during a completion
    callback.

    See `mln_style_source_tile_urls_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    tile_urls: tuple[str, ...]

    @classmethod
    def _from_native(cls, raw):
        return cls(tile_urls=tuple(item for item in raw["tile_urls"]))


@dataclass(frozen=True, slots=True)
class StyleTileSourceOptions:
    """Options for vector and raster tile sources.

    See `mln_style_tile_source_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    min_zoom: float | None = None
    max_zoom: float | None = None
    attribution: str | None = None
    scheme: StyleTileScheme | None = None
    bounds: LatLngBounds | None = None
    tile_size: int | None = None
    vector_encoding: StyleVectorTileEncoding | None = None
    raster_encoding: StyleRasterDemEncoding | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            attribution=raw["attribution"],
            scheme=_maybe(StyleTileScheme, raw["scheme"]),
            bounds=_maybe(LatLngBounds._from_native, raw["bounds"]),
            tile_size=raw["tile_size"],
            vector_encoding=_maybe(StyleVectorTileEncoding, raw["vector_encoding"]),
            raster_encoding=_maybe(StyleRasterDemEncoding, raw["raster_encoding"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_style_tile_source_options())


@dataclass(frozen=True, slots=True)
class StyleTransitionOptions:
    """Global style transition options.

    See `mln_style_transition_options` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    """

    duration_ms: float | None = None
    delay_ms: float | None = None
    enable_placement_transitions: bool | None = None

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
    height: int
    stride: int
    byte_length: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            width=raw["width"],
            height=raw["height"],
            stride=raw["stride"],
            byte_length=raw["byte_length"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_texture_image_info())


@dataclass(frozen=True, slots=True)
class TextureReadbackResult:
    """Texture readback borrowed for a completion callback.

    See `mln_texture_readback_result` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    data: bytes
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
class VulkanBorrowedTextureDescriptor:
    """Vulkan attachment options for a borrowed texture target.

    See `mln_vulkan_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: RenderTargetExtent
    physical_width: int
    physical_height: int
    context: VulkanContextDescriptor
    image: int
    image_view: int
    format: int
    initial_layout: int
    final_layout: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            context=VulkanContextDescriptor._from_native(raw["context"]),
            image=raw["image"],
            image_view=raw["image_view"],
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
    physical_device: int
    device: int
    graphics_queue: int
    graphics_queue_family_index: int
    get_instance_proc_addr: int
    get_device_proc_addr: int

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

    extent: RenderTargetExtent
    context: VulkanContextDescriptor

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=VulkanContextDescriptor._from_native(raw["context"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_vulkan_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class VulkanOwnedTextureFrame:
    """Vulkan frame acquired from a session-owned texture target.

    See `mln_vulkan_owned_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    image: int
    image_view: int
    device: int
    format: int
    layout: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            image=raw["image"],
            image_view=raw["image_view"],
            device=raw["device"],
            format=raw["format"],
            layout=raw["layout"],
        )


@dataclass(frozen=True, slots=True)
class VulkanSurfaceDescriptor:
    """Vulkan attachment options for a native surface.

    See `mln_vulkan_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: RenderTargetExtent
    context: VulkanContextDescriptor
    surface: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=VulkanContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_vulkan_surface_descriptor())


@dataclass(frozen=True, slots=True)
class Wake:
    """Receiver wake callback copied by a successful owning call.

    See `mln_wake` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
    """

    callback: Callable[[], None] | None = None

    def _invoke_callback(self):
        callback = self.callback
        assert callback is not None
        return callback()


@dataclass(frozen=True, slots=True)
class WebglContextDescriptor:
    """WebGL context fields shared by OpenGL render targets in the browser.

    See `mln_webgl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    kind: WebglContextKind
    context: int
    canvas_selector: str

    @classmethod
    def _from_native(cls, raw):
        return cls(
            kind=WebglContextKind(raw["kind"]),
            context=raw["context"],
            canvas_selector=raw["canvas_selector"],
        )


@dataclass(frozen=True, slots=True)
class WebgpuBorrowedTextureDescriptor:
    """WebGPU attachment options for a borrowed texture target.

    See `mln_webgpu_borrowed_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: RenderTargetExtent
    physical_width: int
    physical_height: int
    context: WebgpuContextDescriptor
    texture: int
    texture_view: int
    format: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            physical_width=raw["physical_width"],
            physical_height=raw["physical_height"],
            context=WebgpuContextDescriptor._from_native(raw["context"]),
            texture=raw["texture"],
            texture_view=raw["texture_view"],
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
    device: int
    queue: int

    @classmethod
    def _from_native(cls, raw):
        return cls(instance=raw["instance"], device=raw["device"], queue=raw["queue"])


@dataclass(frozen=True, slots=True)
class WebgpuOwnedTextureDescriptor:
    """WebGPU attachment options for an owned texture target.

    See `mln_webgpu_owned_texture_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    extent: RenderTargetExtent
    context: WebgpuContextDescriptor

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=WebgpuContextDescriptor._from_native(raw["context"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_webgpu_owned_texture_descriptor())


@dataclass(frozen=True, slots=True)
class WebgpuOwnedTextureFrame:
    """WebGPU frame acquired from a session-owned texture target.

    See `mln_webgpu_owned_texture_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    """

    generation: int
    width: int
    height: int
    scale_factor: float
    frame_id: int
    texture: int
    texture_view: int
    device: int
    format: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            generation=raw["generation"],
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            frame_id=raw["frame_id"],
            texture=raw["texture"],
            texture_view=raw["texture_view"],
            device=raw["device"],
            format=raw["format"],
        )


@dataclass(frozen=True, slots=True)
class WebgpuSurfaceDescriptor:
    """WebGPU attachment options for a native surface.

    See `mln_webgpu_surface_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    """

    extent: RenderTargetExtent
    context: WebgpuContextDescriptor
    surface: int
    format: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            extent=RenderTargetExtent._from_native(raw["extent"]),
            context=WebgpuContextDescriptor._from_native(raw["context"]),
            surface=raw["surface"],
            format=raw["format"],
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_webgpu_surface_descriptor())


@dataclass(frozen=True, slots=True)
class WglContextDescriptor:
    """WGL context fields shared by OpenGL render targets on Windows.

    See `mln_wgl_context_descriptor` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
    """

    device_context: int
    share_context: int
    get_proc_address: int

    @classmethod
    def _from_native(cls, raw):
        return cls(
            device_context=raw["device_context"],
            share_context=raw["share_context"],
            get_proc_address=raw["get_proc_address"],
        )
