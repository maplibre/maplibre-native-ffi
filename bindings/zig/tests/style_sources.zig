const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

test "style source JSON buffers expose type info and copied attribution" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    _ = try map.addStyleSourceJson(testing.allocator, "empty-json", "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\",\"features\":[]}}");

    try testing.expect(try support.styleSourceExists(&map, "empty-json"));
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try support.styleSourceType(&map, "empty-json")).?);
    var info = (try support.styleSourceInfo(&map, "empty-json")).?;
    defer info.deinit();
    try testing.expectEqual(maplibre.StyleSourceType.geojson, info.source_type);
    try testing.expectEqual(@as(usize, "empty-json".len), info.id_size);
    try testing.expect(info.attribution == null);
    try testing.expect(info.url == null);
    try testing.expect(info.tile_json == null);

    _ = try map.addStyleSourceJson(testing.allocator, "vector-meta", "{\"type\":\"vector\",\"tiles\":[\"https://example.com/{z}/{x}/{y}.pbf\"],\"attribution\":\"Example attribution\"}");

    var vector_info = (try support.styleSourceInfo(&map, "vector-meta")).?;
    defer vector_info.deinit();
    try testing.expectEqual(maplibre.StyleSourceType.vector, vector_info.source_type);
    try testing.expectEqualStrings("Example attribution", vector_info.attribution.?);

    var attribution = (try support.styleSourceAttribution(&map, "vector-meta")).?;
    defer attribution.deinit();
    try testing.expectEqualStrings("Example attribution", attribution.value);
}

test "style source removal reports state and copies missing results" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const empty_data = try maplibre.GeoJsonSourceDataHandle.create(testing.allocator, "{\"type\":\"FeatureCollection\",\"features\":[]}", null);
    defer empty_data.release();
    _ = try map.addGeoJsonSourceData(testing.allocator, "remove-me", empty_data);
    try testing.expect(try support.styleSourceExists(&map, "remove-me"));
    try testing.expect(try support.removeStyleSource(&runtime, &map, "remove-me"));
    try testing.expect(!try support.styleSourceExists(&map, "remove-me"));

    // A removal of a missing ID is accepted, then fails with NOT_FOUND.
    try support.expectCommandError(&runtime, try map.removeStyleSource(testing.allocator, "remove-me"), error.NotFound);

    try testing.expect((try support.styleSourceInfo(&map, "remove-me")) == null);
    try testing.expect((try support.styleSourceAttribution(&map, "remove-me")) == null);
    try testing.expect((try support.styleSourceUrl(&map, "remove-me")) == null);
}

test "style source volatility round trips through the public API" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const tiles = [_][]const u8{"https://example.com/{z}/{x}/{y}.mvt"};
    _ = try map.addVectorSourceTiles(testing.allocator, "volatile-source", tiles[0..], null);

    {
        var info = (try support.styleSourceInfo(&map, "volatile-source")).?;
        defer info.deinit();
        try testing.expect(!info.is_volatile);
    }

    try testing.expectEqual(.committed, try support.waitForCommandDisposition(&runtime, try map.setStyleSourceVolatile(testing.allocator, "volatile-source", true)));
    {
        var info = (try support.styleSourceInfo(&map, "volatile-source")).?;
        defer info.deinit();
        try testing.expect(info.is_volatile);
    }

    try testing.expectEqual(.committed, try support.waitForCommandDisposition(&runtime, try map.setStyleSourceVolatile(testing.allocator, "volatile-source", false)));
    {
        var info = (try support.styleSourceInfo(&map, "volatile-source")).?;
        defer info.deinit();
        try testing.expect(!info.is_volatile);
    }

    // Setting volatility on a missing ID is accepted, then fails with NOT_FOUND.
    try support.expectCommandError(&runtime, try map.setStyleSourceVolatile(testing.allocator, "missing-source", true), error.NotFound);
}

