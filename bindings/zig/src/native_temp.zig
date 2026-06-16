const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");
const values = @import("values.zig");

pub const TempStorage = struct {
    arena: std.heap.ArenaAllocator,
    diagnostic_store: ?*diagnostics.DiagnosticStore = null,

    pub fn init(allocator: std.mem.Allocator) TempStorage {
        return .{ .arena = std.heap.ArenaAllocator.init(allocator) };
    }

    pub fn initWithDiagnostics(
        allocator: std.mem.Allocator,
        diagnostic_store: ?*diagnostics.DiagnosticStore,
    ) TempStorage {
        return .{
            .arena = std.heap.ArenaAllocator.init(allocator),
            .diagnostic_store = diagnostic_store,
        };
    }

    pub fn deinit(self: *TempStorage) void {
        self.arena.deinit();
    }

    pub fn latLngs(self: *TempStorage, coordinates: []const values.LatLng) std.mem.Allocator.Error![]const c.mln_lat_lng {
        const raw = try self.arena.allocator().alloc(c.mln_lat_lng, coordinates.len);
        for (coordinates, raw) |coordinate, *out| out.* = values.latLngToNative(coordinate);
        return raw;
    }

    pub fn screenPoints(self: *TempStorage, points: []const values.ScreenPoint) std.mem.Allocator.Error![]const c.mln_screen_point {
        const raw = try self.arena.allocator().alloc(c.mln_screen_point, points.len);
        for (points, raw) |point, *out| out.* = values.screenPointToNative(point);
        return raw;
    }

    pub fn stringView(_: *TempStorage, value: []const u8) status.Error!c.mln_string_view {
        return .{ .data = if (value.len == 0) null else value.ptr, .size = value.len };
    }

    pub fn stringViews(self: *TempStorage, source: []const []const u8) status.Error![]const c.mln_string_view {
        const raw = try self.arena.allocator().alloc(c.mln_string_view, source.len);
        for (source, raw) |value, *out| out.* = try self.stringView(value);
        return raw;
    }

    pub fn jsonValue(self: *TempStorage, value: values.JsonValue) status.Error!*const c.mln_json_value {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.create(c.mln_json_value);
        raw.* = try self.jsonValueStruct(value);
        return raw;
    }

    fn jsonValueStruct(self: *TempStorage, value: values.JsonValue) status.Error!c.mln_json_value {
        const arena_allocator = self.arena.allocator();
        var raw: c.mln_json_value = undefined;
        raw.size = @sizeOf(c.mln_json_value);
        switch (value) {
            .null => {
                raw.type = c.MLN_JSON_VALUE_TYPE_NULL;
                raw.data = .{ .uint_value = 0 };
            },
            .bool => |bool_value| {
                raw.type = c.MLN_JSON_VALUE_TYPE_BOOL;
                raw.data = .{ .bool_value = bool_value };
            },
            .uint => |uint_value| {
                raw.type = c.MLN_JSON_VALUE_TYPE_UINT;
                raw.data = .{ .uint_value = uint_value };
            },
            .int => |int_value| {
                raw.type = c.MLN_JSON_VALUE_TYPE_INT;
                raw.data = .{ .int_value = int_value };
            },
            .double => |double_value| {
                raw.type = c.MLN_JSON_VALUE_TYPE_DOUBLE;
                raw.data = .{ .double_value = double_value };
            },
            .string => |string_value| {
                raw.type = c.MLN_JSON_VALUE_TYPE_STRING;
                raw.data = .{ .string_value = try self.stringView(string_value) };
            },
            .array => |items| {
                const raw_items = try arena_allocator.alloc(c.mln_json_value, items.len);
                for (items, raw_items) |item, *raw_item| raw_item.* = (try self.jsonValue(item)).*;
                raw.type = c.MLN_JSON_VALUE_TYPE_ARRAY;
                raw.data = .{ .array_value = .{ .values = if (raw_items.len == 0) null else raw_items.ptr, .value_count = raw_items.len } };
            },
            .object => |members| {
                const raw_members = try arena_allocator.alloc(c.mln_json_member, members.len);
                for (members, raw_members) |member, *raw_member| {
                    raw_member.* = .{ .key = try self.stringView(member.key), .value = try self.jsonValue(member.value) };
                }
                raw.type = c.MLN_JSON_VALUE_TYPE_OBJECT;
                raw.data = .{ .object_value = .{ .members = if (raw_members.len == 0) null else raw_members.ptr, .member_count = raw_members.len } };
            },
        }
        return raw;
    }

    pub fn geometry(self: *TempStorage, value: values.Geometry) status.Error!*const c.mln_geometry {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.create(c.mln_geometry);
        raw.* = try self.geometryStruct(value);
        return raw;
    }

    fn geometryStruct(self: *TempStorage, value: values.Geometry) status.Error!c.mln_geometry {
        var raw: c.mln_geometry = undefined;
        raw.size = @sizeOf(c.mln_geometry);
        switch (value) {
            .empty => {
                raw.type = c.MLN_GEOMETRY_TYPE_EMPTY;
                raw.data = .{ .point = .{ .latitude = 0, .longitude = 0 } };
            },
            .point => |point| {
                raw.type = c.MLN_GEOMETRY_TYPE_POINT;
                raw.data = .{ .point = values.latLngToNative(point) };
            },
            .line_string => |coordinates| {
                raw.type = c.MLN_GEOMETRY_TYPE_LINE_STRING;
                raw.data = .{ .line_string = try self.coordinateSpan(coordinates) };
            },
            .polygon => |rings| {
                raw.type = c.MLN_GEOMETRY_TYPE_POLYGON;
                const raw_rings = try self.coordinateSpans(rings);
                raw.data = .{ .polygon = .{ .rings = if (raw_rings.len == 0) null else raw_rings.ptr, .ring_count = raw_rings.len } };
            },
            .multi_point => |coordinates| {
                raw.type = c.MLN_GEOMETRY_TYPE_MULTI_POINT;
                raw.data = .{ .multi_point = try self.coordinateSpan(coordinates) };
            },
            .multi_line_string => |lines| {
                raw.type = c.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING;
                const raw_lines = try self.coordinateSpans(lines);
                raw.data = .{ .multi_line_string = .{ .lines = if (raw_lines.len == 0) null else raw_lines.ptr, .line_count = raw_lines.len } };
            },
            .multi_polygon => |polygon_values| {
                raw.type = c.MLN_GEOMETRY_TYPE_MULTI_POLYGON;
                const raw_polygons = try self.polygons(polygon_values);
                raw.data = .{ .multi_polygon = .{ .polygons = if (raw_polygons.len == 0) null else raw_polygons.ptr, .polygon_count = raw_polygons.len } };
            },
            .collection => |geometry_values| {
                raw.type = c.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION;
                const raw_geometries = try self.geometries(geometry_values);
                raw.data = .{ .geometry_collection = .{ .geometries = if (raw_geometries.len == 0) null else raw_geometries.ptr, .geometry_count = raw_geometries.len } };
            },
        }
        return raw;
    }

    pub fn geoJson(self: *TempStorage, value: values.GeoJson) status.Error!*const c.mln_geojson {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.create(c.mln_geojson);
        raw.size = @sizeOf(c.mln_geojson);
        switch (value) {
            .geometry => |geometry_value| {
                raw.type = c.MLN_GEOJSON_TYPE_GEOMETRY;
                raw.data = .{ .geometry = try self.geometry(geometry_value) };
            },
            .feature => |feature_value| {
                raw.type = c.MLN_GEOJSON_TYPE_FEATURE;
                raw.data = .{ .feature = try self.feature(feature_value) };
            },
            .feature_collection => |feature_values| {
                const raw_features = try self.features(feature_values);
                raw.type = c.MLN_GEOJSON_TYPE_FEATURE_COLLECTION;
                raw.data = .{ .feature_collection = .{ .features = if (raw_features.len == 0) null else raw_features.ptr, .feature_count = raw_features.len } };
            },
        }
        return raw;
    }

    pub fn offlineRegionDefinition(self: *TempStorage, value: anytype) status.Error!*const c.mln_offline_region_definition {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.create(c.mln_offline_region_definition);
        raw.size = @sizeOf(c.mln_offline_region_definition);
        switch (value) {
            .tile_pyramid => |definition| {
                raw.type = c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
                raw.data = .{ .tile_pyramid = .{
                    .size = @sizeOf(c.mln_offline_tile_pyramid_region_definition),
                    .style_url = (try self.nulTerminatedString(definition.style_url, "offline region style_url contains embedded NUL")).ptr,
                    .bounds = values.latLngBoundsToNative(definition.bounds),
                    .min_zoom = definition.min_zoom,
                    .max_zoom = definition.max_zoom,
                    .pixel_ratio = definition.pixel_ratio,
                    .include_ideographs = definition.include_ideographs,
                } };
            },
            .geometry => |definition| {
                raw.type = c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
                raw.data = .{ .geometry = .{
                    .size = @sizeOf(c.mln_offline_geometry_region_definition),
                    .style_url = (try self.nulTerminatedString(definition.style_url, "offline region style_url contains embedded NUL")).ptr,
                    .geometry = try self.geometry(definition.geometry),
                    .min_zoom = definition.min_zoom,
                    .max_zoom = definition.max_zoom,
                    .pixel_ratio = definition.pixel_ratio,
                    .include_ideographs = definition.include_ideographs,
                } };
            },
        }
        return raw;
    }

    fn nulTerminatedString(self: *TempStorage, value: []const u8, diagnostic_message: []const u8) status.Error![:0]u8 {
        if (std.mem.indexOfScalar(u8, value, 0) != null) {
            try status.setBindingDiagnostic(self.diagnostic_store, diagnostic_message);
            return error.InvalidString;
        }
        return self.arena.allocator().dupeZ(u8, value);
    }

    fn coordinateSpan(self: *TempStorage, coordinates: []const values.LatLng) status.Error!c.mln_coordinate_span {
        const arena_allocator = self.arena.allocator();
        const raw_coordinates = try arena_allocator.alloc(c.mln_lat_lng, coordinates.len);
        for (coordinates, raw_coordinates) |coordinate, *raw_coordinate| raw_coordinate.* = values.latLngToNative(coordinate);
        return .{ .coordinates = if (raw_coordinates.len == 0) null else raw_coordinates.ptr, .coordinate_count = raw_coordinates.len };
    }

    fn coordinateSpans(self: *TempStorage, rings: []const []const values.LatLng) status.Error![]const c.mln_coordinate_span {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.alloc(c.mln_coordinate_span, rings.len);
        for (rings, raw) |ring, *out| out.* = try self.coordinateSpan(ring);
        return raw;
    }

    fn polygons(self: *TempStorage, polygon_values: []const []const []const values.LatLng) status.Error![]const c.mln_polygon_geometry {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.alloc(c.mln_polygon_geometry, polygon_values.len);
        for (polygon_values, raw) |polygon_value, *out| {
            const rings = try self.coordinateSpans(polygon_value);
            out.* = .{ .rings = if (rings.len == 0) null else rings.ptr, .ring_count = rings.len };
        }
        return raw;
    }

    fn geometries(self: *TempStorage, geometry_values: []const values.Geometry) status.Error![]const c.mln_geometry {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.alloc(c.mln_geometry, geometry_values.len);
        for (geometry_values, raw) |geometry_value, *out| out.* = try self.geometryStruct(geometry_value);
        return raw;
    }

    pub fn feature(self: *TempStorage, value: values.Feature) status.Error!*const c.mln_feature {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.create(c.mln_feature);
        raw.* = try self.featureStruct(value);
        return raw;
    }

    fn featureStruct(self: *TempStorage, value: values.Feature) status.Error!c.mln_feature {
        var raw: c.mln_feature = .{
            .size = @sizeOf(c.mln_feature),
            .geometry = try self.geometry(value.geometry),
            .properties = null,
            .property_count = 0,
            .identifier_type = c.MLN_FEATURE_IDENTIFIER_TYPE_NULL,
            .identifier = .{ .uint_value = 0 },
        };
        if (value.properties.len > 0) {
            const properties = try self.jsonMembers(value.properties);
            raw.properties = properties.ptr;
            raw.property_count = properties.len;
        }
        switch (value.identifier) {
            .null => {},
            .uint => |identifier| {
                raw.identifier_type = c.MLN_FEATURE_IDENTIFIER_TYPE_UINT;
                raw.identifier = .{ .uint_value = identifier };
            },
            .int => |identifier| {
                raw.identifier_type = c.MLN_FEATURE_IDENTIFIER_TYPE_INT;
                raw.identifier = .{ .int_value = identifier };
            },
            .double => |identifier| {
                raw.identifier_type = c.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE;
                raw.identifier = .{ .double_value = identifier };
            },
            .string => |identifier| {
                raw.identifier_type = c.MLN_FEATURE_IDENTIFIER_TYPE_STRING;
                raw.identifier = .{ .string_value = try self.stringView(identifier) };
            },
        }
        return raw;
    }

    fn features(self: *TempStorage, feature_values: []const values.Feature) status.Error![]const c.mln_feature {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.alloc(c.mln_feature, feature_values.len);
        for (feature_values, raw) |feature_value, *out| out.* = try self.featureStruct(feature_value);
        return raw;
    }

    fn jsonMembers(self: *TempStorage, members: []const values.JsonMember) status.Error![]const c.mln_json_member {
        const arena_allocator = self.arena.allocator();
        const raw = try arena_allocator.alloc(c.mln_json_member, members.len);
        for (members, raw) |member, *out| out.* = .{ .key = try self.stringView(member.key), .value = try self.jsonValue(member.value) };
        return raw;
    }
};
