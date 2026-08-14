"""Offline database operation values and event payloads."""

from __future__ import annotations

from collections.abc import Callable, Iterator
from contextlib import contextmanager, suppress
from dataclasses import dataclass
from threading import Condition, RLock
from typing import Generic, TypeVar

from ._enum import NativeIntEnum, UnknownIntEnum
from ._lifecycle import ContextHandleMixin, WarnUnclosedMixin
from .errors import (
    InvalidStateError,
    MaplibreStatus,
    _from_native_status,
    _OperationResultConsumedError,
)
from .geo import LatLngBounds
from .resource import ResourceErrorReason

_T = TypeVar("_T")

_OPERATION_HANDLE_CREATE_KEY = object()


class AmbientCacheOperation(NativeIntEnum):
    """Ambient cache maintenance operation kinds."""

    RESET_DATABASE = 1
    PACK_DATABASE = 2
    INVALIDATE = 3
    CLEAR = 4


class OfflineRegionDefinitionType(NativeIntEnum):
    """Offline region definition descriptor variants."""

    TILE_PYRAMID = 1
    GEOMETRY = 2


class OfflineRegionDownloadState(UnknownIntEnum):
    """Offline region download state values."""

    INACTIVE = 0
    ACTIVE = 1

    def native_code_for_set(self) -> int:
        """Return the C enum value for setter calls, rejecting unknown values."""
        return self.known_native_code("offline region download state")


