const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const RuntimeHandle = runtime_module.RuntimeHandle;
const NativeMap = opaque {};
const status = @import("status.zig");
const values = @import("values.zig");

const CustomGeometrySourceState = struct {
    source_id: []const u8,
    fetch_tile: CustomGeometrySourceTileCallback,
    cancel_tile: ?CustomGeometrySourceTileCallback,
    context: ?*anyopaque,
    active_upcalls: std.atomic.Value(usize),
};

const MapState = struct {
    native: ?*NativeMap,
    runtime_registry: *runtime_module.RuntimeRegistry,
    id_value: values.MapId,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    custom_geometry_sources: *std.ArrayList(*CustomGeometrySourceState),
    active_render_sessions: std.atomic.Value(usize),
    closing: bool,
};

pub const RenderSessionRegistration = struct {
    native: *c.mln_map,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

const MapRegistrySlot = struct {
    state: ?*MapState,
    generation: u64,
};

var custom_geometry_state_registry_lock = std.Io.Mutex.init;
var custom_geometry_state_registry: std.ArrayList(*CustomGeometrySourceState) = .empty;

var map_registry_lock = std.atomic.Value(bool).init(false);
var map_registry: std.ArrayList(MapRegistrySlot) = .empty;
var map_free_list: std.ArrayList(usize) = .empty;

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

pub const CanonicalTileId = struct {
    z: u32,
    x: u32,
    y: u32,
};

pub const CustomGeometrySourceTileCallback = *const fn (
    context: ?*anyopaque,
    tile_id: CanonicalTileId,
) void;

pub const CustomGeometrySourceOptions = struct {
    fetch_tile: CustomGeometrySourceTileCallback,
    cancel_tile: ?CustomGeometrySourceTileCallback = null,
    context: ?*anyopaque = null,
    min_zoom: ?f64 = null,
    max_zoom: ?f64 = null,
    tolerance: ?f64 = null,
    tile_size: ?u32 = null,
    buffer: ?u32 = null,
    clip: ?bool = null,
    wrap: ?bool = null,
};

pub const MapHandle = enum(u128) {
    _,

    pub fn create(runtime: *RuntimeHandle, options: MapOptions) status.Error!MapHandle {
        var native_options = c.mln_map_options_default();
        native_options.width = options.width;
        native_options.height = options.height;
        native_options.scale_factor = options.scale_factor;
        native_options.map_mode = options.mode.toRaw();

        const runtime_lease = try runtime_module.lease(runtime);
        defer runtime_lease.release();

        var map: ?*c.mln_map = null;
        const diagnostic_store = runtime_lease.diagnostic_store;
        try status.checkStatus(
            c.mln_map_create(runtime_lease.native, &native_options, &map),
            diagnostic_store,
        );
        errdefer {
            if (map) |handle| _ = c.mln_map_destroy(handle);
        }

        const custom_geometry_sources = try std.heap.smp_allocator.create(std.ArrayList(*CustomGeometrySourceState));
        custom_geometry_sources.* = .empty;
        errdefer std.heap.smp_allocator.destroy(custom_geometry_sources);

        const map_registration = try runtime_module.registerMap(
            runtime,
            map.?,
            releaseDetachedCustomGeometrySourceStatesForStyleLoaded,
            custom_geometry_sources,
        );
        errdefer runtime_module.unregisterMap(map_registration.registry, map.?);

        const map_state = try std.heap.smp_allocator.create(MapState);
        map_state.* = .{
            .native = @ptrCast(map.?),
            .runtime_registry = map_registration.registry,
            .id_value = map_registration.id,
            .diagnostic_store = diagnostic_store,
            .custom_geometry_sources = custom_geometry_sources,
            .active_render_sessions = std.atomic.Value(usize).init(0),
            .closing = false,
        };
        errdefer std.heap.smp_allocator.destroy(map_state);

        return try registerMapState(map_state);
    }

    pub fn id(self: *MapHandle) status.BindingError!values.MapId {
        return mapIdForHandle(self);
    }

    pub fn setStyleJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        json: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const json_z = try nulTerminated(allocator, json, diagnosticStore(self), "style JSON contains embedded NUL");
        defer allocator.free(json_z);
        try status.checkStatus(c.mln_map_set_style_json(native_map, json_z.ptr), diagnosticStore(self));
        clearCustomGeometrySourceStates(self);
    }

    pub fn setStyleUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        url: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const url_z = try nulTerminated(allocator, url, diagnosticStore(self), "style URL contains embedded NUL");
        defer allocator.free(url_z);
        try status.checkStatus(c.mln_map_set_style_url(native_map, url_z.ptr), diagnosticStore(self));
    }

    pub fn setLayerProperty(
        self: *MapHandle,
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
            diagnosticStore(self),
        );
    }

    pub fn getLayerProperty(
        self: *MapHandle,
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
            diagnosticStore(self),
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, diagnosticStore(self));
    }

    pub fn setLayerFilter(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        filter: ?values.JsonValue,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const filter_ptr = if (filter) |value| try temp.jsonValue(value) else null;
        try status.checkStatus(
            c.mln_map_set_layer_filter(try native(self), try temp.stringView(layer_id), filter_ptr),
            diagnosticStore(self),
        );
    }

    pub fn getLayerFilter(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!?values.OwnedJsonValue {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var snapshot: ?*c.mln_json_snapshot = null;
        try status.checkStatus(
            c.mln_map_get_layer_filter(try native(self), try temp.stringView(layer_id), &snapshot),
            diagnosticStore(self),
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, diagnosticStore(self));
    }

    pub fn listStyleSourceIds(self: *MapHandle, allocator: std.mem.Allocator) status.Error!values.StringList {
        var list: ?*c.mln_style_id_list = null;
        try status.checkStatus(c.mln_map_list_style_source_ids(try native(self), &list), diagnosticStore(self));
        defer if (list) |handle| c.mln_style_id_list_destroy(handle);
        return try copyStyleIdList(allocator, list.?, diagnosticStore(self));
    }

    pub fn listStyleLayerIds(self: *MapHandle, allocator: std.mem.Allocator) status.Error!values.StringList {
        var list: ?*c.mln_style_id_list = null;
        try status.checkStatus(c.mln_map_list_style_layer_ids(try native(self), &list), diagnosticStore(self));
        defer if (list) |handle| c.mln_style_id_list_destroy(handle);
        return try copyStyleIdList(allocator, list.?, diagnosticStore(self));
    }

    pub fn addStyleSourceJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        source_json: values.JsonValue,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_style_source_json(try native(self), try temp.stringView(source_id), try temp.jsonValue(source_json)),
            diagnosticStore(self),
        );
    }

    pub fn removeStyleSource(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var removed = false;
        try status.checkStatus(
            c.mln_map_remove_style_source(try native(self), try temp.stringView(source_id), &removed),
            diagnosticStore(self),
        );
        if (removed) releaseCustomGeometrySourceState(try mapStateForHandle(self), source_id);
        return removed;
    }

    pub fn styleSourceExists(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var exists = false;
        try status.checkStatus(
            c.mln_map_style_source_exists(try native(self), try temp.stringView(source_id), &exists),
            diagnosticStore(self),
        );
        return exists;
    }

    pub fn getStyleSourceType(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
    ) status.Error!?values.StyleSourceType {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_type: u32 = c.MLN_STYLE_SOURCE_TYPE_UNKNOWN;
        var found = false;
        try status.checkStatus(
            c.mln_map_get_style_source_type(try native(self), try temp.stringView(source_id), &raw_type, &found),
            diagnosticStore(self),
        );
        if (!found) return null;
        return values.styleSourceTypeFromNative(raw_type);
    }

    pub fn getStyleSourceInfo(
        self: *MapHandle,
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
            diagnosticStore(self),
        );
        if (!found) return null;
        return values.styleSourceInfoFromNative(raw_info);
    }

    pub fn copyStyleSourceAttribution(
        self: *MapHandle,
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
            diagnosticStore(self),
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

    pub fn addStyleLayerJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_json: values.JsonValue,
        before_layer_id: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_style_layer_json(try native(self), try temp.jsonValue(layer_json), stringView(before_layer_id)),
            diagnosticStore(self),
        );
    }

    pub fn removeStyleLayer(self: *MapHandle, layer_id: []const u8) status.Error!bool {
        var removed = false;
        try status.checkStatus(
            c.mln_map_remove_style_layer(try native(self), stringView(layer_id), &removed),
            diagnosticStore(self),
        );
        return removed;
    }

    pub fn styleLayerExists(self: *MapHandle, layer_id: []const u8) status.Error!bool {
        var exists = false;
        try status.checkStatus(
            c.mln_map_style_layer_exists(try native(self), stringView(layer_id), &exists),
            diagnosticStore(self),
        );
        return exists;
    }

    pub fn moveStyleLayer(
        self: *MapHandle,
        layer_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!void {
        try status.checkStatus(
            c.mln_map_move_style_layer(try native(self), stringView(layer_id), stringView(before_layer_id)),
            diagnosticStore(self),
        );
    }

    pub fn getStyleLayerJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!?values.OwnedJsonValue {
        var snapshot: ?*c.mln_json_snapshot = null;
        var found = false;
        try status.checkStatus(
            c.mln_map_get_style_layer_json(try native(self), stringView(layer_id), &snapshot, &found),
            diagnosticStore(self),
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        if (!found) return null;
        return try copyJsonSnapshot(allocator, snapshot, diagnosticStore(self)) orelse error.NativeError;
    }

    pub fn getStyleLayerType(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!?values.OwnedString {
        var layer_type = c.mln_string_view{ .data = null, .size = 0 };
        var found = false;
        try status.checkStatus(
            c.mln_map_get_style_layer_type(try native(self), stringView(layer_id), &layer_type, &found),
            diagnosticStore(self),
        );
        if (!found) return null;
        const copied = if (layer_type.size == 0) try allocator.dupe(u8, "") else try allocator.dupe(u8, layer_type.data[0..layer_type.size]);
        return .{ .allocator = allocator, .value = copied };
    }

    pub fn setStyleLightJson(self: *MapHandle, allocator: std.mem.Allocator, value: values.JsonValue) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_style_light_json(try native(self), try temp.jsonValue(value)),
            diagnosticStore(self),
        );
    }

    pub fn setStyleLightProperty(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        property_name: []const u8,
        value: values.JsonValue,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_style_light_property(try native(self), try temp.stringView(property_name), try temp.jsonValue(value)),
            diagnosticStore(self),
        );
    }

    pub fn getStyleLightProperty(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        property_name: []const u8,
    ) status.Error!?values.OwnedJsonValue {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var snapshot: ?*c.mln_json_snapshot = null;
        try status.checkStatus(
            c.mln_map_get_style_light_property(try native(self), try temp.stringView(property_name), &snapshot),
            diagnosticStore(self),
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, diagnosticStore(self));
    }

    pub fn addVectorSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        try status.checkStatus(
            c.mln_map_add_vector_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null),
            diagnosticStore(self),
        );
    }

    pub fn addVectorSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        try status.checkStatus(
            c.mln_map_add_vector_source_tiles(try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null),
            diagnosticStore(self),
        );
    }

    pub fn addRasterSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        try status.checkStatus(
            c.mln_map_add_raster_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null),
            diagnosticStore(self),
        );
    }

    pub fn addRasterSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        try status.checkStatus(
            c.mln_map_add_raster_source_tiles(try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null),
            diagnosticStore(self),
        );
    }

    pub fn addRasterDemSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        try status.checkStatus(
            c.mln_map_add_raster_dem_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null),
            diagnosticStore(self),
        );
    }

    pub fn addRasterDemSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        try status.checkStatus(
            c.mln_map_add_raster_dem_source_tiles(try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null),
            diagnosticStore(self),
        );
    }

    pub fn addHillshadeLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_hillshade_layer(try native(self), try temp.stringView(layer_id), try temp.stringView(source_id), try temp.stringView(before_layer_id)),
            diagnosticStore(self),
        );
    }

    pub fn addColorReliefLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_color_relief_layer(try native(self), try temp.stringView(layer_id), try temp.stringView(source_id), try temp.stringView(before_layer_id)),
            diagnosticStore(self),
        );
    }

    pub fn setStyleImage(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        image_id: []const u8,
        image: values.PremultipliedRgba8Image,
        options: ?values.StyleImageOptions,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        var raw_options = if (options) |value| values.styleImageOptionsToNative(value) else undefined;
        try status.checkStatus(
            c.mln_map_set_style_image(try native(self), try temp.stringView(image_id), &raw_image, if (options != null) &raw_options else null),
            diagnosticStore(self),
        );
    }

    pub fn removeStyleImage(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var removed = false;
        try status.checkStatus(
            c.mln_map_remove_style_image(try native(self), try temp.stringView(image_id), &removed),
            diagnosticStore(self),
        );
        return removed;
    }

    pub fn styleImageExists(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var exists = false;
        try status.checkStatus(
            c.mln_map_style_image_exists(try native(self), try temp.stringView(image_id), &exists),
            diagnosticStore(self),
        );
        return exists;
    }

    pub fn getStyleImageInfo(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        image_id: []const u8,
    ) status.Error!?values.StyleImageInfo {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var info = c.mln_style_image_info_default();
        var found = false;
        try status.checkStatus(
            c.mln_map_get_style_image_info(try native(self), try temp.stringView(image_id), &info, &found),
            diagnosticStore(self),
        );
        if (!found) return null;
        return values.styleImageInfoFromNative(info);
    }

    pub fn copyStyleImagePremultipliedRgba8(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        image_id: []const u8,
    ) status.Error!?values.OwnedStyleImage {
        const info = (try self.getStyleImageInfo(allocator, image_id)) orelse return null;
        const pixels = try allocator.alloc(u8, info.byte_length);
        errdefer allocator.free(pixels);

        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var copied_size: usize = 0;
        var found = false;
        try status.checkStatus(
            c.mln_map_copy_style_image_premultiplied_rgba8(
                try native(self),
                try temp.stringView(image_id),
                if (pixels.len == 0) null else pixels.ptr,
                pixels.len,
                &copied_size,
                &found,
            ),
            diagnosticStore(self),
        );
        if (!found) {
            allocator.free(pixels);
            return null;
        }
        if (copied_size != pixels.len) return error.NativeError;
        return .{ .allocator = allocator, .info = info, .pixels = pixels };
    }

    pub fn addImageSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        coordinates: [4]values.LatLng,
        url: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        try status.checkStatus(
            c.mln_map_add_image_source_url(
                try native(self),
                try temp.stringView(source_id),
                raw_coordinates.ptr,
                raw_coordinates.len,
                try temp.stringView(url),
            ),
            diagnosticStore(self),
        );
    }

    pub fn addImageSourceImage(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        coordinates: [4]values.LatLng,
        image: values.PremultipliedRgba8Image,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        try status.checkStatus(
            c.mln_map_add_image_source_image(
                try native(self),
                try temp.stringView(source_id),
                raw_coordinates.ptr,
                raw_coordinates.len,
                &raw_image,
            ),
            diagnosticStore(self),
        );
    }

    pub fn setImageSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_image_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url)),
            diagnosticStore(self),
        );
    }

    pub fn setImageSourceImage(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        image: values.PremultipliedRgba8Image,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        try status.checkStatus(
            c.mln_map_set_image_source_image(try native(self), try temp.stringView(source_id), &raw_image),
            diagnosticStore(self),
        );
    }

    pub fn setImageSourceCoordinates(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        coordinates: [4]values.LatLng,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        try status.checkStatus(
            c.mln_map_set_image_source_coordinates(
                try native(self),
                try temp.stringView(source_id),
                raw_coordinates.ptr,
                raw_coordinates.len,
            ),
            diagnosticStore(self),
        );
    }

    pub fn getImageSourceCoordinates(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
    ) status.Error!?[4]values.LatLng {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_coordinates: [4]c.mln_lat_lng = undefined;
        var coordinate_count: usize = 0;
        var found = false;
        try status.checkStatus(
            c.mln_map_get_image_source_coordinates(
                try native(self),
                try temp.stringView(source_id),
                &raw_coordinates,
                raw_coordinates.len,
                &coordinate_count,
                &found,
            ),
            diagnosticStore(self),
        );
        if (!found) return null;
        if (coordinate_count != raw_coordinates.len) return error.NativeError;
        var coordinates: [4]values.LatLng = undefined;
        for (raw_coordinates, &coordinates) |raw_coordinate, *coordinate| coordinate.* = values.latLngFromNative(raw_coordinate);
        return coordinates;
    }

    pub fn addLocationIndicatorLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_location_indicator_layer(try native(self), try temp.stringView(layer_id), try temp.stringView(before_layer_id)),
            diagnosticStore(self),
        );
    }

    pub fn setLocationIndicatorLocation(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        coordinate: values.LatLng,
        altitude: f64,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_location_indicator_location(
                try native(self),
                try temp.stringView(layer_id),
                values.latLngToNative(coordinate),
                altitude,
            ),
            diagnosticStore(self),
        );
    }

    pub fn setLocationIndicatorBearing(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        bearing: f64,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_location_indicator_bearing(try native(self), try temp.stringView(layer_id), bearing),
            diagnosticStore(self),
        );
    }

    pub fn setLocationIndicatorAccuracyRadius(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        radius: f64,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_location_indicator_accuracy_radius(try native(self), try temp.stringView(layer_id), radius),
            diagnosticStore(self),
        );
    }

    pub fn setLocationIndicatorImageName(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        kind: values.LocationIndicatorImageKind,
        image_id: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_location_indicator_image_name(
                try native(self),
                try temp.stringView(layer_id),
                values.locationIndicatorImageKindToNative(kind),
                try temp.stringView(image_id),
            ),
            diagnosticStore(self),
        );
    }

    pub fn addGeoJsonSourceData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        data: values.GeoJson,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_geojson_source_data(try native(self), try temp.stringView(source_id), try temp.geoJson(data)),
            diagnosticStore(self),
        );
    }

    pub fn setGeoJsonSourceData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        data: values.GeoJson,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_geojson_source_data(try native(self), try temp.stringView(source_id), try temp.geoJson(data)),
            diagnosticStore(self),
        );
    }

    pub fn addGeoJsonSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_add_geojson_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url)),
            diagnosticStore(self),
        );
    }

    pub fn setGeoJsonSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_geojson_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url)),
            diagnosticStore(self),
        );
    }

    pub fn addCustomGeometrySource(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        options: CustomGeometrySourceOptions,
    ) status.Error!void {
        _ = try native(self);
        const map_state = try mapStateForHandle(self);
        const owned_source_id = try std.heap.smp_allocator.dupe(u8, source_id);
        errdefer std.heap.smp_allocator.free(owned_source_id);

        const source_state = try std.heap.smp_allocator.create(CustomGeometrySourceState);
        source_state.* = .{
            .source_id = owned_source_id,
            .fetch_tile = options.fetch_tile,
            .cancel_tile = options.cancel_tile,
            .context = options.context,
            .active_upcalls = std.atomic.Value(usize).init(0),
        };
        errdefer std.heap.smp_allocator.destroy(source_state);

        try registerLiveCustomGeometrySourceState(source_state);
        errdefer unregisterLiveCustomGeometrySourceState(source_state);

        try map_state.custom_geometry_sources.append(std.heap.smp_allocator, source_state);
        errdefer _ = map_state.custom_geometry_sources.pop();

        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var native_options = customGeometrySourceOptionsToNative(options, source_state);
        try status.checkStatus(
            c.mln_map_add_custom_geometry_source(try native(self), try temp.stringView(source_id), &native_options),
            map_state.diagnostic_store,
        );
    }

    pub fn setCustomGeometrySourceTileData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
        data: values.GeoJson,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_set_custom_geometry_source_tile_data(
                try native(self),
                try temp.stringView(source_id),
                canonicalTileIdToNative(tile_id),
                try temp.geoJson(data),
            ),
            diagnosticStore(self),
        );
    }

    pub fn invalidateCustomGeometrySourceTile(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_invalidate_custom_geometry_source_tile(try native(self), try temp.stringView(source_id), canonicalTileIdToNative(tile_id)),
            diagnosticStore(self),
        );
    }

    pub fn invalidateCustomGeometrySourceRegion(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        bounds: values.LatLngBounds,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        try status.checkStatus(
            c.mln_map_invalidate_custom_geometry_source_region(try native(self), try temp.stringView(source_id), values.latLngBoundsToNative(bounds)),
            diagnosticStore(self),
        );
    }

    pub fn requestRepaint(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_request_repaint(try native(self)), diagnosticStore(self));
    }

    pub fn setDebugOptions(self: *MapHandle, options: values.DebugOptions) status.Error!void {
        try status.checkStatus(c.mln_map_set_debug_options(try native(self), values.debugOptionsToNative(options)), diagnosticStore(self));
    }

    pub fn getDebugOptions(self: *MapHandle) status.Error!values.DebugOptions {
        var options: u32 = 0;
        try status.checkStatus(c.mln_map_get_debug_options(try native(self), &options), diagnosticStore(self));
        return values.debugOptionsFromNative(options);
    }

    pub fn setRenderingStatsViewEnabled(self: *MapHandle, enabled: bool) status.Error!void {
        try status.checkStatus(c.mln_map_set_rendering_stats_view_enabled(try native(self), enabled), diagnosticStore(self));
    }

    pub fn getRenderingStatsViewEnabled(self: *MapHandle) status.Error!bool {
        var enabled = false;
        try status.checkStatus(c.mln_map_get_rendering_stats_view_enabled(try native(self), &enabled), diagnosticStore(self));
        return enabled;
    }

    pub fn isFullyLoaded(self: *MapHandle) status.Error!bool {
        var loaded = false;
        try status.checkStatus(c.mln_map_is_fully_loaded(try native(self), &loaded), diagnosticStore(self));
        return loaded;
    }

    pub fn dumpDebugLogs(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_dump_debug_logs(try native(self)), diagnosticStore(self));
    }

    pub fn setViewportOptions(self: *MapHandle, options: values.ViewportOptions) status.Error!void {
        var raw_options = values.viewportOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_viewport_options(try native(self), &raw_options), diagnosticStore(self));
    }

    pub fn getViewportOptions(self: *MapHandle) status.Error!values.ViewportOptions {
        var options = c.mln_map_viewport_options_default();
        try status.checkStatus(c.mln_map_get_viewport_options(try native(self), &options), diagnosticStore(self));
        return try values.viewportOptionsFromNative(options);
    }

    pub fn setTileOptions(self: *MapHandle, options: values.TileOptions) status.Error!void {
        var raw_options = values.tileOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_tile_options(try native(self), &raw_options), diagnosticStore(self));
    }

    pub fn getTileOptions(self: *MapHandle) status.Error!values.TileOptions {
        var options = c.mln_map_tile_options_default();
        try status.checkStatus(c.mln_map_get_tile_options(try native(self), &options), diagnosticStore(self));
        return try values.tileOptionsFromNative(options);
    }

    pub fn getCamera(self: *MapHandle) status.Error!values.CameraOptions {
        var camera = c.mln_camera_options_default();
        try status.checkStatus(c.mln_map_get_camera(try native(self), &camera), diagnosticStore(self));
        return values.cameraOptionsFromNative(camera);
    }

    pub fn jumpTo(self: *MapHandle, camera: values.CameraOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        try status.checkStatus(c.mln_map_jump_to(try native(self), &raw_camera), diagnosticStore(self));
    }

    pub fn easeTo(self: *MapHandle, camera: values.CameraOptions, animation: ?values.AnimationOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_ease_to(try native(self), &raw_camera, animation_ptr), diagnosticStore(self));
    }

    pub fn flyTo(self: *MapHandle, camera: values.CameraOptions, animation: ?values.AnimationOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_fly_to(try native(self), &raw_camera, animation_ptr), diagnosticStore(self));
    }

    pub fn moveBy(self: *MapHandle, delta_x: f64, delta_y: f64) status.Error!void {
        try status.checkStatus(c.mln_map_move_by(try native(self), delta_x, delta_y), diagnosticStore(self));
    }

    pub fn moveByAnimated(self: *MapHandle, delta_x: f64, delta_y: f64, animation: ?values.AnimationOptions) status.Error!void {
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_move_by_animated(try native(self), delta_x, delta_y, animation_ptr), diagnosticStore(self));
    }

    pub fn scaleBy(self: *MapHandle, scale: f64, anchor: ?values.ScreenPoint) status.Error!void {
        var raw_anchor = if (anchor) |point| values.screenPointToNative(point) else undefined;
        const anchor_ptr = if (anchor != null) &raw_anchor else null;
        try status.checkStatus(c.mln_map_scale_by(try native(self), scale, anchor_ptr), diagnosticStore(self));
    }

    pub fn scaleByAnimated(self: *MapHandle, scale: f64, anchor: ?values.ScreenPoint, animation: ?values.AnimationOptions) status.Error!void {
        var raw_anchor = if (anchor) |point| values.screenPointToNative(point) else undefined;
        const anchor_ptr = if (anchor != null) &raw_anchor else null;
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_scale_by_animated(try native(self), scale, anchor_ptr, animation_ptr), diagnosticStore(self));
    }

    pub fn rotateBy(self: *MapHandle, first: values.ScreenPoint, second: values.ScreenPoint) status.Error!void {
        try status.checkStatus(
            c.mln_map_rotate_by(try native(self), values.screenPointToNative(first), values.screenPointToNative(second)),
            diagnosticStore(self),
        );
    }

    pub fn rotateByAnimated(self: *MapHandle, first: values.ScreenPoint, second: values.ScreenPoint, animation: ?values.AnimationOptions) status.Error!void {
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(
            c.mln_map_rotate_by_animated(try native(self), values.screenPointToNative(first), values.screenPointToNative(second), animation_ptr),
            diagnosticStore(self),
        );
    }

    pub fn pitchBy(self: *MapHandle, pitch: f64) status.Error!void {
        try status.checkStatus(c.mln_map_pitch_by(try native(self), pitch), diagnosticStore(self));
    }

    pub fn pitchByAnimated(self: *MapHandle, pitch: f64, animation: ?values.AnimationOptions) status.Error!void {
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_pitch_by_animated(try native(self), pitch, animation_ptr), diagnosticStore(self));
    }

    pub fn cancelTransitions(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_cancel_transitions(try native(self)), diagnosticStore(self));
    }

    pub fn requestStillImage(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_request_still_image(try native(self)), diagnosticStore(self));
    }

    pub fn setProjectionMode(self: *MapHandle, mode: values.ProjectionMode) status.Error!void {
        var raw_mode = values.projectionModeToNative(mode);
        try status.checkStatus(c.mln_map_set_projection_mode(try native(self), &raw_mode), diagnosticStore(self));
    }

    pub fn getProjectionMode(self: *MapHandle) status.Error!values.ProjectionMode {
        var mode = c.mln_projection_mode_default();
        try status.checkStatus(c.mln_map_get_projection_mode(try native(self), &mode), diagnosticStore(self));
        return values.projectionModeFromNative(mode);
    }

    pub fn pixelForLatLng(self: *MapHandle, coordinate: values.LatLng) status.Error!values.ScreenPoint {
        var point: c.mln_screen_point = undefined;
        try status.checkStatus(
            c.mln_map_pixel_for_lat_lng(try native(self), values.latLngToNative(coordinate), &point),
            diagnosticStore(self),
        );
        return values.screenPointFromNative(point);
    }

    pub fn latLngForPixel(self: *MapHandle, point: values.ScreenPoint) status.Error!values.LatLng {
        var coordinate: c.mln_lat_lng = undefined;
        try status.checkStatus(
            c.mln_map_lat_lng_for_pixel(try native(self), values.screenPointToNative(point), &coordinate),
            diagnosticStore(self),
        );
        return values.latLngFromNative(coordinate);
    }

    pub fn pixelsForLatLngs(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        coordinates: []const values.LatLng,
        out_points: []values.ScreenPoint,
    ) status.Error!void {
        if (coordinates.len != out_points.len) return error.InvalidArgument;
        if (coordinates.len == 0) {
            try status.checkStatus(c.mln_map_pixels_for_lat_lngs(try native(self), null, 0, null), diagnosticStore(self));
            return;
        }
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        const raw_points = try allocator.alloc(c.mln_screen_point, out_points.len);
        defer allocator.free(raw_points);
        try status.checkStatus(
            c.mln_map_pixels_for_lat_lngs(try native(self), raw_coordinates.ptr, raw_coordinates.len, raw_points.ptr),
            diagnosticStore(self),
        );
        for (raw_points, out_points) |raw_point, *out_point| out_point.* = values.screenPointFromNative(raw_point);
    }

    pub fn latLngsForPixels(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        points: []const values.ScreenPoint,
        out_coordinates: []values.LatLng,
    ) status.Error!void {
        if (points.len != out_coordinates.len) return error.InvalidArgument;
        if (points.len == 0) {
            try status.checkStatus(c.mln_map_lat_lngs_for_pixels(try native(self), null, 0, null), diagnosticStore(self));
            return;
        }
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_points = try temp.screenPoints(points);
        const raw_coordinates = try allocator.alloc(c.mln_lat_lng, out_coordinates.len);
        defer allocator.free(raw_coordinates);
        try status.checkStatus(
            c.mln_map_lat_lngs_for_pixels(try native(self), raw_points.ptr, raw_points.len, raw_coordinates.ptr),
            diagnosticStore(self),
        );
        for (raw_coordinates, out_coordinates) |raw_coordinate, *out_coordinate| out_coordinate.* = values.latLngFromNative(raw_coordinate);
    }

    pub fn cameraForLatLngBounds(
        self: *MapHandle,
        bounds: values.LatLngBounds,
        fit_options: ?values.CameraFitOptions,
    ) status.Error!values.CameraOptions {
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        var camera = c.mln_camera_options_default();
        try status.checkStatus(
            c.mln_map_camera_for_lat_lng_bounds(try native(self), values.latLngBoundsToNative(bounds), fit_ptr, &camera),
            diagnosticStore(self),
        );
        return values.cameraOptionsFromNative(camera);
    }

    pub fn cameraForLatLngs(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        coordinates: []const values.LatLng,
        fit_options: ?values.CameraFitOptions,
    ) status.Error!values.CameraOptions {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        var camera = c.mln_camera_options_default();
        try status.checkStatus(
            c.mln_map_camera_for_lat_lngs(try native(self), if (raw_coordinates.len == 0) null else raw_coordinates.ptr, raw_coordinates.len, fit_ptr, &camera),
            diagnosticStore(self),
        );
        return values.cameraOptionsFromNative(camera);
    }

    pub fn cameraForGeometry(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        geometry: values.Geometry,
        fit_options: ?values.CameraFitOptions,
    ) status.Error!values.CameraOptions {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        var camera = c.mln_camera_options_default();
        try status.checkStatus(
            c.mln_map_camera_for_geometry(try native(self), try temp.geometry(geometry), fit_ptr, &camera),
            diagnosticStore(self),
        );
        return values.cameraOptionsFromNative(camera);
    }

    pub fn latLngBoundsForCamera(self: *MapHandle, camera: values.CameraOptions) status.Error!values.LatLngBounds {
        var raw_camera = values.cameraOptionsToNative(camera);
        var bounds: c.mln_lat_lng_bounds = undefined;
        try status.checkStatus(c.mln_map_lat_lng_bounds_for_camera(try native(self), &raw_camera, &bounds), diagnosticStore(self));
        return values.latLngBoundsFromNative(bounds);
    }

    pub fn latLngBoundsForCameraUnwrapped(self: *MapHandle, camera: values.CameraOptions) status.Error!values.LatLngBounds {
        var raw_camera = values.cameraOptionsToNative(camera);
        var bounds: c.mln_lat_lng_bounds = undefined;
        try status.checkStatus(c.mln_map_lat_lng_bounds_for_camera_unwrapped(try native(self), &raw_camera, &bounds), diagnosticStore(self));
        return values.latLngBoundsFromNative(bounds);
    }

    pub fn getBounds(self: *MapHandle) status.Error!values.BoundOptions {
        var raw_options = c.mln_bound_options_default();
        try status.checkStatus(c.mln_map_get_bounds(try native(self), &raw_options), diagnosticStore(self));
        return values.boundOptionsFromNative(raw_options);
    }

    pub fn setBounds(self: *MapHandle, options: values.BoundOptions) status.Error!void {
        var raw_options = values.boundOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_bounds(try native(self), &raw_options), diagnosticStore(self));
    }

    pub fn getFreeCameraOptions(self: *MapHandle) status.Error!values.FreeCameraOptions {
        var raw_options = c.mln_free_camera_options_default();
        try status.checkStatus(c.mln_map_get_free_camera_options(try native(self), &raw_options), diagnosticStore(self));
        return values.freeCameraOptionsFromNative(raw_options);
    }

    pub fn setFreeCameraOptions(self: *MapHandle, options: values.FreeCameraOptions) status.Error!void {
        var raw_options = values.freeCameraOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_free_camera_options(try native(self), &raw_options), diagnosticStore(self));
    }

    pub fn close(self: *MapHandle) status.Error!void {
        const map_close = beginMapClose(self.*) catch |err| {
            if (err == error.InvalidState) {
                if (diagnosticStore(self)) |store| {
                    try status.setBindingDiagnostic(store, "map has live render sessions");
                }
            }
            return err;
        } orelse return;
        status.checkStatus(c.mln_map_destroy(map_close.native), map_close.diagnostic_store) catch |err| {
            cancelMapClose(map_close.state);
            return err;
        };
        runtime_module.unregisterMap(map_close.runtime_registry, map_close.native);
        freeCustomGeometrySourceStates(map_close.state);
        const map_state = finishMapClose(self.*) orelse map_close.state;
        std.heap.smp_allocator.destroy(map_state);
    }
};

