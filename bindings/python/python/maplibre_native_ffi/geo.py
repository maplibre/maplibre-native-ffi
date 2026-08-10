"""Geographic coordinate values."""

from dataclasses import dataclass


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


__all__ = ["LatLng", "LatLngBounds"]
