const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const RuntimeHandle = runtime_module.RuntimeHandle;
const status = @import("status.zig");
const values = @import("values.zig");

const MapStateHandle = opaque {};
const MapState = struct {
    native: ?*c.mln_map,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

pub const MapMode = enum {
    continuous,
    static,
    tile,

    fn toRaw(self: MapMode) u32 {
        return switch (self) {
            .continuous => c.MLN_MAP_MODE_CONTINUOUS,
            .static => c.MLN_MAP_MODE_STATIC,
            .tile => c.MLN_MAP_MODE_TILE,
        };
    }
};

pub const MapOptions = struct {
    width: u32 = 512,
    height: u32 = 512,
    scale_factor: f64 = 1.0,
    mode: MapMode = .continuous,
};

pub const MapHandle = struct {
    state: *MapStateHandle,

    pub fn create(runtime: RuntimeHandle, options: MapOptions) status.Error!MapHandle {
        var native_options = c.mln_map_options_default();
        native_options.width = options.width;
        native_options.height = options.height;
        native_options.scale_factor = options.scale_factor;
        native_options.map_mode = options.mode.toRaw();

        var map: ?*c.mln_map = null;
        const diagnostic_store = runtime_module.diagnosticStore(runtime);
        try status.checkStatus(
            c.mln_map_create(try runtime_module.native(runtime), &native_options, &map),
            diagnostic_store,
        );
        errdefer {
            if (map) |handle| _ = c.mln_map_destroy(handle);
        }

        const map_state = try std.heap.smp_allocator.create(MapState);
        map_state.* = .{ .native = map.?, .diagnostic_store = diagnostic_store };
        return .{ .state = @ptrCast(map_state) };
    }

    pub fn setStyleJson(
        self: MapHandle,
        allocator: std.mem.Allocator,
        json: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const json_z = try nulTerminated(allocator, json);
        defer allocator.free(json_z);
        try status.checkStatus(c.mln_map_set_style_json(native_map, json_z.ptr), state(self).diagnostic_store);
    }

    pub fn setStyleUrl(
        self: MapHandle,
        allocator: std.mem.Allocator,
        url: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const url_z = try nulTerminated(allocator, url);
        defer allocator.free(url_z);
        try status.checkStatus(c.mln_map_set_style_url(native_map, url_z.ptr), state(self).diagnostic_store);
    }

    pub fn setLayerProperty(
        self: MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        property_name: []const u8,
        value: values.JsonValue,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_layer_property(
                try native(self),
                try temp.stringView(layer_id),
                try temp.stringView(property_name),
                try temp.jsonValue(value),
            ),
            state(self).diagnostic_store,
        );
    }

    pub fn getLayerProperty(
        self: MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        property_name: []const u8,
    ) status.Error!?values.OwnedJsonValue {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var snapshot: ?*c.mln_json_snapshot = null;
        try status.checkStatus(
            c.mln_map_get_layer_property(
                try native(self),
                try temp.stringView(layer_id),
                try temp.stringView(property_name),
                &snapshot,
            ),
            state(self).diagnostic_store,
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, state(self).diagnostic_store);
    }

    pub fn setLayerFilter(
        self: MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        filter: ?values.JsonValue,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const filter_ptr = if (filter) |value| try temp.jsonValue(value) else null;
        try status.checkStatus(
            c.mln_map_set_layer_filter(try native(self), try temp.stringView(layer_id), filter_ptr),
            state(self).diagnostic_store,
        );
    }

    pub fn getLayerFilter(
        self: MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!?values.OwnedJsonValue {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var snapshot: ?*c.mln_json_snapshot = null;
        try status.checkStatus(
            c.mln_map_get_layer_filter(try native(self), try temp.stringView(layer_id), &snapshot),
            state(self).diagnostic_store,
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, state(self).diagnostic_store);
    }

    pub fn listStyleSourceIds(self: MapHandle, allocator: std.mem.Allocator) status.Error!values.StringList {
        var list: ?*c.mln_style_id_list = null;
        try status.checkStatus(c.mln_map_list_style_source_ids(try native(self), &list), state(self).diagnostic_store);
        defer if (list) |handle| c.mln_style_id_list_destroy(handle);
        return try copyStyleIdList(allocator, list.?, state(self).diagnostic_store);
    }

    pub fn listStyleLayerIds(self: MapHandle, allocator: std.mem.Allocator) status.Error!values.StringList {
        var list: ?*c.mln_style_id_list = null;
        try status.checkStatus(c.mln_map_list_style_layer_ids(try native(self), &list), state(self).diagnostic_store);
        defer if (list) |handle| c.mln_style_id_list_destroy(handle);
        return try copyStyleIdList(allocator, list.?, state(self).diagnostic_store);
    }

    pub fn addStyleSourceJson(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        source_json: values.JsonValue,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_style_source_json(try native(self), try temp.stringView(source_id), try temp.jsonValue(source_json)),
            state(self).diagnostic_store,
        );
    }

    pub fn removeStyleSource(self: MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var removed = false;
        try status.checkStatus(
            c.mln_map_remove_style_source(try native(self), try temp.stringView(source_id), &removed),
            state(self).diagnostic_store,
        );
        return removed;
    }

    pub fn styleSourceExists(self: MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var exists = false;
        try status.checkStatus(
            c.mln_map_style_source_exists(try native(self), try temp.stringView(source_id), &exists),
            state(self).diagnostic_store,
        );
        return exists;
    }

    pub fn getStyleSourceType(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
    ) status.Error!?values.StyleSourceType {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_type: u32 = c.MLN_STYLE_SOURCE_TYPE_UNKNOWN;
        var found = false;
        try status.checkStatus(
            c.mln_map_get_style_source_type(try native(self), try temp.stringView(source_id), &raw_type, &found),
            state(self).diagnostic_store,
        );
        if (!found) return null;
        return try values.styleSourceTypeFromNative(raw_type);
    }

    pub fn getStyleSourceInfo(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
    ) status.Error!?values.StyleSourceInfo {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_info: c.mln_style_source_info = .{
            .size = @sizeOf(c.mln_style_source_info),
            .type = c.MLN_STYLE_SOURCE_TYPE_UNKNOWN,
            .id_size = 0,
            .is_volatile = false,
            .has_attribution = false,
            .attribution_size = 0,
        };
        var found = false;
        try status.checkStatus(
            c.mln_map_get_style_source_info(try native(self), try temp.stringView(source_id), &raw_info, &found),
            state(self).diagnostic_store,
        );
        if (!found) return null;
        return try values.styleSourceInfoFromNative(raw_info);
    }

    pub fn copyStyleSourceAttribution(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
    ) status.Error!?values.OwnedString {
        const info = (try self.getStyleSourceInfo(allocator, source_id)) orelse return null;
        if (!info.has_attribution) return null;

        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const buffer = try allocator.alloc(u8, info.attribution_size);
        errdefer allocator.free(buffer);
        var copied_size: usize = 0;
        var found = false;
        try status.checkStatus(
            c.mln_map_copy_style_source_attribution(
                try native(self),
                try temp.stringView(source_id),
                if (buffer.len == 0) null else buffer.ptr,
                buffer.len,
                &copied_size,
                &found,
            ),
            state(self).diagnostic_store,
        );
        if (!found) {
            allocator.free(buffer);
            return null;
        }
        if (copied_size != buffer.len) {
            const exact = try allocator.dupe(u8, buffer[0..copied_size]);
            allocator.free(buffer);
            return .{ .allocator = allocator, .value = exact };
        }
        return .{ .allocator = allocator, .value = buffer };
    }

    pub fn addGeoJsonSourceData(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        data: values.GeoJson,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_geojson_source_data(try native(self), try temp.stringView(source_id), try temp.geoJson(data)),
            state(self).diagnostic_store,
        );
    }

    pub fn setGeoJsonSourceData(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        data: values.GeoJson,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_geojson_source_data(try native(self), try temp.stringView(source_id), try temp.geoJson(data)),
            state(self).diagnostic_store,
        );
    }

    pub fn addGeoJsonSourceUrl(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_geojson_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url)),
            state(self).diagnostic_store,
        );
    }

    pub fn setGeoJsonSourceUrl(
        self: MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_geojson_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url)),
            state(self).diagnostic_store,
        );
    }

    pub fn requestRepaint(self: MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_request_repaint(try native(self)), state(self).diagnostic_store);
    }

    pub fn setDebugOptions(self: MapHandle, options: values.DebugOptions) status.Error!void {
        try status.checkStatus(c.mln_map_set_debug_options(try native(self), values.debugOptionsToNative(options)), state(self).diagnostic_store);
    }

    pub fn getDebugOptions(self: MapHandle) status.Error!values.DebugOptions {
        var options: u32 = 0;
        try status.checkStatus(c.mln_map_get_debug_options(try native(self), &options), state(self).diagnostic_store);
        return values.debugOptionsFromNative(options);
    }

    pub fn setRenderingStatsViewEnabled(self: MapHandle, enabled: bool) status.Error!void {
        try status.checkStatus(c.mln_map_set_rendering_stats_view_enabled(try native(self), enabled), state(self).diagnostic_store);
    }

    pub fn getRenderingStatsViewEnabled(self: MapHandle) status.Error!bool {
        var enabled = false;
        try status.checkStatus(c.mln_map_get_rendering_stats_view_enabled(try native(self), &enabled), state(self).diagnostic_store);
        return enabled;
    }

    pub fn isFullyLoaded(self: MapHandle) status.Error!bool {
        var loaded = false;
        try status.checkStatus(c.mln_map_is_fully_loaded(try native(self), &loaded), state(self).diagnostic_store);
        return loaded;
    }

    pub fn dumpDebugLogs(self: MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_dump_debug_logs(try native(self)), state(self).diagnostic_store);
    }

    pub fn setViewportOptions(self: MapHandle, options: values.ViewportOptions) status.Error!void {
        var raw_options = values.viewportOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_viewport_options(try native(self), &raw_options), state(self).diagnostic_store);
    }

    pub fn getViewportOptions(self: MapHandle) status.Error!values.ViewportOptions {
        var options = c.mln_map_viewport_options_default();
        try status.checkStatus(c.mln_map_get_viewport_options(try native(self), &options), state(self).diagnostic_store);
        return try values.viewportOptionsFromNative(options);
    }

    pub fn setTileOptions(self: MapHandle, options: values.TileOptions) status.Error!void {
        var raw_options = values.tileOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_tile_options(try native(self), &raw_options), state(self).diagnostic_store);
    }

    pub fn getTileOptions(self: MapHandle) status.Error!values.TileOptions {
        var options = c.mln_map_tile_options_default();
        try status.checkStatus(c.mln_map_get_tile_options(try native(self), &options), state(self).diagnostic_store);
        return try values.tileOptionsFromNative(options);
    }

    pub fn getCamera(self: MapHandle) status.Error!values.CameraOptions {
        var camera = c.mln_camera_options_default();
        try status.checkStatus(c.mln_map_get_camera(try native(self), &camera), state(self).diagnostic_store);
        return values.cameraOptionsFromNative(camera);
    }

    pub fn jumpTo(self: MapHandle, camera: values.CameraOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        try status.checkStatus(c.mln_map_jump_to(try native(self), &raw_camera), state(self).diagnostic_store);
    }

    pub fn easeTo(self: MapHandle, camera: values.CameraOptions, animation: ?values.AnimationOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_ease_to(try native(self), &raw_camera, animation_ptr), state(self).diagnostic_store);
    }

    pub fn flyTo(self: MapHandle, camera: values.CameraOptions, animation: ?values.AnimationOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_fly_to(try native(self), &raw_camera, animation_ptr), state(self).diagnostic_store);
    }

    pub fn moveBy(self: MapHandle, delta_x: f64, delta_y: f64) status.Error!void {
        try status.checkStatus(c.mln_map_move_by(try native(self), delta_x, delta_y), state(self).diagnostic_store);
    }

    pub fn scaleBy(self: MapHandle, scale: f64, anchor: ?values.ScreenPoint) status.Error!void {
        var raw_anchor = if (anchor) |point| values.screenPointToNative(point) else undefined;
        const anchor_ptr = if (anchor != null) &raw_anchor else null;
        try status.checkStatus(c.mln_map_scale_by(try native(self), scale, anchor_ptr), state(self).diagnostic_store);
    }

    pub fn rotateBy(self: MapHandle, first: values.ScreenPoint, second: values.ScreenPoint) status.Error!void {
        try status.checkStatus(
            c.mln_map_rotate_by(try native(self), values.screenPointToNative(first), values.screenPointToNative(second)),
            state(self).diagnostic_store,
        );
    }

    pub fn pitchBy(self: MapHandle, pitch: f64) status.Error!void {
        try status.checkStatus(c.mln_map_pitch_by(try native(self), pitch), state(self).diagnostic_store);
    }

    pub fn cancelTransitions(self: MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_cancel_transitions(try native(self)), state(self).diagnostic_store);
    }

    pub fn setProjectionMode(self: MapHandle, mode: values.ProjectionMode) status.Error!void {
        var raw_mode = values.projectionModeToNative(mode);
        try status.checkStatus(c.mln_map_set_projection_mode(try native(self), &raw_mode), state(self).diagnostic_store);
    }

    pub fn getProjectionMode(self: MapHandle) status.Error!values.ProjectionMode {
        var mode = c.mln_projection_mode_default();
        try status.checkStatus(c.mln_map_get_projection_mode(try native(self), &mode), state(self).diagnostic_store);
        return values.projectionModeFromNative(mode);
    }

    pub fn pixelForLatLng(self: MapHandle, coordinate: values.LatLng) status.Error!values.ScreenPoint {
        var point: c.mln_screen_point = undefined;
        try status.checkStatus(
            c.mln_map_pixel_for_lat_lng(try native(self), values.latLngToNative(coordinate), &point),
            state(self).diagnostic_store,
        );
        return values.screenPointFromNative(point);
    }

    pub fn latLngForPixel(self: MapHandle, point: values.ScreenPoint) status.Error!values.LatLng {
        var coordinate: c.mln_lat_lng = undefined;
        try status.checkStatus(
            c.mln_map_lat_lng_for_pixel(try native(self), values.screenPointToNative(point), &coordinate),
            state(self).diagnostic_store,
        );
        return values.latLngFromNative(coordinate);
    }

    pub fn pixelsForLatLngs(
        self: MapHandle,
        allocator: std.mem.Allocator,
        coordinates: []const values.LatLng,
        out_points: []values.ScreenPoint,
    ) status.Error!void {
        if (coordinates.len != out_points.len) return error.InvalidArgument;
        if (coordinates.len == 0) {
            try status.checkStatus(c.mln_map_pixels_for_lat_lngs(try native(self), null, 0, null), state(self).diagnostic_store);
            return;
        }
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        const raw_points = try allocator.alloc(c.mln_screen_point, out_points.len);
        defer allocator.free(raw_points);
        try status.checkStatus(
            c.mln_map_pixels_for_lat_lngs(try native(self), raw_coordinates.ptr, raw_coordinates.len, raw_points.ptr),
            state(self).diagnostic_store,
        );
        for (raw_points, out_points) |raw_point, *out_point| out_point.* = values.screenPointFromNative(raw_point);
    }

    pub fn latLngsForPixels(
        self: MapHandle,
        allocator: std.mem.Allocator,
        points: []const values.ScreenPoint,
        out_coordinates: []values.LatLng,
    ) status.Error!void {
        if (points.len != out_coordinates.len) return error.InvalidArgument;
        if (points.len == 0) {
            try status.checkStatus(c.mln_map_lat_lngs_for_pixels(try native(self), null, 0, null), state(self).diagnostic_store);
            return;
        }
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_points = try temp.screenPoints(points);
        const raw_coordinates = try allocator.alloc(c.mln_lat_lng, out_coordinates.len);
        defer allocator.free(raw_coordinates);
        try status.checkStatus(
            c.mln_map_lat_lngs_for_pixels(try native(self), raw_points.ptr, raw_points.len, raw_coordinates.ptr),
            state(self).diagnostic_store,
        );
        for (raw_coordinates, out_coordinates) |raw_coordinate, *out_coordinate| out_coordinate.* = values.latLngFromNative(raw_coordinate);
    }

    pub fn cameraForLatLngBounds(
        self: MapHandle,
        bounds: values.LatLngBounds,
        fit_options: ?values.CameraFitOptions,
    ) status.Error!values.CameraOptions {
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        var camera = c.mln_camera_options_default();
        try status.checkStatus(
            c.mln_map_camera_for_lat_lng_bounds(try native(self), values.latLngBoundsToNative(bounds), fit_ptr, &camera),
            state(self).diagnostic_store,
        );
        return values.cameraOptionsFromNative(camera);
    }

    pub fn latLngBoundsForCamera(self: MapHandle, camera: values.CameraOptions) status.Error!values.LatLngBounds {
        var raw_camera = values.cameraOptionsToNative(camera);
        var bounds: c.mln_lat_lng_bounds = undefined;
        try status.checkStatus(c.mln_map_lat_lng_bounds_for_camera(try native(self), &raw_camera, &bounds), state(self).diagnostic_store);
        return values.latLngBoundsFromNative(bounds);
    }

    pub fn close(self: MapHandle) status.Error!void {
        const map_state = state(self);
        const map = map_state.native orelse return;
        try status.checkStatus(c.mln_map_destroy(map), map_state.diagnostic_store);
        map_state.native = null;
    }
};