fn styleTileSourceOptionsToNative(
    temp: *native_temp.TempStorage,
    options: values.StyleTileSourceOptions,
) status.Error!c.mln_style_tile_source_options {
    var raw = c.mln_style_tile_source_options_default();
    if (options.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (options.max_zoom) |max_zoom| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    if (options.attribution) |attribution| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
        raw.attribution = try temp.stringView(attribution);
    }
    if (options.scheme) |scheme| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
        raw.scheme = values.styleTileSchemeToNative(scheme);
    }
    if (options.bounds) |bounds| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
        raw.bounds = values.latLngBoundsToNative(bounds);
    }
    if (options.tile_size) |tile_size| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
        raw.tile_size = tile_size;
    }
    if (options.vector_encoding) |encoding| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
        raw.vector_encoding = values.styleVectorTileEncodingToNative(encoding);
    }
    if (options.raster_encoding) |encoding| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
        raw.raster_encoding = values.styleRasterDemEncodingToNative(encoding);
    }
    return raw;
}

fn customGeometrySourceOptionsToNative(
    options: CustomGeometrySourceOptions,
    source_state: *CustomGeometrySourceState,
) c.mln_custom_geometry_source_options {
    var raw = c.mln_custom_geometry_source_options_default();
    raw.fetch_tile = customGeometryFetchTileTrampoline;
    raw.cancel_tile = if (options.cancel_tile != null) customGeometryCancelTileTrampoline else null;
    raw.user_data = source_state;
    if (options.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (options.max_zoom) |max_zoom| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    if (options.tolerance) |tolerance| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
        raw.tolerance = tolerance;
    }
    if (options.tile_size) |tile_size| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
        raw.tile_size = tile_size;
    }
    if (options.buffer) |buffer| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
        raw.buffer = buffer;
    }
    if (options.clip) |clip| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
        raw.clip = clip;
    }
    if (options.wrap) |wrap| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
        raw.wrap = wrap;
    }
    return raw;
}

