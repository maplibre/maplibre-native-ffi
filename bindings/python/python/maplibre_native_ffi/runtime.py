"""Runtime values and handles for the Python binding."""

from __future__ import annotations

import weakref
from collections.abc import Callable
from concurrent.futures import Future
from dataclasses import dataclass, field
from enum import IntFlag
from typing import Any

from . import _native
from ._enum import UnknownIntEnum
from ._future import map_future
from ._lifecycle import NativeHandleMixin
from .errors import InvalidArgumentError
from .resource import (
    HttpHeaderTransformCallback,
    ResourceProviderCallback,
    ResourceTransformCallback,
)


class NetworkStatus(UnknownIntEnum):
    """Process-global network reachability state."""

    ONLINE = 1
    OFFLINE = 2


class RuntimeEventType(UnknownIntEnum):
    """Runtime event type values reported by the C API.

    The event type selects the meaning of ``RuntimeEvent.code`` and the type of
    ``RuntimeEvent.payload``.
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


class RuntimeEventMask(IntFlag):
    """Mask of event types a map or a runtime queues.

    Each bit is ``1 << RuntimeEventType``, so a host derives a bit from a type
    it read from an event. A map or a runtime builds and queues only the types
    that its mask selects.

    :attr:`ALL_MAP_EVENTS` covers every map-originated type and
    :attr:`ALL_RUNTIME_EVENTS` every runtime-originated one.
    :meth:`MapHandle.set_event_mask` reads only the map bits and
    :meth:`RuntimeHandle.set_event_mask` only the runtime bits, so both accept
    :attr:`ALL`, and a read-modify-write of one bit keeps the rest.
    """

    NONE = 0
    MAP_CAMERA_WILL_CHANGE = 1 << RuntimeEventType.MAP_CAMERA_WILL_CHANGE
    MAP_CAMERA_IS_CHANGING = 1 << RuntimeEventType.MAP_CAMERA_IS_CHANGING
    MAP_CAMERA_DID_CHANGE = 1 << RuntimeEventType.MAP_CAMERA_DID_CHANGE
    MAP_STYLE_LOADED = 1 << RuntimeEventType.MAP_STYLE_LOADED
    MAP_LOADING_STARTED = 1 << RuntimeEventType.MAP_LOADING_STARTED
    MAP_LOADING_FINISHED = 1 << RuntimeEventType.MAP_LOADING_FINISHED
    MAP_LOADING_FAILED = 1 << RuntimeEventType.MAP_LOADING_FAILED
    MAP_IDLE = 1 << RuntimeEventType.MAP_IDLE
    MAP_RENDER_UPDATE_AVAILABLE = 1 << RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE
    MAP_RENDER_ERROR = 1 << RuntimeEventType.MAP_RENDER_ERROR
    MAP_STILL_IMAGE_FINISHED = 1 << RuntimeEventType.MAP_STILL_IMAGE_FINISHED
    MAP_STILL_IMAGE_FAILED = 1 << RuntimeEventType.MAP_STILL_IMAGE_FAILED
    MAP_RENDER_FRAME_STARTED = 1 << RuntimeEventType.MAP_RENDER_FRAME_STARTED
    MAP_RENDER_FRAME_FINISHED = 1 << RuntimeEventType.MAP_RENDER_FRAME_FINISHED
    MAP_RENDER_MAP_STARTED = 1 << RuntimeEventType.MAP_RENDER_MAP_STARTED
    MAP_RENDER_MAP_FINISHED = 1 << RuntimeEventType.MAP_RENDER_MAP_FINISHED
    MAP_STYLE_IMAGE_MISSING = 1 << RuntimeEventType.MAP_STYLE_IMAGE_MISSING
    MAP_TILE_ACTION = 1 << RuntimeEventType.MAP_TILE_ACTION
    MAP_CAMERA_TRANSITION_FINISHED = (
        1 << RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED
    )
    OFFLINE_REGION_STATUS_CHANGED = 1 << RuntimeEventType.OFFLINE_REGION_STATUS_CHANGED
    OFFLINE_REGION_RESPONSE_ERROR = 1 << RuntimeEventType.OFFLINE_REGION_RESPONSE_ERROR
    OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = (
        1 << RuntimeEventType.OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED
    )
    ALL_MAP_EVENTS = (
        MAP_CAMERA_WILL_CHANGE
        | MAP_CAMERA_IS_CHANGING
        | MAP_CAMERA_DID_CHANGE
        | MAP_STYLE_LOADED
        | MAP_LOADING_STARTED
        | MAP_LOADING_FINISHED
        | MAP_LOADING_FAILED
        | MAP_IDLE
        | MAP_RENDER_UPDATE_AVAILABLE
        | MAP_RENDER_ERROR
        | MAP_STILL_IMAGE_FINISHED
        | MAP_STILL_IMAGE_FAILED
        | MAP_RENDER_FRAME_STARTED
        | MAP_RENDER_FRAME_FINISHED
        | MAP_RENDER_MAP_STARTED
        | MAP_RENDER_MAP_FINISHED
        | MAP_STYLE_IMAGE_MISSING
        | MAP_TILE_ACTION
        | MAP_CAMERA_TRANSITION_FINISHED
    )
    ALL_RUNTIME_EVENTS = (
        OFFLINE_REGION_STATUS_CHANGED
        | OFFLINE_REGION_RESPONSE_ERROR
        | OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED
    )
    ALL = ALL_MAP_EVENTS | ALL_RUNTIME_EVENTS


# The C options defaults select every type the linked library reports, which is
# a superset of :attr:`RuntimeEventMask.ALL` when that library is newer than
# this build. ``IntFlag`` keeps the unnamed bits, so those types stay selected
# and reach a host as unknown event and payload domains.
def _default_runtime_event_mask() -> RuntimeEventMask:
    return RuntimeEventMask(_native.runtime_options_default_event_mask())


def _default_map_event_mask() -> RuntimeEventMask:
    return RuntimeEventMask(_native.map_options_default_event_mask())


class CameraChangeMode(UnknownIntEnum):
    """Camera change kinds reported as ``code`` by camera change events."""

    IMMEDIATE = 0
    ANIMATED = 1


class CommandDisposition(UnknownIntEnum):
    """Terminal disposition of an accepted command."""

    COMMITTED = 0
    SUPERSEDED = 1
    FAILED = 2
    CANCELLED = 3


@dataclass(frozen=True, slots=True)
class CommandCompletion:
    """Terminal result of an accepted map command."""

    disposition: CommandDisposition
    generation: int
    native_status_code: int
    diagnostic: str

    def __post_init__(self) -> None:
        object.__setattr__(self, "disposition", CommandDisposition(self.disposition))


class RuntimeEventSourceType(UnknownIntEnum):
    """Runtime event source kind values reported by the C API."""

    RUNTIME = 0
    MAP = 1


class RenderMode(UnknownIntEnum):
    """Render modes reported by runtime render events."""

    PARTIAL = 0
    FULL = 1


class TileOperation(UnknownIntEnum):
    """Tile operations reported by runtime tile events."""

    REQUESTED_FROM_CACHE = 0
    REQUESTED_FROM_NETWORK = 1
    LOAD_FROM_NETWORK = 2
    LOAD_FROM_CACHE = 3
    START_PARSE = 4
    END_PARSE = 5
    ERROR = 6
    CANCELLED = 7
    NULL = 8


@dataclass(frozen=True, slots=True)
class RenderingStats:
    """Rendering statistics copied from a render-frame event."""

    encoding_time: float
    rendering_time: float
    frame_count: int
    draw_call_count: int
    total_draw_call_count: int

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> RenderingStats:
        return cls(
            encoding_time=raw["encoding_time"],
            rendering_time=raw["rendering_time"],
            frame_count=raw["frame_count"],
            draw_call_count=raw["draw_call_count"],
            total_draw_call_count=raw["total_draw_call_count"],
        )


@dataclass(frozen=True, slots=True)
class RenderFramePayload:
    """Runtime render-frame event payload."""

    mode: RenderMode
    needs_repaint: bool
    placement_changed: bool
    stats: RenderingStats

    @classmethod
    def _from_runtime_payload(cls, payload: dict[str, object]) -> RenderFramePayload:
        return cls(
            mode=RenderMode(payload["mode"]),
            needs_repaint=payload["needs_repaint"],
            placement_changed=payload["placement_changed"],
            stats=RenderingStats._from_native(payload["stats"]),
        )


@dataclass(frozen=True, slots=True)
class RenderMapPayload:
    """Runtime render-map event payload."""

    mode: RenderMode

    @classmethod
    def _from_runtime_payload(cls, payload: dict[str, object]) -> RenderMapPayload:
        return cls(mode=RenderMode(payload["mode"]))


@dataclass(frozen=True, slots=True)
class TileId:
    """Overscaled tile identity copied from a tile-action event."""

    overscaled_z: int
    wrap: int
    canonical_z: int
    canonical_x: int
    canonical_y: int

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> TileId:
        return cls(
            overscaled_z=raw["overscaled_z"],
            wrap=raw["wrap"],
            canonical_z=raw["canonical_z"],
            canonical_x=raw["canonical_x"],
            canonical_y=raw["canonical_y"],
        )


@dataclass(frozen=True, slots=True)
class TileActionPayload:
    """Runtime tile-action event payload.

    ``RuntimeEvent.message`` carries the source ID.
    """

    operation: TileOperation
    tile_id: TileId

    @classmethod
    def _from_runtime_payload(cls, payload: dict[str, object]) -> TileActionPayload:
        return cls(
            operation=TileOperation(payload["operation"]),
            tile_id=TileId._from_native(payload["tile_id"]),
        )


@dataclass(frozen=True, slots=True)
class CameraTransitionFinishedPayload:
    """Runtime camera transition-finished event payload."""

    transition_id: int

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> CameraTransitionFinishedPayload:
        return cls(transition_id=payload["transition_id"])


@dataclass(frozen=True, slots=True)
class UnknownRuntimeEventPayload:
    """Forward-compatible runtime event payload bytes.

    ``data`` is the event's whole payload window, so a payload type a later
    library version defines arrives unchanged.
    """

    raw_type: int
    data: bytes

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> UnknownRuntimeEventPayload:
        return cls(raw_type=payload["raw_type"], data=payload["bytes"])


@dataclass(frozen=True, slots=True)
class RuntimeEventSource:
    """Copied runtime event source metadata."""

    source_type: RuntimeEventSourceType
    """Kind of runtime object the event came from."""

    source_id: int
    """Native identity of the object the event came from.

    The value names one object for the life of the process, so it stays
    comparable after that object is released, and it is set even when
    ``source_type`` is unknown or ``map_handle`` resolves to None. It is an
    identity value only: no map can be reopened from it.
    """

    map_handle: MapHandle | None = None
    """Source map, or None once the map is closed or unreferenced.

    The runtime holds its maps weakly. Keep a reference to every map whose
    events you route by handle.
    """


@dataclass(frozen=True, slots=True)
class RuntimeEvent:
    """Runtime event copied into Python-owned values.

    ``event_type`` selects the meaning of ``code`` and the type of ``payload``.
    """

    event_type: RuntimeEventType
    source: RuntimeEventSource
    code: int

    message: str | None
    """Event text, which ``event_type`` names.

    A style-image-missing event carries the image ID here and a tile-action
    event the source ID. Failure events carry their native failure text.
    """

    payload: RuntimeEventPayload

    @classmethod
    def _from_native(
        cls,
        raw: dict[str, Any],
        runtime: RuntimeHandle | None = None,
    ) -> RuntimeEvent:
        source_type = RuntimeEventSourceType(raw["source_type"])
        source_id = raw["source_id"]
        return cls(
            event_type=RuntimeEventType(raw["event_type"]),
            source=RuntimeEventSource(
                source_type=source_type,
                source_id=source_id,
                map_handle=(
                    runtime._map_for_source_id(source_id)
                    if runtime is not None and source_type == RuntimeEventSourceType.MAP
                    else None
                ),
            ),
            code=raw["code"],
            message=raw["message"],
            payload=_runtime_payload_from_native(raw["payload"]),
        )


@dataclass(frozen=True, slots=True)
class RuntimeEventBatch:
    """Runtime events copied out of one drain.

    A batch stays readable after later drains and after the maps whose events
    it carries close, because every event is a Python-owned copy.
    """

    events: list[RuntimeEvent]
    """Drained events in queue order."""

    @classmethod
    def _from_native(
        cls,
        raw: dict[str, Any],
        runtime: RuntimeHandle | None = None,
    ) -> RuntimeEventBatch:
        return cls(
            events=[
                RuntimeEvent._from_native(event, runtime) for event in raw["events"]
            ],
        )


@dataclass(slots=True)
class RuntimeOptions:
    """Options used when creating a runtime.

    A path field left as ``None`` takes the C API default.
    """

    asset_path: str | None = None
    cache_path: str | None = None
    event_mask: RuntimeEventMask = field(default_factory=_default_runtime_event_mask)
    """Runtime-originated event types this runtime queues from creation.

    The default takes the C API creation default, which queues every type the
    linked library reports. See :meth:`RuntimeHandle.set_event_mask`.
    """


class RuntimeHandle(NativeHandleMixin):
    """Any-thread runtime handle with autonomous native execution."""

    _handle_name = "RuntimeHandle"

    def __init__(self, options: RuntimeOptions | None = None) -> None:
        options = options or RuntimeOptions()
        self._native = _native.create_runtime(
            options.asset_path,
            options.cache_path,
            int(options.event_mask),
        )
        self._maps: dict[int, weakref.ReferenceType[MapHandle]] = {}

    def close(self) -> None:
        """Release this runtime handle exactly once."""
        self._native.close()

    def barrier(self) -> Future[None]:
        """Return a future for all previously accepted runtime commands."""
        return self._native.barrier()

    def set_event_wake_callback(self, callback: Callable[[], None]) -> None:
        """Install the callback that schedules a later :meth:`drain_events`.

        Native code may invoke the callback from any thread. The callback must
        only arrange receiver work and must not call this binding directly.
        """
        self._native.set_event_wake_callback(callback)

    def clear_event_wake_callback(self) -> None:
        """Clear the scheduling callback after in-flight entries return."""
        self._native.clear_event_wake_callback()

    def _register_map(self, map_handle: MapHandle) -> None:
        self._maps[map_handle._native_id()] = weakref.ref(map_handle)

    def _unregister_map(self, map_handle: MapHandle) -> None:
        self._maps.pop(map_handle._native_id(), None)

    def _map_for_source_id(self, source_id: int) -> MapHandle | None:
        source = self._maps.get(source_id)
        if source is None:
            return None
        map_handle = source()
        if map_handle is None or map_handle.closed:
            self._maps.pop(source_id, None)
            return None
        return map_handle

    def run_ambient_cache_operation(
        self, operation: AmbientCacheOperation
    ) -> Future[None]:
        """Start an ambient cache maintenance operation."""
        from .offline import AmbientCacheOperation

        return self._native.run_ambient_cache_operation(
            AmbientCacheOperation(operation).native_code
        )

    def set_maximum_ambient_cache_size(self, size: int) -> Future[None]:
        """Start a change to this runtime's maximum ambient cache size.

        MapLibre evicts ambient resources to fit the new budget, so lowering it
        discards cached resources. Offline regions are unaffected.
        """
        if not 0 <= size < 2**64:
            # PyO3 raises a bare OverflowError extracting `u64` before the
            # binding's error conversion runs.
            raise InvalidArgumentError(
                f"maximum ambient cache size must fit in 64 unsigned bits, not {size}"
            )
        return self._native.set_maximum_ambient_cache_size(size)

    def create_offline_region(
        self, definition: OfflineRegionDefinition, metadata: bytes = b""
    ) -> Future[OfflineRegionInfo]:
        """Start creating an offline region."""
        return map_future(
            self._native.create_offline_region(definition, metadata),
            _adapt_region_result,
        )

    def get_offline_region(self, region_id: int) -> Future[OfflineRegionInfo | None]:
        """Start getting an offline region snapshot by ID."""
        return map_future(
            self._native.get_offline_region(region_id), _adapt_optional_region_result
        )

    def list_offline_regions(self) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Start listing offline region snapshots."""
        return map_future(
            self._native.list_offline_regions(), _adapt_region_list_result
        )

    def merge_offline_regions_database(
        self, side_database_path: str
    ) -> Future[tuple[OfflineRegionInfo, ...]]:
        """Start merging offline regions from another database path."""
        return map_future(
            self._native.merge_offline_regions_database(side_database_path),
            _adapt_region_list_result,
        )

    def update_offline_region_metadata(
        self,
        region_id: int,
        metadata: bytes,
    ) -> Future[OfflineRegionInfo]:
        """Start updating opaque binary metadata for an offline region."""
        return map_future(
            self._native.update_offline_region_metadata(region_id, metadata),
            _adapt_region_result,
        )

    def get_offline_region_status(self, region_id: int) -> Future[OfflineRegionStatus]:
        """Start getting completed/download status for an offline region."""
        return map_future(
            self._native.get_offline_region_status(region_id), _adapt_status_result
        )

    def set_offline_region_observed(
        self, region_id: int, observed: bool
    ) -> Future[None]:
        """Start enabling or disabling runtime events for an offline region."""
        return self._native.set_offline_region_observed(region_id, observed)

    def set_offline_region_download_state(
        self,
        region_id: int,
        state: OfflineRegionDownloadState,
    ) -> Future[None]:
        """Start setting an offline region's native download state."""
        from .offline import OfflineRegionDownloadState

        native_state = OfflineRegionDownloadState(state).native_code_for_set()
        return self._native.set_offline_region_download_state(region_id, native_state)

    def invalidate_offline_region(self, region_id: int) -> Future[None]:
        """Start invalidating cached resources for an offline region."""
        return self._native.invalidate_offline_region(region_id)

    def delete_offline_region(self, region_id: int) -> Future[None]:
        """Start deleting an offline region."""
        return self._native.delete_offline_region(region_id)

    def set_resource_transform(
        self,
        callback: ResourceTransformCallback,
        *,
        max_pending_callbacks: int = 64,
    ) -> Future[CommandCompletion]:
        """Install or replace the runtime-scoped network URL transform."""
        from .resource import _adapt_resource_transform_callback

        return self._native.set_resource_transform(
            _adapt_resource_transform_callback(callback),
            max_pending_callbacks,
        )

    def clear_resource_transform(self) -> Future[CommandCompletion]:
        """Clear the runtime-scoped network URL transform."""
        return self._native.clear_resource_transform()

    def set_http_header_transform(
        self,
        callback: HttpHeaderTransformCallback,
        *,
        max_pending_callbacks: int = 64,
    ) -> Future[CommandCompletion]:
        """Install or replace the outgoing HTTP header transform."""
        from .resource import _adapt_http_header_transform_callback

        return self._native.set_http_header_transform(
            _adapt_http_header_transform_callback(callback),
            max_pending_callbacks,
        )

    def clear_http_header_transform(self) -> Future[CommandCompletion]:
        """Clear the outgoing HTTP header transform."""
        return self._native.clear_http_header_transform()

    def set_resource_provider(
        self,
        callback: ResourceProviderCallback,
        *,
        max_pending_callbacks: int = 64,
    ) -> Future[CommandCompletion]:
        """Install or replace the runtime-scoped network resource provider.

        Replacement is allowed while maps are live. When this call returns, the
        previous callback is retired, but requests it already took a handle for
        keep that handle and are completed and released as usual.
        """
        from .resource import _adapt_resource_provider_callback

        return self._native.set_resource_provider(
            _adapt_resource_provider_callback(callback),
            max_pending_callbacks,
        )

    def clear_resource_provider(self) -> Future[CommandCompletion]:
        """Clear the runtime-scoped network resource provider.

        Later requests go to MapLibre's online file source. Requests the
        previous callback already took a handle for keep that handle and are
        completed and released as usual.
        """
        return self._native.clear_resource_provider()

    def drain_events(self) -> RuntimeEventBatch:
        """Drain and copy this runtime's queued runtime events into one batch.

        Draining never waits for worker progress; native execution is autonomous.

        `RuntimeEvent.source.map_handle` is None once the caller drops its last
        reference to the source map.
        """
        return RuntimeEventBatch._from_native(self._native.drain_events(), runtime=self)

    def set_event_mask(self, mask: RuntimeEventMask) -> None:
        """Select which runtime-originated event types this runtime queues.

        The call reads the bits in :attr:`RuntimeEventMask.ALL_RUNTIME_EVENTS`
        and ignores the rest, so :attr:`RuntimeEventMask.ALL` selects every
        runtime-originated type. Narrowing gates later events and keeps queued
        ones, so a host drains what it already caused.

        Region status, response error, and tile count limit events also require
        an observed region. This mask narrows that subscription.
        """
        self._native.set_event_mask(int(mask))

    @property
    def event_mask(self) -> RuntimeEventMask:
        """Runtime-originated event types this runtime queues.

        The value is the mask last set, including bits this runtime ignores, so
        a host reads it, changes one bit, and writes it back.
        """
        return RuntimeEventMask(self._native.get_event_mask())

    def create_map(self, options: MapOptions | None = None) -> Future[MapHandle]:
        """Return an eager future for a map owned by this runtime."""
        from .map import MapHandle

        return MapHandle._create(self, options)


