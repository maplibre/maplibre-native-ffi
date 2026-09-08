const std = @import("std");

const c = @import("c.zig").raw;

pub const LatLng = struct {
    latitude: f64,
    longitude: f64,
};

pub const ScreenPoint = struct {
    x: f64,
    y: f64,
};

pub const EdgeInsets = struct {
    top: f64 = 0,
    left: f64 = 0,
    bottom: f64 = 0,
    right: f64 = 0,
};

pub const LatLngBounds = struct {
    southwest: LatLng,
    northeast: LatLng,
};

pub const ProjectedMeters = struct {
    northing: f64,
    easting: f64,
};

pub const MapId = struct {
    value: u64,
};

pub const UnitBezier = struct {
    x1: f64,
    y1: f64,
    x2: f64,
    y2: f64,
};

pub const CameraOptions = struct {
    center: ?LatLng = null,
    center_altitude: ?f64 = null,
    padding: ?EdgeInsets = null,
    /// Input-only screen point the camera pivots around. Atomic jump, ease, and
    /// fly updates honor it; MapLibre leaves it null on every read path,
    /// including camera snapshots and camera-for-bounds helpers.
    anchor: ?ScreenPoint = null,
    zoom: ?f64 = null,
    bearing: ?f64 = null,
    pitch: ?f64 = null,
    roll: ?f64 = null,
    field_of_view: ?f64 = null,
};

pub const AnimationOptions = struct {
    duration_ms: ?f64 = null,
    velocity: ?f64 = null,
    min_zoom: ?f64 = null,
    easing: ?UnitBezier = null,
    /// Caller-chosen identity for the transition these options start. When set,
    /// the transition reports its end once through a
    /// `map_camera_transition_finished` runtime event carrying this value; when
    /// absent, no such event is reported. The value is passed through
    /// uninterpreted.
    transition_id: ?u64 = null,
};

pub const CameraFitOptions = struct {
    padding: ?EdgeInsets = null,
    bearing: ?f64 = null,
    pitch: ?f64 = null,
};

/// Geographic constraint applied to the map camera center. The unbounded case
/// leaves the center free to pan across the antimeridian, unlike world bounds of
/// -90/-180 to 90/180, which clamp longitude to that range.
pub const BoundsConstraint = union(enum) {
    bounded: LatLngBounds,
    unbounded,
};

pub const BoundOptions = struct {
    bounds: ?BoundsConstraint = null,
    min_zoom: ?f64 = null,
    max_zoom: ?f64 = null,
    min_pitch: ?f64 = null,
    max_pitch: ?f64 = null,
};

pub const Vec3 = struct {
    x: f64,
    y: f64,
    z: f64,
};

pub const Quaternion = struct {
    x: f64,
    y: f64,
    z: f64,
    w: f64,
};

pub const FreeCameraOptions = struct {
    position: ?Vec3 = null,
    orientation: ?Quaternion = null,
};

pub const ProjectionMode = struct {
    axonometric: ?bool = null,
    x_skew: ?f64 = null,
    y_skew: ?f64 = null,
};

pub const DebugOptions = struct {
    tile_borders: bool = false,
    parse_status: bool = false,
    timestamps: bool = false,
    collision: bool = false,
    overdraw: bool = false,
    stencil_clip: bool = false,
    depth_buffer: bool = false,
};

pub const NorthOrientation = enum {
    up,
    right,
    down,
    left,
};

pub const ConstrainMode = enum {
    none,
    height_only,
    width_and_height,
    screen,
};

pub const ViewportMode = enum {
    default,
    flipped_y,
};

pub const TileLodMode = enum {
    default,
    distance,
};

pub const ViewportOptions = struct {
    north_orientation: ?NorthOrientation = null,
    constrain_mode: ?ConstrainMode = null,
    viewport_mode: ?ViewportMode = null,
    frustum_offset: ?EdgeInsets = null,
};

pub const TileOptions = struct {
    prefetch_zoom_delta: ?u32 = null,
    lod_min_radius: ?f64 = null,
    lod_scale: ?f64 = null,
    lod_pitch_threshold: ?f64 = null,
    lod_zoom_shift: ?f64 = null,
    lod_mode: ?TileLodMode = null,
};

pub const StyleTileScheme = union(enum) {
    xyz,
    tms,
    unknown: u32,

    pub fn fromRaw(raw: u32) StyleTileScheme {
        return switch (raw) {
            c.MLN_STYLE_TILE_SCHEME_XYZ => .xyz,
            c.MLN_STYLE_TILE_SCHEME_TMS => .tms,
            else => .{ .unknown = raw },
        };
    }

    pub fn toRaw(self: StyleTileScheme) u32 {
        return switch (self) {
            .xyz => c.MLN_STYLE_TILE_SCHEME_XYZ,
            .tms => c.MLN_STYLE_TILE_SCHEME_TMS,
            .unknown => |raw| raw,
        };
    }
};

