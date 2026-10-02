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
    DURATION = 1
    VELOCITY = 2
    MIN_ZOOM = 4
    EASING = 8
    TRANSITION_ID = 16


class BoundOptionField(IntFlag):
    BOUNDS = 1
    MIN_ZOOM = 2
    MAX_ZOOM = 4
    MIN_PITCH = 8
    MAX_PITCH = 16
    UNBOUNDED = 32


class CameraChangeMode(UnknownIntEnum):
    IMMEDIATE = 0
    ANIMATED = 1


class CameraDeltaKind(UnknownIntEnum):
    MOVE = 0
    SCALE = 1
    BEARING = 2
    PITCH = 3


class CameraFitOptionField(IntFlag):
    PADDING = 1
    BEARING = 2
    PITCH = 4


class CameraOptionField(IntFlag):
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
    JUMP = 0
    EASE = 1
    FLY = 2


class CommandDisposition(UnknownIntEnum):
    COMMITTED = 0
    SUPERSEDED = 1
    FAILED = 2
    CANCELLED = 3


class ConstrainMode(UnknownIntEnum):
    NONE = 0
    HEIGHT_ONLY = 1
    WIDTH_AND_HEIGHT = 2
    SCREEN = 3


class CustomGeometrySourceOptionField(IntFlag):
    MIN_ZOOM = 1
    MAX_ZOOM = 2
    TOLERANCE = 4
    TILE_SIZE = 8
    BUFFER = 16
    CLIP = 32
    WRAP = 64


class CustomMvtVectorSourceOptionField(IntFlag):
    MIN_ZOOM = 1
    MAX_ZOOM = 2


class FeatureStateSelectorField(IntFlag):
    SOURCE_LAYER_ID = 1
    FEATURE_ID = 2
    STATE_KEY = 4


class FrameDemandFlag(IntFlag):
    IF_NEEDED = 1
    PRESENT = 2


class FreeCameraOptionField(IntFlag):
    POSITION = 1
    ORIENTATION = 2


class GeojsonSourceOptionField(IntFlag):
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
    NONE = 0
    BEGIN = 1
    UPDATE = 2
    END = 3
    CANCEL = 4


class GpuSyncKind(UnknownIntEnum):
    CPU_COMPLETE = 0
    METAL_SHARED_EVENT = 1
    VULKAN_TIMELINE_SEMAPHORE = 2
    OPENGL_FENCE = 3
    WEBGPU_TOKEN = 4


class LocationIndicatorImageKind(UnknownIntEnum):
    TOP = 0
    BEARING = 1
    SHADOW = 2


class LogEvent(UnknownIntEnum):
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
    INFO = 1
    WARNING = 2
    ERROR = 3


class LogSeverityMask(IntFlag):
    INFO = 2
    WARNING = 4
    ERROR = 8
    DEFAULT = 6
    ALL = 14


class MapDebugOption(IntFlag):
    TILE_BORDERS = 2
    PARSE_STATUS = 4
    TIMESTAMPS = 8
    COLLISION = 16
    OVERDRAW = 32
    STENCIL_CLIP = 64
    DEPTH_BUFFER = 128


class MapMode(UnknownIntEnum):
    CONTINUOUS = 0
    STATIC = 1
    TILE = 2


class MapTileOptionField(IntFlag):
    PREFETCH_ZOOM_DELTA = 1
    LOD_MIN_RADIUS = 2
    LOD_SCALE = 4
    LOD_PITCH_THRESHOLD = 8
    LOD_ZOOM_SHIFT = 16
    LOD_MODE = 32


class MapViewportOptionField(IntFlag):
    NORTH_ORIENTATION = 1
    CONSTRAIN_MODE = 2
    VIEWPORT_MODE = 4
    FRUSTUM_OFFSET = 8


class NetworkStatus(UnknownIntEnum):
    ONLINE = 1
    OFFLINE = 2


class NorthOrientation(UnknownIntEnum):
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
    UNSPECIFIED = 0
    GL = 1
    GLES = 2


class OpenglContextOwnership(UnknownIntEnum):
    SHARED = 0
    DEDICATED = 1


class OpenglContextPlatform(UnknownIntEnum):
    UNSPECIFIED = 0
    WGL = 1
    EGL = 2
    WEBGL = 3


class OpenglContextProviderFlag(IntFlag):
    WGL = 1
    EGL = 2
    WEBGL = 4


class ProjectionModeField(IntFlag):
    AXONOMETRIC = 1
    X_SKEW = 2
    Y_SKEW = 4


class QueriedFeatureField(IntFlag):
    SOURCE_ID = 1
    SOURCE_LAYER_ID = 2
    STATE = 4


class RenderAbandonDisposition(UnknownIntEnum):
    CLEAN = 0
    QUARANTINED = 1


class RenderBackendFlag(IntFlag):
    METAL = 1
    VULKAN = 2
    OPENGL = 4
    WEBGPU = 8


class RenderDriverKind(UnknownIntEnum):
    CORE_WORKER = 1
    CALLER_GRAPHICS_THREAD = 2


