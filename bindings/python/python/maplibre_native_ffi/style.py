"""Style source, layer, image, light, and property APIs."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from ._enum import NativeIntEnum, UnknownIntEnum
from ._lifecycle import NativeHandleMixin
from .errors import InvalidArgumentError
from .geo import LatLng, LatLngBounds
from .render import PremultipliedRgba8Image, TextureImageInfo

_CUSTOM_GEOMETRY_SOURCE_HANDLE_CREATE_KEY = object()


class TileScheme(UnknownIntEnum):
    """Tile URL coordinate scheme values."""

    XYZ = 0
    TMS = 1


class VectorTileEncoding(UnknownIntEnum):
    """Vector tile encoding values."""

    MVT = 0
    MLT = 1


class RasterDemEncoding(UnknownIntEnum):
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


@dataclass(frozen=True, slots=True)
class GeoJsonSourceOptions:
    """Options for GeoJSON sources.

    MapLibre Native fixes these options when the source is created, so updating
    a GeoJSON source's URL or data keeps the options it was added with.
    """

    min_zoom: float | None = None
    max_zoom: float | None = None
    tolerance: float | None = None
    cluster_max_zoom: float | None = None
    cluster_properties: bytes | None = None
    """Cluster aggregation expressions keyed by property name, as a JSON object
    whose members follow the MapLibre Style Spec `clusterProperties` form."""
    tile_size: int | None = None
    buffer: int | None = None
    cluster_radius: int | None = None
    cluster_min_points: int | None = None
    line_metrics: bool | None = None
    cluster: bool | None = None
    synchronous_update: bool | None = None
    """Applies data updates synchronously, so data set through
    `set_geojson_source_data` reaches the next rendered frame."""

    def __post_init__(self) -> None:
        for name in ("tile_size", "buffer", "cluster_radius", "cluster_min_points"):
            value = getattr(self, name)
            if value is not None and not 0 <= value <= 0xFFFF_FFFF:
                raise InvalidArgumentError(
                    None, f"{name} must be within [0, 4294967295]"
                )


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
    CUSTOM_MVT_VECTOR = 9


class StyleLayerVisibility(UnknownIntEnum):
    """Whether a style layer draws."""

    VISIBLE = 0
    NONE = 1


@dataclass(frozen=True, slots=True)
class TileJsonInfo:
    """Copied fields from an inline TileJSON source description."""

    tiles: tuple[str, ...]
    min_zoom: float
    max_zoom: float
    scheme: TileScheme
    bounds: LatLngBounds | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> TileJsonInfo:
        bounds = raw["bounds"]
        return cls(
            tiles=tuple(raw["tiles"]),
            min_zoom=raw["min_zoom"],
            max_zoom=raw["max_zoom"],
            scheme=TileScheme(raw["scheme"]),
            bounds=None
            if bounds is None
            else LatLngBounds(
                southwest=LatLng(**bounds["southwest"]),
                northeast=LatLng(**bounds["northeast"]),
            ),
        )


@dataclass(frozen=True, slots=True)
class StyleSourceInfo:
    """Copied retained metadata for one style source."""

    source_type: StyleSourceType
    is_volatile: bool
    attribution: str | None = None
    url: str | None = None
    tile_json: TileJsonInfo | None = None
    tile_size: int | None = None
    vector_encoding: VectorTileEncoding | None = None
    raster_dem_encoding: RasterDemEncoding | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> StyleSourceInfo:
        return cls(
            source_type=StyleSourceType(raw["source_type"]),
            is_volatile=raw["is_volatile"],
            attribution=raw["attribution"],
            url=raw["url"],
            tile_json=None
            if raw["tile_json"] is None
            else TileJsonInfo._from_native(raw["tile_json"]),
            tile_size=raw["tile_size"],
            vector_encoding=None
            if raw["vector_encoding"] is None
            else VectorTileEncoding(raw["vector_encoding"]),
            raster_dem_encoding=None
            if raw["raster_dem_encoding"] is None
            else RasterDemEncoding(raw["raster_dem_encoding"]),
        )


@dataclass(frozen=True, slots=True)
class ImageStretch:
    """One stretchable interval along an image axis, in image pixels."""

    from_: float
    to: float


@dataclass(frozen=True, slots=True)
class ImageContent:
    """Content-box insets in image pixels, from the image's top-left."""

    left: float
    top: float
    right: float
    bottom: float


