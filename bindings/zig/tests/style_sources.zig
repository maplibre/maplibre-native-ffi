const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

test "style source JSON buffers expose type info and copied attribution" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapAddStyleSourceJson(support.handle(map), "empty-json", "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\",\"features\":[]}}", null));

    try testing.expect(try support.styleSourceExists(&map, "empty-json"));
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try support.styleSourceType(&map, "empty-json")).?);
    var info = (try support.styleSourceInfo(&map, "empty-json")).?;
    defer info.deinit();
    try testing.expectEqual(maplibre.StyleSourceType.geojson, info.value.info.type);
    try testing.expectEqual(@as(usize, "empty-json".len), info.value.info.id_size);
    try testing.expect(info.value.attribution == null);
    try testing.expect(info.value.url == null);
    try testing.expect(info.value.info.tilejson == null);

    try support.expectCommitted(try maplibre.mapAddStyleSourceJson(support.handle(map), "vector-meta", "{\"type\":\"vector\",\"tiles\":[\"https://example.com/{z}/{x}/{y}.pbf\"],\"attribution\":\"Example attribution\"}", null));

    var vector_info = (try support.styleSourceInfo(&map, "vector-meta")).?;
    defer vector_info.deinit();
    try testing.expectEqual(maplibre.StyleSourceType.vector, vector_info.value.info.type);
    try testing.expectEqualStrings("Example attribution", vector_info.value.attribution.?);

    var attribution = (try support.styleSourceAttribution(&map, "vector-meta")).?;
    defer attribution.deinit();
    try testing.expectEqualStrings("Example attribution", attribution.value);
}

test "style source removal reports state and copies missing results" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const empty_data = try maplibre.geojsonSourceDataCreate(testing.allocator, "{\"type\":\"FeatureCollection\",\"features\":[]}", null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(empty_data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "remove-me", empty_data, null));
    try testing.expect(try support.styleSourceExists(&map, "remove-me"));
    try testing.expect(try support.removeStyleSource(&map, "remove-me"));
    try testing.expect(!try support.styleSourceExists(&map, "remove-me"));

    // A removal of a missing ID is accepted, then fails with NOT_FOUND.
    try support.expectCommandError(try maplibre.mapRemoveStyleSource(support.handle(map), "remove-me", null), error.NotFound);

    try testing.expect((try support.styleSourceInfo(&map, "remove-me")) == null);
    try testing.expect((try support.styleSourceAttribution(&map, "remove-me")) == null);
    try testing.expect((try support.styleSourceUrl(&map, "remove-me")) == null);
}

test "style source volatility round trips through the public API" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const tiles = [_][]const u8{"https://example.com/{z}/{x}/{y}.mvt"};
    try support.expectCommitted(try maplibre.mapAddVectorSourceTiles(testing.allocator, support.handle(map), "volatile-source", tiles[0..], null, null));

    {
        var info = (try support.styleSourceInfo(&map, "volatile-source")).?;
        defer info.deinit();
        try testing.expect(!info.value.info.is_volatile);
    }

    try support.expectCommitted(try maplibre.mapSetStyleSourceVolatile(support.handle(map), "volatile-source", true, null));
    {
        var info = (try support.styleSourceInfo(&map, "volatile-source")).?;
        defer info.deinit();
        try testing.expect(info.value.info.is_volatile);
    }

    try support.expectCommitted(try maplibre.mapSetStyleSourceVolatile(support.handle(map), "volatile-source", false, null));
    {
        var info = (try support.styleSourceInfo(&map, "volatile-source")).?;
        defer info.deinit();
        try testing.expect(!info.value.info.is_volatile);
    }

    // Setting volatility on a missing ID is accepted, then fails with NOT_FOUND.
    try support.expectCommandError(try maplibre.mapSetStyleSourceVolatile(support.handle(map), "missing-source", true, null), error.NotFound);
}