fn state(handle: MapHandle) *MapState {
    return @ptrCast(@alignCast(handle.state));
}

pub fn native(handle: MapHandle) status.BindingError!*c.mln_map {
    return state(handle).native orelse error.ClosedHandle;
}

pub fn diagnosticStore(handle: MapHandle) ?*diagnostics.DiagnosticStore {
    return state(handle).diagnostic_store;
}

fn copyJsonSnapshot(
    allocator: std.mem.Allocator,
    snapshot: ?*c.mln_json_snapshot,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!?values.OwnedJsonValue {
    const handle = snapshot orelse return null;
    var raw: ?*const c.mln_json_value = null;
    try status.checkStatus(c.mln_json_snapshot_get(handle, &raw), diagnostic_store);
    return try values.ownedJsonValueFromNative(allocator, raw.?);
}

fn copyStyleIdList(
    allocator: std.mem.Allocator,
    list: *c.mln_style_id_list,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!values.StringList {
    var count: usize = 0;
    try status.checkStatus(c.mln_style_id_list_count(list, &count), diagnostic_store);
    const items = try allocator.alloc([]const u8, count);
    var initialized: usize = 0;
    errdefer {
        for (items[0..initialized]) |item| allocator.free(item);
        allocator.free(items);
    }
    for (items, 0..) |*item, index| {
        var view = c.mln_string_view{ .data = null, .size = 0 };
        try status.checkStatus(c.mln_style_id_list_get(list, index, &view), diagnostic_store);
        item.* = if (view.size == 0) try allocator.dupe(u8, "") else try allocator.dupe(u8, view.data[0..view.size]);
        initialized += 1;
    }
    return .{ .allocator = allocator, .items = items };
}

fn nulTerminated(allocator: std.mem.Allocator, value: []const u8) status.Error![:0]u8 {
    if (std.mem.indexOfScalar(u8, value, 0) != null) return error.InvalidString;
    return allocator.dupeZ(u8, value);
}