fn customGeometryFetchTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomGeometrySourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomGeometryUpcall(source_state)) return;
    defer endCustomGeometryUpcall(source_state);

    source_state.fetch_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn customGeometryCancelTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomGeometrySourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomGeometryUpcall(source_state)) return;
    defer endCustomGeometryUpcall(source_state);

    const cancel_tile = source_state.cancel_tile orelse return;
    cancel_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn beginCustomGeometryUpcall(source_state: *CustomGeometrySourceState) bool {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);

    for (custom_geometry_state_registry.items) |live_state| {
        if (live_state == source_state) {
            _ = source_state.active_upcalls.fetchAdd(1, .seq_cst);
            return true;
        }
    }
    return false;
}

fn endCustomGeometryUpcall(source_state: *CustomGeometrySourceState) void {
    _ = source_state.active_upcalls.fetchSub(1, .seq_cst);
}

fn canonicalTileIdToNative(tile_id: CanonicalTileId) c.mln_canonical_tile_id {
    return .{ .z = tile_id.z, .x = tile_id.x, .y = tile_id.y };
}

fn canonicalTileIdFromNative(tile_id: c.mln_canonical_tile_id) CanonicalTileId {
    return .{ .z = tile_id.z, .x = tile_id.x, .y = tile_id.y };
}