test "tile source helpers expose copied reconstructible source information" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const vector_tiles = [_][]const u8{
        "https://a.example.com/vector/{z}/{x}/{y}.mvt",
        "https://b.example.com/vector/{z}/{x}/{y}.mvt",
    };
    try support.expectCommitted(try maplibre.mapAddVectorSourceTiles(testing.allocator, support.handle(map), "vector-helper", vector_tiles[0..], .{
        .min_zoom = 1.0,
        .max_zoom = 14.0,
        .attribution = "Helper attribution",
        .scheme = .tms,
        .bounds = .{
            .southwest = .{ .latitude = -45.0, .longitude = -120.0 },
            .northeast = .{ .latitude = 45.0, .longitude = 120.0 },
        },
        .vector_encoding = .mlt,
    }, null));
    try testing.expectEqual(maplibre.StyleSourceType.vector, (try support.styleSourceType(&map, "vector-helper")).?);
    var vector_info = (try support.styleSourceInfo(&map, "vector-helper")).?;
    defer vector_info.deinit();
    try testing.expectEqualStrings("Helper attribution", vector_info.value.attribution.?);
    try testing.expect(vector_info.value.url == null);
    try testing.expectEqual(@as(?u32, 512), vector_info.value.info.tile_size);
    try testing.expectEqual(maplibre.StyleVectorTileEncoding.mlt, vector_info.value.info.vector_encoding.?);
    const vector_tile_json = vector_info.value.info.tilejson.?;
    try testing.expectEqual(@as(usize, 2), vector_info.value.tile_urls.?.len);
    try testing.expectEqualStrings(vector_tiles[0], vector_info.value.tile_urls.?[0]);
    try testing.expectEqualStrings(vector_tiles[1], vector_info.value.tile_urls.?[1]);
    try testing.expectEqual(@as(f64, 1.0), vector_tile_json.min_zoom);
    try testing.expectEqual(@as(f64, 14.0), vector_tile_json.max_zoom);
    try testing.expectEqual(maplibre.StyleTileScheme.tms, vector_tile_json.scheme);
    try testing.expectEqual(@as(f64, -45.0), vector_info.value.info.bounds.?.southwest.latitude);
    try testing.expectEqual(@as(f64, 120.0), vector_info.value.info.bounds.?.northeast.longitude);

    try support.expectCommitted(try maplibre.mapAddVectorSourceUrl(testing.allocator, support.handle(map), "vector-url-helper", "https://example.com/vector.json", null, null));
    try testing.expectEqual(maplibre.StyleSourceType.vector, (try support.styleSourceType(&map, "vector-url-helper")).?);
    var vector_url_info = (try support.styleSourceInfo(&map, "vector-url-helper")).?;
    defer vector_url_info.deinit();
    try testing.expectEqualStrings("https://example.com/vector.json", vector_url_info.value.url.?);
    try testing.expect(vector_url_info.value.info.tilejson == null);

    var copied_url = (try support.styleSourceUrl(&map, "vector-url-helper")).?;
    defer copied_url.deinit();
    try testing.expectEqualStrings("https://example.com/vector.json", copied_url.value);

    const raster_tiles = [_][]const u8{"https://example.com/raster/{z}/{x}/{y}.png"};
    try support.expectCommitted(try maplibre.mapAddRasterSourceTiles(testing.allocator, support.handle(map), "raster-helper", raster_tiles[0..], .{ .tile_size = 256 }, null));
    try testing.expectEqual(maplibre.StyleSourceType.raster, (try support.styleSourceType(&map, "raster-helper")).?);
    var raster_info = (try support.styleSourceInfo(&map, "raster-helper")).?;
    defer raster_info.deinit();
    try testing.expectEqual(@as(?u32, 256), raster_info.value.info.tile_size);
    try testing.expect(raster_info.value.info.vector_encoding == null);
    try testing.expect(raster_info.value.info.raster_encoding == null);
    try support.expectCommitted(try maplibre.mapAddRasterSourceUrl(testing.allocator, support.handle(map), "raster-url-helper", "https://example.com/raster.json", .{ .tile_size = 256 }, null));

    const dem_tiles = [_][]const u8{"https://example.com/dem/{z}/{x}/{y}.png"};
    try support.expectCommitted(try maplibre.mapAddRasterDemSourceTiles(testing.allocator, support.handle(map), "dem", dem_tiles[0..], .{
        .min_zoom = 0.0,
        .max_zoom = 14.0,
        .tile_size = 256,
        .raster_encoding = .terrarium,
    }, null));
    try support.expectCommitted(try maplibre.mapAddRasterDemSourceUrl(testing.allocator, support.handle(map), "dem-url", "https://example.com/dem.json", .{ .tile_size = 256, .raster_encoding = .mapbox }, null));
    try testing.expectEqual(maplibre.StyleSourceType.raster_dem, (try support.styleSourceType(&map, "dem")).?);
    var dem_info = (try support.styleSourceInfo(&map, "dem")).?;
    defer dem_info.deinit();
    try testing.expectEqual(@as(?u32, 256), dem_info.value.info.tile_size);
    try testing.expectEqual(maplibre.StyleRasterDemEncoding.terrarium, dem_info.value.info.raster_encoding.?);

    try support.expectCommitted(try maplibre.mapAddHillshadeLayer(support.handle(map), "dem-hillshade", "dem", "point-circle", null));
    try support.expectCommitted(try maplibre.mapAddColorReliefLayer(support.handle(map), "dem-relief", "dem", "", null));
    try support.expectStyleLayerType(&map, "dem-hillshade", "hillshade");
    try support.expectStyleLayerType(&map, "dem-relief", "color-relief");

    try support.expectCommitted(try maplibre.mapSetLayerProperty(support.handle(map), "dem-relief", "color-relief-color", "[\"interpolate\",[\"linear\"],[\"elevation\"],0,\"black\",1000,\"white\"]", null));
    try support.expectCommandError(try maplibre.mapSetLayerProperty(support.handle(map), "dem-relief", "color-relief-color", "[\"interpolate\",[\"linear\"],[\"zoom\"],0,\"black\",1,\"white\"]", null), error.InvalidArgument);
    try support.expectCommandError(try maplibre.mapAddHillshadeLayer(support.handle(map), "bad-hillshade", "point", "", null), error.InvalidArgument);
    try testing.expectError(error.InvalidArgument, maplibre.mapAddRasterSourceTiles(testing.allocator, support.handle(map), "bad-raster", raster_tiles[0..], .{ .raster_encoding = .mapbox }, null));

    try testing.expect(try support.removeStyleSource(&map, "vector-helper"));
    try support.closeMap(&map);
    try testing.expectEqualStrings(vector_tiles[0], vector_info.value.tile_urls.?[0]);
    try testing.expectEqualStrings("https://example.com/vector.json", vector_url_info.value.url.?);
}

