const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.pump(0);
        while (try runtime.pollEvent(testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            if (std.meta.eql(owned_event.event_type, event_type)) return true;
        }
        try std.Thread.yield();
    }
    return false;
}

fn createLoadedMap(runtime: *maplibre.RuntimeHandle) !maplibre.MapHandle {
    var map = try maplibre.MapHandle.create(runtime, .{});
    errdefer map.close() catch {};
    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
    return map;
}

fn expectListContains(list: maplibre.StringList, expected: []const u8) !void {
    for (list.items) |item| {
        if (std.mem.eql(u8, item, expected)) return;
    }
    return error.MissingListEntry;
}

test "GeoJSON descriptors add and update sources through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const empty_features = [_]maplibre.Feature{};
    try map.addGeoJsonSourceData(testing.allocator, "empty", .{ .feature_collection = empty_features[0..] }, null);
    try testing.expect(try map.styleSourceExists(testing.allocator, "empty"));

    var source_ids = try map.listStyleSourceIds(testing.allocator);
    defer source_ids.deinit();
    try expectListContains(source_ids, "empty");

    const properties = [_]maplibre.JsonMember{
        .{ .key = "name", .value = .{ .string = "San Francisco" } },
        .{ .key = "visible", .value = .{ .bool = true } },
    };
    const features = [_]maplibre.Feature{.{
        .geometry = .{ .point = .{ .latitude = 37.7749, .longitude = -122.4194 } },
        .properties = properties[0..],
        .identifier = .{ .string = "sf" },
    }};
    try map.setGeoJsonSourceData(testing.allocator, "empty", .{ .feature_collection = features[0..] });
    try map.setGeoJsonSourceUrl(testing.allocator, "empty", "https://example.com/data.geojson");

    try map.addGeoJsonSourceUrl(testing.allocator, "geo-url", "https://example.com/initial.geojson", .{
        .min_zoom = 1,
        .max_zoom = 16,
        .tolerance = 0.5,
        .buffer = 0,
        .tile_size = 256,
        .line_metrics = true,
    });
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try map.getStyleSourceType(testing.allocator, "geo-url")).?);
    try map.setGeoJsonSourceUrl(testing.allocator, "geo-url", "https://example.com/updated.geojson");

    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceUrl(testing.allocator, "empty", "https://example.com/again.geojson", null));
    try map.addVectorSourceUrl(testing.allocator, "vector-url", "https://example.com/vector.json", null);
    try testing.expectError(error.InvalidArgument, map.setGeoJsonSourceUrl(testing.allocator, "vector-url", "https://example.com/not-geojson"));
}

test "geometry descriptor graphs support nested collections" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const line = [_]maplibre.LatLng{
        .{ .latitude = 37.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
    };
    const ring = [_]maplibre.LatLng{
        .{ .latitude = 37.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -123.0 },
    };
    const rings = [_][]const maplibre.LatLng{ring[0..]};
    const children = [_]maplibre.Geometry{
        .{ .line_string = line[0..] },
        .{ .polygon = rings[0..] },
    };

    try map.addGeoJsonSourceData(testing.allocator, "collection", .{ .geometry = .{ .collection = children[0..] } }, null);
    try testing.expect(try map.styleSourceExists(testing.allocator, "collection"));
}

test "GeoJSON descriptors reject invalid native values and pass explicit-length strings" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try testing.expectError(
        error.InvalidArgument,
        map.addGeoJsonSourceData(testing.allocator, "", .{ .geometry = .{ .empty = {} } }, null),
    );
    try testing.expectError(
        error.InvalidArgument,
        map.addGeoJsonSourceData(
            testing.allocator,
            "bad-coordinate",
            .{ .geometry = .{ .point = .{ .latitude = std.math.inf(f64), .longitude = 0.0 } } },
            null,
        ),
    );
    const features = [_]maplibre.Feature{.{
        .geometry = .{ .point = .{ .latitude = 0.0, .longitude = 0.0 } },
        .identifier = .{ .string = "bad\x00id" },
    }};
    try map.addGeoJsonSourceData(testing.allocator, "embedded-nul-id", .{ .feature_collection = features[0..] }, null);
    try testing.expect(try map.styleSourceExists(testing.allocator, "embedded-nul-id"));
}

test "geometry coordinate spans remain stable across nested materialization" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var first_line: [96]maplibre.LatLng = undefined;
    var second_line: [96]maplibre.LatLng = undefined;
    for (&first_line, 0..) |*coordinate, index| {
        coordinate.* = .{ .latitude = @as(f64, @floatFromInt(index)) / 1000.0, .longitude = -122.0 };
    }
    for (&second_line, 0..) |*coordinate, index| {
        coordinate.* = .{ .latitude = @as(f64, @floatFromInt(index)) / 1000.0, .longitude = -123.0 };
    }
    const lines = [_][]const maplibre.LatLng{ first_line[0..], second_line[0..] };

    try map.addGeoJsonSourceData(
        testing.allocator,
        "many-lines",
        .{ .geometry = .{ .multi_line_string = lines[0..] } },
        null,
    );
    try testing.expect(try map.styleSourceExists(testing.allocator, "many-lines"));
}

test "GeoJSON source options carry cluster settings and reject malformed cluster properties" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const first_properties = [_]maplibre.JsonMember{.{ .key = "rank", .value = .{ .uint = 1 } }};
    const second_properties = [_]maplibre.JsonMember{.{ .key = "rank", .value = .{ .uint = 2 } }};
    const features = [_]maplibre.Feature{
        .{ .geometry = .{ .point = .{ .latitude = 0.0, .longitude = 0.0 } }, .properties = first_properties[0..] },
        .{ .geometry = .{ .point = .{ .latitude = 0.001, .longitude = 0.001 } }, .properties = second_properties[0..] },
    };
    const rank_expression = [_]maplibre.JsonValue{ .{ .string = "get" }, .{ .string = "rank" } };
    const total_expression = [_]maplibre.JsonValue{ .{ .string = "+" }, .{ .array = rank_expression[0..] } };
    const cluster_properties = [_]maplibre.JsonMember{.{ .key = "total", .value = .{ .array = total_expression[0..] } }};
    // MapLibre Native parses the aggregation expressions while the descriptor is borrowed, so a
    // source that adds successfully proves the nested cluster property tree crossed the boundary.
    try map.addGeoJsonSourceData(
        testing.allocator,
        "clustered",
        .{ .feature_collection = features[0..] },
        .{
            .min_zoom = 0,
            .buffer = 0,
            .cluster = true,
            .cluster_radius = 50,
            .cluster_min_points = 2,
            .cluster_max_zoom = 14,
            .cluster_properties = .{ .object = cluster_properties[0..] },
        },
    );
    try testing.expect(try map.styleSourceExists(testing.allocator, "clustered"));

    // Options are fixed at creation, so replacing the data keeps the clustered source usable.
    try map.setGeoJsonSourceData(testing.allocator, "clustered", .{ .feature_collection = features[0..] });
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try map.getStyleSourceType(testing.allocator, "clustered")).?);

    const malformed_expression = [_]maplibre.JsonValue{.{ .string = "+" }};
    const malformed_properties = [_]maplibre.JsonMember{.{ .key = "total", .value = .{ .array = malformed_expression[0..] } }};
    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceData(
        testing.allocator,
        "malformed-clustered",
        .{ .feature_collection = features[0..] },
        .{ .cluster = true, .cluster_properties = .{ .object = malformed_properties[0..] } },
    ));
}
