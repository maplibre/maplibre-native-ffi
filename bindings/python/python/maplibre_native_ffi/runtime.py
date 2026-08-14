"""Runtime values and handles for the Python binding."""

from __future__ import annotations

import weakref
from collections.abc import Callable
from dataclasses import dataclass, field
from enum import IntFlag
from typing import Any

from . import _native
from ._enum import UnknownIntEnum
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
    OFFLINE_OPERATION_COMPLETED = 22
    MAP_CAMERA_TRANSITION_FINISHED = 23


class RuntimeEventMask(IntFlag):
    """Mask of event types a map or a runtime queues.

    Each bit is ``1 << RuntimeEventType``, so a host derives a bit from a type
    it read from an event. A map or a runtime builds and queues only the types
    its mask selects, so narrowing a mask also removes those events' wake
    signals.

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
    OFFLINE_OPERATION_COMPLETED = 1 << RuntimeEventType.OFFLINE_OPERATION_COMPLETED
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
        | OFFLINE_OPERATION_COMPLETED
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
    """Runtime camera transition-finished event payload.

    A transition carrying a ``transition_id`` reports its end once for every
    terminal outcome, without naming which outcome occurred.
    """

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

    remaining_count: int
    """Events still queued after this batch.

    An unbounded drain leaves this at zero. A nonzero value means the
    ``max_events`` bound ended the batch early, so another drain reports more
    events.
    """

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
            remaining_count=raw["remaining_count"],
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


class WakeSource(NativeHandleMixin):
    """Releases a runtime owner thread parked in :meth:`RuntimeHandle.pump`.

    Usable from any thread. It stays usable after its runtime closes, and
    signalling it then does nothing.
    """

    _handle_name = "WakeSource"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _WAKE_SOURCE_CREATE_KEY:
            msg = "WakeSource instances are created by RuntimeHandle.wake_source()"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> WakeSource:
        return cls(native, _create_key=_WAKE_SOURCE_CREATE_KEY)

    def signal(self) -> None:
        """Set the runtime's wake flag and release the parked owner thread.

        A signal raised while the owner thread runs makes the next
        :meth:`RuntimeHandle.pump` return without parking.
        """
        self._native.signal()


_WAKE_SOURCE_CREATE_KEY = object()


class RuntimeHandle(NativeHandleMixin):
    """Owner-thread runtime handle."""

    _handle_name = "RuntimeHandle"

    def __init__(self, options: RuntimeOptions | None = None) -> None:
        options = options or RuntimeOptions()
        self._native = _native.create_runtime(
            options.asset_path,
            options.cache_path,
            int(options.event_mask),
        )
        self._offline_operations: weakref.WeakSet[OfflineOperationHandle] = (
            weakref.WeakSet()
        )
        self._maps: dict[int, weakref.ReferenceType[MapHandle]] = {}

    def close(self) -> None:
        """Release this runtime handle exactly once."""
        if self._offline_operations:
            from .errors import InvalidStateError

            raise InvalidStateError(None, "runtime has live offline operation handles")
        self._native.close()

    def _register_offline_operation(self, operation: OfflineOperationHandle) -> None:
        self._offline_operations.add(operation)

    def _unregister_offline_operation(self, operation: OfflineOperationHandle) -> None:
        self._offline_operations.discard(operation)

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

    def pump(self, timeout: float | None = 0.0, budget: float | None = None) -> None:
        """Advance this runtime.

        The call parks the owner thread when ``timeout`` allows it, then drains
        the owner-thread task queues. Take the queued runtime events with
        :meth:`drain_events` afterwards.

        ``timeout`` is in seconds and bounds the park: zero drains and returns,
        a positive value parks for up to that long, and ``None`` parks until a
        wake arrives. Timers and ready file descriptors set the wake flag only
        when they queue owner-thread work, so pass a bounded timeout to cap how
        long a call waits.

        ``budget`` is in seconds and bounds the drain: ``None`` drains without
        a bound, and a value stops the drain at the first task boundary after
        that long, measured from the start of the drain. The first queued task
        always runs, so a bounded pump always makes progress, and tasks left
        behind set the wake flag so the next pump returns without parking and
        continues them. The budget bounds the task queues alone: expired
        timers and ready file descriptors are serviced regardless, and a
        single task runs to completion once started, so one long task can
        overrun the budget.

        A non-zero timeout releases the GIL while it parks. Call it outside any
        lock that a signalling thread takes.
        """
        # A negative timeout collapses to no wait; ``None`` spells an unbounded
        # park.
        timeout_ms = -1 if timeout is None else max(0, int(timeout * 1000))
        # ``None`` spells an unbounded drain.
        budget_ms = -1 if budget is None else max(0, int(budget * 1000))
        self._native.pump(timeout_ms, budget_ms)

    def wake_source(self) -> WakeSource:
        """Acquire a wake source for this runtime, usable from any thread."""
        return WakeSource._from_native(self._native.wake_source())

    def _offline_operation(
        self, start: Callable[..., int], *args: object
    ) -> OfflineOperationHandle:
        from .offline import OfflineOperationHandle

        return OfflineOperationHandle._from_native(self, start(*args))

    def run_ambient_cache_operation(
        self, operation: AmbientCacheOperation
    ) -> OfflineOperationHandle:
        """Start an ambient cache maintenance operation."""
        from .offline import AmbientCacheOperation

        return self._offline_operation(
            self._native.run_ambient_cache_operation_start,
            AmbientCacheOperation(operation).native_code,
        )

    def set_maximum_ambient_cache_size(self, size: int) -> OfflineOperationHandle:
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
        return self._offline_operation(
            self._native.set_maximum_ambient_cache_size_start,
            size,
        )

    def create_offline_region(
        self, definition: OfflineRegionDefinition, metadata: bytes = b""
    ) -> OfflineOperationHandle:
        """Start creating an offline region."""
        return self._offline_operation(
            self._native.offline_region_create_start,
            definition,
            metadata,
        )

    def get_offline_region(self, region_id: int) -> OfflineOperationHandle:
        """Start getting an offline region snapshot by ID."""
        return self._offline_operation(self._native.offline_region_get_start, region_id)

    def list_offline_regions(self) -> OfflineOperationHandle:
        """Start listing offline region snapshots."""
        return self._offline_operation(self._native.offline_regions_list_start)

    def merge_offline_regions_database(
        self, side_database_path: str
    ) -> OfflineOperationHandle:
        """Start merging offline regions from another database path."""
        return self._offline_operation(
            self._native.offline_regions_merge_database_start,
            side_database_path,
        )

    def update_offline_region_metadata(
        self,
        region_id: int,
        metadata: bytes,
    ) -> OfflineOperationHandle:
        """Start updating opaque binary metadata for an offline region."""
        return self._offline_operation(
            self._native.offline_region_update_metadata_start,
            region_id,
            metadata,
        )

    def get_offline_region_status(self, region_id: int) -> OfflineOperationHandle:
        """Start getting completed/download status for an offline region."""
        return self._offline_operation(
            self._native.offline_region_get_status_start, region_id
        )

    def set_offline_region_observed(
        self, region_id: int, observed: bool
    ) -> OfflineOperationHandle:
        """Start enabling or disabling runtime events for an offline region."""
        return self._offline_operation(
            self._native.offline_region_set_observed_start,
            region_id,
            observed,
        )

    def set_offline_region_download_state(
        self,
        region_id: int,
        state: OfflineRegionDownloadState,
    ) -> OfflineOperationHandle:
        """Start setting an offline region's native download state."""
        from .offline import OfflineRegionDownloadState

        return self._offline_operation(
            self._native.offline_region_set_download_state_start,
            region_id,
            OfflineRegionDownloadState(state).native_code_for_set(),
        )

    def invalidate_offline_region(self, region_id: int) -> OfflineOperationHandle:
        """Start invalidating cached resources for an offline region."""
        return self._offline_operation(
            self._native.offline_region_invalidate_start, region_id
        )

    def delete_offline_region(self, region_id: int) -> OfflineOperationHandle:
        """Start deleting an offline region."""
        return self._offline_operation(
            self._native.offline_region_delete_start, region_id
        )

    def set_resource_transform(
        self,
        callback: ResourceTransformCallback,
        *,
        max_pending_callbacks: int = 64,
    ) -> None:
        """Install or replace the runtime-scoped network URL transform."""
        from .resource import _adapt_resource_transform_callback

        self._native.set_resource_transform(
            _adapt_resource_transform_callback(callback),
            max_pending_callbacks,
        )

    def clear_resource_transform(self) -> None:
        """Clear the runtime-scoped network URL transform."""
        self._native.clear_resource_transform()

    def set_http_header_transform(
        self,
        callback: HttpHeaderTransformCallback,
        *,
        max_pending_callbacks: int = 64,
    ) -> None:
        """Install or replace the outgoing HTTP header transform."""
        from .resource import _adapt_http_header_transform_callback

        self._native.set_http_header_transform(
            _adapt_http_header_transform_callback(callback),
            max_pending_callbacks,
        )

    def clear_http_header_transform(self) -> None:
        """Clear the outgoing HTTP header transform."""
        self._native.clear_http_header_transform()

    def set_resource_provider(
        self,
        callback: ResourceProviderCallback,
        *,
        max_pending_callbacks: int = 64,
    ) -> None:
        """Install or replace the runtime-scoped network resource provider.

        Replacement is allowed while maps are live. When this call returns, the
        previous callback is retired, but requests it already took a handle for
        keep that handle and are completed and released as usual.
        """
        from .resource import _adapt_resource_provider_callback

        self._native.set_resource_provider(
            _adapt_resource_provider_callback(callback),
            max_pending_callbacks,
        )

    def clear_resource_provider(self) -> None:
        """Clear the runtime-scoped network resource provider.

        Later requests go to MapLibre's online file source. Requests the
        previous callback already took a handle for keep that handle and are
        completed and released as usual.
        """
        self._native.clear_resource_provider()

    def drain_events(self, max_events: int = 0) -> RuntimeEventBatch:
        """Drain and copy this runtime's queued runtime events into one batch.

        ``max_events`` bounds the drain: zero drains every queued event, and a
        positive value takes at most that many and reports the rest as
        :attr:`RuntimeEventBatch.remaining_count`.

        A drain is a queue operation: it never parks and runs no owner-thread
        work. Call :meth:`pump` to advance the runtime, then drain what the pump
        produced.

        `RuntimeEvent.source.map_handle` is None once the caller drops its last
        reference to the source map.
        """
        if not 0 <= max_events < 2**64:
            # PyO3 raises a bare OverflowError extracting `usize` before the
            # binding's error conversion runs.
            raise InvalidArgumentError(
                f"max_events must fit in 64 unsigned bits, not {max_events}"
            )
        return RuntimeEventBatch._from_native(
            self._native.drain_events(max_events), runtime=self
        )

    def set_event_mask(self, mask: RuntimeEventMask) -> None:
        """Select which runtime-originated event types this runtime queues.

        The call reads the bits in :attr:`RuntimeEventMask.ALL_RUNTIME_EVENTS`
        and ignores the rest, so :attr:`RuntimeEventMask.ALL` selects every
        runtime-originated type. Narrowing gates later events and keeps queued
        ones, so a host drains what it already caused.

        Region status, response error, and tile count limit events also need an
        observed region, so this mask narrows that subscription rather than
        replacing it. An offline operation records its result before the mask is
        read, so an :class:`offline.OfflineOperationHandle` still reports its
        result with the completion type unselected.
        """
        self._native.set_event_mask(int(mask))

    @property
    def event_mask(self) -> RuntimeEventMask:
        """Runtime-originated event types this runtime queues.

        The value is the mask last set, including bits this runtime ignores, so
        a host reads it, changes one bit, and writes it back.
        """
        return RuntimeEventMask(self._native.get_event_mask())

    def create_map(self, options: MapOptions | None = None) -> MapHandle:
        """Create a map owned by this runtime."""
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
    if kind == "offline_operation_completed":
        return OfflineOperationCompleted._from_runtime_payload(payload)
    if kind == "camera_transition_finished":
        return CameraTransitionFinishedPayload._from_runtime_payload(payload)
    return UnknownRuntimeEventPayload._from_runtime_payload(payload)


__all__ = [
    "CameraChangeMode",
    "CameraTransitionFinishedPayload",
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
    OfflineOperationCompleted,
    OfflineOperationHandle,
    OfflineRegionDefinition,
    OfflineRegionDownloadState,
    OfflineRegionResponseError,
    OfflineRegionStatusChanged,
    OfflineRegionTileCountLimitExceeded,
)

RuntimeEventPayload = (
    None
    | RenderFramePayload
    | RenderMapPayload
    | TileActionPayload
    | OfflineRegionStatusChanged
    | OfflineRegionResponseError
    | OfflineRegionTileCountLimitExceeded
    | OfflineOperationCompleted
    | CameraTransitionFinishedPayload
    | UnknownRuntimeEventPayload
)