fn releaseCustomGeometrySourceState(map_state: *MapState, source_id: []const u8) void {
    for (map_state.custom_geometry_sources.items, 0..) |source_state, index| {
        if (std.mem.eql(u8, source_state.source_id, source_id)) {
            _ = map_state.custom_geometry_sources.orderedRemove(index);
            freeCustomGeometrySourceState(source_state);
            return;
        }
    }
}

fn releaseDetachedCustomGeometrySourceStatesForStyleLoaded(map: *c.mln_map, context: ?*anyopaque) void {
    const custom_geometry_sources: *std.ArrayList(*CustomGeometrySourceState) = @ptrCast(@alignCast(context orelse return));
    var index: usize = 0;
    while (index < custom_geometry_sources.items.len) {
        const source_state = custom_geometry_sources.items[index];
        var source_type: u32 = c.MLN_STYLE_SOURCE_TYPE_UNKNOWN;
        var found = false;
        const check = c.mln_map_get_style_source_type(map, stringView(source_state.source_id), &source_type, &found);
        if (check != c.MLN_STATUS_OK or (found and source_type == c.MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR)) {
            index += 1;
            continue;
        }
        _ = custom_geometry_sources.orderedRemove(index);
        freeCustomGeometrySourceState(source_state);
    }
}

fn clearCustomGeometrySourceStates(map_handle: *MapHandle) void {
    const map_state = mapStateForHandle(map_handle) catch return;
    clearCustomGeometrySourceStatesForState(map_state);
}

