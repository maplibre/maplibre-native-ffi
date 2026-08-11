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

    try map.addGeoJsonSourceData(testing.allocator, "empty", empty_collection, null);
    try testing.expect(try map.styleSourceExists(testing.allocator, "empty"));
    var source_ids = try map.listStyleSourceIds(testing.allocator);
    defer source_ids.deinit();
    try expectListContains(source_ids, "empty");

    try map.setGeoJsonSourceData(testing.allocator, "empty", point_collection);
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
    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceUrl(testing.allocator, "empty", "https://example.com/again.geojson", null));
}

test "GeoJSON buffers support nested geometry collections" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const collection = "{\"type\":\"GeometryCollection\",\"geometries\":[{\"type\":\"LineString\",\"coordinates\":[[-123,37],[-122,38]]},{\"type\":\"Polygon\",\"coordinates\":[[[-123,37],[-123,38],[-122,38],[-123,37]]]}]}";
    try map.addGeoJsonSourceData(testing.allocator, "collection", collection, null);
    try testing.expect(try map.styleSourceExists(testing.allocator, "collection"));
}

test "GeoJSON buffers reject invalid data and pass explicit-length strings" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceData(testing.allocator, "", empty_collection, null));
    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceData(testing.allocator, "bad-coordinate", "{\"type\":\"Point\",\"coordinates\":[0,1e999]}", null));
    const embedded_nul_id = "{\"type\":\"Feature\",\"id\":\"bad\\u0000id\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{}}";
    try map.addGeoJsonSourceData(testing.allocator, "embedded-nul-id", embedded_nul_id, null);
    try testing.expect(try map.styleSourceExists(testing.allocator, "embedded-nul-id"));
}

test "GeoJSON source options carry serialized cluster properties" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const features = "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{\"rank\":1}},{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0.001,0.001]},\"properties\":{\"rank\":2}}]}";
    try map.addGeoJsonSourceData(testing.allocator, "clustered", features, .{
        .cluster = true,
        .cluster_radius = 50,
        .cluster_min_points = 2,
        .cluster_max_zoom = 14,
        .cluster_properties = "{\"total\":[\"+\",[\"get\",\"rank\"]]}",
    });
    try testing.expect(try map.styleSourceExists(testing.allocator, "clustered"));
    try map.setGeoJsonSourceData(testing.allocator, "clustered", features);

    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceData(
        testing.allocator,
        "malformed-clustered",
        features,
        .{ .cluster = true, .cluster_properties = "{\"total\":[\"+\"]}" },
    ));
}