class RenderMode(UnknownIntEnum):
    PARTIAL = 0
    FULL = 1


class RenderResult(UnknownIntEnum):
    RENDERED = 0
    NO_UPDATE = 1
    SIZE_PENDING = 2
    TARGET_NOT_READY = 3
    SUPERSEDED = 4
    DEADLINE_MISSED = 5


class RenderSessionCapabilityFlag(IntFlag):
    FRAME_ACQUISITION = 1
    READBACK = 2
    CONSUMER_SYNC = 4
    PRESENTATION = 8


class RenderSessionState(UnknownIntEnum):
    ATTACHING = 1
    ATTACHED = 2
    DETACHING = 3
    DETACHED = 4
    TARGET_LOST = 5
    ABANDONED = 6


class RenderedFeatureQueryOptionField(IntFlag):
    IDS = 1


class RenderedQueryGeometryType(UnknownIntEnum):
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
    NONE = 0
    RENDER_FRAME = 1
    RENDER_MAP = 2
    TILE_ACTION = 4
    OFFLINE_REGION_STATUS = 5
    OFFLINE_REGION_RESPONSE_ERROR = 6
    OFFLINE_REGION_TILE_COUNT_LIMIT = 7
    CAMERA_TRANSITION_FINISHED = 9


class RuntimeEventSourceType(UnknownIntEnum):
    RUNTIME = 0
    MAP = 1


class RuntimeEventType(UnknownIntEnum):
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
    IDS = 1


class Status(UnknownIntEnum):
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
    PIXEL_RATIO = 1
    SDF = 2
    STRETCH_X = 4
    STRETCH_Y = 8
    CONTENT = 16
    TEXT_FIT_WIDTH = 32
    TEXT_FIT_HEIGHT = 64


class StyleImageTextFit(UnknownIntEnum):
    STRETCH_OR_SHRINK = 0
    STRETCH_ONLY = 1
    PROPORTIONAL = 2


class StyleLayerVisibility(UnknownIntEnum):
    VISIBLE = 0
    NONE = 1


class StyleRasterDemEncoding(UnknownIntEnum):
    MAPBOX = 0
    TERRARIUM = 1


class StyleSourceInfoField(IntFlag):
    URL = 1
    TILEJSON = 2
    BOUNDS = 4
    TILE_SIZE = 8
    VECTOR_ENCODING = 16
    RASTER_ENCODING = 32


class StyleSourceType(UnknownIntEnum):
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
    XYZ = 0
    TMS = 1


class StyleTileSourceOptionField(IntFlag):
    MIN_ZOOM = 1
    MAX_ZOOM = 2
    ATTRIBUTION = 4
    SCHEME = 8
    BOUNDS = 16
    TILE_SIZE = 32
    VECTOR_ENCODING = 64
    RASTER_ENCODING = 128


class StyleTransitionOptionField(IntFlag):
    DURATION = 1
    DELAY = 2
    ENABLE_PLACEMENT_TRANSITIONS = 4


class StyleVectorTileEncoding(UnknownIntEnum):
    MVT = 0
    MLT = 1


class TileLodMode(UnknownIntEnum):
    DEFAULT = 0
    DISTANCE = 1


class TileOperation(UnknownIntEnum):
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
    DEFAULT = 0
    FLIPPED_Y = 1


class WebglContextKind(UnknownIntEnum):
    EXISTING = 0
    TRANSFERRED_CANVAS = 1


@dataclass(frozen=True, slots=True)
class AnimationOptions:
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
    z: int
    x: int
    y: int

    @classmethod
    def _from_native(cls, raw):
        return cls(z=raw["z"], x=raw["x"], y=raw["y"])