fn clearCustomGeometrySourceStatesForState(map_state: *MapState) void {
    for (map_state.custom_geometry_sources.items) |source_state| {
        retireLiveCustomGeometrySourceState(source_state);
    }
    for (map_state.custom_geometry_sources.items) |source_state| {
        waitForCustomGeometryUpcalls(source_state);
        std.heap.smp_allocator.free(source_state.source_id);
        std.heap.smp_allocator.destroy(source_state);
    }
    map_state.custom_geometry_sources.clearRetainingCapacity();
}

fn freeCustomGeometrySourceStates(map_state: *MapState) void {
    clearCustomGeometrySourceStatesForState(map_state);
    map_state.custom_geometry_sources.deinit(std.heap.smp_allocator);
    std.heap.smp_allocator.destroy(map_state.custom_geometry_sources);
}

fn freeCustomGeometrySourceState(source_state: *CustomGeometrySourceState) void {
    retireLiveCustomGeometrySourceState(source_state);
    waitForCustomGeometryUpcalls(source_state);
    std.heap.smp_allocator.free(source_state.source_id);
    std.heap.smp_allocator.destroy(source_state);
}

fn registerLiveCustomGeometrySourceState(source_state: *CustomGeometrySourceState) std.mem.Allocator.Error!void {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);
    try custom_geometry_state_registry.append(std.heap.smp_allocator, source_state);
}