test "style source tile URLs separate an empty list from a missing source" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const tiles = [_][]const u8{
        "https://a.example.com/tiles/{z}/{x}/{y}.mvt",
        "https://b.example.com/tiles/{z}/{x}/{y}.mvt",
    };
    try support.expectCommitted(try maplibre.mapAddVectorSourceTiles(testing.allocator, support.handle(map), "inline-vector", tiles[0..], null, null));
    try support.expectCommitted(try maplibre.mapAddVectorSourceUrl(testing.allocator, support.handle(map), "url-vector", "https://example.com/vector.json", null, null));

    var inline_urls = (try support.styleSourceTileUrls(&map, "inline-vector")).?;
    defer inline_urls.deinit();
    try testing.expectEqual(@as(usize, 2), inline_urls.value.tile_urls.len);
    try testing.expectEqualStrings(tiles[0], inline_urls.value.tile_urls[0]);
    try testing.expectEqualStrings(tiles[1], inline_urls.value.tile_urls[1]);

    var url_backed = (try support.styleSourceTileUrls(&map, "url-vector")).?;
    defer url_backed.deinit();
    try testing.expectEqual(@as(usize, 0), url_backed.value.tile_urls.len);

    try testing.expect((try support.styleSourceTileUrls(&map, "missing-source")) == null);
}

