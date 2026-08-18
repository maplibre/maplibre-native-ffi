"""Offline database operation values and event payloads."""

from __future__ import annotations

from dataclasses import dataclass

from ._enum import NativeIntEnum, UnknownIntEnum
from .geo import LatLngBounds
from .resource import ResourceErrorReason


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
]
