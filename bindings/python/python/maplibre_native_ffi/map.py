"""Map handles, options, and map lifecycle entry points."""

from __future__ import annotations

from collections.abc import Callable
from concurrent.futures import Future
from dataclasses import dataclass, field
from enum import IntFlag
from typing import Any

from . import _native
from ._enum import NativeIntEnum
from ._future import map_future, retain_future
from ._lifecycle import NativeHandleMixin
from .camera import (
    AnimationOptions,
    Bounded,
    BoundOptions,
    CameraDelta,
    CameraFitOptions,
    CameraOptions,
    EdgeInsets,
    FreeCameraOptions,
    ProjectionMode,
    ScreenPoint,
    Unbounded,
)
from .errors import InvalidArgumentError
from .geo import LatLng, LatLngBounds
from .query import FeatureStateSelector
from .render import (
    MetalBorrowedTextureDescriptor,
    MetalOwnedTextureDescriptor,
    MetalSurfaceDescriptor,
    OpenGLBorrowedTextureDescriptor,
    OpenGLOwnedTextureDescriptor,
    OpenGLSurfaceDescriptor,
    PremultipliedRgba8Image,
    RenderDriver,
    RenderSessionAttachOptions,
    RenderSessionHandle,
    VulkanBorrowedTextureDescriptor,
    VulkanOwnedTextureDescriptor,
    VulkanSurfaceDescriptor,
    WebGPUBorrowedTextureDescriptor,
    WebGPUOwnedTextureDescriptor,
    WebGPUSurfaceDescriptor,
    _opengl_context_parts,
)
from .style import (
    CanonicalTileId,
    CustomGeometrySourceHandle,
    CustomGeometrySourceOptions,
    CustomMvtVectorSourceHandle,
    CustomMvtVectorSourceOptions,
    GeoJsonSourceDataHandle,
    GeoJsonSourceOptions,
    ImageStretch,
    LocationIndicatorImageKind,
    StyleImage,
    StyleImageInfo,
    StyleImageOptions,
    StyleLayerInfo,
    StyleLayerVisibility,
    StyleSourceInfo,
    StyleTransitionOptions,
    TileSourceOptions,
    _geojson_source_parts,
)

_CORE_WORKER_ATTACH_OPTIONS = RenderSessionAttachOptions(RenderDriver.CORE_WORKER)
_CORE_WORKER_OWNED_ATTACH_OPTIONS = RenderSessionAttachOptions(
    RenderDriver.CORE_WORKER, 2
)
_CALLER_GRAPHICS_ATTACH_OPTIONS = RenderSessionAttachOptions(
    RenderDriver.CALLER_GRAPHICS_THREAD
)
_CALLER_GRAPHICS_OWNED_ATTACH_OPTIONS = RenderSessionAttachOptions(
    RenderDriver.CALLER_GRAPHICS_THREAD, 2
)


_MAP_HANDLE_CREATE_KEY = object()
_MAP_PROJECTION_HANDLE_CREATE_KEY = object()


class MapDebugOptions(IntFlag):
    """Map debug overlay mask bits."""

    NONE = 0
    TILE_BORDERS = 1 << 1
    PARSE_STATUS = 1 << 2
    TIMESTAMPS = 1 << 3
    COLLISION = 1 << 4
    OVERDRAW = 1 << 5
    STENCIL_CLIP = 1 << 6
    DEPTH_BUFFER = 1 << 7


class NorthOrientation(NativeIntEnum):
    """Map north orientation values."""

    UP = 0
    RIGHT = 1
    DOWN = 2
    LEFT = 3


class ConstrainMode(NativeIntEnum):
    """Map camera constraint modes."""

    NONE = 0
    HEIGHT_ONLY = 1
    WIDTH_AND_HEIGHT = 2
    SCREEN = 3


class ViewportMode(NativeIntEnum):
    """Viewport orientation modes."""

    DEFAULT = 0
    FLIPPED_Y = 1


class TileLodMode(NativeIntEnum):
    """Tile LOD algorithm values."""

    DEFAULT = 0
    DISTANCE = 1


class MapMode(NativeIntEnum):
    """Map rendering mode used when creating a map."""

    CONTINUOUS = 0
    STATIC = 1
    TILE = 2


@dataclass(slots=True)
class MapOptions:
    """Options used when creating a map.

    A field left as ``None`` takes the C API default.
    """

    width: int | None = None
    height: int | None = None
    scale_factor: float | None = None
    mode: MapMode | None = None
    fast_pfor_enabled: bool | None = None
    event_mask: RuntimeEventMask = field(
        # The factory arrives from the deferred import at the end of this
        # module, and it reads the C API creation default.
        default_factory=lambda: _default_map_event_mask()
    )
    """Map-originated event types this map queues from creation.

    The default takes the C API creation default, which queues every type the
    linked library reports. Set it here to narrow the map from its first style
    load, which produces the most tile and frame events. See
    :meth:`MapHandle.set_event_mask`.
    """


@dataclass(frozen=True, slots=True)
class MapViewportOptions:
    """Live map viewport and render-transform controls."""

    north_orientation: NorthOrientation | None = None
    constrain_mode: ConstrainMode | None = None
    viewport_mode: ViewportMode | None = None
    frustum_offset: EdgeInsets | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> MapViewportOptions:
        frustum_offset = raw["frustum_offset"]
        return cls(
            north_orientation=NorthOrientation(raw["north_orientation"])
            if raw["north_orientation"] is not None
            else None,
            constrain_mode=ConstrainMode(raw["constrain_mode"])
            if raw["constrain_mode"] is not None
            else None,
            viewport_mode=ViewportMode(raw["viewport_mode"])
            if raw["viewport_mode"] is not None
            else None,
            frustum_offset=EdgeInsets(**frustum_offset)
            if isinstance(frustum_offset, dict)
            else None,
        )


@dataclass(frozen=True, slots=True)
class MapTileOptions:
    """Tile prefetch and LOD tuning controls."""

    prefetch_zoom_delta: int | None = None
    lod_min_radius: float | None = None
    lod_scale: float | None = None
    lod_pitch_threshold: float | None = None
    lod_zoom_shift: float | None = None
    lod_mode: TileLodMode | None = None

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> MapTileOptions:
        return cls(
            prefetch_zoom_delta=raw["prefetch_zoom_delta"],
            lod_min_radius=raw["lod_min_radius"],
            lod_scale=raw["lod_scale"],
            lod_pitch_threshold=raw["lod_pitch_threshold"],
            lod_zoom_shift=raw["lod_zoom_shift"],
            lod_mode=TileLodMode(raw["lod_mode"])
            if raw["lod_mode"] is not None
            else None,
        )


@dataclass(frozen=True, slots=True)
class CameraSnapshot:
    """Camera copied from a complete map-state generation."""

    generation: int
    camera: CameraOptions

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> CameraSnapshot:
        return cls(
            generation=raw["generation"],
            camera=CameraOptions._from_native(raw["camera"]),
        )