test "tile source helpers expose copied reconstructible source information" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const vector_tiles = [_][]const u8{
        "https://a.example.com/vector/{z}/{x}/{y}.mvt",
        "https://b.example.com/vector/{z}/{x}/{y}.mvt",
    };
    _ = try map.addVectorSourceTiles(testing.allocator, "vector-helper", vector_tiles[0..], .{
        .min_zoom = 1.0,
        .max_zoom = 14.0,
        .attribution = "Helper attribution",
        .scheme = .tms,
        .bounds = .{
            .southwest = .{ .latitude = -45.0, .longitude = -120.0 },
            .northeast = .{ .latitude = 45.0, .longitude = 120.0 },
        },
        .vector_encoding = .mlt,
    });
    try testing.expectEqual(maplibre.StyleSourceType.vector, (try support.styleSourceType(&map, "vector-helper")).?);
    var vector_info = (try support.styleSourceInfo(&map, "vector-helper")).?;
    defer vector_info.deinit();
    try testing.expectEqualStrings("Helper attribution", vector_info.attribution.?);
    try testing.expect(vector_info.url == null);
    try testing.expectEqual(@as(?u32, 512), vector_info.tile_size);
    try testing.expectEqual(maplibre.StyleVectorTileEncoding.mlt, vector_info.vector_encoding.?);
    const vector_tile_json = vector_info.tile_json.?;
    try testing.expectEqual(@as(usize, 2), vector_tile_json.tile_urls.len);
    try testing.expectEqualStrings(vector_tiles[0], vector_tile_json.tile_urls[0]);
    try testing.expectEqualStrings(vector_tiles[1], vector_tile_json.tile_urls[1]);
    try testing.expectEqual(@as(f64, 1.0), vector_tile_json.min_zoom);
    try testing.expectEqual(@as(f64, 14.0), vector_tile_json.max_zoom);
    try testing.expectEqual(maplibre.StyleTileScheme.tms, vector_tile_json.scheme);
    try testing.expectEqual(@as(f64, -45.0), vector_tile_json.bounds.?.southwest.latitude);
    try testing.expectEqual(@as(f64, 120.0), vector_tile_json.bounds.?.northeast.longitude);

    _ = try map.addVectorSourceUrl(testing.allocator, "vector-url-helper", "https://example.com/vector.json", null);
    try testing.expectEqual(maplibre.StyleSourceType.vector, (try support.styleSourceType(&map, "vector-url-helper")).?);
    var vector_url_info = (try support.styleSourceInfo(&map, "vector-url-helper")).?;
    defer vector_url_info.deinit();
    try testing.expectEqualStrings("https://example.com/vector.json", vector_url_info.url.?);
    try testing.expect(vector_url_info.tile_json == null);

    var copied_url = (try support.styleSourceUrl(&map, "vector-url-helper")).?;
    defer copied_url.deinit();
    try testing.expectEqualStrings("https://example.com/vector.json", copied_url.value);

    const raster_tiles = [_][]const u8{"https://example.com/raster/{z}/{x}/{y}.png"};
    _ = try map.addRasterSourceTiles(testing.allocator, "raster-helper", raster_tiles[0..], .{ .tile_size = 256 });
    try testing.expectEqual(maplibre.StyleSourceType.raster, (try support.styleSourceType(&map, "raster-helper")).?);
    var raster_info = (try support.styleSourceInfo(&map, "raster-helper")).?;
    defer raster_info.deinit();
    try testing.expectEqual(@as(?u32, 256), raster_info.tile_size);
    try testing.expect(raster_info.vector_encoding == null);
    try testing.expect(raster_info.raster_encoding == null);
    _ = try map.addRasterSourceUrl(testing.allocator, "raster-url-helper", "https://example.com/raster.json", .{ .tile_size = 256 });

    const dem_tiles = [_][]const u8{"https://example.com/dem/{z}/{x}/{y}.png"};
    _ = try map.addRasterDemSourceTiles(testing.allocator, "dem", dem_tiles[0..], .{
        .min_zoom = 0.0,
        .max_zoom = 14.0,
        .tile_size = 256,
        .raster_encoding = .terrarium,
    });
    _ = try map.addRasterDemSourceUrl(testing.allocator, "dem-url", "https://example.com/dem.json", .{ .tile_size = 256, .raster_encoding = .mapbox });
    try testing.expectEqual(maplibre.StyleSourceType.raster_dem, (try support.styleSourceType(&map, "dem")).?);
    var dem_info = (try support.styleSourceInfo(&map, "dem")).?;
    defer dem_info.deinit();
    try testing.expectEqual(@as(?u32, 256), dem_info.tile_size);
    try testing.expectEqual(maplibre.StyleRasterDemEncoding.terrarium, dem_info.raster_encoding.?);

    _ = try map.addHillshadeLayer(testing.allocator, "dem-hillshade", "dem", "point-circle");
    _ = try map.addColorReliefLayer(testing.allocator, "dem-relief", "dem", "");
    try testing.expectEqualStrings("hillshade", (try support.styleLayerType(&map, "dem-hillshade")).?);
    try testing.expectEqualStrings("color-relief", (try support.styleLayerType(&map, "dem-relief")).?);

    _ = try map.setLayerProperty(testing.allocator, "dem-relief", "color-relief-color", "[\"interpolate\",[\"linear\"],[\"elevation\"],0,\"black\",1000,\"white\"]");
    try support.expectCommandError(&runtime, try map.setLayerProperty(testing.allocator, "dem-relief", "color-relief-color", "[\"interpolate\",[\"linear\"],[\"zoom\"],0,\"black\",1,\"white\"]"), error.InvalidArgument);
    try support.expectCommandError(&runtime, try map.addHillshadeLayer(testing.allocator, "bad-hillshade", "point", ""), error.InvalidArgument);
    try testing.expectError(error.InvalidArgument, map.addRasterSourceTiles(testing.allocator, "bad-raster", raster_tiles[0..], .{ .raster_encoding = .mapbox }));

    try testing.expect(try support.removeStyleSource(&runtime, &map, "vector-helper"));
    try map.close();
    try testing.expectEqualStrings(vector_tiles[0], vector_info.tile_json.?.tile_urls[0]);
    try testing.expectEqualStrings("https://example.com/vector.json", vector_url_info.url.?);
}

