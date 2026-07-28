"""Runtime values and handles for the Python binding."""

from __future__ import annotations

from ._enum import UnknownIntEnum
from ._lifecycle import NativeHandleMixin
from collections.abc import Callable
from dataclasses import dataclass
from typing import Any
import weakref

from . import _native
from .resource import ResourceProviderCallback, ResourceTransformCallback


class NetworkStatus(UnknownIntEnum):
    """Process-global network reachability state."""

    ONLINE = 1
    OFFLINE = 2


class RuntimeEventType(UnknownIntEnum):
    """Runtime event type values reported by the C API.

    The event type selects the meaning of ``RuntimeEvent.code`` and the payload
    value carried by ``RuntimeEvent.payload``:

    - ``MAP_CAMERA_WILL_CHANGE`` and ``MAP_CAMERA_DID_CHANGE`` carry a
      :class:`CameraChangeMode` as ``code`` and no payload.
    - ``MAP_CAMERA_TRANSITION_FINISHED`` carries a
      :class:`CameraTransitionFinishedPayload`.
    - ``MAP_LOADING_FAILED`` carries the ordinal of MapLibre Native's internal
      map load error kind as ``code`` and the failure text as ``message``.
    - ``MAP_RENDER_FRAME_FINISHED`` carries a :class:`RenderFramePayload`,
      ``MAP_RENDER_MAP_FINISHED`` a :class:`RenderMapPayload`,
      ``MAP_STYLE_IMAGE_MISSING`` a :class:`StyleImageMissingPayload`, and
      ``MAP_TILE_ACTION`` a :class:`TileActionPayload`.
    - ``OFFLINE_OPERATION_COMPLETED`` carries the operation result as an
      ``MaplibreStatus`` value in ``code`` and an
      ``OfflineOperationCompleted`` payload reporting the same status.
    - The remaining offline event types carry their matching offline payload.
    - Every other event type reports ``code`` as ``0`` and no payload.
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


class CameraChangeMode(UnknownIntEnum):
    """Camera change kinds reported as ``code`` by camera change events.

    ``MAP_CAMERA_WILL_CHANGE`` and ``MAP_CAMERA_DID_CHANGE`` events report one
    of these values.
    """

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
    def _from_native(cls, raw: dict[str, object]) -> "RenderingStats":
        """Build rendering statistics from private native bridge values."""
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
    def _from_runtime_payload(cls, payload: dict[str, object]) -> "RenderFramePayload":
        """Build a render-frame payload from RuntimeEvent.payload."""
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
    def _from_runtime_payload(cls, payload: dict[str, object]) -> "RenderMapPayload":
        """Build a render-map payload from RuntimeEvent.payload."""
        return cls(mode=RenderMode(payload["mode"]))


@dataclass(frozen=True, slots=True)
class StyleImageMissingPayload:
    """Runtime style-image-missing event payload."""

    image_id: str

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> "StyleImageMissingPayload":
        """Build a style-image-missing payload from RuntimeEvent.payload."""
        return cls(image_id=payload["image_id"])


@dataclass(frozen=True, slots=True)
class TileId:
    """Overscaled tile identity copied from a tile-action event."""

    overscaled_z: int
    wrap: int
    canonical_z: int
    canonical_x: int
    canonical_y: int

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> "TileId":
        """Build a tile identity from private native bridge values."""
        return cls(
            overscaled_z=raw["overscaled_z"],
            wrap=raw["wrap"],
            canonical_z=raw["canonical_z"],
            canonical_x=raw["canonical_x"],
            canonical_y=raw["canonical_y"],
        )


@dataclass(frozen=True, slots=True)
class TileActionPayload:
    """Runtime tile-action event payload."""

    operation: TileOperation
    tile_id: TileId
    source_id: str

    @classmethod
    def _from_runtime_payload(cls, payload: dict[str, object]) -> "TileActionPayload":
        """Build a tile-action payload from RuntimeEvent.payload."""
        return cls(
            operation=TileOperation(payload["operation"]),
            tile_id=TileId._from_native(payload["tile_id"]),
            source_id=payload["source_id"],
        )


@dataclass(frozen=True, slots=True)
class CameraTransitionFinishedPayload:
    """Runtime camera transition-finished event payload.

    A transition that carried a ``transition_id`` on its ``AnimationOptions``
    reports its end once for every terminal outcome: running to completion,
    being superseded by a later camera command, being cancelled by
    ``MapHandle.cancel_transitions``, or completing instantly as a
    zero-duration jump. A command this API rejects, such as one carrying a
    non-finite enabled camera field, starts no transition and emits no such
    event. MapLibre Native reports the moment the transition releases the camera without naming which
    outcome occurred, so this payload establishes transition identity rather
    than a completion reason.
    """

    transition_id: int

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> "CameraTransitionFinishedPayload":
        """Build a camera transition-finished payload from RuntimeEvent.payload."""
        return cls(transition_id=payload["transition_id"])


@dataclass(frozen=True, slots=True)
class UnknownRuntimeEventPayload:
    """Forward-compatible runtime event payload bytes."""

    raw_type: int
    data: bytes

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> "UnknownRuntimeEventPayload":
        """Build an unknown payload from RuntimeEvent.payload."""
        return cls(raw_type=payload["raw_type"], data=payload["bytes"])


@dataclass(frozen=True, slots=True)
class RuntimeEventSource:
    """Copied runtime event source metadata."""

    source_type: RuntimeEventSourceType
    """Kind of runtime object the event came from."""

    map_handle: MapHandle | None = None
    """Source map when this binding can prove its identity.

    The runtime holds its maps weakly, so a `MAP` event carries None here once
    the caller drops its last reference to the source map or closes it. Keep a
    reference to every map whose events you route by handle.
    """


@dataclass(frozen=True, slots=True)
class RuntimeEvent:
    """Runtime event copied into Python-owned values.

    ``event_type`` selects the meaning of ``code`` and the type of ``payload``.
    ``code`` carries a :class:`CameraChangeMode`, a ``MaplibreStatus`` value, a
    MapLibre Native error ordinal, or ``0``; see :class:`RuntimeEventType` for
    the per-type meaning.
    """

    event_type: RuntimeEventType
    source: RuntimeEventSource
    code: int
    message: str | None
    payload: RuntimeEventPayload

    @classmethod
    def _from_native(
        cls,
        raw: dict[str, Any],
        runtime: RuntimeHandle | None = None,
    ) -> "RuntimeEvent":
        """Build a copied Python event from a private native event dictionary."""
        source_type = RuntimeEventSourceType(raw["source_type"])
        source_address = raw["source_address"]
        return cls(
            event_type=RuntimeEventType(raw["event_type"]),
            source=RuntimeEventSource(
                source_type=source_type,
                map_handle=(
                    runtime._map_for_source_address(source_address)  # noqa: SLF001
                    if runtime is not None and source_type == RuntimeEventSourceType.MAP
                    else None
                ),
            ),
            code=raw["code"],
            message=raw["message"],
            payload=_runtime_payload_from_native(raw["payload"]),
        )


@dataclass(slots=True)
class RuntimeOptions:
    """Options used when creating a runtime."""

    asset_path: str | None = None
    cache_path: str | None = None
    maximum_cache_size: int | None = None


class WakeSource(NativeHandleMixin):
    """Releases a runtime owner thread parked in :meth:`RuntimeHandle.pump`.

    A wake source is usable from any thread, which a host's task submission and
    shutdown paths rely on. It stays usable after its runtime closes, and
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

        A signal raised while the owner thread is running sets the wake flag,
        so the next :meth:`RuntimeHandle.pump` returns without parking.
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
            options.maximum_cache_size,
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
        self._maps[map_handle._native_address()] = weakref.ref(map_handle)  # noqa: SLF001

    def _unregister_map(self, map_handle: MapHandle) -> None:
        self._maps.pop(map_handle._native_address(), None)  # noqa: SLF001

    def _map_for_source_address(self, source_address: int) -> MapHandle | None:
        source = self._maps.get(source_address)
        if source is None:
            return None
        map_handle = source()
        if map_handle is None or map_handle.closed:
            self._maps.pop(source_address, None)
            return None
        return map_handle

    def pump(self, timeout: float | None = 0.0) -> None:
        """Advance this runtime.

        The call parks the owner thread when ``timeout`` allows it, then drains
        the owner-thread task queues. Drain the queued runtime events with
        :meth:`poll_event` afterwards.

        ``timeout`` is in seconds and sets the park bound. Zero drains and
        returns; hosts pumping from a frame callback pass it. A positive value
        parks for up to that long; hosts that own their pump thread pass one and
        take their cadence from the runtime's own work. ``None`` parks until a
        wake arrives.

        The drain runs every task queued when it begins plus every task those
        enqueue, so a single call can span a full style parse.

        The runtime holds a wake flag. Style, tile, offline, and resource
        responses set it, as do queued runtime events and
        :meth:`WakeSource.signal`. A parking call returns as soon as the flag is
        set and clears it before returning, and work arriving during the drain
        sets it again. A call also returns without parking while unread runtime
        events are queued. Timers and ready file descriptors set the flag only
        when they queue owner-thread work, so pass a bounded timeout to cap how
        long a call waits.

        A non-zero timeout releases the GIL while it parks, so other Python
        threads run and can signal a wake source. Call it outside any lock that
        a signalling thread takes.
        """
        # A negative timeout is a caller mistake rather than a request for an
        # unbounded park, which ``None`` spells, so it collapses to no wait.
        timeout_ms = -1 if timeout is None else max(0, int(timeout * 1000))
        self._native.pump(timeout_ms)

    def wake_source(self) -> WakeSource:
        """Acquire a wake source for this runtime, usable from any thread."""
        return WakeSource._from_native(self._native.wake_source())  # noqa: SLF001

    def _offline_operation(
        self, start: Callable[..., int], *args: object
    ) -> OfflineOperationHandle:
        from .offline import OfflineOperationHandle

        return OfflineOperationHandle._from_native(self, start(*args))  # noqa: SLF001

    def run_ambient_cache_operation(
        self, operation: AmbientCacheOperation
    ) -> OfflineOperationHandle:
        """Start an ambient cache maintenance operation."""
        from .offline import AmbientCacheOperation

        return self._offline_operation(
            self._native.run_ambient_cache_operation_start,
            AmbientCacheOperation(operation).native_code,
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

    def set_resource_provider(
        self,
        callback: ResourceProviderCallback,
        *,
        max_pending_callbacks: int = 64,
    ) -> None:
        """Install or replace the runtime-scoped network resource provider.

        Replacement is allowed while maps are live. When this call returns, the
        previous callback is retired and no in-flight request can still reach
        it. Requests it already took a handle for keep that handle, and each one
        is completed and released as usual.
        """
        from .resource import _adapt_resource_provider_callback

        self._native.set_resource_provider(
            _adapt_resource_provider_callback(callback),
            max_pending_callbacks,
        )

    def clear_resource_provider(self) -> None:
        """Clear the runtime-scoped network resource provider.

        Later requests go to MapLibre's online file source. When this call
        returns, the previous callback is retired and no in-flight request can
        still reach it. Requests it already took a handle for keep that handle,
        and each one is completed and released as usual.
        """
        self._native.clear_resource_provider()

    def poll_event(self) -> RuntimeEvent | None:
        """Poll and copy one queued runtime event.

        Map-originated events resolve their source map through this runtime's
        weakly held map table, so `RuntimeEvent.source.map_handle` is None once
        the caller drops its last reference to that map. Keep a reference to
        every map whose events you route by handle.
        """
        event = self._native.poll_event()
        if event is None:
            return None
        return RuntimeEvent._from_native(event, runtime=self)

    def create_map(self, options: MapOptions | None = None) -> MapHandle:
        """Create a map owned by this runtime."""
        from .map import MapHandle

        return MapHandle._create(self, options)  # noqa: SLF001


def _runtime_payload_from_native(payload: dict[str, object]) -> RuntimeEventPayload:
    kind = payload["kind"]
    if kind == "none":
        return None
    if kind == "render_frame":
        return RenderFramePayload._from_runtime_payload(payload)
    if kind == "render_map":
        return RenderMapPayload._from_runtime_payload(payload)
    if kind == "style_image_missing":
        return StyleImageMissingPayload._from_runtime_payload(payload)
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
    "RuntimeEventPayload",
    "RuntimeEventSource",
    "RuntimeEventSourceType",
    "RuntimeEventType",
    "RuntimeHandle",
    "RuntimeOptions",
    "StyleImageMissingPayload",
    "TileActionPayload",
    "TileId",
    "TileOperation",
    "UnknownRuntimeEventPayload",
]

from .map import MapHandle, MapOptions  # noqa: E402
from .offline import (  # noqa: E402
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
    | StyleImageMissingPayload
    | TileActionPayload
    | OfflineRegionStatusChanged
    | OfflineRegionResponseError
    | OfflineRegionTileCountLimitExceeded
    | OfflineOperationCompleted
    | CameraTransitionFinishedPayload
    | UnknownRuntimeEventPayload
)
