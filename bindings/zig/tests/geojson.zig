const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

const empty_collection = "{\"type\":\"FeatureCollection\",\"features\":[]}";
const point_collection =
    "{\"type\":\"FeatureCollection\",\"features\":[{" ++
    "\"type\":\"Feature\",\"id\":\"sf\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[-122.4194,37.7749]}," ++
    "\"properties\":{\"name\":\"San Francisco\",\"visible\":true}}]}";

fn expectListContains(list: maplibre.StringList, expected: []const u8) !void {
    for (list.items) |item| if (std.mem.eql(u8, item, expected)) return;
    return error.MissingListEntry;
}

test "GeoJSON buffers add and update sources through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    _ = try map.addGeoJsonSourceData(testing.allocator, "empty", empty_collection, null);
    try testing.expect(try support.styleSourceExists(&map, "empty"));
    var source_ids = try support.listStyleSourceIds(&map);
    defer source_ids.deinit();
    try expectListContains(source_ids, "empty");

    _ = try map.setGeoJsonSourceData(testing.allocator, "empty", point_collection);
    _ = try map.setGeoJsonSourceUrl(testing.allocator, "empty", "https://example.com/data.geojson");
    _ = try map.addGeoJsonSourceUrl(testing.allocator, "geo-url", "https://example.com/initial.geojson", .{
        .min_zoom = 1,
        .max_zoom = 16,
        .tolerance = 0.5,
        .buffer = 0,
        .tile_size = 256,
        .line_metrics = true,
    });
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try support.styleSourceType(&map, "geo-url")).?);
    try testing.expect((try map.addGeoJsonSourceUrl(testing.allocator, "empty", "https://example.com/again.geojson", null)) != 0);
}

test "GeoJSON buffers support nested geometry collections" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const collection = "{\"type\":\"GeometryCollection\",\"geometries\":[{\"type\":\"LineString\",\"coordinates\":[[-123,37],[-122,38]]},{\"type\":\"Polygon\",\"coordinates\":[[[-123,37],[-123,38],[-122,38],[-123,37]]]}]}";
    _ = try map.addGeoJsonSourceData(testing.allocator, "collection", collection, null);
    try testing.expect(try support.styleSourceExists(&map, "collection"));
}

test "GeoJSON buffers reject invalid data and pass explicit-length strings" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try testing.expect((try map.addGeoJsonSourceData(testing.allocator, "", empty_collection, null)) != 0);
    try testing.expect((try map.addGeoJsonSourceData(testing.allocator, "bad-coordinate", "{\"type\":\"Point\",\"coordinates\":[0,1e999]}", null)) != 0);
    const embedded_nul_id = "{\"type\":\"Feature\",\"id\":\"bad\\u0000id\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{}}";
    _ = try map.addGeoJsonSourceData(testing.allocator, "embedded-nul-id", embedded_nul_id, null);
    try testing.expect(try support.styleSourceExists(&map, "embedded-nul-id"));
}

test "GeoJSON source options carry serialized cluster properties" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const features = "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{\"rank\":1}},{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0.001,0.001]},\"properties\":{\"rank\":2}}]}";
    _ = try map.addGeoJsonSourceData(testing.allocator, "clustered", features, .{
        .cluster = true,
        .cluster_radius = 50,
        .cluster_min_points = 2,
        .cluster_max_zoom = 14,
        .cluster_properties = "{\"total\":[\"+\",[\"get\",\"rank\"]]}",
    });
    try testing.expect(try support.styleSourceExists(&map, "clustered"));
    _ = try map.setGeoJsonSourceData(testing.allocator, "clustered", features);

    try testing.expect((try map.addGeoJsonSourceData(
        testing.allocator,
        "malformed-clustered",
        features,
        .{ .cluster = true, .cluster_properties = "{\"total\":[\"+\"]}" },
    )) != 0);
}
