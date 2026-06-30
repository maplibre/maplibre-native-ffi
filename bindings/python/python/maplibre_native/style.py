"""Style source, layer, image, light, and property APIs."""

from __future__ import annotations

from ._enum import NativeIntEnum, UnknownIntEnum
from ._lifecycle import NativeHandleMixin
from dataclasses import dataclass
from typing import Any

from .geo import LatLngBounds
from .render import PremultipliedRgba8Image, TextureImageInfo

_CUSTOM_GEOMETRY_SOURCE_HANDLE_CREATE_KEY = object()


class TileScheme(NativeIntEnum):
    """Tile URL coordinate scheme values."""

    XYZ = 0
    TMS = 1


class VectorTileEncoding(NativeIntEnum):
    """Vector tile encoding values."""

    MVT = 0
    MLT = 1


class RasterDemEncoding(NativeIntEnum):
    """DEM raster encoding values."""

    MAPBOX = 0
    TERRARIUM = 1


@dataclass(frozen=True, slots=True)
class TileSourceOptions:
    """Options for vector, raster, and raster DEM tile sources."""

    min_zoom: float | None = None
    max_zoom: float | None = None
    attribution: str | None = None
    scheme: TileScheme | None = None
    bounds: LatLngBounds | None = None
    tile_size: int | None = None
    vector_encoding: VectorTileEncoding | None = None
    raster_dem_encoding: RasterDemEncoding | None = None


class StyleSourceType(UnknownIntEnum):
    """Style source type values returned by MapLibre Native."""

    UNKNOWN = 0
    VECTOR = 1
    RASTER = 2
    RASTER_DEM = 3
    GEOJSON = 4
    IMAGE = 5
    VIDEO = 6
    ANNOTATIONS = 7
    CUSTOM_VECTOR = 8


@dataclass(frozen=True, slots=True)
class StyleSourceInfo:
    """Copied fixed metadata for one style source."""

    source_type: StyleSourceType
    is_volatile: bool
    attribution: str | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> "StyleSourceInfo":
        """Build source metadata from private native bridge values."""
        return cls(
            source_type=StyleSourceType(raw["source_type"]),
            is_volatile=raw["is_volatile"],
            attribution=raw["attribution"],
        )


@dataclass(frozen=True, slots=True)
class StyleImageOptions:
    """Options for adding or replacing a runtime style image."""

    pixel_ratio: float | None = None
    sdf: bool | None = None


@dataclass(frozen=True, slots=True)
class StyleImageInfo:
    """Fixed metadata for one runtime style image."""

    width: int
    height: int
    stride: int
    byte_length: int
    pixel_ratio: float
    sdf: bool

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> "StyleImageInfo":
        """Build style image metadata from private native bridge values."""
        return cls(
            width=raw["width"],
            height=raw["height"],
            stride=raw["stride"],
            byte_length=raw["byte_length"],
            pixel_ratio=raw["pixel_ratio"],
            sdf=raw["sdf"],
        )


@dataclass(frozen=True, slots=True)
class StyleImage:
    """Copied runtime style image pixels with style-specific metadata."""

    image: PremultipliedRgba8Image
    pixel_ratio: float
    sdf: bool

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> "StyleImage":
        """Build a copied style image from private native bridge values."""
        info = StyleImageInfo._from_native(raw["info"])
        return cls(
            image=PremultipliedRgba8Image(
                TextureImageInfo(
                    width=info.width,
                    height=info.height,
                    stride=info.stride,
                    byte_length=info.byte_length,
                ),
                raw["data"],
            ),
            pixel_ratio=info.pixel_ratio,
            sdf=info.sdf,
        )


class LocationIndicatorImageKind(NativeIntEnum):
    """Location indicator image-name properties."""

    TOP = 0
    BEARING = 1
    SHADOW = 2


class CustomGeometrySourceEventType(NativeIntEnum):
    """Custom geometry source callback event kind."""

    FETCH_TILE = 0
    CANCEL_TILE = 1


@dataclass(frozen=True, slots=True)
class CanonicalTileId:
    """Canonical tile identity used by custom geometry source callbacks."""

    z: int
    x: int
    y: int


@dataclass(frozen=True, slots=True)
class CustomGeometrySourceEvent:
    """Queued custom geometry source callback event."""

    event_type: CustomGeometrySourceEventType
    tile_id: CanonicalTileId

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> "CustomGeometrySourceEvent":
        """Build an event from private native values."""
        return cls(
            event_type=CustomGeometrySourceEventType(raw["kind"]),
            tile_id=CanonicalTileId(z=raw["z"], x=raw["x"], y=raw["y"]),
        )


@dataclass(frozen=True, slots=True)
class CustomGeometrySourceOptions:
    """Options used when adding a custom geometry source."""

    min_zoom: float | None = None
    max_zoom: float | None = None
    tolerance: float | None = None
    tile_size: int | None = None
    buffer: int | None = None
    clip: bool | None = None
    wrap: bool | None = None
    has_cancel_tile: bool = False
    max_queued_events: int = 1024


class CustomGeometrySourceHandle(NativeHandleMixin):
    """Owner-thread handle for queued custom geometry source callback events."""

    _handle_name = "CustomGeometrySourceHandle"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _CUSTOM_GEOMETRY_SOURCE_HANDLE_CREATE_KEY:
            msg = "CustomGeometrySourceHandle instances are created by MapHandle"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> "CustomGeometrySourceHandle":
        return cls(native, _create_key=_CUSTOM_GEOMETRY_SOURCE_HANDLE_CREATE_KEY)

    @property
    def dropped_event_count(self) -> int:
        """Return how many callback events were dropped because the queue was full."""
        return self._native.dropped_event_count

    def poll_event(self) -> CustomGeometrySourceEvent | None:
        """Return one queued fetch/cancel event copied into Python values."""
        event = self._native.poll_event()
        if event is None:
            return None
        return CustomGeometrySourceEvent._from_native(event)


__all__ = [
    "CanonicalTileId",
    "CustomGeometrySourceEvent",
    "CustomGeometrySourceEventType",
    "CustomGeometrySourceHandle",
    "CustomGeometrySourceOptions",
    "LocationIndicatorImageKind",
    "RasterDemEncoding",
    "StyleImage",
    "StyleImageInfo",
    "StyleImageOptions",
    "StyleSourceInfo",
    "StyleSourceType",
    "TileScheme",
    "TileSourceOptions",
    "VectorTileEncoding",
]