pub const StyleVectorTileEncoding = union(enum) {
    mvt,
    mlt,
    unknown: u32,

    pub fn fromRaw(raw: u32) StyleVectorTileEncoding {
        return switch (raw) {
            c.MLN_STYLE_VECTOR_TILE_ENCODING_MVT => .mvt,
            c.MLN_STYLE_VECTOR_TILE_ENCODING_MLT => .mlt,
            else => .{ .unknown = raw },
        };
    }

    pub fn toRaw(self: StyleVectorTileEncoding) u32 {
        return switch (self) {
            .mvt => c.MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
            .mlt => c.MLN_STYLE_VECTOR_TILE_ENCODING_MLT,
            .unknown => |raw| raw,
        };
    }
};

pub const StyleRasterDemEncoding = union(enum) {
    mapbox,
    terrarium,
    unknown: u32,

    pub fn fromRaw(raw: u32) StyleRasterDemEncoding {
        return switch (raw) {
            c.MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX => .mapbox,
            c.MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM => .terrarium,
            else => .{ .unknown = raw },
        };
    }

    pub fn toRaw(self: StyleRasterDemEncoding) u32 {
        return switch (self) {
            .mapbox => c.MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX,
            .terrarium => c.MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM,
            .unknown => |raw| raw,
        };
    }
};

/// Whether a style layer draws. This is an open domain, so a `switch` over it
/// needs an `else` branch.
pub const StyleLayerVisibility = union(enum) {
    visible,
    none,
    unknown: u32,

    pub fn fromRaw(raw: u32) StyleLayerVisibility {
        return switch (raw) {
            c.MLN_STYLE_LAYER_VISIBILITY_VISIBLE => .visible,
            c.MLN_STYLE_LAYER_VISIBILITY_NONE => .none,
            else => .{ .unknown = raw },
        };
    }

    pub fn toRaw(self: StyleLayerVisibility) u32 {
        return switch (self) {
            .visible => c.MLN_STYLE_LAYER_VISIBILITY_VISIBLE,
            .none => c.MLN_STYLE_LAYER_VISIBILITY_NONE,
            .unknown => |raw| raw,
        };
    }
};

pub const StyleTileSourceOptions = struct {
    min_zoom: ?f64 = null,
    max_zoom: ?f64 = null,
    attribution: ?[]const u8 = null,
    scheme: ?StyleTileScheme = null,
    bounds: ?LatLngBounds = null,
    tile_size: ?u32 = null,
    vector_encoding: ?StyleVectorTileEncoding = null,
    raster_encoding: ?StyleRasterDemEncoding = null,
};

/// Options for GeoJSON sources, fixed when the source is created.
pub const StyleGeoJsonSourceOptions = struct {
    min_zoom: ?f64 = null,
    max_zoom: ?f64 = null,
    tolerance: ?f64 = null,
    cluster_max_zoom: ?f64 = null,
    /// Cluster aggregation expressions as a JSON object in the MapLibre Style
    /// Spec `clusterProperties` form.
    cluster_properties: ?[]const u8 = null,
    tile_size: ?u32 = null,
    buffer: ?u32 = null,
    cluster_radius: ?u32 = null,
    cluster_min_points: ?u32 = null,
    line_metrics: ?bool = null,
    cluster: ?bool = null,
    /// Slices requested tiles inline during the update pass, so data installed
    /// through `setGeoJsonSourceData` reaches the next rendered frame.
    /// `MapHandle.setGeoJsonSourceSynchronousTiling` overrides this at runtime.
    synchronous_tiling: ?bool = null,

    /// Copies this descriptor and owns its cluster-property bytes.
    pub fn copy(self: StyleGeoJsonSourceOptions, allocator: std.mem.Allocator) std.mem.Allocator.Error!OwnedStyleGeoJsonSourceOptions {
        var copied = self;
        copied.cluster_properties = if (self.cluster_properties) |value| try allocator.dupe(u8, value) else null;
        return .{ .allocator = allocator, .options = copied };
    }
};

pub const OwnedStyleGeoJsonSourceOptions = struct {
    allocator: std.mem.Allocator,
    options: StyleGeoJsonSourceOptions,

    pub fn deinit(self: *OwnedStyleGeoJsonSourceOptions) void {
        if (self.options.cluster_properties) |value| self.allocator.free(value);
        self.options.cluster_properties = null;
    }
};

pub const PremultipliedRgba8Image = struct {
    width: u32,
    height: u32,
    stride: u32,
    pixels: []const u8,
};

/// One stretchable interval along an image axis, in image pixels.
pub const ImageStretch = struct {
    from: f32,
    to: f32,
};

/// Content-box insets in image pixels, measured from the image's top-left.
pub const ImageContent = struct {
    left: f32,
    top: f32,
    right: f32,
    bottom: f32,
};

/// How a stretchable image fits text along one axis. This is an open domain, so
/// a `switch` over it needs an `else` branch.
pub const StyleImageTextFit = union(enum) {
    stretch_or_shrink,
    stretch_only,
    proportional,
    unknown: u32,

    pub fn fromRaw(raw: u32) StyleImageTextFit {
        return switch (raw) {
            c.MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK => .stretch_or_shrink,
            c.MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY => .stretch_only,
            c.MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL => .proportional,
            else => .{ .unknown = raw },
        };
    }

    pub fn toRaw(self: StyleImageTextFit) u32 {
        return switch (self) {
            .stretch_or_shrink => c.MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
            .stretch_only => c.MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY,
            .proportional => c.MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL,
            .unknown => |raw| raw,
        };
    }
};