def _runtime_payload_from_native(payload: dict[str, object]) -> RuntimeEventPayload:
    kind = payload["kind"]
    if kind == "none":
        return None
    if kind == "render_frame":
        return RenderFramePayload._from_runtime_payload(payload)
    if kind == "render_map":
        return RenderMapPayload._from_runtime_payload(payload)
    if kind == "tile_action":
        return TileActionPayload._from_runtime_payload(payload)
    if kind == "offline_region_status":
        return OfflineRegionStatusChanged._from_runtime_payload(payload)
    if kind == "offline_region_response_error":
        return OfflineRegionResponseError._from_runtime_payload(payload)
    if kind == "offline_region_tile_count_limit":
        return OfflineRegionTileCountLimitExceeded._from_runtime_payload(payload)
    if kind == "camera_transition_finished":
        return CameraTransitionFinishedPayload._from_runtime_payload(payload)
    return UnknownRuntimeEventPayload._from_runtime_payload(payload)


__all__ = [
    "CameraChangeMode",
    "CameraTransitionFinishedPayload",
    "CommandCompletion",
    "CommandDisposition",
    "NetworkStatus",
    "RenderFramePayload",
    "RenderMapPayload",
    "RenderMode",
    "RenderingStats",
    "RuntimeEvent",
    "RuntimeEventBatch",
    "RuntimeEventMask",
    "RuntimeEventPayload",
    "RuntimeEventSource",
    "RuntimeEventSourceType",
    "RuntimeEventType",
    "RuntimeHandle",
    "RuntimeOptions",
    "TileActionPayload",
    "TileId",
    "TileOperation",
    "UnknownRuntimeEventPayload",
]

from .map import MapHandle, MapOptions
from .offline import (
    AmbientCacheOperation,
    OfflineRegionDefinition,
    OfflineRegionDownloadState,
    OfflineRegionInfo,
    OfflineRegionResponseError,
    OfflineRegionStatus,
    OfflineRegionStatusChanged,
    OfflineRegionTileCountLimitExceeded,
    _adapt_optional_region_result,
    _adapt_region_list_result,
    _adapt_region_result,
    _adapt_status_result,
)

RuntimeEventPayload = (
    None
    | RenderFramePayload
    | RenderMapPayload
    | TileActionPayload
    | OfflineRegionStatusChanged
    | OfflineRegionResponseError
    | OfflineRegionTileCountLimitExceeded
    | CameraTransitionFinishedPayload
    | UnknownRuntimeEventPayload
)