@dataclass(frozen=True, slots=True)
class MapSnapshot:
    """Latest immutable state published by the map worker.

    Every committed map command publishes a snapshot, so a snapshot whose
    ``generation`` is at or past a command's reported generation observes that
    commit.
    """

    generation: int
    camera: CameraOptions
    width: int
    height: int
    scale_factor: float
    projection_mode: ProjectionMode
    viewport: MapViewportOptions
    debug_options: MapDebugOptions
    fully_loaded: bool
    """Whether every requested style and tile resource finished loading."""
    rendering_stats_view_enabled: bool
    repaint_demand: bool
    event_mask: RuntimeEventMask
    latest_render_update_generation: int
    tile: MapTileOptions
    bounds: BoundOptions
    free_camera: FreeCameraOptions

    @classmethod
    def _from_native(cls, raw: dict[str, object]) -> MapSnapshot:
        return cls(
            generation=raw["generation"],
            camera=CameraOptions._from_native(raw["camera"]),
            width=raw["width"],
            height=raw["height"],
            scale_factor=raw["scale_factor"],
            projection_mode=ProjectionMode._from_native(raw["projection_mode"]),
            viewport=MapViewportOptions._from_native(raw["viewport"]),
            debug_options=MapDebugOptions(raw["debug_options"]),
            fully_loaded=raw["fully_loaded"],
            rendering_stats_view_enabled=raw["rendering_stats_view_enabled"],
            repaint_demand=raw["repaint_demand"],
            event_mask=RuntimeEventMask(raw["event_mask"]),
            latest_render_update_generation=raw["latest_render_update_generation"],
            tile=MapTileOptions._from_native(raw["tile"]),
            bounds=BoundOptions._from_native(raw["bounds"]),
            free_camera=FreeCameraOptions._from_native(raw["free_camera"]),
        )


@dataclass(frozen=True, slots=True)
class ProjectedMeters:
    """Spherical Mercator projected-meter coordinate."""

    northing: float
    easting: float


def _camera_parts(
    camera: CameraOptions,
) -> tuple[
    tuple[float, float] | None,
    float | None,
    float | None,
    float | None,
    float | None,
    tuple[float, float, float, float] | None,
    tuple[float, float] | None,
    float | None,
    float | None,
]:
    center = (
        (camera.center.latitude, camera.center.longitude)
        if camera.center is not None
        else None
    )
    padding = (
        (
            camera.padding.top,
            camera.padding.left,
            camera.padding.bottom,
            camera.padding.right,
        )
        if camera.padding is not None
        else None
    )
    anchor = (camera.anchor.x, camera.anchor.y) if camera.anchor is not None else None
    return (
        center,
        camera.zoom,
        camera.bearing,
        camera.pitch,
        camera.center_altitude,
        padding,
        anchor,
        camera.roll,
        camera.field_of_view,
    )


def _fit_parts(
    fit: CameraFitOptions | None,
) -> tuple[tuple[float, float, float, float] | None, float | None, float | None]:
    if fit is None:
        return None, None, None
    padding = (
        (fit.padding.top, fit.padding.left, fit.padding.bottom, fit.padding.right)
        if fit.padding is not None
        else None
    )
    return padding, fit.bearing, fit.pitch


def _bounds_parts(
    bounds: BoundOptions,
) -> tuple[
    tuple[tuple[float, float], tuple[float, float]] | None,
    bool,
    float | None,
    float | None,
    float | None,
    float | None,
]:
    constraint = bounds.bounds
    raw_bounds: tuple[tuple[float, float], tuple[float, float]] | None = None
    if isinstance(constraint, Bounded):
        box = constraint.bounds
        raw_bounds = (
            (box.southwest.latitude, box.southwest.longitude),
            (box.northeast.latitude, box.northeast.longitude),
        )
    elif constraint is not None and not isinstance(constraint, Unbounded):
        # Annotations do not bind at runtime; without this an unsupported
        # value would read as "leave the constraint alone".
        raise InvalidArgumentError(
            "BoundOptions.bounds must be Bounded, Unbounded, or None, "
            f"not {type(constraint).__name__}"
        )
    return (
        raw_bounds,
        isinstance(constraint, Unbounded),
        bounds.min_zoom,
        bounds.max_zoom,
        bounds.min_pitch,
        bounds.max_pitch,
    )


def _animation_parts(
    animation: AnimationOptions | None,
) -> (
    tuple[
        float | None,
        float | None,
        float | None,
        tuple[float, float, float, float] | None,
        int | None,
    ]
    | None
):
    if animation is None:
        return None
    easing = (
        (
            animation.easing.p1x,
            animation.easing.p1y,
            animation.easing.p2x,
            animation.easing.p2y,
        )
        if animation.easing is not None
        else None
    )
    transition_id = animation.transition_id
    if transition_id is not None and not 0 <= transition_id < 2**64:
        # PyO3 raises a bare OverflowError extracting `Option<u64>` before the
        # binding's error conversion runs.
        raise InvalidArgumentError(
            f"AnimationOptions.transition_id must fit in 64 unsigned bits, "
            f"not {transition_id}"
        )
    return (
        animation.duration_ms,
        animation.velocity,
        animation.min_zoom,
        easing,
        transition_id,
    )


def _coordinate_parts(
    coordinates: list[LatLng] | tuple[LatLng, ...],
) -> list[tuple[float, float]]:
    return [(coordinate.latitude, coordinate.longitude) for coordinate in coordinates]


def _image_parts(image: PremultipliedRgba8Image) -> tuple[int, int, int, bytes]:
    return image.info.width, image.info.height, image.info.stride, image.data


def _tile_source_parts(
    options: TileSourceOptions | None,
) -> tuple[
    float | None,
    float | None,
    str | None,
    int | None,
    tuple[tuple[float, float], tuple[float, float]] | None,
    int | None,
    int | None,
    int | None,
]:
    if options is None:
        return None, None, None, None, None, None, None, None
    bounds = (
        (
            (options.bounds.southwest.latitude, options.bounds.southwest.longitude),
            (options.bounds.northeast.latitude, options.bounds.northeast.longitude),
        )
        if options.bounds is not None
        else None
    )
    return (
        options.min_zoom,
        options.max_zoom,
        options.attribution,
        int(options.scheme) if options.scheme is not None else None,
        bounds,
        options.tile_size,
        int(options.vector_encoding) if options.vector_encoding is not None else None,
        int(options.raster_dem_encoding)
        if options.raster_dem_encoding is not None
        else None,
    )


def projected_meters_for_lat_lng(coordinate: LatLng) -> ProjectedMeters:
    """Convert a geographic coordinate to spherical Mercator projected meters."""
    raw = _native.projected_meters_for_lat_lng(
        coordinate.latitude,
        coordinate.longitude,
    )
    return ProjectedMeters(northing=raw["northing"], easting=raw["easting"])


def lat_lng_for_projected_meters(meters: ProjectedMeters) -> LatLng:
    """Convert spherical Mercator projected meters to a geographic coordinate."""
    from .geo import LatLng

    raw = _native.lat_lng_for_projected_meters(meters.northing, meters.easting)
    return LatLng(latitude=raw["latitude"], longitude=raw["longitude"])


class MapProjectionHandle(NativeHandleMixin):
    """Any-thread standalone projection helper.

    Every method is synchronous, thread-safe, and internally serialized. A
    projection copies the map transform once at creation and never observes
    map changes made after that, while its own setters apply before they
    return, so a later read or conversion observes them. The projection remains
    usable after its source map and runtime close.
    """

    _handle_name = "MapProjectionHandle"

    def __init__(self, native: object, *, _create_key: object | None = None) -> None:
        if _create_key is not _MAP_PROJECTION_HANDLE_CREATE_KEY:
            msg = "MapProjectionHandle instances are created by MapHandle"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: object) -> MapProjectionHandle:
        return cls(native, _create_key=_MAP_PROJECTION_HANDLE_CREATE_KEY)

    def set_camera(self, camera: CameraOptions) -> None:
        """Apply camera fields to this projection before returning."""
        self._native.set_camera(*_camera_parts(camera))

    def set_visible_coordinates(
        self,
        coordinates: list[LatLng] | tuple[LatLng, ...],
        padding: EdgeInsets,
    ) -> None:
        """Apply a visible-coordinate camera fit before returning."""
        self._native.set_visible_coordinates(
            [(coordinate.latitude, coordinate.longitude) for coordinate in coordinates],
            (padding.top, padding.left, padding.bottom, padding.right),
        )

    def set_visible_geometry(self, geometry: bytes, padding: EdgeInsets) -> None:
        """Apply a visible-geometry camera fit before returning."""
        self._native.set_visible_geometry(
            geometry,
            (padding.top, padding.left, padding.bottom, padding.right),
        )

    @property
    def closed(self) -> bool:
        """Whether this projection handle has been released."""
        return self._native.closed

    def close(self) -> None:
        """Release this projection handle exactly once.

        The close is synchronous and waits for projection calls already running
        on other threads before it returns.
        """
        self._native.close()

    def get_camera(self) -> CameraOptions:
        """Return the projection camera, observing every earlier setter."""
        return CameraOptions._from_native(self._native.get_camera())

    def pixel_for_lat_lng(self, coordinate: LatLng) -> ScreenPoint:
        """Convert a geographic coordinate to a screen-space point."""
        raw = self._native.pixel_for_lat_lng(
            coordinate.latitude,
            coordinate.longitude,
        )
        return ScreenPoint(x=raw["x"], y=raw["y"])

    def lat_lng_for_pixel(self, point: ScreenPoint) -> LatLng:
        """Convert a screen-space point to a geographic coordinate."""
        raw = self._native.lat_lng_for_pixel(point.x, point.y)
        return LatLng(latitude=raw["latitude"], longitude=raw["longitude"])