pub const StyleImageOptions = struct {
    pixel_ratio: ?f32 = null,
    sdf: ?bool = null,
    /// Stretchable intervals along each axis, borrowed for the call. A present
    /// empty slice stays distinguishable from an absent one.
    stretch_x: ?[]const ImageStretch = null,
    stretch_y: ?[]const ImageStretch = null,
    /// Content box used when icon-text-fit applies.
    content: ?ImageContent = null,
    text_fit_width: ?StyleImageTextFit = null,
    text_fit_height: ?StyleImageTextFit = null,
};

/// The style's global transition options, covering paint property changes and
/// symbol placement cross-fade. These are distinct from camera animation
/// options and from the per-property transitions a style declares.
pub const StyleTransitionOptions = struct {
    /// Transition duration in milliseconds. Absent falls back to the duration
    /// the style declares for each transitioning property.
    duration_ms: ?f64 = null,
    /// Transition delay in milliseconds. Absent falls back to the delay the
    /// style declares for each transitioning property.
    delay_ms: ?f64 = null,
    /// Whether symbol placement changes cross-fade. Absent leaves the
    /// cross-fade on; clearing it applies placement changes to the next
    /// rendered frame. Reads always report a value.
    enable_placement_transitions: ?bool = null,
};

pub const StyleImageInfo = struct {
    width: u32,
    height: u32,
    stride: u32,
    byte_length: usize,
    pixel_ratio: f32,
    sdf: bool,
    /// Interval counts for the stretchable axes. Read the intervals themselves
    /// with `copyStyleImageStretches`.
    stretch_x_count: usize = 0,
    stretch_y_count: usize = 0,
    /// Content box, absent when the image carries none.
    content: ?ImageContent = null,
    text_fit_width: ?StyleImageTextFit = null,
    text_fit_height: ?StyleImageTextFit = null,
};

/// Owned stretchable intervals copied out of a runtime style image.
pub const OwnedImageStretches = struct {
    allocator: std.mem.Allocator,
    stretch_x: []ImageStretch,
    stretch_y: []ImageStretch,

    pub fn deinit(self: *OwnedImageStretches) void {
        self.allocator.free(self.stretch_x);
        self.allocator.free(self.stretch_y);
        self.stretch_x = &.{};
        self.stretch_y = &.{};
    }
};

pub const OwnedStyleImage = struct {
    allocator: std.mem.Allocator,
    info: StyleImageInfo,
    pixels: []u8,

    pub fn deinit(self: *OwnedStyleImage) void {
        self.allocator.free(self.pixels);
        self.pixels = &.{};
        self.info = .{ .width = 0, .height = 0, .stride = 0, .byte_length = 0, .pixel_ratio = 1.0, .sdf = false };
    }
};

pub const LocationIndicatorImageKind = enum {
    top,
    bearing,
    shadow,
};

pub const StringList = struct {
    allocator: std.mem.Allocator,
    items: []const []const u8,

    pub fn deinit(self: *StringList) void {
        for (self.items) |item| self.allocator.free(item);
        self.allocator.free(self.items);
        self.items = &.{};
    }
};

pub const StyleSourceType = union(enum) {
    unknown,
    vector,
    raster,
    raster_dem,
    geojson,
    image,
    video,
    annotations,
    custom_vector,
    custom_mvt_vector,
    raw: u32,
};

pub const StyleTileJsonInfo = struct {
    tile_urls: []const []const u8,
    min_zoom: f64,
    max_zoom: f64,
    scheme: StyleTileScheme,
    bounds: ?LatLngBounds,
};

pub const StyleSourceInfo = struct {
    allocator: std.mem.Allocator,
    source_type: StyleSourceType,
    id_size: usize,
    is_volatile: bool,
    attribution: ?[]const u8,
    url: ?[]const u8,
    tile_json: ?StyleTileJsonInfo,
    tile_size: ?u32,
    vector_encoding: ?StyleVectorTileEncoding,
    raster_encoding: ?StyleRasterDemEncoding,

    pub fn deinit(self: *StyleSourceInfo) void {
        if (self.attribution) |attribution| self.allocator.free(attribution);
        if (self.url) |url| self.allocator.free(url);
        if (self.tile_json) |tile_json| {
            for (tile_json.tile_urls) |tile_url| self.allocator.free(tile_url);
            self.allocator.free(tile_json.tile_urls);
        }
        self.attribution = null;
        self.url = null;
        self.tile_json = null;
    }
};

pub const OwnedString = struct {
    allocator: std.mem.Allocator,
    value: []const u8,

    pub fn deinit(self: *OwnedString) void {
        self.allocator.free(self.value);
        self.value = "";
    }
};

fn copyStringView(allocator: std.mem.Allocator, view: c.mln_buffer_view) std.mem.Allocator.Error![]const u8 {
    if (view.size == 0) return allocator.dupe(u8, "");
    return allocator.dupe(u8, view.data[0..view.size]);
}