class StyleImageTextFit(UnknownIntEnum):
    """How a stretchable image fits text along one axis."""

    STRETCH_OR_SHRINK = 0
    STRETCH_ONLY = 1
    PROPORTIONAL = 2


@dataclass(frozen=True, slots=True)
class StyleImageOptions:
    """Options for adding or replacing a runtime style image."""

    pixel_ratio: float | None = None
    sdf: bool | None = None
    stretch_x: tuple[ImageStretch, ...] | None = None
    """Horizontally stretchable intervals. A present empty tuple stays
    distinguishable from an absent one."""
    stretch_y: tuple[ImageStretch, ...] | None = None
    content: ImageContent | None = None
    """Content box used when `icon-text-fit` applies."""
    text_fit_width: StyleImageTextFit | None = None
    text_fit_height: StyleImageTextFit | None = None


@dataclass(frozen=True, slots=True)
class StyleImageInfo:
    """Fixed metadata for one runtime style image."""

    width: int
    height: int
    stride: int
    byte_length: int
    pixel_ratio: float
    sdf: bool
    stretch_x_count: int = 0
    stretch_y_count: int = 0
    """Interval counts for the stretchable axes. Read the intervals themselves
    with `get_style_image_stretches`."""
    content: ImageContent | None = None
    text_fit_width: StyleImageTextFit | None = None
    text_fit_height: StyleImageTextFit | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> StyleImageInfo:
        content = raw["content"]
        text_fit_width = raw["text_fit_width"]
        text_fit_height = raw["text_fit_height"]
        return cls(
            width=raw["width"],
            height=raw["height"],
            stride=raw["stride"],
            byte_length=raw["byte_length"],
            pixel_ratio=raw["pixel_ratio"],
            sdf=raw["sdf"],
            stretch_x_count=raw["stretch_x_count"],
            stretch_y_count=raw["stretch_y_count"],
            content=None if content is None else ImageContent(*content),
            text_fit_width=(
                None if text_fit_width is None else StyleImageTextFit(text_fit_width)
            ),
            text_fit_height=(
                None if text_fit_height is None else StyleImageTextFit(text_fit_height)
            ),
        )


@dataclass(frozen=True, slots=True)
class StyleTransitionOptions:
    """The style's global transition options.

    These control how the style animates paint property changes and whether
    symbol placement changes cross-fade, and are distinct from camera animation
    options.
    """

    duration_ms: float | None = None
    """Transition duration in milliseconds. `None` falls back to the duration
    the style declares for each transitioning property."""
    delay_ms: float | None = None
    """Transition delay in milliseconds. `None` falls back to the delay the
    style declares for each transitioning property."""
    enable_placement_transitions: bool | None = None
    """Whether symbol placement changes cross-fade. `None` leaves the cross-fade
    on. Clearing it makes symbol placement changes apply to the next rendered
    frame. Reading the options always reports a value."""

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> StyleTransitionOptions:
        return cls(
            duration_ms=raw["duration_ms"],
            delay_ms=raw["delay_ms"],
            enable_placement_transitions=raw["enable_placement_transitions"],
        )


@dataclass(frozen=True, slots=True)
class StyleImage:
    """Copied runtime style image pixels with style-specific metadata."""

    image: PremultipliedRgba8Image
    pixel_ratio: float
    sdf: bool

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> StyleImage:
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
    def _from_native(cls, raw: dict[str, Any]) -> CustomGeometrySourceEvent:
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
    def _from_native(cls, native: Any) -> CustomGeometrySourceHandle:
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
    "GeoJsonSourceOptions",
    "LocationIndicatorImageKind",
    "RasterDemEncoding",
    "StyleImage",
    "StyleImageInfo",
    "StyleImageOptions",
    "StyleSourceInfo",
    "StyleSourceType",
    "StyleTransitionOptions",
    "TileJsonInfo",
    "TileScheme",
    "TileSourceOptions",
    "VectorTileEncoding",
]