@dataclass(frozen=True, slots=True)
class CustomGeometrySourceOptions:
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
            fetch_tile=raw["fetch_tile"],
            cancel_tile=raw["cancel_tile"],
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
    fetch_tile: Callable[[CanonicalTileId], None] | None = None
    cancel_tile: Callable[[CanonicalTileId], None] | None = None
    min_zoom: float | None = None
    max_zoom: float | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(
            fetch_tile=raw["fetch_tile"],
            cancel_tile=raw["cancel_tile"],
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
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

        return cls._from_native(_native._default_custom_mvt_vector_source_options())


@dataclass(frozen=True, slots=True)
class EdgeInsets:
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
class FeatureStateSelector:
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

    @classmethod
    def _from_native(cls, raw):
        return cls(callback=raw["callback"])

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
    from_: float
    to: float

    @classmethod
    def _from_native(cls, raw):
        return cls(from_=raw["from_"], to=raw["to"])


@dataclass(frozen=True, slots=True)
class LatLng:
    latitude: float
    longitude: float

    @classmethod
    def _from_native(cls, raw):
        return cls(latitude=raw["latitude"], longitude=raw["longitude"])


@dataclass(frozen=True, slots=True)
class LatLngBounds:
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

    @classmethod
    def _from_native(cls, raw):
        return cls(callback=raw["callback"])

    def _invoke_callback(self, severity, event, code, message):
        callback = self.callback
        assert callback is not None
        return callback(LogSeverity(severity), LogEvent(event), code, message)


@dataclass(frozen=True, slots=True)
class LogicalExtent:
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
    device: int

    @classmethod
    def _from_native(cls, raw):
        return cls(device=raw["device"])


@dataclass(frozen=True, slots=True)
class MetalOwnedTextureDescriptor:
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
    northing: float
    easting: float

    @classmethod
    def _from_native(cls, raw):
        return cls(northing=raw["northing"], easting=raw["easting"])


@dataclass(frozen=True, slots=True)
class ProjectionMode:
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
    x: float
    y: float
    z: float
    w: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"], z=raw["z"], w=raw["w"])


@dataclass(frozen=True, slots=True)
class QueriedFeature:
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
class RenderFrameResult:
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
    driver: RenderDriverKind
    requested_texture_ring_depth: int
    frame_wake: Wake
    driver_work_wake: Wake

    @classmethod
    def _from_native(cls, raw):
        return cls(
            driver=RenderDriverKind(raw["driver"]),
            requested_texture_ring_depth=raw["requested_texture_ring_depth"],
            frame_wake=Wake._from_native(raw["frame_wake"]),
            driver_work_wake=Wake._from_native(raw["driver_work_wake"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_render_session_attach_options())


@dataclass(frozen=True, slots=True)
class RenderSessionCapabilities:
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

    @classmethod
    def _from_native(cls, raw):
        return cls(callback=raw["callback"])

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

    @classmethod
    def _from_native(cls, raw):
        return cls(callback=raw["callback"])

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
class RuntimeEventBatchView:
    events: tuple[RuntimeEvent, ...]

    @classmethod
    def _from_native(cls, raw):
        return cls(
            events=tuple(RuntimeEvent._from_native(item) for item in raw["events"])
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventCameraTransitionFinished:
    transition_id: int

    @classmethod
    def _from_native(cls, raw):
        return cls(transition_id=raw["transition_id"])


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionResponseError:
    region_id: int
    reason: ResourceErrorReason

    @classmethod
    def _from_native(cls, raw):
        return cls(
            region_id=raw["region_id"], reason=ResourceErrorReason(raw["reason"])
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventOfflineRegionStatus:
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
    region_id: int
    limit: int

    @classmethod
    def _from_native(cls, raw):
        return cls(region_id=raw["region_id"], limit=raw["limit"])


@dataclass(frozen=True, slots=True)
class RuntimeEventRenderFrame:
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
    mode: RenderMode

    @classmethod
    def _from_native(cls, raw):
        return cls(mode=RenderMode(raw["mode"]))


@dataclass(frozen=True, slots=True)
class RuntimeEventTileAction:
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
            event_wake=Wake._from_native(raw["event_wake"]),
        )

    @classmethod
    def default(cls):
        from . import _native

        return cls._from_native(_native._default_runtime_options())


@dataclass(frozen=True, slots=True)
class ScreenBox:
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
    points: tuple[ScreenPoint, ...]

    @classmethod
    def _from_native(cls, raw):
        return cls(
            points=tuple(ScreenPoint._from_native(item) for item in raw["points"])
        )


@dataclass(frozen=True, slots=True)
class ScreenPoint:
    x: float
    y: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"])


@dataclass(frozen=True, slots=True)
class SourceFeatureQueryOptions:
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
    tile_urls: tuple[str, ...]

    @classmethod
    def _from_native(cls, raw):
        return cls(tile_urls=tuple(item for item in raw["tile_urls"]))


@dataclass(frozen=True, slots=True)
class StyleTileSourceOptions:
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
    data: bytes
    info: TextureImageInfo

    @classmethod
    def _from_native(cls, raw):
        return cls(data=raw["data"], info=TextureImageInfo._from_native(raw["info"]))


@dataclass(frozen=True, slots=True)
class TileId:
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
    x1: float
    y1: float
    x2: float
    y2: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x1=raw["x1"], y1=raw["y1"], x2=raw["x2"], y2=raw["y2"])


@dataclass(frozen=True, slots=True)
class Vec3:
    x: float
    y: float
    z: float

    @classmethod
    def _from_native(cls, raw):
        return cls(x=raw["x"], y=raw["y"], z=raw["z"])


@dataclass(frozen=True, slots=True)
class VulkanBorrowedTextureDescriptor:
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
    callback: Callable[[], None] | None = None

    @classmethod
    def _from_native(cls, raw):
        return cls(callback=raw["callback"])

    def _invoke_callback(self):
        callback = self.callback
        assert callback is not None
        return callback()


@dataclass(frozen=True, slots=True)
class WebglContextDescriptor:
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
    instance: int
    device: int
    queue: int

    @classmethod
    def _from_native(cls, raw):
        return cls(instance=raw["instance"], device=raw["device"], queue=raw["queue"])


@dataclass(frozen=True, slots=True)
class WebgpuOwnedTextureDescriptor:
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