class MapHandle(NativeHandleMixin):
    """Any-thread map handle backed by autonomous native execution."""

    _handle_name = "MapHandle"

    def __init__(
        self,
        runtime: RuntimeHandle,
        native: Any,
        *,
        _create_key: object | None = None,
    ) -> None:
        if _create_key is not _MAP_HANDLE_CREATE_KEY:
            msg = "MapHandle instances are created by RuntimeHandle.create_map"
            raise TypeError(msg)
        self._runtime = runtime
        self._native = native
        self._native_id_value = int(self._native.id())
        runtime._register_map(self)

    @classmethod
    def _create(
        cls, runtime: RuntimeHandle, options: MapOptions | None = None
    ) -> Future[MapHandle]:
        options = options or MapOptions()
        map_mode = (
            None
            if options.mode is None
            else (
                options.mode
                if isinstance(options.mode, MapMode)
                else MapMode(options.mode)
            )
        )
        return map_future(
            _native.create_map(
                runtime._native,
                options.width,
                options.height,
                options.scale_factor,
                None if map_mode is None else map_mode.native_code,
                options.fast_pfor_enabled,
                int(options.event_mask),
            ),
            lambda native: cls(runtime, native, _create_key=_MAP_HANDLE_CREATE_KEY),
        )

    def _native_id(self) -> int:
        return self._native_id_value

    def close(self) -> None:
        """Release this map handle exactly once.

        Closing prevents future runtime events from this map and leaves queued
        events unchanged. A queued event keeps the map's copied source ID.
        """
        self._native.close()
        self._runtime._unregister_map(self)

    def request_repaint(self) -> Future[CommandCompletion]:
        """Request a repaint and return its completion future."""
        return self._native.request_repaint()

    def request_still_image(self) -> Future[CommandCompletion]:
        """Start one noncoalescing still-image operation."""
        return self._native.request_still_image()

    def set_event_mask(self, mask: RuntimeEventMask) -> Future[CommandCompletion]:
        """Select queued map event types and return its completion future.

        The call reads the bits in :attr:`RuntimeEventMask.ALL_MAP_EVENTS` and
        ignores the rest, so :attr:`RuntimeEventMask.ALL` selects every
        map-originated type. Narrowing gates later events and keeps queued ones,
        so a host drains what it already caused.

        Select every type the host reads. Render-update is the map's only
        invalidation report, the two still-image types are the only reports that
        a still-image request finished, and loading-failure and render-error
        carry native failure text. A style failure that MapLibre raises inside
        :meth:`set_style_url` or :meth:`set_style_json` reaches the caller as an
        exception whatever this mask selects.
        """
        return self._native.set_event_mask(int(mask))

    @property
    def event_mask(self) -> RuntimeEventMask:
        """Map-originated event types this map queues.

        The value is the mask last set, including bits this map ignores, so a
        host reads it, changes one bit, and writes it back.
        """
        return RuntimeEventMask(self._native.get_event_mask())

    def set_debug_options(self, options: MapDebugOptions) -> Future[CommandCompletion]:
        """Submit debug overlay mask bits and return its completion future.

        Read the committed value back from :attr:`MapSnapshot.debug_options`.
        """
        return self._native.set_debug_options(int(options))

    def set_rendering_stats_view_enabled(
        self, enabled: bool
    ) -> Future[CommandCompletion]:
        """Submit rendering-stats visibility and return its completion future.

        Read the committed value back from
        :attr:`MapSnapshot.rendering_stats_view_enabled`.
        """
        return self._native.set_rendering_stats_view_enabled(enabled)

    def dump_debug_logs(self) -> Future[CommandCompletion]:
        """Submit a debug-log command and return its completion future."""
        return self._native.dump_debug_logs()

    def get_size(self) -> tuple[int, int, float]:
        """Return the map's logical width, height, and pixel ratio.

        The scale factor is fixed for the lifetime of the map and is
        independent of any render target's scale factor.
        """
        return self._native.get_size()

    def resize(
        self, width: int, height: int, scale_factor: float
    ) -> Future[CommandCompletion]:
        """Submit a logical extent update and return its completion future."""
        return self._native.resize(width, height, scale_factor)

    def snapshot(self) -> MapSnapshot:
        """Copy the latest immutable map-state generation."""
        return MapSnapshot._from_native(self._native.snapshot())

    def set_viewport_options(
        self, options: MapViewportOptions
    ) -> Future[CommandCompletion]:
        """Submit viewport controls and return its completion future.

        Read the committed values back from :attr:`MapSnapshot.viewport`.
        """
        frustum_offset = (
            (
                options.frustum_offset.top,
                options.frustum_offset.left,
                options.frustum_offset.bottom,
                options.frustum_offset.right,
            )
            if options.frustum_offset is not None
            else None
        )
        return self._native.set_viewport_options(
            int(options.north_orientation)
            if options.north_orientation is not None
            else None,
            int(options.constrain_mode) if options.constrain_mode is not None else None,
            int(options.viewport_mode) if options.viewport_mode is not None else None,
            frustum_offset,
        )

    def set_tile_options(self, options: MapTileOptions) -> Future[CommandCompletion]:
        """Submit tile prefetch and LOD controls and return its completion future.

        Read the committed values back from :attr:`MapSnapshot.tile`.
        """
        return self._native.set_tile_options(
            options.prefetch_zoom_delta,
            options.lod_min_radius,
            options.lod_scale,
            options.lod_pitch_threshold,
            options.lod_zoom_shift,
            int(options.lod_mode) if options.lod_mode is not None else None,
        )

    def set_style_url(self, url: str) -> Future[CommandCompletion]:
        """Load a style URL through MapLibre Native style APIs.

        Loading is asynchronous: a style that fails to fetch or parse returns
        normally here and reports through a later loading-failed runtime event.
        A well-formed style with semantically invalid contents loads without an
        error and without an event.
        """
        return self._native.set_style_url(url)

    def set_style_json(self, json: bytes) -> Future[CommandCompletion]:
        """Submit inline style JSON and return its completion future.

        Malformed JSON is accepted as a copied command, then reports a failed
        command disposition and a loading-failed runtime event. A failed parse
        leaves the previously loaded style unchanged.
        """
        return self._native.set_style_json(json)

    def set_feature_state(
        self,
        selector: FeatureStateSelector,
        state: bytes,
    ) -> Future[CommandCompletion]:
        """Submit per-feature state and return its completion future.

        Feature state belongs to the map. A render session pushes it into the
        renderer on the next render update, including the first presented
        frame that contains the source.
        """
        return self._native.set_feature_state(
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
            state,
        )

    def get_feature_state(self, selector: FeatureStateSelector) -> Future[bytes]:
        """Start an ordered read of copied per-feature state JSON.

        The read observes every map command accepted before it and copies the
        map store, not the last rendered frame. Missing feature state resolves
        to an empty JSON object.
        """
        return self._native.get_feature_state(
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
        )

    def remove_feature_state(
        self, selector: FeatureStateSelector
    ) -> Future[CommandCompletion]:
        """Submit per-feature state removal and return its completion future."""
        return self._native.remove_feature_state(
            selector.source_id,
            selector.source_layer_id,
            selector.feature_id,
            selector.state_key,
        )

    def get_loaded_style_json(self) -> Future[bytes]:
        """Return the style document this map's style was last parsed from.

        This is the loaded document, not a serialization of the live style, so
        runtime mutations do not change it and a failed parse leaves the
        previously parsed document in place. The result is empty when no
        document has been parsed.
        """
        return self._native.copy_loaded_style_json()

    def get_style_url(self) -> Future[str]:
        """Return the URL this map's style was last requested from.

        set_style_url() records the URL before the response arrives, and
        set_style_json() clears it, so this can disagree with
        get_loaded_style_json() while a load is in flight or after one fails.
        The result is empty when no URL bytes are available.
        """
        return self._native.copy_style_url()

    def add_style_source_json(
        self, source_id: str, source_json: bytes
    ) -> Future[CommandCompletion]:
        """Add one style source from a style-spec source JSON object."""
        return self._native.add_style_source_json(source_id, source_json)

    def add_geojson_source_url(
        self,
        source_id: str,
        url: str,
        options: GeoJsonSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add a GeoJSON source that loads data from a URL."""
        return self._native.add_geojson_source_url(
            source_id, url, *_geojson_source_parts(options)
        )

    def add_geojson_source_data(
        self,
        source_id: str,
        data: GeoJsonSourceDataHandle,
    ) -> Future[CommandCompletion]:
        """Add a GeoJSON source backed by prepared data.

        The call borrows the handle, and the source adopts the options the
        data was prepared with.
        """
        return self._native.add_geojson_source_data(source_id, data._native)

    def set_geojson_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Update one GeoJSON source to load data from a URL."""
        return self._native.set_geojson_source_url(source_id, url)

    def set_geojson_source_data(
        self, source_id: str, data: GeoJsonSourceDataHandle
    ) -> Future[CommandCompletion]:
        """Update one GeoJSON source with prepared data.

        The call borrows the handle and rejects data whose baked-in options
        differ from the source's, cluster properties excepted.
        """
        return self._native.set_geojson_source_data(source_id, data._native)

    def set_geojson_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> Future[CommandCompletion]:
        """Override one GeoJSON source's synchronous tiling at runtime.

        Tiling runs inline when either the source's baked-in option or this
        override enables it.
        """
        return self._native.set_geojson_source_synchronous_tiling(source_id, enabled)

    def _add_tile_source_url(
        self,
        add: Callable[..., Future[CommandCompletion]],
        source_id: str,
        url: str,
        options: TileSourceOptions | None,
    ) -> Future[CommandCompletion]:
        return add(source_id, url, *_tile_source_parts(options))

    def _add_tile_source_tiles(
        self,
        add: Callable[..., Future[CommandCompletion]],
        source_id: str,
        tiles: list[str] | tuple[str, ...],
        options: TileSourceOptions | None,
    ) -> Future[CommandCompletion]:
        return add(source_id, list(tiles), *_tile_source_parts(options))

    def add_vector_source_url(
        self,
        source_id: str,
        url: str,
        options: TileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add a vector source with a TileJSON URL."""
        return self._add_tile_source_url(
            self._native.add_vector_source_url, source_id, url, options
        )

    def add_raster_source_url(
        self,
        source_id: str,
        url: str,
        options: TileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add a raster source with a TileJSON URL."""
        return self._add_tile_source_url(
            self._native.add_raster_source_url, source_id, url, options
        )

    def add_raster_dem_source_url(
        self,
        source_id: str,
        url: str,
        options: TileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add a raster DEM source with a TileJSON URL."""
        return self._add_tile_source_url(
            self._native.add_raster_dem_source_url, source_id, url, options
        )

    def add_vector_source_tiles(
        self,
        source_id: str,
        tiles: list[str] | tuple[str, ...],
        options: TileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add a vector source with inline tile URLs."""
        return self._add_tile_source_tiles(
            self._native.add_vector_source_tiles, source_id, tiles, options
        )

    def add_raster_source_tiles(
        self,
        source_id: str,
        tiles: list[str] | tuple[str, ...],
        options: TileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add a raster source with inline tile URLs."""
        return self._add_tile_source_tiles(
            self._native.add_raster_source_tiles, source_id, tiles, options
        )

    def add_raster_dem_source_tiles(
        self,
        source_id: str,
        tiles: list[str] | tuple[str, ...],
        options: TileSourceOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add a raster DEM source with inline tile URLs."""
        return self._add_tile_source_tiles(
            self._native.add_raster_dem_source_tiles, source_id, tiles, options
        )

    def remove_style_source(self, source_id: str) -> Future[CommandCompletion]:
        """Submit a style source removal and return its completion future.

        The command's terminal event reports ``FAILED`` with
        :attr:`MaplibreStatus.NOT_FOUND` when no style source has the ID and
        ``FAILED`` with :attr:`MaplibreStatus.INVALID_STATE` while a layer
        still uses the source. Re-check existence through
        :meth:`get_style_source_info`.
        """
        return self._native.remove_style_source(source_id)

    def get_style_source_info(self, source_id: str) -> Future[StyleSourceInfo | None]:
        """Return copied retained metadata for one style source.

        The result is None when the source is missing, so this is also the
        existence check.
        """
        from .style import StyleSourceInfo

        return map_future(
            self._native.get_style_source_info(source_id),
            lambda raw: StyleSourceInfo._from_native(raw) if raw is not None else None,
        )

    def list_style_source_ids(self) -> Future[tuple[str, ...]]:
        """Return style source IDs in style order."""
        return map_future(self._native.list_style_source_ids(), tuple)

    def add_hillshade_layer(
        self,
        layer_id: str,
        source_id: str,
        before_layer_id: str | None = None,
    ) -> Future[CommandCompletion]:
        """Add a hillshade layer for a raster DEM source."""
        return self._native.add_hillshade_layer(layer_id, source_id, before_layer_id)

    def add_color_relief_layer(
        self,
        layer_id: str,
        source_id: str,
        before_layer_id: str | None = None,
    ) -> Future[CommandCompletion]:
        """Add a color-relief layer for a raster DEM source."""
        return self._native.add_color_relief_layer(layer_id, source_id, before_layer_id)

    def add_location_indicator_layer(
        self,
        layer_id: str,
        before_layer_id: str | None = None,
    ) -> Future[CommandCompletion]:
        """Add a source-free location indicator layer."""
        return self._native.add_location_indicator_layer(layer_id, before_layer_id)

    def set_location_indicator_location(
        self,
        layer_id: str,
        coordinate: LatLng,
        altitude: float,
    ) -> Future[CommandCompletion]:
        """Set a location indicator layer location."""
        return self._native.set_location_indicator_location(
            layer_id,
            coordinate.latitude,
            coordinate.longitude,
            altitude,
        )

    def set_location_indicator_bearing(
        self, layer_id: str, bearing: float
    ) -> Future[CommandCompletion]:
        """Set a location indicator layer bearing in degrees."""
        return self._native.set_location_indicator_bearing(layer_id, bearing)

    def set_location_indicator_accuracy_radius(
        self,
        layer_id: str,
        radius: float,
    ) -> Future[CommandCompletion]:
        """Set a location indicator layer accuracy radius in meters."""
        return self._native.set_location_indicator_accuracy_radius(layer_id, radius)

    def set_location_indicator_image_name(
        self,
        layer_id: str,
        image_kind: LocationIndicatorImageKind,
        image_id: str,
    ) -> Future[CommandCompletion]:
        """Set one location indicator image-name property."""
        return self._native.set_location_indicator_image_name(
            layer_id,
            image_kind.native_code,
            image_id,
        )

    def remove_style_layer(self, layer_id: str) -> Future[CommandCompletion]:
        """Submit a style layer removal and return its completion future.

        The command's terminal event reports ``FAILED`` with
        :attr:`MaplibreStatus.NOT_FOUND` when no style layer has the ID.
        Re-check existence through :meth:`get_style_layer_info`.
        """
        return self._native.remove_style_layer(layer_id)

    def get_style_layer_info(self, layer_id: str) -> Future[StyleLayerInfo | None]:
        """Return copied fixed metadata for one style layer.

        The result is None when the layer is missing, so this is also the
        existence check.
        """
        from .style import StyleLayerInfo

        return map_future(
            self._native.get_style_layer_info(layer_id),
            lambda raw: StyleLayerInfo._from_native(raw) if raw is not None else None,
        )

    def list_style_layer_ids(self) -> Future[tuple[str, ...]]:
        """Return style layer IDs in style order."""
        return map_future(self._native.list_style_layer_ids(), tuple)

    def move_style_layer(
        self,
        layer_id: str,
        before_layer_id: str | None = None,
    ) -> Future[CommandCompletion]:
        """Move one style layer before another layer or to the top."""
        return self._native.move_style_layer(layer_id, before_layer_id)

    def add_style_layer_json(
        self,
        layer_json: bytes,
        before_layer_id: str | None = None,
    ) -> Future[CommandCompletion]:
        """Add one style layer from a full style-spec layer JSON object."""
        return self._native.add_style_layer_json(layer_json, before_layer_id)

    def get_style_layer_json(self, layer_id: str) -> Future[bytes | None]:
        """Return one style layer as a full style-spec layer JSON object."""
        return self._native.get_style_layer_json(layer_id)

    def set_style_light_json(self, light_json: bytes) -> Future[CommandCompletion]:
        """Set the style light from a style-spec light JSON object."""
        return self._native.set_style_light_json(light_json)

    def set_style_light_property(
        self, property_name: str, value: bytes
    ) -> Future[CommandCompletion]:
        """Set one style light property by style-spec property name."""
        return self._native.set_style_light_property(property_name, value)

    def get_style_light_property(self, property_name: str) -> Future[bytes | None]:
        """Return one style light property as a style-spec JSON value."""
        return self._native.get_style_light_property(property_name)

    def set_style_transition_options(
        self, options: StyleTransitionOptions
    ) -> Future[CommandCompletion]:
        """Set the style's global transition options.

        This call replaces the whole transition configuration rather than
        merging into it. Loading a style replaces these options with the ones
        that style declares, so apply an override after the style loads.
        """
        return self._native.set_style_transition_options(
            options.duration_ms,
            options.delay_ms,
            options.enable_placement_transitions,
        )

    def get_style_transition_options(self) -> Future[StyleTransitionOptions]:
        """Return the style's global transition options."""
        return map_future(
            self._native.get_style_transition_options(),
            StyleTransitionOptions._from_native,
        )

    def set_layer_property(
        self,
        layer_id: str,
        property_name: str,
        value: bytes,
    ) -> Future[CommandCompletion]:
        """Set one layer property by style-spec property name."""
        return self._native.set_layer_property(layer_id, property_name, value)

    def get_layer_property(
        self, layer_id: str, property_name: str
    ) -> Future[bytes | None]:
        """Return one layer property as a style-spec JSON value."""
        return self._native.get_layer_property(layer_id, property_name)

    def set_layer_filter(
        self, layer_id: str, filter: bytes | None
    ) -> Future[CommandCompletion]:
        """Set or clear one layer filter."""
        return self._native.set_layer_filter(layer_id, filter)

    def get_layer_filter(self, layer_id: str) -> Future[bytes | None]:
        """Return one layer filter as a style-spec JSON value."""
        return self._native.get_layer_filter(layer_id)

    def set_layer_source_layer(
        self, layer_id: str, source_layer: str
    ) -> Future[CommandCompletion]:
        """Set one layer's source-layer ID.

        Layer types that take no source, such as background, are rejected.
        """
        return self._native.set_layer_source_layer(layer_id, source_layer)

    def get_layer_source_layer(self, layer_id: str) -> Future[str]:
        """Return one layer's source-layer ID, empty when it carries none."""
        return self._native.copy_layer_source_layer(layer_id)

    def set_layer_source_id(
        self, layer_id: str, source_id: str
    ) -> Future[CommandCompletion]:
        """Set one layer's source ID.

        Layer types that take no source, such as background, are rejected. The
        named source need not exist yet.
        """
        return self._native.set_layer_source_id(layer_id, source_id)

    def get_layer_source_id(self, layer_id: str) -> Future[str]:
        """Return one layer's source ID, empty when it carries none."""
        return self._native.copy_layer_source_id(layer_id)

    def set_layer_min_zoom(
        self, layer_id: str, min_zoom: float
    ) -> Future[CommandCompletion]:
        """Set the lowest zoom at which one layer draws.

        Pass ``-math.inf`` for no lower bound. Read the value back from
        :attr:`StyleLayerInfo.min_zoom`.
        """
        return self._native.set_layer_min_zoom(layer_id, min_zoom)

    def set_layer_max_zoom(
        self, layer_id: str, max_zoom: float
    ) -> Future[CommandCompletion]:
        """Set the highest zoom at which one layer draws.

        Pass ``math.inf`` for no upper bound. Read the value back from
        :attr:`StyleLayerInfo.max_zoom`.
        """
        return self._native.set_layer_max_zoom(layer_id, max_zoom)

    def set_layer_visibility(
        self, layer_id: str, visibility: StyleLayerVisibility
    ) -> Future[CommandCompletion]:
        """Set whether one layer draws.

        Read the value back from :attr:`StyleLayerInfo.visibility`.
        """
        return self._native.set_layer_visibility(layer_id, int(visibility))

    def set_style_image(
        self,
        image_id: str,
        image: PremultipliedRgba8Image,
        options: StyleImageOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Add or replace one runtime style image."""
        from .render import PremultipliedRgba8Image
        from .style import StyleImageOptions

        options = options or StyleImageOptions()
        if not isinstance(image, PremultipliedRgba8Image):
            msg = "image must be a PremultipliedRgba8Image"
            raise TypeError(msg)
        return self._native.set_style_image(
            image_id,
            image.info.width,
            image.info.height,
            image.info.stride,
            image.data,
            options.pixel_ratio,
            options.sdf,
            None
            if options.stretch_x is None
            else [(s.from_, s.to) for s in options.stretch_x],
            None
            if options.stretch_y is None
            else [(s.from_, s.to) for s in options.stretch_y],
            None
            if options.content is None
            else (
                options.content.left,
                options.content.top,
                options.content.right,
                options.content.bottom,
            ),
            None if options.text_fit_width is None else int(options.text_fit_width),
            None if options.text_fit_height is None else int(options.text_fit_height),
        )

    def get_style_image_stretches(
        self, image_id: str
    ) -> Future[tuple[tuple[ImageStretch, ...], tuple[ImageStretch, ...]] | None]:
        """Return one style image's stretchable intervals, or None when missing."""
        from .style import ImageStretch

        def adapt(copied: object):
            if copied is None:
                return None
            stretch_x, stretch_y = copied
            return (
                tuple(ImageStretch(from_, to) for from_, to in stretch_x),
                tuple(ImageStretch(from_, to) for from_, to in stretch_y),
            )

        return map_future(
            self._native.copy_style_image_stretches(image_id),
            adapt,
        )

    def remove_style_image(self, image_id: str) -> Future[CommandCompletion]:
        """Submit a runtime style image removal and return its completion future.

        The command's terminal event reports ``FAILED`` with
        :attr:`MaplibreStatus.NOT_FOUND` when no runtime style image has the
        ID. Re-check existence through :meth:`get_style_image_info`.
        """
        return self._native.remove_style_image(image_id)

    def get_style_image_info(self, image_id: str) -> Future[StyleImageInfo | None]:
        """Return fixed metadata for one runtime style image.

        The result is None when the image is missing, so this is also the
        existence check.
        """
        from .style import StyleImageInfo

        return map_future(
            self._native.get_style_image_info(image_id),
            lambda raw: StyleImageInfo._from_native(raw) if raw is not None else None,
        )

    def copy_style_image_premultiplied_rgba8(
        self, image_id: str
    ) -> Future[StyleImage | None]:
        """Copy one runtime style image as premultiplied RGBA8 pixels."""
        from .style import StyleImage

        return map_future(
            self._native.copy_style_image_premultiplied_rgba8(image_id),
            lambda raw: StyleImage._from_native(raw) if raw is not None else None,
        )

    def add_image_source_url(
        self,
        source_id: str,
        coordinates: list[LatLng] | tuple[LatLng, ...],
        url: str,
    ) -> Future[CommandCompletion]:
        """Add an image source that loads its image from a URL."""
        return self._native.add_image_source_url(
            source_id, _coordinate_parts(coordinates), url
        )

    def add_image_source_image(
        self,
        source_id: str,
        coordinates: list[LatLng] | tuple[LatLng, ...],
        image: PremultipliedRgba8Image,
    ) -> Future[CommandCompletion]:
        """Add an image source with inline image pixels."""
        return self._native.add_image_source_image(
            source_id,
            _coordinate_parts(coordinates),
            *_image_parts(image),
        )

    def set_image_source_url(
        self, source_id: str, url: str
    ) -> Future[CommandCompletion]:
        """Update an image source to load its image from a URL."""
        return self._native.set_image_source_url(source_id, url)

    def set_image_source_image(
        self,
        source_id: str,
        image: PremultipliedRgba8Image,
    ) -> Future[CommandCompletion]:
        """Update an image source with inline image pixels."""
        return self._native.set_image_source_image(source_id, *_image_parts(image))

    def set_image_source_coordinates(
        self,
        source_id: str,
        coordinates: list[LatLng] | tuple[LatLng, ...],
    ) -> Future[CommandCompletion]:
        """Update image source coordinates."""
        return self._native.set_image_source_coordinates(
            source_id, _coordinate_parts(coordinates)
        )

    def get_image_source_coordinates(
        self, source_id: str
    ) -> Future[tuple[LatLng, ...] | None]:
        """Return copied image source coordinates, or None when missing."""
        from .geo import LatLng

        return map_future(
            self._native.get_image_source_coordinates(source_id),
            lambda raw: (
                None
                if raw is None
                else tuple(LatLng(**coordinate) for coordinate in raw)
            ),
        )

    def create_projection(self) -> Future[MapProjectionHandle]:
        """Create a standalone projection helper from the current map transform."""
        return map_future(
            self._native.create_projection(), MapProjectionHandle._from_native
        )

    def get_camera(self) -> CameraOptions:
        """Return the current camera snapshot."""
        from .camera import CameraOptions

        return CameraOptions._from_native(self._native.get_camera())

    def get_camera_ordered(self) -> Future[CameraSnapshot]:
        """Return a future for a camera read ordered after prior commands."""
        return map_future(
            self._native.get_camera_ordered(), CameraSnapshot._from_native
        )

    def set_camera(self, camera: CameraOptions) -> Future[CommandCompletion]:
        """Submit camera fields and return its completion future."""
        return self._native.set_camera(*_camera_parts(camera))

    def set_visible_coordinates(
        self,
        coordinates: list[LatLng] | tuple[LatLng, ...],
        padding: EdgeInsets,
    ) -> Future[CommandCompletion]:
        """Submit a visible-coordinate fit and return its completion future."""
        return self._native.set_visible_coordinates(
            [(coordinate.latitude, coordinate.longitude) for coordinate in coordinates],
            (padding.top, padding.left, padding.bottom, padding.right),
        )

    def set_visible_geometry(
        self, geometry: bytes, padding: EdgeInsets
    ) -> Future[CommandCompletion]:
        """Submit a visible-geometry fit and return its completion future."""
        return self._native.set_visible_geometry(
            geometry,
            (padding.top, padding.left, padding.bottom, padding.right),
        )

    def jump_to(self, camera: CameraOptions) -> Future[CommandCompletion]:
        """Submit a camera jump and return its completion future."""
        return self._native.jump_to(*_camera_parts(camera))

    def ease_to(
        self,
        camera: CameraOptions,
        animation: AnimationOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Submit a camera ease transition and return its completion future."""
        return self._native.ease_to(*_camera_parts(camera), _animation_parts(animation))

    def fly_to(
        self,
        camera: CameraOptions,
        animation: AnimationOptions | None = None,
    ) -> Future[CommandCompletion]:
        """Submit a camera fly transition and return its completion future."""
        return self._native.fly_to(*_camera_parts(camera), _animation_parts(animation))

    def apply_camera_delta(self, delta: CameraDelta) -> Future[CommandCompletion]:
        """Submit one relative camera operation."""
        anchor = delta.anchor
        return self._native.apply_camera_delta(
            int(delta.kind),
            (delta.offset.x, delta.offset.y),
            delta.amount,
            None if anchor is None else (anchor.x, anchor.y),
            _animation_parts(delta.animation),
        )

    def camera_for_lat_lng_bounds(
        self,
        bounds: LatLngBounds,
        fit: CameraFitOptions | None = None,
    ) -> Future[CameraOptions]:
        """Compute a camera that fits geographic bounds in the current viewport."""
        from .camera import CameraOptions

        return map_future(
            self._native.camera_for_lat_lng_bounds(
                (bounds.southwest.latitude, bounds.southwest.longitude),
                (bounds.northeast.latitude, bounds.northeast.longitude),
                *_fit_parts(fit),
            ),
            CameraOptions._from_native,
        )

    def camera_for_lat_lngs(
        self,
        coordinates: list[LatLng] | tuple[LatLng, ...],
        fit: CameraFitOptions | None = None,
    ) -> Future[CameraOptions]:
        """Compute a camera that fits geographic coordinates in the current viewport."""
        from .camera import CameraOptions

        return map_future(
            self._native.camera_for_lat_lngs(
                _coordinate_parts(coordinates),
                *_fit_parts(fit),
            ),
            CameraOptions._from_native,
        )

    def camera_for_geometry(
        self,
        geometry: bytes,
        fit: CameraFitOptions | None = None,
    ) -> Future[CameraOptions]:
        """Compute a camera that fits a geometry in the current viewport."""
        from .camera import CameraOptions

        return map_future(
            self._native.camera_for_geometry(
                geometry,
                *_fit_parts(fit),
            ),
            CameraOptions._from_native,
        )

    def lat_lng_bounds_for_camera(
        self,
        camera: CameraOptions,
        *,
        unwrapped: bool = False,
    ) -> Future[LatLngBounds]:
        """Compute geographic bounds for a camera from two viewport corners.

        The box is the hull of the top-left and bottom-right screen corners
        for that camera in the current viewport. When bearing and pitch are
        zero, the box equals the visible area. Those corners are the northwest
        and southeast of the viewport. Longitudes stay in -180 to 180.

        Pass `unwrapped=True` for the axis-aligned hull of all four screen
        corners and the center. That hull encompasses the projected viewport.
        Longitudes unwrap onto the shortest path through the center. A
        viewport that crosses the antimeridian reports values outside -180
        to 180.
        """
        from .geo import LatLng, LatLngBounds

        return map_future(
            self._native.lat_lng_bounds_for_camera(*_camera_parts(camera), unwrapped),
            lambda raw: LatLngBounds(
                southwest=LatLng(**raw["southwest"]),
                northeast=LatLng(**raw["northeast"]),
            ),
        )

    def set_bounds(self, bounds: BoundOptions) -> Future[CommandCompletion]:
        """Submit map camera constraints and return its completion future.

        Read the committed values back from :attr:`MapSnapshot.bounds`.
        """
        return self._native.set_bounds(*_bounds_parts(bounds))

    def set_free_camera_options(
        self, options: FreeCameraOptions
    ) -> Future[CommandCompletion]:
        """Submit free-camera fields and return its completion future.

        Read the committed values back from :attr:`MapSnapshot.free_camera`.
        """
        position = (
            (options.position.x, options.position.y, options.position.z)
            if options.position is not None
            else None
        )
        orientation = (
            (
                options.orientation.x,
                options.orientation.y,
                options.orientation.z,
                options.orientation.w,
            )
            if options.orientation is not None
            else None
        )
        return self._native.set_free_camera_options(position, orientation)

    def get_projection_mode(self) -> ProjectionMode:
        """Return the latest published axonometric rendering options."""
        return self.snapshot().projection_mode

    def set_projection_mode(self, mode: ProjectionMode) -> Future[CommandCompletion]:
        """Apply axonometric rendering option fields to the map."""
        return self._native.set_projection_mode(
            mode.axonometric, mode.x_skew, mode.y_skew
        )

    def pixel_for_lat_lng(self, coordinate: LatLng) -> Future[ScreenPoint]:
        """Convert a geographic world coordinate to a screen point for this map."""
        from .camera import ScreenPoint

        return map_future(
            self._native.pixel_for_lat_lng(
                coordinate.latitude,
                coordinate.longitude,
            ),
            lambda raw: ScreenPoint(x=raw["x"], y=raw["y"]),
        )

    def lat_lng_for_pixel(self, point: ScreenPoint) -> Future[LatLng]:
        """Convert a screen point to a geographic world coordinate for this map."""
        from .geo import LatLng

        return map_future(
            self._native.lat_lng_for_pixel(point.x, point.y),
            lambda raw: LatLng(latitude=raw["latitude"], longitude=raw["longitude"]),
        )

    def pixels_for_lat_lngs(
        self,
        coordinates: list[LatLng] | tuple[LatLng, ...],
    ) -> Future[tuple[ScreenPoint, ...]]:
        """Convert geographic world coordinates to screen points for this map."""
        from .camera import ScreenPoint

        return map_future(
            self._native.pixels_for_lat_lngs(_coordinate_parts(coordinates)),
            lambda raw: tuple(ScreenPoint(x=point["x"], y=point["y"]) for point in raw),
        )

    def lat_lngs_for_pixels(
        self,
        points: list[ScreenPoint] | tuple[ScreenPoint, ...],
    ) -> Future[tuple[LatLng, ...]]:
        """Convert screen points to geographic world coordinates for this map."""
        from .geo import LatLng

        return map_future(
            self._native.lat_lngs_for_pixels([(point.x, point.y) for point in points]),
            lambda raw: tuple(LatLng(**coordinate) for coordinate in raw),
        )

    def add_custom_geometry_source(
        self,
        source_id: str,
        options: CustomGeometrySourceOptions | None = None,
    ) -> tuple[CustomGeometrySourceHandle, Future[CommandCompletion]]:
        """Add a custom geometry source and return its queued-event handle.

        The handle closes when the source goes away: an explicit removal, a
        style load that leaves a style without the source, or closing this map.
        """
        from .style import CustomGeometrySourceHandle, CustomGeometrySourceOptions

        options = options or CustomGeometrySourceOptions()
        native, completion = self._native.add_custom_geometry_source(
            source_id,
            options.max_queued_events,
            options.min_zoom,
            options.max_zoom,
            options.tolerance,
            options.tile_size,
            options.buffer,
            options.clip,
            options.wrap,
            options.has_cancel_tile,
        )
        return CustomGeometrySourceHandle._from_native(native), completion

    def set_custom_geometry_source_tile_data(
        self,
        source_id: str,
        tile_id: CanonicalTileId,
        data: bytes,
    ) -> Future[CommandCompletion]:
        """Set custom geometry source data for one canonical tile."""
        return self._native.set_custom_geometry_source_tile_data(
            source_id,
            tile_id.z,
            tile_id.x,
            tile_id.y,
            data,
        )

    def invalidate_custom_geometry_source_tile(
        self,
        source_id: str,
        tile_id: CanonicalTileId,
    ) -> Future[CommandCompletion]:
        """Invalidate custom geometry source data for one canonical tile."""
        return self._native.invalidate_custom_geometry_source_tile(
            source_id,
            tile_id.z,
            tile_id.x,
            tile_id.y,
        )

    def invalidate_custom_geometry_source_region(
        self,
        source_id: str,
        bounds: LatLngBounds,
    ) -> Future[CommandCompletion]:
        """Invalidate custom geometry source data inside one geographic region."""
        return self._native.invalidate_custom_geometry_source_region(
            source_id,
            (bounds.southwest.latitude, bounds.southwest.longitude),
            (bounds.northeast.latitude, bounds.northeast.longitude),
        )

    def add_custom_mvt_vector_source(
        self,
        source_id: str,
        options: CustomMvtVectorSourceOptions | None = None,
    ) -> tuple[CustomMvtVectorSourceHandle, Future[CommandCompletion]]:
        """Add a custom MVT vector source and return its queued-event handle.

        The handle closes when the source goes away: an explicit removal, a
        style load that leaves a style without the source, or closing this map.
        """
        from .style import CustomMvtVectorSourceHandle, CustomMvtVectorSourceOptions

        options = options or CustomMvtVectorSourceOptions()
        native, completion = self._native.add_custom_mvt_vector_source(
            source_id,
            options.max_queued_events,
            options.min_zoom,
            options.max_zoom,
            options.has_cancel_tile,
        )
        return CustomMvtVectorSourceHandle._from_native(native), completion

    def set_custom_mvt_vector_source_tile_data(
        self,
        source_id: str,
        tile_id: CanonicalTileId,
        data: bytes,
    ) -> Future[CommandCompletion]:
        """Set custom MVT vector source data for one canonical tile."""
        return self._native.set_custom_mvt_vector_source_tile_data(
            source_id,
            tile_id.z,
            tile_id.x,
            tile_id.y,
            data,
        )

    def set_custom_mvt_vector_source_tile_error(
        self,
        source_id: str,
        tile_id: CanonicalTileId,
        message: str,
    ) -> Future[CommandCompletion]:
        """Report a custom MVT vector source error for one canonical tile."""
        return self._native.set_custom_mvt_vector_source_tile_error(
            source_id,
            tile_id.z,
            tile_id.x,
            tile_id.y,
            message,
        )

    def invalidate_custom_mvt_vector_source_tile(
        self,
        source_id: str,
        tile_id: CanonicalTileId,
    ) -> Future[CommandCompletion]:
        """Invalidate custom MVT vector source data for one canonical tile."""
        return self._native.invalidate_custom_mvt_vector_source_tile(
            source_id,
            tile_id.z,
            tile_id.x,
            tile_id.y,
        )

    def _attach_render_session(
        self,
        attach: Callable[..., object],
        descriptor: Any,
        options: RenderSessionAttachOptions,
        *args: object,
    ) -> tuple[RenderSessionHandle, Any]:
        extent = descriptor.extent
        native, completion = attach(
            self._native,
            extent.width,
            extent.height,
            extent.scale_factor,
            *args,
            options.driver.native_code,
            options.requested_texture_ring_depth,
        )
        session = RenderSessionHandle._from_native(native, self)
        return session, retain_future(completion, session)

    def attach_metal_surface(
        self,
        descriptor: MetalSurfaceDescriptor,
        options: RenderSessionAttachOptions = _CORE_WORKER_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a Metal native surface render target."""
        return self._attach_render_session(
            _native.attach_metal_surface,
            descriptor,
            options,
            descriptor.context.device.address,
            descriptor.layer.address,
        )

    def attach_vulkan_surface(
        self,
        descriptor: VulkanSurfaceDescriptor,
        options: RenderSessionAttachOptions = _CORE_WORKER_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a Vulkan native surface render target."""
        return self._attach_render_session(
            _native.attach_vulkan_surface,
            descriptor,
            options,
            descriptor.context.instance.address,
            descriptor.context.physical_device.address,
            descriptor.context.device.address,
            descriptor.context.graphics_queue.address,
            descriptor.context.graphics_queue_family_index,
            descriptor.context.get_instance_proc_addr.address,
            descriptor.context.get_device_proc_addr.address,
            descriptor.surface.address,
        )

    def attach_webgpu_surface(
        self,
        descriptor: WebGPUSurfaceDescriptor,
        options: RenderSessionAttachOptions = _CALLER_GRAPHICS_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a WebGPU native surface render target."""
        context = descriptor.context
        return self._attach_render_session(
            _native.attach_webgpu_surface,
            descriptor,
            options,
            context.instance.address,
            context.device.address,
            context.queue.address,
            descriptor.surface.address,
            descriptor.format,
        )

    def attach_metal_owned_texture(
        self,
        descriptor: MetalOwnedTextureDescriptor,
        options: RenderSessionAttachOptions = _CORE_WORKER_OWNED_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a Metal session-owned texture ring."""
        return self._attach_render_session(
            _native.attach_metal_owned_texture,
            descriptor,
            options,
            descriptor.context.device.address,
        )

    def attach_metal_borrowed_texture(
        self,
        descriptor: MetalBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions = _CORE_WORKER_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a caller-owned Metal texture."""
        return self._attach_render_session(
            _native.attach_metal_borrowed_texture,
            descriptor,
            options,
            descriptor.physical_width,
            descriptor.physical_height,
            descriptor.texture.address,
        )

    def attach_vulkan_owned_texture(
        self,
        descriptor: VulkanOwnedTextureDescriptor,
        options: RenderSessionAttachOptions = _CORE_WORKER_OWNED_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a Vulkan session-owned texture ring."""
        return self._attach_render_session(
            _native.attach_vulkan_owned_texture,
            descriptor,
            options,
            descriptor.context.instance.address,
            descriptor.context.physical_device.address,
            descriptor.context.device.address,
            descriptor.context.graphics_queue.address,
            descriptor.context.graphics_queue_family_index,
            descriptor.context.get_instance_proc_addr.address,
            descriptor.context.get_device_proc_addr.address,
        )

    def attach_webgpu_owned_texture(
        self,
        descriptor: WebGPUOwnedTextureDescriptor,
        options: RenderSessionAttachOptions = _CALLER_GRAPHICS_OWNED_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a WebGPU session-owned texture ring."""
        context = descriptor.context
        return self._attach_render_session(
            _native.attach_webgpu_owned_texture,
            descriptor,
            options,
            context.instance.address,
            context.device.address,
            context.queue.address,
        )

    def attach_webgpu_borrowed_texture(
        self,
        descriptor: WebGPUBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions = _CALLER_GRAPHICS_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a caller-owned WebGPU texture."""
        context = descriptor.context
        return self._attach_render_session(
            _native.attach_webgpu_borrowed_texture,
            descriptor,
            options,
            descriptor.physical_width,
            descriptor.physical_height,
            context.instance.address,
            context.device.address,
            context.queue.address,
            descriptor.texture.address,
            descriptor.texture_view.address,
            descriptor.format,
        )

    def attach_vulkan_borrowed_texture(
        self,
        descriptor: VulkanBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions = _CORE_WORKER_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a caller-owned Vulkan texture."""
        return self._attach_render_session(
            _native.attach_vulkan_borrowed_texture,
            descriptor,
            options,
            descriptor.physical_width,
            descriptor.physical_height,
            descriptor.context.instance.address,
            descriptor.context.physical_device.address,
            descriptor.context.device.address,
            descriptor.context.graphics_queue.address,
            descriptor.context.graphics_queue_family_index,
            descriptor.context.get_instance_proc_addr.address,
            descriptor.context.get_device_proc_addr.address,
            descriptor.image.address,
            descriptor.image_view.address,
            descriptor.format,
            descriptor.initial_layout,
            descriptor.final_layout,
        )

    def attach_opengl_surface(
        self,
        descriptor: OpenGLSurfaceDescriptor,
        options: RenderSessionAttachOptions = _CALLER_GRAPHICS_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching an OpenGL native surface render target."""
        platform, ownership, first, second, share, client_api, get_proc = (
            _opengl_context_parts(descriptor.context)
        )
        return self._attach_render_session(
            _native.attach_opengl_surface,
            descriptor,
            options,
            platform,
            ownership,
            first,
            second,
            share,
            client_api,
            get_proc,
            descriptor.surface.address,
        )

    def attach_opengl_owned_texture(
        self,
        descriptor: OpenGLOwnedTextureDescriptor,
        options: RenderSessionAttachOptions = _CALLER_GRAPHICS_OWNED_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching an OpenGL session-owned texture ring."""
        platform, ownership, first, second, share, client_api, get_proc = (
            _opengl_context_parts(descriptor.context)
        )
        return self._attach_render_session(
            _native.attach_opengl_owned_texture,
            descriptor,
            options,
            platform,
            ownership,
            first,
            second,
            share,
            client_api,
            get_proc,
        )

    def attach_opengl_borrowed_texture(
        self,
        descriptor: OpenGLBorrowedTextureDescriptor,
        options: RenderSessionAttachOptions = _CALLER_GRAPHICS_ATTACH_OPTIONS,
    ) -> tuple[RenderSessionHandle, Any]:
        """Start attaching a caller-owned OpenGL texture."""
        platform, ownership, first, second, share, client_api, get_proc = (
            _opengl_context_parts(descriptor.context)
        )
        return self._attach_render_session(
            _native.attach_opengl_borrowed_texture,
            descriptor,
            options,
            descriptor.physical_width,
            descriptor.physical_height,
            platform,
            ownership,
            first,
            second,
            share,
            client_api,
            get_proc,
            descriptor.texture,
            descriptor.target,
        )


__all__ = [
    "CameraSnapshot",
    "ConstrainMode",
    "MapDebugOptions",
    "MapHandle",
    "MapMode",
    "MapOptions",
    "MapProjectionHandle",
    "MapSnapshot",
    "MapTileOptions",
    "MapViewportOptions",
    "NorthOrientation",
    "ProjectedMeters",
    "TileLodMode",
    "ViewportMode",
    "lat_lng_for_projected_meters",
    "projected_meters_for_lat_lng",
]

from .runtime import (
    CommandCompletion,
    RuntimeEventMask,
    RuntimeHandle,
    _default_map_event_mask,
)