fn unregisterLiveCustomGeometrySourceState(source_state: *CustomGeometrySourceState) void {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);
    removeLiveCustomGeometrySourceStateLocked(source_state);
}

fn retireLiveCustomGeometrySourceState(source_state: *CustomGeometrySourceState) void {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);
    removeLiveCustomGeometrySourceStateLocked(source_state);
}

fn removeLiveCustomGeometrySourceStateLocked(source_state: *CustomGeometrySourceState) void {
    for (custom_geometry_state_registry.items, 0..) |live_state, index| {
        if (live_state == source_state) {
            _ = custom_geometry_state_registry.orderedRemove(index);
            return;
        }
    }
}

fn waitForCustomGeometryUpcalls(source_state: *CustomGeometrySourceState) void {
    while (source_state.active_upcalls.load(.seq_cst) != 0) {
        std.Thread.yield() catch {};
    }
}

fn mapStateForHandle(handle: *MapHandle) status.BindingError!*MapState {
    return mapState(handle.*) orelse error.ClosedHandle;
}

fn mapIdForHandle(handle: *MapHandle) status.BindingError!values.MapId {
    lockMapRegistry();
    defer unlockMapRegistry();

    const map_state = mapStateLocked(handle.*) orelse return error.ClosedHandle;
    return map_state.id_value;
}

