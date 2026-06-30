"""Offline database operation values and event payloads."""

from __future__ import annotations

from ._enum import NativeIntEnum, UnknownIntEnum
from ._lifecycle import ContextHandleMixin, WarnUnclosedMixin
from collections.abc import Callable
from dataclasses import dataclass

from .geo import Geometry, LatLngBounds
from .resource import ResourceErrorReason

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


class OfflineOperationKind(UnknownIntEnum):
    """Offline database operation kinds reported by completion events."""

    AMBIENT_CACHE = 1
    REGION_CREATE = 2
    REGION_GET = 3
    REGIONS_LIST = 4
    REGIONS_MERGE_DATABASE = 5
    REGION_UPDATE_METADATA = 6
    REGION_GET_STATUS = 7
    REGION_SET_OBSERVED = 8
    REGION_SET_DOWNLOAD_STATE = 9
    REGION_INVALIDATE = 10
    REGION_DELETE = 11


class OfflineOperationResultKind(UnknownIntEnum):
    """Offline database operation result kinds reported by completion events."""

    NONE = 0
    REGION = 1
    OPTIONAL_REGION = 2
    REGION_LIST = 3
    REGION_STATUS = 4


class OfflineOperationHandle(WarnUnclosedMixin, ContextHandleMixin):
    """Runtime-owned offline database operation token."""

    _handle_name = "OfflineOperationHandle"

    def __init__(
        self,
        runtime: "RuntimeHandle",
        operation_id: int,
        *,
        _create_key: object | None = None,
    ) -> None:
        if _create_key is not _OPERATION_HANDLE_CREATE_KEY:
            msg = "OfflineOperationHandle instances are created by RuntimeHandle"
            raise TypeError(msg)
        self._runtime = runtime
        self._operation_id = operation_id
        self._closed = False
        runtime._register_offline_operation(self)  # noqa: SLF001

    @classmethod
    def _from_native(
        cls, runtime: "RuntimeHandle", operation_id: int
    ) -> "OfflineOperationHandle":
        return cls(
            runtime,
            operation_id,
            _create_key=_OPERATION_HANDLE_CREATE_KEY,
        )

    @property
    def closed(self) -> bool:
        """Return whether this operation token has been discarded."""
        return self._closed

    def close(self) -> None:
        """Discard runtime-owned state for this operation."""
        if self._closed:
            return
        self._runtime._native.offline_operation_discard(self._operation_id)  # noqa: SLF001
        self._mark_closed()

    def _mark_closed(self) -> None:
        if self._closed:
            return
        self._closed = True
        self._runtime._unregister_offline_operation(self)  # noqa: SLF001

    def _mark_runtime_closed(self) -> None:
        self._mark_closed()

    def _ensure_open(self) -> None:
        if self._closed:
            from .errors import InvalidStateError

            raise InvalidStateError(None, "offline operation handle is already closed")

    def _take(self, take: Callable[[int], object]) -> object:
        self._ensure_open()
        raw = take(self._operation_id)
        self._mark_closed()
        return raw

    def take_region(self) -> "OfflineRegionInfo":
        """Take a completed region snapshot result."""
        return OfflineRegionInfo._from_native(
            self._take(self._runtime._native.offline_region_create_take_result)  # noqa: SLF001
        )

    def take_optional_region(self) -> "OfflineRegionInfo | None":
        """Take a completed optional region snapshot result."""
        raw = self._take(self._runtime._native.offline_region_get_take_result)  # noqa: SLF001
        return OfflineRegionInfo._from_native(raw) if raw is not None else None

    def take_region_list(
        self,
        *,
        merge_result: bool = False,
    ) -> tuple["OfflineRegionInfo", ...]:
        """Take a completed region-list result."""
        take = (
            self._runtime._native.offline_regions_merge_database_take_result  # noqa: SLF001
            if merge_result
            else self._runtime._native.offline_regions_list_take_result  # noqa: SLF001
        )
        return tuple(
            OfflineRegionInfo._from_native(region) for region in self._take(take)
        )

    def take_updated_region(self) -> "OfflineRegionInfo":
        """Take a completed updated region snapshot result."""
        return OfflineRegionInfo._from_native(
            self._take(
                self._runtime._native.offline_region_update_metadata_take_result  # noqa: SLF001
            )
        )

    def take_status(self) -> "OfflineRegionStatus":
        """Take a completed offline region status result."""
        return OfflineRegionStatus._from_native(
            self._take(self._runtime._native.offline_region_get_status_take_result)  # noqa: SLF001
        )


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
    def _from_native(cls, raw: dict[str, object]) -> "OfflineRegionStatus":
        """Build a status snapshot from private native bridge values."""
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
    geometry: Geometry
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
    def _from_native(cls, raw: dict[str, object]) -> "OfflineRegionInfo":
        """Build region metadata from private native bridge values."""
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
    ) -> "OfflineRegionStatusChanged":
        """Build a status-change payload from RuntimeEvent.payload."""
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
    ) -> "OfflineRegionResponseError":
        """Build a response-error payload from RuntimeEvent.payload."""
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
    ) -> "OfflineRegionTileCountLimitExceeded":
        """Build a tile-count-limit payload from RuntimeEvent.payload."""
        return cls(
            region_id=_payload_int(payload, "region_id"),
            limit=_payload_int(payload, "limit"),
        )


@dataclass(frozen=True, slots=True)
class OfflineOperationCompleted:
    """Offline operation completion event payload."""

    operation_id: int
    operation_kind: OfflineOperationKind
    result_kind: OfflineOperationResultKind
    result_status: int
    found: bool

    @classmethod
    def _from_runtime_payload(
        cls, payload: dict[str, object]
    ) -> "OfflineOperationCompleted":
        """Build a completion payload from RuntimeEvent.payload."""
        return cls(
            operation_id=_payload_int(payload, "operation_id"),
            operation_kind=OfflineOperationKind(
                _payload_int(payload, "operation_kind")
            ),
            result_kind=OfflineOperationResultKind(
                _payload_int(payload, "result_kind")
            ),
            result_status=_payload_int(payload, "result_status"),
            found=_payload_bool(payload, "found"),
        )


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


def _payload_bool(payload: dict[str, object], key: str) -> bool:
    value = payload[key]
    if not isinstance(value, bool):
        msg = f"offline payload {key} must be a bool"
        raise TypeError(msg)
    return value


__all__ = [
    "AmbientCacheOperation",
    "OfflineGeometryRegionDefinition",
    "OfflineOperationCompleted",
    "OfflineOperationHandle",
    "OfflineOperationKind",
    "OfflineOperationResultKind",
    "OfflineRegionDefinition",
    "OfflineRegionDefinitionType",
    "OfflineRegionDownloadState",
    "OfflineRegionInfo",
    "OfflineRegionResponseError",
    "OfflineRegionStatus",
    "OfflineRegionStatusChanged",
    "OfflineRegionTileCountLimitExceeded",
    "OfflineTilePyramidRegionDefinition",
]

from .runtime import RuntimeHandle  # noqa: E402
