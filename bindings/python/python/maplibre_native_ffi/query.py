"""Rendered and source query descriptors."""

from __future__ import annotations

from dataclasses import dataclass

from ._enum import NativeIntEnum
from .camera import ScreenPoint


class RenderedQueryGeometryType(NativeIntEnum):
    """Rendered feature query geometry variants."""

    POINT = 1
    BOX = 2
    LINE_STRING = 3


@dataclass(frozen=True, slots=True)
class ScreenBox:
    """Screen-space box in logical map pixels.

    Corners may be given in any order and may extend past the viewport;
    rendered queries normalize and clip them.
    """

    min: ScreenPoint
    max: ScreenPoint


@dataclass(frozen=True, slots=True)
class RenderedQueryGeometry:
    """Screen-space geometry used for rendered feature queries."""

    type: RenderedQueryGeometryType
    point: ScreenPoint | None = None
    box: ScreenBox | None = None
    line_string: tuple[ScreenPoint, ...] | None = None

    @classmethod
    def point_geometry(cls, point: ScreenPoint) -> RenderedQueryGeometry:
        """Create a point query geometry."""
        return cls(RenderedQueryGeometryType.POINT, point=point)

    @classmethod
    def box_geometry(cls, box_: ScreenBox) -> RenderedQueryGeometry:
        """Create a box query geometry."""
        return cls(RenderedQueryGeometryType.BOX, box=box_)

    @classmethod
    def line_string_geometry(
        cls, points: list[ScreenPoint] | tuple[ScreenPoint, ...]
    ) -> RenderedQueryGeometry:
        """Create a line-string query geometry."""
        return cls(RenderedQueryGeometryType.LINE_STRING, line_string=tuple(points))

    def __post_init__(self) -> None:
        active = sum(
            value is not None for value in (self.point, self.box, self.line_string)
        )
        if active != 1:
            raise ValueError(
                "rendered query geometry must contain exactly one geometry value"
            )
        if self.type is RenderedQueryGeometryType.POINT and self.point is None:
            raise ValueError("point query geometry requires point")
        if self.type is RenderedQueryGeometryType.BOX and self.box is None:
            raise ValueError("box query geometry requires box")
        if (
            self.type is RenderedQueryGeometryType.LINE_STRING
            and self.line_string is None
        ):
            raise ValueError("line-string query geometry requires points")


@dataclass(frozen=True, slots=True)
class RenderedFeatureQueryOptions:
    """Options for rendered feature queries."""

    layer_ids: tuple[str, ...] | None = None
    filter: bytes | None = None


@dataclass(frozen=True, slots=True)
class SourceFeatureQueryOptions:
    """Options for source feature queries."""

    source_layer_ids: tuple[str, ...] | None = None
    filter: bytes | None = None


@dataclass(frozen=True, slots=True)
class FeatureStateSelector:
    """Source, feature, and state-key selector for render-session feature state."""

    source_id: str
    source_layer_id: str | None = None
    feature_id: str | None = None
    state_key: str | None = None

    def __post_init__(self) -> None:
        if self.state_key is not None and self.feature_id is None:
            raise ValueError("state_key requires feature_id")


def _point_to_native_wire(point: ScreenPoint) -> tuple[float, float]:
    return (point.x, point.y)


def _geometry_to_native_wire(geometry: RenderedQueryGeometry) -> dict[str, object]:
    if geometry.type is RenderedQueryGeometryType.POINT:
        if geometry.point is None:
            raise ValueError("point query geometry requires point")
        return {"type": "point", "point": _point_to_native_wire(geometry.point)}
    if geometry.type is RenderedQueryGeometryType.BOX:
        if geometry.box is None:
            raise ValueError("box query geometry requires box")
        return {
            "type": "box",
            "min": _point_to_native_wire(geometry.box.min),
            "max": _point_to_native_wire(geometry.box.max),
        }
    if geometry.type is RenderedQueryGeometryType.LINE_STRING:
        if geometry.line_string is None:
            raise ValueError("line-string query geometry requires points")
        return {
            "type": "line_string",
            "points": [_point_to_native_wire(point) for point in geometry.line_string],
        }
    raise TypeError(f"unknown rendered query geometry type: {geometry.type!r}")


__all__ = [
    "FeatureStateSelector",
    "RenderedFeatureQueryOptions",
    "RenderedQueryGeometry",
    "RenderedQueryGeometryType",
    "ScreenBox",
    "SourceFeatureQueryOptions",
]