fn registerMapState(map_state: *MapState) std.mem.Allocator.Error!MapHandle {
    lockMapRegistry();
    defer unlockMapRegistry();

    if (map_free_list.items.len > 0) {
        const slot_index = map_free_list.pop().?;
        map_registry.items[slot_index].state = map_state;
        map_registry.items[slot_index].generation = runtime_module.nextHandleGeneration();
        return mapHandle(slot_index + 1, map_registry.items[slot_index].generation);
    }

    const generation = runtime_module.nextHandleGeneration();
    try map_free_list.ensureTotalCapacity(std.heap.smp_allocator, map_registry.items.len + 1);
    try map_registry.append(std.heap.smp_allocator, .{ .state = map_state, .generation = generation });
    return mapHandle(map_registry.items.len, generation);
}

fn mapHandle(index: usize, generation: u64) MapHandle {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn mapHandleIndex(handle: MapHandle) ?usize {
    const index = @intFromEnum(handle) & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn mapHandleGeneration(handle: MapHandle) u64 {
    return @intCast(@intFromEnum(handle) >> 64);
}

fn mapState(handle: MapHandle) ?*MapState {
    lockMapRegistry();
    defer unlockMapRegistry();
    return mapStateLocked(handle);
}

fn mapStateLocked(handle: MapHandle) ?*MapState {
    const index = mapHandleIndex(handle) orelse return null;
    if (index > map_registry.items.len) return null;
    const slot = map_registry.items[index - 1];
    if (slot.generation != mapHandleGeneration(handle)) return null;
    return slot.state;
}

const MapClose = struct {
    state: *MapState,
    native: *c.mln_map,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    runtime_registry: *runtime_module.RuntimeRegistry,
};

fn beginMapClose(handle: MapHandle) status.BindingError!?MapClose {
    lockMapRegistry();
    defer unlockMapRegistry();

    const index = mapHandleIndex(handle) orelse return null;
    if (index > map_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &map_registry.items[slot_index];
    if (slot.generation != mapHandleGeneration(handle)) return null;
    const map_state = slot.state orelse return null;
    if (map_state.closing) return error.ActiveBorrow;
    if (map_state.active_render_sessions.load(.seq_cst) != 0) return error.InvalidState;
    const map: *c.mln_map = @ptrCast(map_state.native orelse return null);
    map_state.closing = true;
    return .{
        .state = map_state,
        .native = map,
        .diagnostic_store = map_state.diagnostic_store,
        .runtime_registry = map_state.runtime_registry,
    };
}

fn cancelMapClose(map_state: *MapState) void {
    lockMapRegistry();
    defer unlockMapRegistry();

    map_state.closing = false;
}

fn finishMapClose(handle: MapHandle) ?*MapState {
    lockMapRegistry();
    defer unlockMapRegistry();

    const index = mapHandleIndex(handle) orelse return null;
    if (index > map_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &map_registry.items[slot_index];
    if (slot.generation != mapHandleGeneration(handle)) return null;
    const map_state = slot.state orelse return null;
    slot.state = null;
    slot.generation = runtime_module.nextHandleGeneration();
    map_state.native = null;
    map_free_list.appendAssumeCapacity(slot_index);
    return map_state;
}

fn lockMapRegistry() void {
    while (map_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockMapRegistry() void {
    map_registry_lock.store(false, .seq_cst);
}

pub fn native(handle: *MapHandle) status.BindingError!*c.mln_map {
    const map_state = mapState(handle.*) orelse return error.ClosedHandle;
    if (map_state.closing) return error.ActiveBorrow;
    return @ptrCast(map_state.native orelse return error.ClosedHandle);
}

pub fn diagnosticStore(handle: *MapHandle) ?*diagnostics.DiagnosticStore {
    const map_state = mapState(handle.*) orelse return null;
    return map_state.diagnostic_store;
}

pub fn registerRenderSession(handle: *MapHandle) status.BindingError!RenderSessionRegistration {
    lockMapRegistry();
    defer unlockMapRegistry();

    const map_state = mapStateLocked(handle.*) orelse return error.ClosedHandle;
    if (map_state.closing) return error.ActiveBorrow;
    const map: *c.mln_map = @ptrCast(map_state.native orelse return error.ClosedHandle);
    _ = map_state.active_render_sessions.fetchAdd(1, .seq_cst);
    return .{
        .native = map,
        .diagnostic_store = map_state.diagnostic_store,
    };
}

pub fn unregisterRenderSession(handle: MapHandle) void {
    const map_state = mapState(handle) orelse return;
    _ = map_state.active_render_sessions.fetchSub(1, .seq_cst);
}

fn customGeometrySourceCountForTesting(handle: *MapHandle) status.BindingError!usize {
    return (try mapStateForHandle(handle)).custom_geometry_sources.items.len;
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

fn stringView(value: []const u8) c.mln_string_view {
    return .{ .data = if (value.len == 0) null else value.ptr, .size = value.len };
}

fn nulTerminated(
    allocator: std.mem.Allocator,
    value: []const u8,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    diagnostic_message: []const u8,
) status.Error![:0]u8 {
    if (std.mem.indexOfScalar(u8, value, 0) != null) {
        try status.setBindingDiagnostic(diagnostic_store, diagnostic_message);
        return error.InvalidString;
    }
    return allocator.dupeZ(u8, value);
}

const TestCustomGeometryCallbackState = struct {
    fetch_count: usize = 0,
    cancel_count: usize = 0,
    last_tile: CanonicalTileId = .{ .z = 0, .x = 0, .y = 0 },
};

fn testFetchCustomGeometryTile(context: ?*anyopaque, tile_id: CanonicalTileId) void {
    const test_state: *TestCustomGeometryCallbackState = @ptrCast(@alignCast(context.?));
    test_state.fetch_count += 1;
    test_state.last_tile = tile_id;
}

fn testCancelCustomGeometryTile(context: ?*anyopaque, tile_id: CanonicalTileId) void {
    const test_state: *TestCustomGeometryCallbackState = @ptrCast(@alignCast(context.?));
    test_state.cancel_count += 1;
    test_state.last_tile = tile_id;
}

test "custom geometry trampolines route semantic tile ids" {
    var test_state = TestCustomGeometryCallbackState{};
    var source_state = CustomGeometrySourceState{
        .source_id = "custom"[0..],
        .fetch_tile = testFetchCustomGeometryTile,
        .cancel_tile = testCancelCustomGeometryTile,
        .context = &test_state,
        .active_upcalls = std.atomic.Value(usize).init(0),
    };
    try registerLiveCustomGeometrySourceState(&source_state);
    defer unregisterLiveCustomGeometrySourceState(&source_state);

    customGeometryFetchTileTrampoline(&source_state, .{ .z = 3, .x = 4, .y = 5 });
    try std.testing.expectEqual(@as(usize, 1), test_state.fetch_count);
    try std.testing.expectEqual(CanonicalTileId{ .z = 3, .x = 4, .y = 5 }, test_state.last_tile);

    customGeometryCancelTileTrampoline(&source_state, .{ .z = 6, .x = 7, .y = 8 });
    try std.testing.expectEqual(@as(usize, 1), test_state.cancel_count);
    try std.testing.expectEqual(CanonicalTileId{ .z = 6, .x = 7, .y = 8 }, test_state.last_tile);

    source_state.cancel_tile = null;
    customGeometryCancelTileTrampoline(&source_state, .{ .z = 9, .x = 10, .y = 11 });
    try std.testing.expectEqual(@as(usize, 1), test_state.cancel_count);

    retireLiveCustomGeometrySourceState(&source_state);
    customGeometryFetchTileTrampoline(&source_state, .{ .z = 12, .x = 13, .y = 14 });
    try std.testing.expectEqual(@as(usize, 1), test_state.fetch_count);
    try std.testing.expectEqual(@as(usize, 0), source_state.active_upcalls.load(.seq_cst));
}

const test_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-test",
    \\  "sources": {
    \\    "point": {
    \\      "type": "geojson",
    \\      "data": {"type":"FeatureCollection","features":[]}
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
    \\  ]
    \\}
;

fn createLoadedMapForTesting(runtime: *RuntimeHandle) !MapHandle {
    var map = try MapHandle.create(runtime, .{});
    errdefer map.close() catch {};
    try map.setStyleJson(std.testing.allocator, test_style_json);
    try std.testing.expect(try waitForRuntimeEventForTesting(runtime, .map_style_loaded));
    return map;
}

fn waitForRuntimeEventForTesting(runtime: *RuntimeHandle, event_type: runtime_module.RuntimeEventType) !bool {
    var attempts: usize = 0;
    while (attempts < 200) : (attempts += 1) {
        try runtime.runOnce();
        while (try runtime.pollEvent(std.testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            if (std.meta.eql(owned_event.event_type, event_type)) return true;
        }
        try std.testing.io.sleep(.fromMilliseconds(10), .awake);
    }
    return false;
}

fn testStyleJsonProvider(
    context: ?*anyopaque,
    request: runtime_module.ResourceRequest,
    maybe_handle: ?runtime_module.ResourceRequestHandle,
) runtime_module.ResourceProviderDecision {
    _ = context;
    if (!std.mem.eql(u8, request.url, "custom://style.json")) return .pass_through;
    const handle = maybe_handle orelse return .pass_through;
    handle.complete(.{ .bytes = test_style_json }) catch {
        handle.release();
        return .pass_through;
    };
    handle.release();
    return .handle;
}

test "custom geometry source state is released on source removal" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    defer map.close() catch @panic("map close failed");

    var state = TestCustomGeometryCallbackState{};
    try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .context = &state,
    });
    try std.testing.expectEqual(@as(usize, 1), try customGeometrySourceCountForTesting(&map));

    try std.testing.expect(try map.removeStyleSource(std.testing.allocator, "custom"));
    try std.testing.expectEqual(@as(usize, 0), try customGeometrySourceCountForTesting(&map));
}

test "custom geometry source states are released on inline style replacement" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    defer map.close() catch @panic("map close failed");

    var state = TestCustomGeometryCallbackState{};
    try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .context = &state,
    });
    try std.testing.expectEqual(@as(usize, 1), try customGeometrySourceCountForTesting(&map));

    try map.setStyleJson(std.testing.allocator, test_style_json);
    try std.testing.expectEqual(@as(usize, 0), try customGeometrySourceCountForTesting(&map));
}

test "custom geometry source states are released after style URL load detaches them" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    try runtime.setResourceProvider(.{ .handler = testStyleJsonProvider });

    var map = try createLoadedMapForTesting(&runtime);
    defer map.close() catch @panic("map close failed");

    var state = TestCustomGeometryCallbackState{};
    try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .context = &state,
    });
    try std.testing.expectEqual(@as(usize, 1), try customGeometrySourceCountForTesting(&map));

    try map.setStyleUrl(std.testing.allocator, "custom://style.json");
    try std.testing.expectEqual(@as(usize, 1), try customGeometrySourceCountForTesting(&map));
    try std.testing.expect(try waitForRuntimeEventForTesting(&runtime, .map_style_loaded));
    try std.testing.expectEqual(@as(usize, 0), try customGeometrySourceCountForTesting(&map));
}
