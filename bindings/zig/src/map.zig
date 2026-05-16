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
    fetch_tile: CustomGeometrySourceTileCallback,
    cancel_tile: ?CustomGeometrySourceTileCallback,
    context: ?*anyopaque,
    retired: std.atomic.Value(bool),
    active_upcalls: std.atomic.Value(usize),
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

pub const MapHandle = struct {
    native: ?*NativeMap,
    runtime_registry: *runtime_module.RuntimeRegistry,
    id_value: values.MapId,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    custom_geometry_sources: std.ArrayList(*CustomGeometrySourceState),

    pub fn create(runtime: *RuntimeHandle, options: MapOptions) status.Error!MapHandle {
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

        const runtime_registry = try runtime_module.registry(runtime);
        const map_id = try runtime_module.registerMap(runtime_registry, map.?);
        errdefer runtime_module.unregisterMap(runtime_registry, map.?);
        return .{
            .native = @ptrCast(map.?),
            .runtime_registry = runtime_registry,
            .id_value = map_id,
            .diagnostic_store = diagnostic_store,
            .custom_geometry_sources = .empty,
        };
    }

    pub fn id(self: *MapHandle) status.BindingError!values.MapId {
        const map_state = self;
        _ = map_state.native orelse return error.ClosedHandle;
        return map_state.id_value;
    }

    pub fn setStyleJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        json: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const json_z = try nulTerminated(allocator, json);
        defer allocator.free(json_z);
        try status.checkStatus(c.mln_map_set_style_json(native_map, json_z.ptr), self.diagnostic_store);
    }

    pub fn setStyleUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        url: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const url_z = try nulTerminated(allocator, url);
        defer allocator.free(url_z);
        try status.checkStatus(c.mln_map_set_style_url(native_map, url_z.ptr), self.diagnostic_store);
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, self.diagnostic_store);
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, self.diagnostic_store);
    }

    pub fn listStyleSourceIds(self: *MapHandle, allocator: std.mem.Allocator) status.Error!values.StringList {
        var list: ?*c.mln_style_id_list = null;
        try status.checkStatus(c.mln_map_list_style_source_ids(try native(self), &list), self.diagnostic_store);
        defer if (list) |handle| c.mln_style_id_list_destroy(handle);
        return try copyStyleIdList(allocator, list.?, self.diagnostic_store);
    }

    pub fn listStyleLayerIds(self: *MapHandle, allocator: std.mem.Allocator) status.Error!values.StringList {
        var list: ?*c.mln_style_id_list = null;
        try status.checkStatus(c.mln_map_list_style_layer_ids(try native(self), &list), self.diagnostic_store);
        defer if (list) |handle| c.mln_style_id_list_destroy(handle);
        return try copyStyleIdList(allocator, list.?, self.diagnostic_store);
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
            self.diagnostic_store,
        );
    }

    pub fn removeStyleSource(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var removed = false;
        try status.checkStatus(
            c.mln_map_remove_style_source(try native(self), try temp.stringView(source_id), &removed),
            self.diagnostic_store,
        );
        return removed;
    }

    pub fn styleSourceExists(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var exists = false;
        try status.checkStatus(
            c.mln_map_style_source_exists(try native(self), try temp.stringView(source_id), &exists),
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
    }

    pub fn removeStyleLayer(self: *MapHandle, layer_id: []const u8) status.Error!bool {
        var removed = false;
        try status.checkStatus(
            c.mln_map_remove_style_layer(try native(self), stringView(layer_id), &removed),
            self.diagnostic_store,
        );
        return removed;
    }

    pub fn styleLayerExists(self: *MapHandle, layer_id: []const u8) status.Error!bool {
        var exists = false;
        try status.checkStatus(
            c.mln_map_style_layer_exists(try native(self), stringView(layer_id), &exists),
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        if (!found) return null;
        return try copyJsonSnapshot(allocator, snapshot, self.diagnostic_store) orelse error.NativeError;
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
        defer if (snapshot) |handle| c.mln_json_snapshot_destroy(handle);
        return try copyJsonSnapshot(allocator, snapshot, self.diagnostic_store);
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
    }

    pub fn removeStyleImage(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var removed = false;
        try status.checkStatus(
            c.mln_map_remove_style_image(try native(self), try temp.stringView(image_id), &removed),
            self.diagnostic_store,
        );
        return removed;
    }

    pub fn styleImageExists(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!bool {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var exists = false;
        try status.checkStatus(
            c.mln_map_style_image_exists(try native(self), try temp.stringView(image_id), &exists),
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
    }

    pub fn addCustomGeometrySource(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        options: CustomGeometrySourceOptions,
    ) status.Error!void {
        const map_state = self;
        const source_state = try std.heap.smp_allocator.create(CustomGeometrySourceState);
        source_state.* = .{
            .fetch_tile = options.fetch_tile,
            .cancel_tile = options.cancel_tile,
            .context = options.context,
            .retired = std.atomic.Value(bool).init(false),
            .active_upcalls = std.atomic.Value(usize).init(0),
        };
        errdefer std.heap.smp_allocator.destroy(source_state);

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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
    }

    pub fn requestRepaint(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_request_repaint(try native(self)), self.diagnostic_store);
    }

    pub fn setDebugOptions(self: *MapHandle, options: values.DebugOptions) status.Error!void {
        try status.checkStatus(c.mln_map_set_debug_options(try native(self), values.debugOptionsToNative(options)), self.diagnostic_store);
    }

    pub fn getDebugOptions(self: *MapHandle) status.Error!values.DebugOptions {
        var options: u32 = 0;
        try status.checkStatus(c.mln_map_get_debug_options(try native(self), &options), self.diagnostic_store);
        return values.debugOptionsFromNative(options);
    }

    pub fn setRenderingStatsViewEnabled(self: *MapHandle, enabled: bool) status.Error!void {
        try status.checkStatus(c.mln_map_set_rendering_stats_view_enabled(try native(self), enabled), self.diagnostic_store);
    }

    pub fn getRenderingStatsViewEnabled(self: *MapHandle) status.Error!bool {
        var enabled = false;
        try status.checkStatus(c.mln_map_get_rendering_stats_view_enabled(try native(self), &enabled), self.diagnostic_store);
        return enabled;
    }

    pub fn isFullyLoaded(self: *MapHandle) status.Error!bool {
        var loaded = false;
        try status.checkStatus(c.mln_map_is_fully_loaded(try native(self), &loaded), self.diagnostic_store);
        return loaded;
    }

    pub fn dumpDebugLogs(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_dump_debug_logs(try native(self)), self.diagnostic_store);
    }

    pub fn setViewportOptions(self: *MapHandle, options: values.ViewportOptions) status.Error!void {
        var raw_options = values.viewportOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_viewport_options(try native(self), &raw_options), self.diagnostic_store);
    }

    pub fn getViewportOptions(self: *MapHandle) status.Error!values.ViewportOptions {
        var options = c.mln_map_viewport_options_default();
        try status.checkStatus(c.mln_map_get_viewport_options(try native(self), &options), self.diagnostic_store);
        return try values.viewportOptionsFromNative(options);
    }

    pub fn setTileOptions(self: *MapHandle, options: values.TileOptions) status.Error!void {
        var raw_options = values.tileOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_tile_options(try native(self), &raw_options), self.diagnostic_store);
    }

    pub fn getTileOptions(self: *MapHandle) status.Error!values.TileOptions {
        var options = c.mln_map_tile_options_default();
        try status.checkStatus(c.mln_map_get_tile_options(try native(self), &options), self.diagnostic_store);
        return try values.tileOptionsFromNative(options);
    }

    pub fn getCamera(self: *MapHandle) status.Error!values.CameraOptions {
        var camera = c.mln_camera_options_default();
        try status.checkStatus(c.mln_map_get_camera(try native(self), &camera), self.diagnostic_store);
        return values.cameraOptionsFromNative(camera);
    }

    pub fn jumpTo(self: *MapHandle, camera: values.CameraOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        try status.checkStatus(c.mln_map_jump_to(try native(self), &raw_camera), self.diagnostic_store);
    }

    pub fn easeTo(self: *MapHandle, camera: values.CameraOptions, animation: ?values.AnimationOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_ease_to(try native(self), &raw_camera, animation_ptr), self.diagnostic_store);
    }

    pub fn flyTo(self: *MapHandle, camera: values.CameraOptions, animation: ?values.AnimationOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_fly_to(try native(self), &raw_camera, animation_ptr), self.diagnostic_store);
    }

    pub fn moveBy(self: *MapHandle, delta_x: f64, delta_y: f64) status.Error!void {
        try status.checkStatus(c.mln_map_move_by(try native(self), delta_x, delta_y), self.diagnostic_store);
    }

    pub fn moveByAnimated(self: *MapHandle, delta_x: f64, delta_y: f64, animation: ?values.AnimationOptions) status.Error!void {
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_move_by_animated(try native(self), delta_x, delta_y, animation_ptr), self.diagnostic_store);
    }

    pub fn scaleBy(self: *MapHandle, scale: f64, anchor: ?values.ScreenPoint) status.Error!void {
        var raw_anchor = if (anchor) |point| values.screenPointToNative(point) else undefined;
        const anchor_ptr = if (anchor != null) &raw_anchor else null;
        try status.checkStatus(c.mln_map_scale_by(try native(self), scale, anchor_ptr), self.diagnostic_store);
    }

    pub fn scaleByAnimated(self: *MapHandle, scale: f64, anchor: ?values.ScreenPoint, animation: ?values.AnimationOptions) status.Error!void {
        var raw_anchor = if (anchor) |point| values.screenPointToNative(point) else undefined;
        const anchor_ptr = if (anchor != null) &raw_anchor else null;
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_scale_by_animated(try native(self), scale, anchor_ptr, animation_ptr), self.diagnostic_store);
    }

    pub fn rotateBy(self: *MapHandle, first: values.ScreenPoint, second: values.ScreenPoint) status.Error!void {
        try status.checkStatus(
            c.mln_map_rotate_by(try native(self), values.screenPointToNative(first), values.screenPointToNative(second)),
            self.diagnostic_store,
        );
    }

    pub fn rotateByAnimated(self: *MapHandle, first: values.ScreenPoint, second: values.ScreenPoint, animation: ?values.AnimationOptions) status.Error!void {
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(
            c.mln_map_rotate_by_animated(try native(self), values.screenPointToNative(first), values.screenPointToNative(second), animation_ptr),
            self.diagnostic_store,
        );
    }

    pub fn pitchBy(self: *MapHandle, pitch: f64) status.Error!void {
        try status.checkStatus(c.mln_map_pitch_by(try native(self), pitch), self.diagnostic_store);
    }

    pub fn pitchByAnimated(self: *MapHandle, pitch: f64, animation: ?values.AnimationOptions) status.Error!void {
        var raw_animation = if (animation) |options| values.animationOptionsToNative(options) else undefined;
        const animation_ptr = if (animation != null) &raw_animation else null;
        try status.checkStatus(c.mln_map_pitch_by_animated(try native(self), pitch, animation_ptr), self.diagnostic_store);
    }

    pub fn cancelTransitions(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_cancel_transitions(try native(self)), self.diagnostic_store);
    }

    pub fn requestStillImage(self: *MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_request_still_image(try native(self)), self.diagnostic_store);
    }

    pub fn setProjectionMode(self: *MapHandle, mode: values.ProjectionMode) status.Error!void {
        var raw_mode = values.projectionModeToNative(mode);
        try status.checkStatus(c.mln_map_set_projection_mode(try native(self), &raw_mode), self.diagnostic_store);
    }

    pub fn getProjectionMode(self: *MapHandle) status.Error!values.ProjectionMode {
        var mode = c.mln_projection_mode_default();
        try status.checkStatus(c.mln_map_get_projection_mode(try native(self), &mode), self.diagnostic_store);
        return values.projectionModeFromNative(mode);
    }

    pub fn pixelForLatLng(self: *MapHandle, coordinate: values.LatLng) status.Error!values.ScreenPoint {
        var point: c.mln_screen_point = undefined;
        try status.checkStatus(
            c.mln_map_pixel_for_lat_lng(try native(self), values.latLngToNative(coordinate), &point),
            self.diagnostic_store,
        );
        return values.screenPointFromNative(point);
    }

    pub fn latLngForPixel(self: *MapHandle, point: values.ScreenPoint) status.Error!values.LatLng {
        var coordinate: c.mln_lat_lng = undefined;
        try status.checkStatus(
            c.mln_map_lat_lng_for_pixel(try native(self), values.screenPointToNative(point), &coordinate),
            self.diagnostic_store,
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
            try status.checkStatus(c.mln_map_pixels_for_lat_lngs(try native(self), null, 0, null), self.diagnostic_store);
            return;
        }
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        const raw_points = try allocator.alloc(c.mln_screen_point, out_points.len);
        defer allocator.free(raw_points);
        try status.checkStatus(
            c.mln_map_pixels_for_lat_lngs(try native(self), raw_coordinates.ptr, raw_coordinates.len, raw_points.ptr),
            self.diagnostic_store,
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
            try status.checkStatus(c.mln_map_lat_lngs_for_pixels(try native(self), null, 0, null), self.diagnostic_store);
            return;
        }
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_points = try temp.screenPoints(points);
        const raw_coordinates = try allocator.alloc(c.mln_lat_lng, out_coordinates.len);
        defer allocator.free(raw_coordinates);
        try status.checkStatus(
            c.mln_map_lat_lngs_for_pixels(try native(self), raw_points.ptr, raw_points.len, raw_coordinates.ptr),
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
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
            self.diagnostic_store,
        );
        return values.cameraOptionsFromNative(camera);
    }

    pub fn latLngBoundsForCamera(self: *MapHandle, camera: values.CameraOptions) status.Error!values.LatLngBounds {
        var raw_camera = values.cameraOptionsToNative(camera);
        var bounds: c.mln_lat_lng_bounds = undefined;
        try status.checkStatus(c.mln_map_lat_lng_bounds_for_camera(try native(self), &raw_camera, &bounds), self.diagnostic_store);
        return values.latLngBoundsFromNative(bounds);
    }

    pub fn latLngBoundsForCameraUnwrapped(self: *MapHandle, camera: values.CameraOptions) status.Error!values.LatLngBounds {
        var raw_camera = values.cameraOptionsToNative(camera);
        var bounds: c.mln_lat_lng_bounds = undefined;
        try status.checkStatus(c.mln_map_lat_lng_bounds_for_camera_unwrapped(try native(self), &raw_camera, &bounds), self.diagnostic_store);
        return values.latLngBoundsFromNative(bounds);
    }

    pub fn getBounds(self: *MapHandle) status.Error!values.BoundOptions {
        var raw_options = c.mln_bound_options_default();
        try status.checkStatus(c.mln_map_get_bounds(try native(self), &raw_options), self.diagnostic_store);
        return values.boundOptionsFromNative(raw_options);
    }

    pub fn setBounds(self: *MapHandle, options: values.BoundOptions) status.Error!void {
        var raw_options = values.boundOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_bounds(try native(self), &raw_options), self.diagnostic_store);
    }

    pub fn getFreeCameraOptions(self: *MapHandle) status.Error!values.FreeCameraOptions {
        var raw_options = c.mln_free_camera_options_default();
        try status.checkStatus(c.mln_map_get_free_camera_options(try native(self), &raw_options), self.diagnostic_store);
        return values.freeCameraOptionsFromNative(raw_options);
    }

    pub fn setFreeCameraOptions(self: *MapHandle, options: values.FreeCameraOptions) status.Error!void {
        var raw_options = values.freeCameraOptionsToNative(options);
        try status.checkStatus(c.mln_map_set_free_camera_options(try native(self), &raw_options), self.diagnostic_store);
    }

    pub fn close(self: *MapHandle) status.Error!void {
        const map: *c.mln_map = @ptrCast(self.native orelse return);
        try status.checkStatus(c.mln_map_destroy(map), self.diagnostic_store);
        runtime_module.unregisterMap(self.runtime_registry, map);
        freeCustomGeometrySourceStates(self);
        self.native = null;
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
    if (source_state.retired.load(.seq_cst)) return false;
    _ = source_state.active_upcalls.fetchAdd(1, .seq_cst);
    if (source_state.retired.load(.seq_cst)) {
        endCustomGeometryUpcall(source_state);
        return false;
    }
    return true;
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

fn freeCustomGeometrySourceStates(map_state: *MapHandle) void {
    for (map_state.custom_geometry_sources.items) |source_state| {
        source_state.retired.store(true, .seq_cst);
    }
    for (map_state.custom_geometry_sources.items) |source_state| {
        while (source_state.active_upcalls.load(.seq_cst) != 0) {
            std.Thread.yield() catch {};
        }
        std.heap.smp_allocator.destroy(source_state);
    }
    map_state.custom_geometry_sources.deinit(std.heap.smp_allocator);
    map_state.custom_geometry_sources = .empty;
}

pub fn native(handle: *MapHandle) status.BindingError!*c.mln_map {
    return @ptrCast(handle.native orelse return error.ClosedHandle);
}

pub fn diagnosticStore(handle: *MapHandle) ?*diagnostics.DiagnosticStore {
    return handle.diagnostic_store;
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

fn nulTerminated(allocator: std.mem.Allocator, value: []const u8) status.Error![:0]u8 {
    if (std.mem.indexOfScalar(u8, value, 0) != null) return error.InvalidString;
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
        .fetch_tile = testFetchCustomGeometryTile,
        .cancel_tile = testCancelCustomGeometryTile,
        .context = &test_state,
        .retired = std.atomic.Value(bool).init(false),
        .active_upcalls = std.atomic.Value(usize).init(0),
    };

    customGeometryFetchTileTrampoline(&source_state, .{ .z = 3, .x = 4, .y = 5 });
    try std.testing.expectEqual(@as(usize, 1), test_state.fetch_count);
    try std.testing.expectEqual(CanonicalTileId{ .z = 3, .x = 4, .y = 5 }, test_state.last_tile);

    customGeometryCancelTileTrampoline(&source_state, .{ .z = 6, .x = 7, .y = 8 });
    try std.testing.expectEqual(@as(usize, 1), test_state.cancel_count);
    try std.testing.expectEqual(CanonicalTileId{ .z = 6, .x = 7, .y = 8 }, test_state.last_tile);

    source_state.cancel_tile = null;
    customGeometryCancelTileTrampoline(&source_state, .{ .z = 9, .x = 10, .y = 11 });
    try std.testing.expectEqual(@as(usize, 1), test_state.cancel_count);

    source_state.retired.store(true, .seq_cst);
    customGeometryFetchTileTrampoline(&source_state, .{ .z = 12, .x = 13, .y = 14 });
    try std.testing.expectEqual(@as(usize, 1), test_state.fetch_count);
    try std.testing.expectEqual(@as(usize, 0), source_state.active_upcalls.load(.seq_cst));
}