pub fn styleSourceTypeFromNative(raw: u32) StyleSourceType {
    return switch (raw) {
        c.MLN_STYLE_SOURCE_TYPE_UNKNOWN => .unknown,
        c.MLN_STYLE_SOURCE_TYPE_VECTOR => .vector,
        c.MLN_STYLE_SOURCE_TYPE_RASTER => .raster,
        c.MLN_STYLE_SOURCE_TYPE_RASTER_DEM => .raster_dem,
        c.MLN_STYLE_SOURCE_TYPE_GEOJSON => .geojson,
        c.MLN_STYLE_SOURCE_TYPE_IMAGE => .image,
        c.MLN_STYLE_SOURCE_TYPE_VIDEO => .video,
        c.MLN_STYLE_SOURCE_TYPE_ANNOTATIONS => .annotations,
        c.MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR => .custom_vector,
        c.MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR => .custom_mvt_vector,
        else => .{ .raw = raw },
    };
}

pub fn latLngToNative(value: LatLng) c.mln_lat_lng {
    return .{ .latitude = value.latitude, .longitude = value.longitude };
}

pub fn latLngFromNative(value: c.mln_lat_lng) LatLng {
    return .{ .latitude = value.latitude, .longitude = value.longitude };
}

pub fn screenPointToNative(value: ScreenPoint) c.mln_screen_point {
    return .{ .x = value.x, .y = value.y };
}

pub fn screenPointFromNative(value: c.mln_screen_point) ScreenPoint {
    return .{ .x = value.x, .y = value.y };
}

pub fn edgeInsetsToNative(value: EdgeInsets) c.mln_edge_insets {
    return .{ .top = value.top, .left = value.left, .bottom = value.bottom, .right = value.right };
}

pub fn latLngBoundsToNative(value: LatLngBounds) c.mln_lat_lng_bounds {
    return .{ .southwest = latLngToNative(value.southwest), .northeast = latLngToNative(value.northeast) };
}

pub fn latLngBoundsFromNative(value: c.mln_lat_lng_bounds) LatLngBounds {
    return .{ .southwest = latLngFromNative(value.southwest), .northeast = latLngFromNative(value.northeast) };
}

pub fn projectedMetersToNative(value: ProjectedMeters) c.mln_projected_meters {
    return .{ .northing = value.northing, .easting = value.easting };
}

pub fn projectedMetersFromNative(value: c.mln_projected_meters) ProjectedMeters {
    return .{ .northing = value.northing, .easting = value.easting };
}

pub fn cameraOptionsToNative(value: CameraOptions) c.mln_camera_options {
    var raw = c.mln_camera_options_default();
    if (value.center) |center| {
        raw.fields |= c.MLN_CAMERA_OPTION_CENTER;
        raw.latitude = center.latitude;
        raw.longitude = center.longitude;
    }
    if (value.center_altitude) |center_altitude| {
        raw.fields |= c.MLN_CAMERA_OPTION_CENTER_ALTITUDE;
        raw.center_altitude = center_altitude;
    }
    if (value.padding) |padding| {
        raw.fields |= c.MLN_CAMERA_OPTION_PADDING;
        raw.padding = edgeInsetsToNative(padding);
    }
    if (value.anchor) |anchor| {
        raw.fields |= c.MLN_CAMERA_OPTION_ANCHOR;
        raw.anchor = screenPointToNative(anchor);
    }
    if (value.zoom) |zoom| {
        raw.fields |= c.MLN_CAMERA_OPTION_ZOOM;
        raw.zoom = zoom;
    }
    if (value.bearing) |bearing| {
        raw.fields |= c.MLN_CAMERA_OPTION_BEARING;
        raw.bearing = bearing;
    }
    if (value.pitch) |pitch| {
        raw.fields |= c.MLN_CAMERA_OPTION_PITCH;
        raw.pitch = pitch;
    }
    if (value.roll) |roll| {
        raw.fields |= c.MLN_CAMERA_OPTION_ROLL;
        raw.roll = roll;
    }
    if (value.field_of_view) |field_of_view| {
        raw.fields |= c.MLN_CAMERA_OPTION_FOV;
        raw.field_of_view = field_of_view;
    }
    return raw;
}

