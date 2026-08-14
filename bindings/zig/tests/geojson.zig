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

test "prepared GeoJSON data adds and updates sources through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const empty_data = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, empty_collection, null);
    defer empty_data.release();
    try map.addGeoJsonSourceData(testing.allocator, "empty", empty_data);
    try testing.expect(try map.styleSourceExists(testing.allocator, "empty"));
    var source_ids = try map.listStyleSourceIds(testing.allocator);
    defer source_ids.deinit();
    try expectListContains(source_ids, "empty");

    const point_data = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, point_collection, null);
    defer point_data.release();
    try map.setGeoJsonSourceData(testing.allocator, "empty", point_data);
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

test "prepared GeoJSON data supports nested geometry collections" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const collection = "{\"type\":\"GeometryCollection\",\"geometries\":[{\"type\":\"LineString\",\"coordinates\":[[-123,37],[-122,38]]},{\"type\":\"Polygon\",\"coordinates\":[[[-123,37],[-123,38],[-122,38],[-123,37]]]}]}";
    const data = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, collection, null);
    defer data.release();
    try map.addGeoJsonSourceData(testing.allocator, "collection", data);
    try testing.expect(try map.styleSourceExists(testing.allocator, "collection"));
}

test "GeoJSON preparation rejects invalid data and passes explicit-length strings" {
    try testing.expectError(
        error.InvalidArgument,
        maplibre.GeoJsonSourceDataHandle.create(testing.allocator, "{\"type\":\"Point\",\"coordinates\":[0,1e999]}", null),
    );

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const empty_data = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, empty_collection, null);
    defer empty_data.release();
    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceData(testing.allocator, "", empty_data));

    const embedded_nul_id = "{\"type\":\"Feature\",\"id\":\"bad\\u0000id\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{}}";
    const nul_data = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, embedded_nul_id, null);
    defer nul_data.release();
    try map.addGeoJsonSourceData(testing.allocator, "embedded-nul-id", nul_data);
    try testing.expect(try map.styleSourceExists(testing.allocator, "embedded-nul-id"));
}

test "GeoJSON preparation bakes in options and validates clustering" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const features = "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{\"rank\":1}},{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0.001,0.001]},\"properties\":{\"rank\":2}}]}";
    const cluster_options = maplibre.StyleGeoJsonSourceOptions{
        .cluster = true,
        .cluster_radius = 50,
        .cluster_min_points = 2,
        .cluster_max_zoom = 14,
        .cluster_properties = "{\"total\":[\"+\",[\"get\",\"rank\"]]}",
    };
    const clustered = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, features, cluster_options);
    defer clustered.release();
    try map.addGeoJsonSourceData(testing.allocator, "clustered", clustered);
    try testing.expect(try map.styleSourceExists(testing.allocator, "clustered"));
    try map.setGeoJsonSourceData(testing.allocator, "clustered", clustered);

    // Cluster validation happens at preparation, before any map is involved.
    try testing.expectError(error.InvalidArgument, maplibre.GeoJsonSourceDataHandle.create(
        testing.allocator,
        features,
        .{ .cluster = true, .cluster_properties = "{\"total\":[\"+\"]}" },
    ));
    try testing.expectError(error.InvalidArgument, maplibre.GeoJsonSourceDataHandle.create(
        testing.allocator,
        "{\"type\":\"Point\",\"coordinates\":[0,0]}",
        .{ .cluster = true },
    ));

    // A set rejects data prepared with options that differ from the source's,
    // cluster_properties excepted.
    const unclustered = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, features, null);
    defer unclustered.release();
    try testing.expectError(error.InvalidArgument, map.setGeoJsonSourceData(testing.allocator, "clustered", unclustered));

    var reproperty_options = cluster_options;
    reproperty_options.cluster_properties = "{\"total\":[\"max\",[\"get\",\"rank\"]]}";
    const repropertied = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, features, reproperty_options);
    defer repropertied.release();
    try map.setGeoJsonSourceData(testing.allocator, "clustered", repropertied);
}

test "prepared GeoJSON data installs on many sources and outlives release" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const shared = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, point_collection, null);
    try map.addGeoJsonSourceData(testing.allocator, "shared-a", shared);
    try map.addGeoJsonSourceData(testing.allocator, "shared-b", shared);
    try map.setGeoJsonSourceData(testing.allocator, "shared-a", shared);

    // Sources keep their own reference, so release never invalidates them,
    // and a second release is a no-op.
    shared.release();
    shared.release();
    try testing.expect(try map.styleSourceExists(testing.allocator, "shared-a"));
    try testing.expect(try map.styleSourceExists(testing.allocator, "shared-b"));

    // A released handle is stale for new installs.
    try testing.expectError(error.InvalidArgument, map.setGeoJsonSourceData(testing.allocator, "shared-a", shared));
    try testing.expectError(error.InvalidArgument, map.addGeoJsonSourceData(testing.allocator, "shared-c", shared));
}

test "GeoJSON preparation runs on a worker thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const Prepare = struct {
        fn run(out: *(maplibre.Error!maplibre.GeoJsonSourceDataHandle)) void {
            out.* = maplibre.GeoJsonSourceDataHandle.create(std.heap.smp_allocator, point_collection, null);
        }
    };
    var result: maplibre.Error!maplibre.GeoJsonSourceDataHandle = error.InvalidArgument;
    const thread = try std.Thread.spawn(.{}, Prepare.run, .{&result});
    thread.join();
    const data = try result;
    defer data.release();
    try map.addGeoJsonSourceData(testing.allocator, "worker-prepared", data);
    try testing.expect(try map.styleSourceExists(testing.allocator, "worker-prepared"));
}

test "synchronous tiling override applies at runtime" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const data = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, point_collection, null);
    defer data.release();
    try map.addGeoJsonSourceData(testing.allocator, "tracked", data);

    try map.setGeoJsonSourceSynchronousTiling(testing.allocator, "tracked", true);
    try map.setGeoJsonSourceData(testing.allocator, "tracked", data);
    try map.setGeoJsonSourceSynchronousTiling(testing.allocator, "tracked", false);

    try testing.expectError(error.InvalidArgument, map.setGeoJsonSourceSynchronousTiling(testing.allocator, "missing", true));
}