test "image source helpers add update and copy coordinates" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const coordinates = [4]maplibre.LatLng{
        .{ .latitude = 38.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -123.0 },
    };
    try support.expectCommitted(try maplibre.mapAddImageSourceUrl(testing.allocator, support.handle(map), "image-url-source", &coordinates, "https://example.com/image.png", null));
    try testing.expectEqual(maplibre.StyleSourceType.image, (try support.styleSourceType(&map, "image-url-source")).?);

    const copied = (try support.imageSourceCoordinates(&map, "image-url-source")).?;
    try testing.expectApproxEqAbs(coordinates[0].latitude, copied[0].latitude, 0.000001);
    try testing.expectApproxEqAbs(coordinates[0].longitude, copied[0].longitude, 0.000001);

    var image_pixels = [_]u8{ 1, 2, 3, 4 };
    try support.expectCommitted(try maplibre.mapAddImageSourceImage(testing.allocator, support.handle(map), "image-inline-source", &coordinates, .{
        .width = 1,
        .height = 1,
        .stride = 4,
        .pixels = image_pixels[0..],
    }, null));
    image_pixels[0] = 9;
    try support.expectCommitted(try maplibre.mapSetImageSourceUrl(support.handle(map), "image-inline-source", "https://example.com/replacement.png", null));
    image_pixels[0] = 5;
    try support.expectCommitted(try maplibre.mapSetImageSourceImage(testing.allocator, support.handle(map), "image-inline-source", .{
        .width = 1,
        .height = 1,
        .stride = 4,
        .pixels = image_pixels[0..],
    }, null));

    const updated_coordinates = [4]maplibre.LatLng{
        .{ .latitude = 39.0, .longitude = -124.0 },
        .{ .latitude = 39.0, .longitude = -121.0 },
        .{ .latitude = 36.0, .longitude = -121.0 },
        .{ .latitude = 36.0, .longitude = -124.0 },
    };
    try support.expectCommitted(try maplibre.mapSetImageSourceCoordinates(testing.allocator, support.handle(map), "image-inline-source", &updated_coordinates, null));
    const updated = (try support.imageSourceCoordinates(&map, "image-inline-source")).?;
    try testing.expectApproxEqAbs(updated_coordinates[0].latitude, updated[0].latitude, 0.000001);
    try testing.expectApproxEqAbs(updated_coordinates[0].longitude, updated[0].longitude, 0.000001);

    try testing.expect((try support.imageSourceCoordinates(&map, "missing-image-source")) == null);
    try support.expectCommandError(try maplibre.mapAddImageSourceUrl(testing.allocator, support.handle(map), "image-url-source", &coordinates, "https://example.com/duplicate.png", null), error.InvalidArgument);
    try support.expectCommandError(try maplibre.mapSetImageSourceUrl(support.handle(map), "point", "https://example.com/not-image.png", null), error.InvalidArgument);
}

test "style source JSON buffers reject invalid source data and pass explicit-length IDs" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommandError(try maplibre.mapAddStyleSourceJson(support.handle(map), "invalid-json-source", "{\"type\":\"definitely-not-a-source-type\"}", null), error.InvalidArgument);
    try support.expectCommitted(try maplibre.mapAddStyleSourceJson(support.handle(map), "nul\x00source", "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\",\"features\":[]}}", null));
    try testing.expect(try support.styleSourceExists(&map, "nul\x00source"));
    var info = (try support.styleSourceInfo(&map, "nul\x00source")).?;
    defer info.deinit();
    try testing.expectEqual(@as(usize, "nul\x00source".len), info.value.info.id_size);
}

// Tile callbacks run only once a render session draws the source, so these
// tests watch the release callback that every source lifetime ends with.
const CustomGeometryState = struct {
    release_count: usize = 0,
};

fn fetchCustomGeometryTile(_: ?*anyopaque, _: maplibre.CanonicalTileId) maplibre.Error!void {}

fn cancelCustomGeometryTile(_: ?*anyopaque, _: maplibre.CanonicalTileId) maplibre.Error!void {}

fn releaseCustomGeometryContext(context: ?*anyopaque) void {
    const state: *CustomGeometryState = @ptrCast(@alignCast(context.?));
    state.release_count += 1;
}

test "custom geometry source helpers add sources and accept tile updates" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    var state = CustomGeometryState{};
    try support.expectCommitted(try maplibre.mapAddCustomGeometrySource(testing.allocator, support.handle(map), "custom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .cancel_tile = cancelCustomGeometryTile,
        .context = &state,
        .min_zoom = 0.0,
        .max_zoom = 14.0,
        .tolerance = 0.5,
        .tile_size = 256,
        .buffer = 64,
        .clip = true,
        .wrap = true,
    }, null));

    try testing.expect(try support.styleSourceExists(&map, "custom"));
    try testing.expectEqual(maplibre.StyleSourceType.custom_vector, (try support.styleSourceType(&map, "custom")).?);

    const tile_id = maplibre.CanonicalTileId{ .z = 0, .x = 0, .y = 0 };
    try support.expectCommitted(try maplibre.mapSetCustomGeometrySourceTileData(support.handle(map), "custom", tile_id, "{\"type\":\"FeatureCollection\",\"features\":[]}", null));
    try support.expectCommitted(try maplibre.mapInvalidateCustomGeometrySourceTile(support.handle(map), "custom", tile_id, null));
    try support.expectCommitted(try maplibre.mapInvalidateCustomGeometrySourceRegion(support.handle(map), "custom", .{
        .southwest = .{ .latitude = -1.0, .longitude = -1.0 },
        .northeast = .{ .latitude = 1.0, .longitude = 1.0 },
    }, null));

    const duplicate_custom = try maplibre.mapAddCustomGeometrySource(testing.allocator, support.handle(map), "custom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
    }, null);
    try support.expectCommandError(duplicate_custom, error.InvalidArgument);
    try testing.expectError(error.InvalidArgument, maplibre.mapAddCustomGeometrySource(testing.allocator, support.handle(map), "bad-zoom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
        .max_zoom = 33.0,
    }, null));
    try support.expectCommandError(try maplibre.mapSetCustomGeometrySourceTileData(
        support.handle(map),

        "custom",
        .{ .z = 1, .x = 2, .y = 0 },
        "{\"type\":\"FeatureCollection\",\"features\":[]}",
        null,
    ), error.InvalidArgument);
    try support.expectCommandError(try maplibre.mapInvalidateCustomGeometrySourceTile(support.handle(map), "point", tile_id, null), error.InvalidArgument);
}