test "image source helpers add update and copy coordinates" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const coordinates = [4]maplibre.LatLng{
        .{ .latitude = 38.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -123.0 },
    };
    _ = try map.addImageSourceUrl(testing.allocator, "image-url-source", coordinates, "https://example.com/image.png");
    try testing.expectEqual(maplibre.StyleSourceType.image, (try support.styleSourceType(&map, "image-url-source")).?);

    const copied = (try support.imageSourceCoordinates(&map, "image-url-source")).?;
    try testing.expectApproxEqAbs(coordinates[0].latitude, copied[0].latitude, 0.000001);
    try testing.expectApproxEqAbs(coordinates[0].longitude, copied[0].longitude, 0.000001);

    var image_pixels = [_]u8{ 1, 2, 3, 4 };
    _ = try map.addImageSourceImage(testing.allocator, "image-inline-source", coordinates, .{
        .width = 1,
        .height = 1,
        .stride = 4,
        .pixels = image_pixels[0..],
    });
    image_pixels[0] = 9;
    _ = try map.setImageSourceUrl(testing.allocator, "image-inline-source", "https://example.com/replacement.png");
    image_pixels[0] = 5;
    _ = try map.setImageSourceImage(testing.allocator, "image-inline-source", .{
        .width = 1,
        .height = 1,
        .stride = 4,
        .pixels = image_pixels[0..],
    });

    const updated_coordinates = [4]maplibre.LatLng{
        .{ .latitude = 39.0, .longitude = -124.0 },
        .{ .latitude = 39.0, .longitude = -121.0 },
        .{ .latitude = 36.0, .longitude = -121.0 },
        .{ .latitude = 36.0, .longitude = -124.0 },
    };
    _ = try map.setImageSourceCoordinates(testing.allocator, "image-inline-source", updated_coordinates);
    const updated = (try support.imageSourceCoordinates(&map, "image-inline-source")).?;
    try testing.expectApproxEqAbs(updated_coordinates[0].latitude, updated[0].latitude, 0.000001);
    try testing.expectApproxEqAbs(updated_coordinates[0].longitude, updated[0].longitude, 0.000001);

    try testing.expect((try support.imageSourceCoordinates(&map, "missing-image-source")) == null);
    try support.expectCommandError(&runtime, try map.addImageSourceUrl(testing.allocator, "image-url-source", coordinates, "https://example.com/duplicate.png"), error.InvalidArgument);
    try support.expectCommandError(&runtime, try map.setImageSourceUrl(testing.allocator, "point", "https://example.com/not-image.png"), error.InvalidArgument);
}

test "style source JSON buffers reject invalid source data and pass explicit-length IDs" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try support.expectCommandError(&runtime, try map.addStyleSourceJson(testing.allocator, "invalid-json-source", "{\"type\":\"definitely-not-a-source-type\"}"), error.InvalidArgument);
    _ = try map.addStyleSourceJson(testing.allocator, "nul\x00source", "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\",\"features\":[]}}");
    try testing.expect(try support.styleSourceExists(&map, "nul\x00source"));
    var info = (try support.styleSourceInfo(&map, "nul\x00source")).?;
    defer info.deinit();
    try testing.expectEqual(@as(usize, "nul\x00source".len), info.id_size);
}

const CustomGeometryState = struct {
    fetch_count: usize = 0,
    cancel_count: usize = 0,
    release_count: usize = 0,
    last_tile: maplibre.CanonicalTileId = .{ .z = 0, .x = 0, .y = 0 },
};

fn fetchCustomGeometryTile(context: ?*anyopaque, tile_id: maplibre.CanonicalTileId) void {
    const state: *CustomGeometryState = @ptrCast(@alignCast(context.?));
    state.fetch_count += 1;
    state.last_tile = tile_id;
}

fn cancelCustomGeometryTile(context: ?*anyopaque, tile_id: maplibre.CanonicalTileId) void {
    const state: *CustomGeometryState = @ptrCast(@alignCast(context.?));
    state.cancel_count += 1;
    state.last_tile = tile_id;
}

fn releaseCustomGeometryContext(context: ?*anyopaque) void {
    const state: *CustomGeometryState = @ptrCast(@alignCast(context.?));
    state.release_count += 1;
}