pub fn cameraOptionsFromNative(raw: c.mln_camera_options) CameraOptions {
    return .{
        .center = if ((raw.fields & c.MLN_CAMERA_OPTION_CENTER) != 0) .{ .latitude = raw.latitude, .longitude = raw.longitude } else null,
        .center_altitude = if ((raw.fields & c.MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0) raw.center_altitude else null,
        .padding = if ((raw.fields & c.MLN_CAMERA_OPTION_PADDING) != 0) .{ .top = raw.padding.top, .left = raw.padding.left, .bottom = raw.padding.bottom, .right = raw.padding.right } else null,
        .anchor = if ((raw.fields & c.MLN_CAMERA_OPTION_ANCHOR) != 0) screenPointFromNative(raw.anchor) else null,
        .zoom = if ((raw.fields & c.MLN_CAMERA_OPTION_ZOOM) != 0) raw.zoom else null,
        .bearing = if ((raw.fields & c.MLN_CAMERA_OPTION_BEARING) != 0) raw.bearing else null,
        .pitch = if ((raw.fields & c.MLN_CAMERA_OPTION_PITCH) != 0) raw.pitch else null,
        .roll = if ((raw.fields & c.MLN_CAMERA_OPTION_ROLL) != 0) raw.roll else null,
        .field_of_view = if ((raw.fields & c.MLN_CAMERA_OPTION_FOV) != 0) raw.field_of_view else null,
    };
}

pub fn animationOptionsToNative(value: AnimationOptions) c.mln_animation_options {
    var raw = c.mln_animation_options_default();
    if (value.duration_ms) |duration_ms| {
        raw.fields |= c.MLN_ANIMATION_OPTION_DURATION;
        raw.duration_ms = duration_ms;
    }
    if (value.velocity) |velocity| {
        raw.fields |= c.MLN_ANIMATION_OPTION_VELOCITY;
        raw.velocity = velocity;
    }
    if (value.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_ANIMATION_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (value.easing) |easing| {
        raw.fields |= c.MLN_ANIMATION_OPTION_EASING;
        raw.easing = .{ .x1 = easing.x1, .y1 = easing.y1, .x2 = easing.x2, .y2 = easing.y2 };
    }
    if (value.transition_id) |transition_id| {
        raw.fields |= c.MLN_ANIMATION_OPTION_TRANSITION_ID;
        raw.transition_id = transition_id;
    }
    return raw;
}

pub fn cameraFitOptionsToNative(value: CameraFitOptions) c.mln_camera_fit_options {
    var raw = c.mln_camera_fit_options_default();
    if (value.padding) |padding| {
        raw.fields |= c.MLN_CAMERA_FIT_OPTION_PADDING;
        raw.padding = edgeInsetsToNative(padding);
    }
    if (value.bearing) |bearing| {
        raw.fields |= c.MLN_CAMERA_FIT_OPTION_BEARING;
        raw.bearing = bearing;
    }
    if (value.pitch) |pitch| {
        raw.fields |= c.MLN_CAMERA_FIT_OPTION_PITCH;
        raw.pitch = pitch;
    }
    return raw;
}

pub fn boundOptionsToNative(value: BoundOptions) c.mln_bound_options {
    var raw = c.mln_bound_options_default();
    if (value.bounds) |constraint| switch (constraint) {
        .bounded => |bounds| {
            raw.fields |= c.MLN_BOUND_OPTION_BOUNDS;
            raw.bounds = latLngBoundsToNative(bounds);
        },
        .unbounded => raw.fields |= c.MLN_BOUND_OPTION_UNBOUNDED,
    };
    if (value.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_BOUND_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (value.max_zoom) |max_zoom| {
        raw.fields |= c.MLN_BOUND_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    if (value.min_pitch) |min_pitch| {
        raw.fields |= c.MLN_BOUND_OPTION_MIN_PITCH;
        raw.min_pitch = min_pitch;
    }
    if (value.max_pitch) |max_pitch| {
        raw.fields |= c.MLN_BOUND_OPTION_MAX_PITCH;
        raw.max_pitch = max_pitch;
    }
    return raw;
}

pub fn boundOptionsFromNative(raw: c.mln_bound_options) BoundOptions {
    return .{
        .bounds = if ((raw.fields & c.MLN_BOUND_OPTION_BOUNDS) != 0)
            .{ .bounded = latLngBoundsFromNative(raw.bounds) }
        else if ((raw.fields & c.MLN_BOUND_OPTION_UNBOUNDED) != 0)
            .unbounded
        else
            null,
        .min_zoom = if ((raw.fields & c.MLN_BOUND_OPTION_MIN_ZOOM) != 0) raw.min_zoom else null,
        .max_zoom = if ((raw.fields & c.MLN_BOUND_OPTION_MAX_ZOOM) != 0) raw.max_zoom else null,
        .min_pitch = if ((raw.fields & c.MLN_BOUND_OPTION_MIN_PITCH) != 0) raw.min_pitch else null,
        .max_pitch = if ((raw.fields & c.MLN_BOUND_OPTION_MAX_PITCH) != 0) raw.max_pitch else null,
    };
}

pub fn freeCameraOptionsToNative(value: FreeCameraOptions) c.mln_free_camera_options {
    var raw = c.mln_free_camera_options_default();
    if (value.position) |position| {
        raw.fields |= c.MLN_FREE_CAMERA_OPTION_POSITION;
        raw.position = .{ .x = position.x, .y = position.y, .z = position.z };
    }
    if (value.orientation) |orientation| {
        raw.fields |= c.MLN_FREE_CAMERA_OPTION_ORIENTATION;
        raw.orientation = .{ .x = orientation.x, .y = orientation.y, .z = orientation.z, .w = orientation.w };
    }
    return raw;
}

pub fn freeCameraOptionsFromNative(raw: c.mln_free_camera_options) FreeCameraOptions {
    return .{
        .position = if ((raw.fields & c.MLN_FREE_CAMERA_OPTION_POSITION) != 0) .{ .x = raw.position.x, .y = raw.position.y, .z = raw.position.z } else null,
        .orientation = if ((raw.fields & c.MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0) .{ .x = raw.orientation.x, .y = raw.orientation.y, .z = raw.orientation.z, .w = raw.orientation.w } else null,
    };
}

pub fn projectionModeToNative(value: ProjectionMode) c.mln_projection_mode {
    var raw = c.mln_projection_mode_default();
    if (value.axonometric) |axonometric| {
        raw.fields |= c.MLN_PROJECTION_MODE_AXONOMETRIC;
        raw.axonometric = axonometric;
    }
    if (value.x_skew) |x_skew| {
        raw.fields |= c.MLN_PROJECTION_MODE_X_SKEW;
        raw.x_skew = x_skew;
    }
    if (value.y_skew) |y_skew| {
        raw.fields |= c.MLN_PROJECTION_MODE_Y_SKEW;
        raw.y_skew = y_skew;
    }
    return raw;
}

pub fn projectionModeFromNative(raw: c.mln_projection_mode) ProjectionMode {
    return .{
        .axonometric = if ((raw.fields & c.MLN_PROJECTION_MODE_AXONOMETRIC) != 0) raw.axonometric else null,
        .x_skew = if ((raw.fields & c.MLN_PROJECTION_MODE_X_SKEW) != 0) raw.x_skew else null,
        .y_skew = if ((raw.fields & c.MLN_PROJECTION_MODE_Y_SKEW) != 0) raw.y_skew else null,
    };
}

pub fn debugOptionsToNative(value: DebugOptions) u32 {
    var raw: u32 = 0;
    if (value.tile_borders) raw |= c.MLN_MAP_DEBUG_TILE_BORDERS;
    if (value.parse_status) raw |= c.MLN_MAP_DEBUG_PARSE_STATUS;
    if (value.timestamps) raw |= c.MLN_MAP_DEBUG_TIMESTAMPS;
    if (value.collision) raw |= c.MLN_MAP_DEBUG_COLLISION;
    if (value.overdraw) raw |= c.MLN_MAP_DEBUG_OVERDRAW;
    if (value.stencil_clip) raw |= c.MLN_MAP_DEBUG_STENCIL_CLIP;
    if (value.depth_buffer) raw |= c.MLN_MAP_DEBUG_DEPTH_BUFFER;
    return raw;
}

pub fn debugOptionsFromNative(raw: u32) DebugOptions {
    return .{
        .tile_borders = (raw & c.MLN_MAP_DEBUG_TILE_BORDERS) != 0,
        .parse_status = (raw & c.MLN_MAP_DEBUG_PARSE_STATUS) != 0,
        .timestamps = (raw & c.MLN_MAP_DEBUG_TIMESTAMPS) != 0,
        .collision = (raw & c.MLN_MAP_DEBUG_COLLISION) != 0,
        .overdraw = (raw & c.MLN_MAP_DEBUG_OVERDRAW) != 0,
        .stencil_clip = (raw & c.MLN_MAP_DEBUG_STENCIL_CLIP) != 0,
        .depth_buffer = (raw & c.MLN_MAP_DEBUG_DEPTH_BUFFER) != 0,
    };
}

pub fn northOrientationToNative(value: NorthOrientation) u32 {
    return switch (value) {
        .up => c.MLN_NORTH_ORIENTATION_UP,
        .right => c.MLN_NORTH_ORIENTATION_RIGHT,
        .down => c.MLN_NORTH_ORIENTATION_DOWN,
        .left => c.MLN_NORTH_ORIENTATION_LEFT,
    };
}

pub fn northOrientationFromNative(raw: u32) error{UnknownStatus}!NorthOrientation {
    return switch (raw) {
        c.MLN_NORTH_ORIENTATION_UP => .up,
        c.MLN_NORTH_ORIENTATION_RIGHT => .right,
        c.MLN_NORTH_ORIENTATION_DOWN => .down,
        c.MLN_NORTH_ORIENTATION_LEFT => .left,
        else => error.UnknownStatus,
    };
}

pub fn constrainModeToNative(value: ConstrainMode) u32 {
    return switch (value) {
        .none => c.MLN_CONSTRAIN_MODE_NONE,
        .height_only => c.MLN_CONSTRAIN_MODE_HEIGHT_ONLY,
        .width_and_height => c.MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT,
        .screen => c.MLN_CONSTRAIN_MODE_SCREEN,
    };
}

pub fn constrainModeFromNative(raw: u32) error{UnknownStatus}!ConstrainMode {
    return switch (raw) {
        c.MLN_CONSTRAIN_MODE_NONE => .none,
        c.MLN_CONSTRAIN_MODE_HEIGHT_ONLY => .height_only,
        c.MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT => .width_and_height,
        c.MLN_CONSTRAIN_MODE_SCREEN => .screen,
        else => error.UnknownStatus,
    };
}

pub fn viewportModeToNative(value: ViewportMode) u32 {
    return switch (value) {
        .default => c.MLN_VIEWPORT_MODE_DEFAULT,
        .flipped_y => c.MLN_VIEWPORT_MODE_FLIPPED_Y,
    };
}

pub fn viewportModeFromNative(raw: u32) error{UnknownStatus}!ViewportMode {
    return switch (raw) {
        c.MLN_VIEWPORT_MODE_DEFAULT => .default,
        c.MLN_VIEWPORT_MODE_FLIPPED_Y => .flipped_y,
        else => error.UnknownStatus,
    };
}

pub fn tileLodModeToNative(value: TileLodMode) u32 {
    return switch (value) {
        .default => c.MLN_TILE_LOD_MODE_DEFAULT,
        .distance => c.MLN_TILE_LOD_MODE_DISTANCE,
    };
}

pub fn tileLodModeFromNative(raw: u32) error{UnknownStatus}!TileLodMode {
    return switch (raw) {
        c.MLN_TILE_LOD_MODE_DEFAULT => .default,
        c.MLN_TILE_LOD_MODE_DISTANCE => .distance,
        else => error.UnknownStatus,
    };
}

pub fn viewportOptionsToNative(value: ViewportOptions) c.mln_map_viewport_options {
    var raw = c.mln_map_viewport_options_default();
    if (value.north_orientation) |north_orientation| {
        raw.fields |= c.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
        raw.north_orientation = northOrientationToNative(north_orientation);
    }
    if (value.constrain_mode) |constrain_mode| {
        raw.fields |= c.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
        raw.constrain_mode = constrainModeToNative(constrain_mode);
    }
    if (value.viewport_mode) |viewport_mode| {
        raw.fields |= c.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
        raw.viewport_mode = viewportModeToNative(viewport_mode);
    }
    if (value.frustum_offset) |frustum_offset| {
        raw.fields |= c.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
        raw.frustum_offset = edgeInsetsToNative(frustum_offset);
    }
    return raw;
}

pub fn viewportOptionsFromNative(raw: c.mln_map_viewport_options) error{UnknownStatus}!ViewportOptions {
    return .{
        .north_orientation = if ((raw.fields & c.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0) try northOrientationFromNative(raw.north_orientation) else null,
        .constrain_mode = if ((raw.fields & c.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0) try constrainModeFromNative(raw.constrain_mode) else null,
        .viewport_mode = if ((raw.fields & c.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0) try viewportModeFromNative(raw.viewport_mode) else null,
        .frustum_offset = if ((raw.fields & c.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0) .{ .top = raw.frustum_offset.top, .left = raw.frustum_offset.left, .bottom = raw.frustum_offset.bottom, .right = raw.frustum_offset.right } else null,
    };
}

pub fn tileOptionsToNative(value: TileOptions) c.mln_map_tile_options {
    var raw = c.mln_map_tile_options_default();
    if (value.prefetch_zoom_delta) |prefetch_zoom_delta| {
        raw.fields |= c.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
        raw.prefetch_zoom_delta = prefetch_zoom_delta;
    }
    if (value.lod_min_radius) |lod_min_radius| {
        raw.fields |= c.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
        raw.lod_min_radius = lod_min_radius;
    }
    if (value.lod_scale) |lod_scale| {
        raw.fields |= c.MLN_MAP_TILE_OPTION_LOD_SCALE;
        raw.lod_scale = lod_scale;
    }
    if (value.lod_pitch_threshold) |lod_pitch_threshold| {
        raw.fields |= c.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
        raw.lod_pitch_threshold = lod_pitch_threshold;
    }
    if (value.lod_zoom_shift) |lod_zoom_shift| {
        raw.fields |= c.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
        raw.lod_zoom_shift = lod_zoom_shift;
    }
    if (value.lod_mode) |lod_mode| {
        raw.fields |= c.MLN_MAP_TILE_OPTION_LOD_MODE;
        raw.lod_mode = tileLodModeToNative(lod_mode);
    }
    return raw;
}

pub fn tileOptionsFromNative(raw: c.mln_map_tile_options) error{UnknownStatus}!TileOptions {
    return .{
        .prefetch_zoom_delta = if ((raw.fields & c.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0) raw.prefetch_zoom_delta else null,
        .lod_min_radius = if ((raw.fields & c.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0) raw.lod_min_radius else null,
        .lod_scale = if ((raw.fields & c.MLN_MAP_TILE_OPTION_LOD_SCALE) != 0) raw.lod_scale else null,
        .lod_pitch_threshold = if ((raw.fields & c.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0) raw.lod_pitch_threshold else null,
        .lod_zoom_shift = if ((raw.fields & c.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0) raw.lod_zoom_shift else null,
        .lod_mode = if ((raw.fields & c.MLN_MAP_TILE_OPTION_LOD_MODE) != 0) try tileLodModeFromNative(raw.lod_mode) else null,
    };
}

pub fn styleTileSchemeToNative(value: StyleTileScheme) u32 {
    return value.toRaw();
}

pub fn styleVectorTileEncodingToNative(value: StyleVectorTileEncoding) u32 {
    return value.toRaw();
}

pub fn styleRasterDemEncodingToNative(value: StyleRasterDemEncoding) u32 {
    return value.toRaw();
}

pub fn premultipliedRgba8ImageToNative(value: PremultipliedRgba8Image) c.mln_premultiplied_rgba8_image {
    var raw = c.mln_premultiplied_rgba8_image_default();
    raw.width = value.width;
    raw.height = value.height;
    raw.stride = value.stride;
    raw.pixels = if (value.pixels.len == 0) null else value.pixels.ptr;
    raw.byte_length = value.pixels.len;
    return raw;
}

/// Materializes native image options. The stretch slices are borrowed, so
/// `value` and the scratch buffers must outlive the native call.
pub fn styleImageOptionsToNative(
    value: StyleImageOptions,
    stretch_x: []c.mln_image_stretch,
    stretch_y: []c.mln_image_stretch,
) c.mln_style_image_options {
    var raw = c.mln_style_image_options_default();
    if (value.pixel_ratio) |pixel_ratio| {
        raw.fields |= c.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
        raw.pixel_ratio = pixel_ratio;
    }
    if (value.sdf) |sdf| {
        raw.fields |= c.MLN_STYLE_IMAGE_OPTION_SDF;
        raw.sdf = sdf;
    }
    if (value.stretch_x) |stretches| {
        raw.fields |= c.MLN_STYLE_IMAGE_OPTION_STRETCH_X;
        for (stretches, 0..) |stretch, index| {
            stretch_x[index] = .{ .from = stretch.from, .to = stretch.to };
        }
        raw.stretch_x = if (stretches.len == 0) null else stretch_x.ptr;
        raw.stretch_x_count = stretches.len;
    }
    if (value.stretch_y) |stretches| {
        raw.fields |= c.MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
        for (stretches, 0..) |stretch, index| {
            stretch_y[index] = .{ .from = stretch.from, .to = stretch.to };
        }
        raw.stretch_y = if (stretches.len == 0) null else stretch_y.ptr;
        raw.stretch_y_count = stretches.len;
    }
    if (value.content) |content| {
        raw.fields |= c.MLN_STYLE_IMAGE_OPTION_CONTENT;
        raw.content = .{
            .left = content.left,
            .top = content.top,
            .right = content.right,
            .bottom = content.bottom,
        };
    }
    if (value.text_fit_width) |fit| {
        raw.fields |= c.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
        raw.text_fit_width = fit.toRaw();
    }
    if (value.text_fit_height) |fit| {
        raw.fields |= c.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
        raw.text_fit_height = fit.toRaw();
    }
    return raw;
}

pub fn styleTransitionOptionsToNative(value: StyleTransitionOptions) c.mln_style_transition_options {
    var raw = c.mln_style_transition_options_default();
    if (value.enable_placement_transitions) |enable| {
        raw.fields |= c.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
        raw.enable_placement_transitions = enable;
    }
    if (value.duration_ms) |duration_ms| {
        raw.fields |= c.MLN_STYLE_TRANSITION_OPTION_DURATION;
        raw.duration_ms = duration_ms;
    }
    if (value.delay_ms) |delay_ms| {
        raw.fields |= c.MLN_STYLE_TRANSITION_OPTION_DELAY;
        raw.delay_ms = delay_ms;
    }
    return raw;
}

pub fn styleTransitionOptionsFromNative(raw: c.mln_style_transition_options) StyleTransitionOptions {
    return .{
        .duration_ms = if (raw.fields & c.MLN_STYLE_TRANSITION_OPTION_DURATION != 0) raw.duration_ms else null,
        .delay_ms = if (raw.fields & c.MLN_STYLE_TRANSITION_OPTION_DELAY != 0) raw.delay_ms else null,
        .enable_placement_transitions = if (raw.fields & c.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0) raw.enable_placement_transitions else null,
    };
}

pub fn styleImageInfoFromNative(raw: c.mln_style_image_info) StyleImageInfo {
    return .{
        .width = raw.width,
        .height = raw.height,
        .stride = raw.stride,
        .byte_length = raw.byte_length,
        .pixel_ratio = raw.pixel_ratio,
        .sdf = raw.sdf,
        .stretch_x_count = raw.stretch_x_count,
        .stretch_y_count = raw.stretch_y_count,
        .content = if (raw.has_content) .{
            .left = raw.content.left,
            .top = raw.content.top,
            .right = raw.content.right,
            .bottom = raw.content.bottom,
        } else null,
        .text_fit_width = if (raw.has_text_fit_width)
            StyleImageTextFit.fromRaw(raw.text_fit_width)
        else
            null,
        .text_fit_height = if (raw.has_text_fit_height)
            StyleImageTextFit.fromRaw(raw.text_fit_height)
        else
            null,
    };
}

pub fn locationIndicatorImageKindToNative(value: LocationIndicatorImageKind) u32 {
    return switch (value) {
        .top => c.MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP,
        .bearing => c.MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING,
        .shadow => c.MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW,
    };
}

test "growable style source type preserves unknown raw values" {
    try std.testing.expect(std.meta.eql(styleSourceTypeFromNative(0xbeef), StyleSourceType{ .raw = 0xbeef }));
}

test "style source metadata enums preserve unknown raw values" {
    const scheme = StyleTileScheme.fromRaw(81);
    try std.testing.expectEqual(@as(u32, 81), scheme.toRaw());
    const vector_encoding = StyleVectorTileEncoding.fromRaw(82);
    try std.testing.expectEqual(@as(u32, 82), vector_encoding.toRaw());
    const raster_encoding = StyleRasterDemEncoding.fromRaw(83);
    try std.testing.expectEqual(@as(u32, 83), raster_encoding.toRaw());
}

test "GeoJSON source option copy owns nested cluster properties" {
    var cluster_properties = "{\"total\":[\"+\",1]}".*;
    const original = StyleGeoJsonSourceOptions{
        .max_zoom = 18,
        .cluster_properties = cluster_properties[0..],
    };

    var copied = try original.copy(std.testing.allocator);
    defer copied.deinit();
    copied.options.max_zoom = 12;
    cluster_properties[2] = 'x';

    try std.testing.expectEqual(@as(f64, 18), original.max_zoom.?);
    try std.testing.expectEqual(@as(f64, 12), copied.options.max_zoom.?);
    try std.testing.expectEqualStrings("{\"total\":[\"+\",1]}", copied.options.cluster_properties.?);
}
