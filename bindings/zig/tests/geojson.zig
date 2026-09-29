const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

const empty_collection = "{\"type\":\"FeatureCollection\",\"features\":[]}";
const point_collection =
    "{\"type\":\"FeatureCollection\",\"features\":[{" ++
    "\"type\":\"Feature\",\"id\":\"sf\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[-122.4194,37.7749]}," ++
    "\"properties\":{\"name\":\"San Francisco\",\"visible\":true}}]}";

test "prepared GeoJSON data adds and updates sources through public binding" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const empty_data = try maplibre.geojsonSourceDataCreate(testing.allocator, empty_collection, null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(empty_data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "empty", empty_data, null));
    try testing.expect(try support.styleSourceExists(&map, "empty"));
    var source_ids = try support.listStyleSourceIds(&map);
    defer source_ids.deinit();
    try support.expectListContains(source_ids, "empty");

    const point_data = try maplibre.geojsonSourceDataCreate(testing.allocator, point_collection, null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(point_data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapSetGeojsonSourceData(support.handle(map), "empty", point_data, null));
    try support.expectCommitted(try maplibre.mapSetGeojsonSourceUrl(support.handle(map), "empty", "https://example.com/data.geojson", null));
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceUrl(testing.allocator, support.handle(map), "geo-url", "https://example.com/initial.geojson", .{
        .min_zoom = 1,
        .max_zoom = 16,
        .tolerance = 0.5,
        .buffer = 0,
        .tile_size = 256,
        .line_metrics = true,
    }, null));
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try support.styleSourceType(&map, "geo-url")).?);
    try support.expectCommandError(try maplibre.mapAddGeojsonSourceUrl(testing.allocator, support.handle(map), "empty", "https://example.com/again.geojson", null, null), error.InvalidArgument);
}

test "prepared GeoJSON data supports nested geometry collections" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const collection = "{\"type\":\"GeometryCollection\",\"geometries\":[{\"type\":\"LineString\",\"coordinates\":[[-123,37],[-122,38]]},{\"type\":\"Polygon\",\"coordinates\":[[[-123,37],[-123,38],[-122,38],[-123,37]]]}]}";
    const data = try maplibre.geojsonSourceDataCreate(testing.allocator, collection, null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "collection", data, null));
    try testing.expect(try support.styleSourceExists(&map, "collection"));
}

test "GeoJSON preparation rejects invalid data and passes explicit-length strings" {
    try testing.expectError(
        error.InvalidArgument,
        maplibre.geojsonSourceDataCreate(testing.allocator, "{\"type\":\"Point\",\"coordinates\":[0,1e999]}", null, null),
    );

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const empty_data = try maplibre.geojsonSourceDataCreate(testing.allocator, empty_collection, null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(empty_data)) catch @panic("prepared data destroy failed");
    // An empty source ID submits, then the command reports the rejection.
    const empty_id = try maplibre.mapAddGeojsonSourceData(support.handle(map), "", empty_data, null);
    try support.expectCommandError(empty_id, error.InvalidArgument);

    const embedded_nul_id = "{\"type\":\"Feature\",\"id\":\"bad\\u0000id\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{}}";
    const nul_data = try maplibre.geojsonSourceDataCreate(testing.allocator, embedded_nul_id, null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(nul_data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "embedded-nul-id", nul_data, null));
    try testing.expect(try support.styleSourceExists(&map, "embedded-nul-id"));
}