class OperationHandle(
    WarnUnclosedMixin,
    ContextHandleMixin,
    Generic[_T],  # noqa: UP046
):
    _handle_name = "OperationHandle"

    def __init__(
        self,
        runtime: RuntimeHandle,
        operation: int,
        take_result: Callable[[int], object] | None,
        adapt_result: Callable[[object], _T] | None,
        retained_owner: object | None = None,
        _create_key: object | None = None,
    ) -> None:
        if _create_key is not _OPERATION_HANDLE_CREATE_KEY:
            msg = "OperationHandle instances are created by RuntimeHandle"
            raise TypeError(msg)
        self._runtime = runtime
        self._operation = operation
        self._take_result = take_result
        self._adapt_result = adapt_result
        self._retained_owner = retained_owner
        self._state_lock = RLock()
        self._idle = Condition(self._state_lock)
        self._active_uses = 0
        self._closing = False
        self._closed = False
        self._result_consumed = False
        self._result_in_use = False
        runtime._register_operation(self)

    @staticmethod
    def _from_native[U](
        runtime: RuntimeHandle,
        operation: int,
        take_result: Callable[[int], object] | None = None,
        adapt_result: Callable[[object], U] | None = None,
        retained_owner: object | None = None,
    ) -> OperationHandle[U]:
        return OperationHandle(
            runtime,
            operation,
            take_result,
            adapt_result,
            retained_owner,
            _create_key=_OPERATION_HANDLE_CREATE_KEY,
        )

    @property
    def closed(self) -> bool:
        """Return whether this operation observer has been released."""
        with self._state_lock:
            return self._closed

    @contextmanager
    def _use(self, *, allow_while_closing: bool = False) -> Iterator[int]:
        with self._state_lock:
            if self._closed or (self._closing and not allow_while_closing):
                from .errors import InvalidStateError

                raise InvalidStateError(None, "operation handle is already closed")
            self._active_uses += 1
            operation = self._operation
        try:
            yield operation
        finally:
            with self._state_lock:
                self._active_uses -= 1
                if self._active_uses == 0:
                    self._idle.notify_all()

    def close(self) -> None:
        """Release the public observer after in-flight calls return."""
        with self._state_lock:
            if self._closed:
                return
            if self._closing:
                while not self._closed:
                    self._idle.wait()
                return
            self._closing = True
            while self._active_uses:
                self._idle.wait()
            self._runtime._native.operation_release(self._operation)
            self._closed = True
            self._closing = False
            self._idle.notify_all()
        self._runtime._unregister_operation(self)
        self._retained_owner = None

    def __del__(self) -> None:
        super().__del__()
        with suppress(BaseException):
            self.close()

    def poll(self) -> bool:
        """Return whether this operation has completed."""
        with self._use() as operation:
            return self._runtime._native.operation_poll(operation)

    def wait(self, timeout_ms: int = -1) -> bool:
        """Wait for completion, or until the timeout expires."""
        with self._use() as operation:
            return self._runtime._native.operation_wait(operation, timeout_ms)

    def cancel(self) -> None:
        """Request cancellation of this operation."""
        with self._use(allow_while_closing=True) as operation:
            self._runtime._native.operation_cancel(operation)

    def _status_and_diagnostic(self) -> tuple[int, str]:
        with self._use() as operation:
            return self._runtime._native.operation_status(operation)

    @property
    def status(self) -> MaplibreStatus:
        """Return the terminal status category."""
        raw_status, _ = self._status_and_diagnostic()
        return MaplibreStatus._from_native(raw_status)

    @property
    def diagnostic(self) -> str:
        """Return a copy of the terminal diagnostic."""
        _, diagnostic = self._status_and_diagnostic()
        return diagnostic

    def raise_for_status(self) -> None:
        """Raise the terminal operation error, if any."""
        raw_status, diagnostic = self._status_and_diagnostic()
        if raw_status != MaplibreStatus.OK.native_code:
            raise _from_native_status(raw_status, diagnostic)

    def discard(self) -> None:
        """Discard the completed result while keeping the observer live."""
        with self._use() as operation:
            with self._state_lock:
                if self._result_consumed or self._result_in_use:
                    from .errors import InvalidStateError

                    raise InvalidStateError(
                        None, "operation result is already consumed"
                    )
                self._result_in_use = True
            try:
                self._runtime._native.operation_discard(operation)
            except BaseException:
                with self._state_lock:
                    self._result_in_use = False
                raise
            with self._state_lock:
                self._result_in_use = False
                self._result_consumed = True

    def take(self) -> _T:
        """Take this operation's typed result while keeping the observer live."""
        with self._use() as operation:
            with self._state_lock:
                if self._result_consumed or self._result_in_use:
                    raise InvalidStateError(
                        None, "operation result is already consumed"
                    )
                take_result = self._take_result
                adapt_result = self._adapt_result
                if take_result is None or adapt_result is None:
                    raise InvalidStateError(None, "operation has no typed result")
                self._result_in_use = True
            try:
                self.raise_for_status()
                raw = take_result(operation)
            except _OperationResultConsumedError:
                with self._state_lock:
                    self._result_in_use = False
                    self._result_consumed = True
                raise
            except BaseException:
                with self._state_lock:
                    self._result_in_use = False
                raise
            with self._state_lock:
                self._result_in_use = False
                self._result_consumed = True
            return adapt_result(raw)


@dataclass(frozen=True, slots=True)
class OfflineRegionStatus:
    """Offline region status snapshot."""

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
    def _from_native(cls, raw: dict[str, object]) -> OfflineRegionStatus:
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
    """Tile-pyramid offline region definition."""

    style_url: str
    bounds: LatLngBounds
    min_zoom: float
    max_zoom: float
    pixel_ratio: float
    include_ideographs: bool = True

    @property
    def definition_type(self) -> OfflineRegionDefinitionType:
        """Return this definition variant."""
        return OfflineRegionDefinitionType.TILE_PYRAMID


@dataclass(frozen=True, slots=True)
class OfflineGeometryRegionDefinition:
    """Geometry offline region definition."""

    style_url: str
    geometry: bytes
    min_zoom: float
    max_zoom: float
    pixel_ratio: float
    include_ideographs: bool = True

    @property
    def definition_type(self) -> OfflineRegionDefinitionType:
        """Return this definition variant."""
        return OfflineRegionDefinitionType.GEOMETRY


OfflineRegionDefinition = (
    OfflineTilePyramidRegionDefinition | OfflineGeometryRegionDefinition
)


