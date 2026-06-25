"""Camera descriptors and map camera operations."""

from __future__ import annotations

from dataclasses import dataclass

from .geo import LatLng, LatLngBounds


@dataclass(frozen=True, slots=True)
class ScreenPoint:
    """Screen-space point in logical map pixels."""

    x: float
    y: float


@dataclass(frozen=True, slots=True)
class EdgeInsets:
    """Screen-space inset in logical map pixels."""

    top: float = 0.0
    left: float = 0.0
    bottom: float = 0.0
    right: float = 0.0


@dataclass(frozen=True, slots=True)
class UnitBezier:
    """Cubic Bezier control points for animation easing."""

    p1x: float
    p1y: float
    p2x: float
    p2y: float


@dataclass(frozen=True, slots=True)
class Vec3:
    """Three-component vector used by free camera options."""

    x: float
    y: float
    z: float


@dataclass(frozen=True, slots=True)
class Quaternion:
    """Quaternion stored as x, y, z, w components."""

    x: float
    y: float
    z: float
    w: float


@dataclass(frozen=True, slots=True)
class CameraOptions:
    """Camera fields used for snapshots and camera commands."""

    center: LatLng | None = None
    zoom: float | None = None
    bearing: float | None = None
    pitch: float | None = None
    center_altitude: float | None = None
    padding: EdgeInsets | None = None
    anchor: ScreenPoint | None = None
    roll: float | None = None
    field_of_view: float | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> "CameraOptions":
        """Build camera options from private native bridge values."""
        center = raw["center"]
        padding = raw["padding"]
        anchor = raw["anchor"]
        return cls(
            center=LatLng(**center) if isinstance(center, dict) else None,
            zoom=raw["zoom"],
            bearing=raw["bearing"],
            pitch=raw["pitch"],
            center_altitude=raw["center_altitude"],
            padding=EdgeInsets(**padding) if isinstance(padding, dict) else None,
            anchor=ScreenPoint(**anchor) if isinstance(anchor, dict) else None,
            roll=raw["roll"],
            field_of_view=raw["field_of_view"],
        )


@dataclass(frozen=True, slots=True)
class AnimationOptions:
    """Optional animation controls for camera transitions."""

    duration_ms: float | None = None
    velocity: float | None = None
    min_zoom: float | None = None
    easing: UnitBezier | None = None


@dataclass(frozen=True, slots=True)
class CameraFitOptions:
    """Optional fitting controls for camera-for-viewport queries."""

    padding: EdgeInsets | None = None
    bearing: float | None = None
    pitch: float | None = None


@dataclass(frozen=True, slots=True)
class BoundOptions:
    """Optional map camera constraint fields."""

    bounds: LatLngBounds | None = None
    min_zoom: float | None = None
    max_zoom: float | None = None
    min_pitch: float | None = None
    max_pitch: float | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> "BoundOptions":
        """Build bound options from private native bridge values."""
        bounds = raw["bounds"]
        if isinstance(bounds, dict):
            bounds = LatLngBounds(
                southwest=LatLng(**bounds["southwest"]),
                northeast=LatLng(**bounds["northeast"]),
            )
        else:
            bounds = None
        return cls(
            bounds=bounds,
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            min_pitch=raw["min_pitch"],
            max_pitch=raw["max_pitch"],
        )


@dataclass(frozen=True, slots=True)
class FreeCameraOptions:
    """Free camera position and orientation in MapLibre camera space."""

    position: Vec3 | None = None
    orientation: Quaternion | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> "FreeCameraOptions":
        """Build free camera options from private native bridge values."""
        position = raw["position"]
        orientation = raw["orientation"]
        return cls(
            position=Vec3(**position) if isinstance(position, dict) else None,
            orientation=Quaternion(**orientation)
            if isinstance(orientation, dict)
            else None,
        )


@dataclass(frozen=True, slots=True)
class ProjectionMode:
    """Axonometric rendering options for the live map render transform."""

    axonometric: bool | None = None
    x_skew: float | None = None
    y_skew: float | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> "ProjectionMode":
        """Build projection mode options from private native bridge values."""
        return cls(
            axonometric=raw["axonometric"],
            x_skew=raw["x_skew"],
            y_skew=raw["y_skew"],
        )


__all__ = [
    "AnimationOptions",
    "BoundOptions",
    "CameraFitOptions",
    "CameraOptions",
    "EdgeInsets",
    "FreeCameraOptions",
    "ProjectionMode",
    "Quaternion",
    "ScreenPoint",
    "UnitBezier",
    "Vec3",
]