test "custom geometry source helpers add sources and accept tile updates" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var state = CustomGeometryState{};
    _ = try map.addCustomGeometrySource(testing.allocator, "custom", .{
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
    });

    try testing.expect(try support.styleSourceExists(&map, "custom"));
    try testing.expectEqual(maplibre.StyleSourceType.custom_vector, (try support.styleSourceType(&map, "custom")).?);

    const tile_id = maplibre.CanonicalTileId{ .z = 0, .x = 0, .y = 0 };
    _ = try map.setCustomGeometrySourceTileData(testing.allocator, "custom", tile_id, "{\"type\":\"FeatureCollection\",\"features\":[]}");
    _ = try map.invalidateCustomGeometrySourceTile(testing.allocator, "custom", tile_id);
    _ = try map.invalidateCustomGeometrySourceRegion(testing.allocator, "custom", .{
        .southwest = .{ .latitude = -1.0, .longitude = -1.0 },
        .northeast = .{ .latitude = 1.0, .longitude = 1.0 },
    });

    const duplicate_custom = try map.addCustomGeometrySource(testing.allocator, "custom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
    });
    try support.expectCommandError(&runtime, duplicate_custom, error.InvalidArgument);
    try testing.expectError(error.InvalidArgument, map.addCustomGeometrySource(testing.allocator, "bad-zoom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
        .max_zoom = 33.0,
    }));
    try support.expectCommandError(&runtime, try map.setCustomGeometrySourceTileData(
        testing.allocator,
        "custom",
        .{ .z = 1, .x = 2, .y = 0 },
        "{\"type\":\"FeatureCollection\",\"features\":[]}",
    ), error.InvalidArgument);
    try support.expectCommandError(&runtime, try map.invalidateCustomGeometrySourceTile(testing.allocator, "point", tile_id), error.InvalidArgument);
}

test "custom MVT vector source helpers add sources and accept tile updates" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var state = CustomGeometryState{};
    _ = try map.addCustomMvtVectorSource(testing.allocator, "custom-mvt", .{
        .fetch_tile = fetchCustomGeometryTile,
        .cancel_tile = cancelCustomGeometryTile,
        .context = &state,
        .min_zoom = 0.0,
        .max_zoom = 14.0,
    });

    try testing.expect(try support.styleSourceExists(&map, "custom-mvt"));
    try testing.expectEqual(maplibre.StyleSourceType.custom_mvt_vector, (try support.styleSourceType(&map, "custom-mvt")).?);

    const tile_id = maplibre.CanonicalTileId{ .z = 0, .x = 0, .y = 0 };
    _ = try map.setCustomMvtVectorSourceTileData(testing.allocator, "custom-mvt", tile_id, "");
    _ = try map.setCustomMvtVectorSourceTileError(testing.allocator, "custom-mvt", tile_id, "missing");
    _ = try map.invalidateCustomMvtVectorSourceTile(testing.allocator, "custom-mvt", tile_id);

    const duplicate_mvt = try map.addCustomMvtVectorSource(testing.allocator, "custom-mvt", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
    });
    try support.expectCommandError(&runtime, duplicate_mvt, error.InvalidArgument);
    try testing.expectError(error.InvalidArgument, map.addCustomMvtVectorSource(testing.allocator, "bad-zoom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
        .max_zoom = 33.0,
    }));
    try support.expectCommandError(&runtime, try map.setCustomMvtVectorSourceTileData(
        testing.allocator,
        "custom",
        tile_id,
        "",
    ), error.InvalidArgument);
    try support.expectCommandError(&runtime, try map.invalidateCustomMvtVectorSourceTile(testing.allocator, "point", tile_id), error.InvalidArgument);
}

// A host owns the context its callbacks read, and the release callback is the
// only report that the map stopped referencing it.
test "a custom geometry source releases its context once per lifetime end" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    var map_open = true;
    defer if (map_open) map.close() catch @panic("map close failed");

    var removed = CustomGeometryState{};
    _ = try map.addCustomGeometrySource(testing.allocator, "removed", .{
        .fetch_tile = fetchCustomGeometryTile,
        .release_context = releaseCustomGeometryContext,
        .context = &removed,
    });
    var retained = CustomGeometryState{};
    _ = try map.addCustomGeometrySource(testing.allocator, "retained", .{
        .fetch_tile = fetchCustomGeometryTile,
        .release_context = releaseCustomGeometryContext,
        .context = &retained,
    });

    try testing.expect(try support.removeStyleSource(&runtime, &map, "removed"));
    try testing.expectEqual(@as(usize, 1), removed.release_count);
    try testing.expectEqual(@as(usize, 0), retained.release_count);

    // The map is what still holds the second source, so its destruction is what
    // releases that context.
    try map.close();
    map_open = false;
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(@as(usize, 1), removed.release_count);
    try testing.expectEqual(@as(usize, 1), retained.release_count);
}