@dataclass(frozen=True, slots=True)
class OfflineRegionInfo:
    """Copied offline region metadata."""

    id: int
    definition: OfflineRegionDefinition
    metadata: bytes

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> OfflineRegionInfo:
        return cls(
            id=raw["id"],
            definition=_definition_from_native_wire(raw["definition"]),
            metadata=raw["metadata"],
        )


@dataclass(frozen=True, slots=True)
class OfflineRegionStatusChanged:
    """Offline region status-change event payload."""

    region_id: int
    status: OfflineRegionStatus

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> OfflineRegionStatusChanged:
        return cls(
            region_id=_payload_int(payload, "region_id"),
            status=OfflineRegionStatus._from_native(payload["status"]),
        )


@dataclass(frozen=True, slots=True)
class OfflineRegionResponseError:
    """Offline region response-error event payload."""

    region_id: int
    reason: ResourceErrorReason

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> OfflineRegionResponseError:
        from .resource import ResourceErrorReason

        return cls(
            region_id=_payload_int(payload, "region_id"),
            reason=ResourceErrorReason(_payload_int(payload, "reason")),
        )


@dataclass(frozen=True, slots=True)
class OfflineRegionTileCountLimitExceeded:
    """Offline region tile-count-limit event payload."""

    region_id: int
    limit: int

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> OfflineRegionTileCountLimitExceeded:
        return cls(
            region_id=_payload_int(payload, "region_id"),
            limit=_payload_int(payload, "limit"),
        )


def _adapt_region_result(raw: object) -> OfflineRegionInfo:
    return OfflineRegionInfo._from_native(raw)


def _adapt_optional_region_result(raw: object) -> OfflineRegionInfo | None:
    return OfflineRegionInfo._from_native(raw) if raw is not None else None


def _adapt_region_list_result(raw: object) -> tuple[OfflineRegionInfo, ...]:
    return tuple(OfflineRegionInfo._from_native(region) for region in raw)


def _adapt_status_result(raw: object) -> OfflineRegionStatus:
    return OfflineRegionStatus._from_native(raw)


def _definition_from_native_wire(raw: dict[str, object]) -> OfflineRegionDefinition:
    kind = raw["type"]
    if kind == "tile_pyramid":
        bounds = raw["bounds"]
        return OfflineTilePyramidRegionDefinition(
            style_url=raw["style_url"],
            bounds=LatLngBounds(
                southwest=_lat_lng_from_native_wire(bounds["southwest"]),
                northeast=_lat_lng_from_native_wire(bounds["northeast"]),
            ),
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            pixel_ratio=raw["pixel_ratio"],
            include_ideographs=raw["include_ideographs"],
        )
    if kind == "geometry":
        return OfflineGeometryRegionDefinition(
            style_url=raw["style_url"],
            geometry=raw["geometry"],
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            pixel_ratio=raw["pixel_ratio"],
            include_ideographs=raw["include_ideographs"],
        )
    msg = f"unsupported native offline region definition kind: {kind}"
    raise TypeError(msg)


def _lat_lng_from_native_wire(raw: dict[str, object]):
    from .geo import LatLng

    return LatLng(latitude=raw["latitude"], longitude=raw["longitude"])


def _payload_int(payload: dict[str, object], key: str) -> int:
    value = payload[key]
    if not isinstance(value, int):
        msg = f"offline payload {key} must be an int"
        raise TypeError(msg)
    return value


__all__ = [
    "AmbientCacheOperation",
    "OfflineGeometryRegionDefinition",
    "OfflineRegionDefinition",
    "OfflineRegionDefinitionType",
    "OfflineRegionDownloadState",
    "OfflineRegionInfo",
    "OfflineRegionResponseError",
    "OfflineRegionStatus",
    "OfflineRegionStatusChanged",
    "OfflineRegionTileCountLimitExceeded",
    "OfflineTilePyramidRegionDefinition",
    "OperationHandle",
]

from .runtime import RuntimeHandle
