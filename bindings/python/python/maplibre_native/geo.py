"""Geographic coordinates, geometry, and GeoJSON value trees."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TypeAlias

from .json import JsonMember


@dataclass(frozen=True, slots=True)
class LatLng:
    """Geographic coordinate in degrees."""

    latitude: float
    longitude: float


@dataclass(frozen=True, slots=True)
class LatLngBounds:
    """Geographic bounds in degrees."""

    southwest: LatLng
    northeast: LatLng


@dataclass(frozen=True, slots=True)
class EmptyGeometry:
    """Empty geometry value."""


@dataclass(frozen=True, slots=True)
class Point:
    """Point geometry."""

    coordinate: LatLng


@dataclass(frozen=True, slots=True)
class LineString:
    """Line string geometry."""

    coordinates: tuple[LatLng, ...]


@dataclass(frozen=True, slots=True)
class Polygon:
    """Polygon geometry represented as rings."""

    rings: tuple[tuple[LatLng, ...], ...]


@dataclass(frozen=True, slots=True)
class MultiPoint:
    """Multi-point geometry."""

    coordinates: tuple[LatLng, ...]


@dataclass(frozen=True, slots=True)
class MultiLineString:
    """Multi-line-string geometry."""

    lines: tuple[tuple[LatLng, ...], ...]


@dataclass(frozen=True, slots=True)
class MultiPolygon:
    """Multi-polygon geometry."""

    polygons: tuple[tuple[tuple[LatLng, ...], ...], ...]


@dataclass(frozen=True, slots=True)
class GeometryCollection:
    """Geometry collection value."""

    geometries: tuple[Geometry, ...]


Geometry: TypeAlias = (
    EmptyGeometry
    | Point
    | LineString
    | Polygon
    | MultiPoint
    | MultiLineString
    | MultiPolygon
    | GeometryCollection
)


@dataclass(frozen=True, slots=True)
class FeatureIdentifierUInt:
    """Unsigned GeoJSON feature identifier."""

    value: int

    def __post_init__(self) -> None:
        if self.value < 0:
            msg = "FeatureIdentifierUInt value must be non-negative"
            raise ValueError(msg)


@dataclass(frozen=True, slots=True)
class FeatureIdentifierInt:
    """Signed GeoJSON feature identifier."""

    value: int


@dataclass(frozen=True, slots=True)
class FeatureIdentifierDouble:
    """Double GeoJSON feature identifier."""

    value: float


@dataclass(frozen=True, slots=True)
class FeatureIdentifierString:
    """String GeoJSON feature identifier."""

    value: str


FeatureIdentifier: TypeAlias = (
    None
    | FeatureIdentifierUInt
    | FeatureIdentifierInt
    | FeatureIdentifierDouble
    | FeatureIdentifierString
)


@dataclass(frozen=True, slots=True)
class Feature:
    """GeoJSON feature descriptor."""

    geometry: Geometry
    properties: tuple[JsonMember, ...] = ()
    identifier: FeatureIdentifier = None


@dataclass(frozen=True, slots=True)
class GeometryGeoJson:
    """GeoJSON descriptor containing one geometry."""

    geometry: Geometry


@dataclass(frozen=True, slots=True)
class FeatureGeoJson:
    """GeoJSON descriptor containing one feature."""

    feature: Feature


@dataclass(frozen=True, slots=True)
class FeatureCollection:
    """GeoJSON descriptor containing an ordered feature collection."""

    features: tuple[Feature, ...]


GeoJson: TypeAlias = GeometryGeoJson | FeatureGeoJson | FeatureCollection


__all__ = [
    "EmptyGeometry",
    "Feature",
    "FeatureCollection",
    "FeatureGeoJson",
    "FeatureIdentifier",
    "FeatureIdentifierDouble",
    "FeatureIdentifierInt",
    "FeatureIdentifierString",
    "FeatureIdentifierUInt",
    "GeoJson",
    "Geometry",
    "GeometryCollection",
    "GeometryGeoJson",
    "LatLng",
    "LatLngBounds",
    "LineString",
    "MultiLineString",
    "MultiPoint",
    "MultiPolygon",
    "Point",
    "Polygon",
]