test "custom MVT vector source helpers add sources and accept tile updates" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    var state = CustomGeometryState{};
    try support.expectCommitted(try maplibre.mapAddCustomMvtVectorSource(testing.allocator, support.handle(map), "custom-mvt", .{
        .fetch_tile = fetchCustomGeometryTile,
        .cancel_tile = cancelCustomGeometryTile,
        .context = &state,
        .min_zoom = 0.0,
        .max_zoom = 14.0,
    }, null));

    try testing.expect(try support.styleSourceExists(&map, "custom-mvt"));
    try testing.expectEqual(maplibre.StyleSourceType.custom_mvt_vector, (try support.styleSourceType(&map, "custom-mvt")).?);

    const tile_id = maplibre.CanonicalTileId{ .z = 0, .x = 0, .y = 0 };
    try support.expectCommitted(try maplibre.mapSetCustomMvtVectorSourceTileData(support.handle(map), "custom-mvt", tile_id, "", null));
    try support.expectCommitted(try maplibre.mapSetCustomMvtVectorSourceTileError(support.handle(map), "custom-mvt", tile_id, "missing", null));
    try support.expectCommitted(try maplibre.mapInvalidateCustomMvtVectorSourceTile(support.handle(map), "custom-mvt", tile_id, null));

    const duplicate_mvt = try maplibre.mapAddCustomMvtVectorSource(testing.allocator, support.handle(map), "custom-mvt", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
    }, null);
    try support.expectCommandError(duplicate_mvt, error.InvalidArgument);
    try testing.expectError(error.InvalidArgument, maplibre.mapAddCustomMvtVectorSource(testing.allocator, support.handle(map), "bad-zoom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
        .max_zoom = 33.0,
    }, null));
    // A source ID that names nothing reports not-found through the command.
    try support.expectCommandError(try maplibre.mapSetCustomMvtVectorSourceTileData(
        support.handle(map),

        "custom",
        tile_id,
        "",
        null,
    ), error.NotFound);
    // An ID that names a source of another type keeps the argument rejection.
    try support.expectCommandError(try maplibre.mapInvalidateCustomMvtVectorSourceTile(support.handle(map), "point", tile_id, null), error.InvalidArgument);
}

// A host owns the context its callbacks read, and the release callback is the
// only report that the map stopped referencing it.
test "a custom geometry source releases its context once per lifetime end" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    var map_open = true;
    defer if (map_open) support.closeMap(&map) catch @panic("map close failed");

    var removed = CustomGeometryState{};
    try support.expectCommitted(try maplibre.mapAddCustomGeometrySource(testing.allocator, support.handle(map), "removed", .{
        .fetch_tile = fetchCustomGeometryTile,
        .release_context = releaseCustomGeometryContext,
        .context = &removed,
    }, null));
    var retained = CustomGeometryState{};
    try support.expectCommitted(try maplibre.mapAddCustomGeometrySource(testing.allocator, support.handle(map), "retained", .{
        .fetch_tile = fetchCustomGeometryTile,
        .release_context = releaseCustomGeometryContext,
        .context = &retained,
    }, null));

    try testing.expect(try support.removeStyleSource(&map, "removed"));
    try testing.expectEqual(@as(usize, 1), removed.release_count);
    try testing.expectEqual(@as(usize, 0), retained.release_count);

    // The map is what still holds the second source, so its destruction is what
    // releases that context.
    try support.closeMap(&map);
    map_open = false;
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(@as(usize, 1), removed.release_count);
    try testing.expectEqual(@as(usize, 1), retained.release_count);
}