test "GeoJSON preparation bakes in options and validates clustering" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const features = "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{\"rank\":1}},{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[0.001,0.001]},\"properties\":{\"rank\":2}}]}";
    const cluster_options = maplibre.GeojsonSourceOptions{
        .cluster = true,
        .cluster_radius = 50,
        .cluster_min_points = 2,
        .cluster_max_zoom = 14,
        .cluster_properties = "{\"total\":[\"+\",[\"get\",\"rank\"]]}",
    };
    const clustered = try maplibre.geojsonSourceDataCreate(testing.allocator, features, cluster_options, null);
    defer maplibre.geojsonSourceDataDestroy(clustered) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "clustered", clustered, null));
    try testing.expect(try support.styleSourceExists(&map, "clustered"));
    try support.expectCommitted(try maplibre.mapSetGeojsonSourceData(support.handle(map), "clustered", clustered, null));

    // Cluster validation happens at preparation, before any map is involved.
    try testing.expectError(error.InvalidArgument, maplibre.geojsonSourceDataCreate(
        testing.allocator,
        features,
        .{ .cluster = true, .cluster_properties = "{\"total\":[\"+\"]}" },
        null,
    ));
    try testing.expectError(error.InvalidArgument, maplibre.geojsonSourceDataCreate(
        testing.allocator,
        "{\"type\":\"Point\",\"coordinates\":[0,0]}",
        .{ .cluster = true },
        null,
    ));

    // A set rejects data prepared with options that differ from the source's,
    // cluster aggregation expressions included; the command reports it.
    const unclustered = try maplibre.geojsonSourceDataCreate(testing.allocator, features, null, null);
    defer maplibre.geojsonSourceDataDestroy(unclustered) catch @panic("prepared data destroy failed");
    const unclustered_set = try maplibre.mapSetGeojsonSourceData(support.handle(map), "clustered", unclustered, null);
    try support.expectCommandError(unclustered_set, error.InvalidArgument);

    var reproperty_options = cluster_options;
    reproperty_options.cluster_properties = "{\"total\":[\"max\",[\"get\",\"rank\"]]}";
    const repropertied = try maplibre.geojsonSourceDataCreate(testing.allocator, features, reproperty_options, null);
    defer maplibre.geojsonSourceDataDestroy(repropertied) catch @panic("prepared data destroy failed");
    const repropertied_set = try maplibre.mapSetGeojsonSourceData(support.handle(map), "clustered", repropertied, null);
    try support.expectCommandError(repropertied_set, error.InvalidArgument);

    // Aggregations compare by parsed equality, so equivalent JSON with
    // different formatting still matches.
    var reformatted_options = cluster_options;
    reformatted_options.cluster_properties = " { \"total\" : [\"+\", [\"get\", \"rank\"]] } ";
    const reformatted = try maplibre.geojsonSourceDataCreate(testing.allocator, features, reformatted_options, null);
    defer maplibre.geojsonSourceDataDestroy(reformatted) catch @panic("prepared data destroy failed");
    const reformatted_set = try maplibre.mapSetGeojsonSourceData(support.handle(map), "clustered", reformatted, null);
    try support.expectCommitted(reformatted_set);
}

test "prepared GeoJSON data installs on many sources and outlives release" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const shared = try maplibre.geojsonSourceDataCreate(testing.allocator, point_collection, null, null);
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "shared-a", shared, null));
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "shared-b", shared, null));
    try support.expectCommitted(try maplibre.mapSetGeojsonSourceData(support.handle(map), "shared-a", shared, null));

    // Submitted installs keep their own lease, so a release right after
    // submit never invalidates them, and a second release is a no-op.
    maplibre.geojsonSourceDataDestroy(shared) catch @panic("prepared data destroy failed");
    maplibre.geojsonSourceDataDestroy(shared) catch @panic("prepared data destroy failed");
    try testing.expect(try support.styleSourceExists(&map, "shared-a"));
    try testing.expect(try support.styleSourceExists(&map, "shared-b"));

    // A released handle is stale for new installs, rejected at submit.
    try testing.expectError(error.InvalidState, maplibre.mapSetGeojsonSourceData(support.handle(map), "shared-a", shared, null));
    try testing.expectError(error.InvalidState, maplibre.mapAddGeojsonSourceData(support.handle(map), "shared-c", shared, null));
}

test "GeoJSON preparation runs on a worker thread" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const Prepare = struct {
        fn run(out: *(maplibre.Error!maplibre.GeojsonSourceData)) void {
            out.* = maplibre.geojsonSourceDataCreate(std.heap.smp_allocator, point_collection, null, null);
        }
    };
    var result: maplibre.Error!maplibre.GeojsonSourceData = error.InvalidArgument;
    const thread = try std.Thread.spawn(.{}, Prepare.run, .{&result});
    thread.join();
    const data = try result;
    defer maplibre.geojsonSourceDataDestroy(support.handle(data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "worker-prepared", data, null));
    try testing.expect(try support.styleSourceExists(&map, "worker-prepared"));
}

test "synchronous tiling override applies at runtime" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const data = try maplibre.geojsonSourceDataCreate(testing.allocator, point_collection, null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "tracked", data, null));

    const enable_id = try maplibre.mapSetGeojsonSourceSynchronousTiling(support.handle(map), "tracked", true, null);
    try support.expectCommitted(enable_id);
    const set_id = try maplibre.mapSetGeojsonSourceData(support.handle(map), "tracked", data, null);
    try support.expectCommitted(set_id);
    try support.expectCommitted(try maplibre.mapSetGeojsonSourceSynchronousTiling(support.handle(map), "tracked", false, null));

    // The override rejects a source ID that names nothing, reported by the
    // command since the style is only readable at commit.
    const missing_id = try maplibre.mapSetGeojsonSourceSynchronousTiling(support.handle(map), "missing", true, null);
    try support.expectCommandError(missing_id, error.NotFound);
}
