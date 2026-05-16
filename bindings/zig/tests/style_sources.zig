const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const support = @import("support.zig");

fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent()) |event| {
            if (std.meta.eql(event.event_type, event_type)) return true;
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

test "style source JSON descriptors expose type info and copied attribution" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const empty_features = [_]maplibre.JsonValue{};
    const data_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "FeatureCollection" } },
        .{ .key = "features", .value = .{ .array = empty_features[0..] } },
    };
    const source_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "geojson" } },
        .{ .key = "data", .value = .{ .object = data_members[0..] } },
    };
    try map.addStyleSourceJson(testing.allocator, "empty-json", .{ .object = source_members[0..] });

    try testing.expect(try map.styleSourceExists(testing.allocator, "empty-json"));
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try map.getStyleSourceType(testing.allocator, "empty-json")).?);
    const info = (try map.getStyleSourceInfo(testing.allocator, "empty-json")).?;
    try testing.expectEqual(maplibre.StyleSourceType.geojson, info.source_type);
    try testing.expectEqual(@as(usize, "empty-json".len), info.id_size);
    try testing.expect(!info.has_attribution);

    const tile_values = [_]maplibre.JsonValue{.{ .string = "https://example.com/{z}/{x}/{y}.pbf" }};
    const vector_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "vector" } },
        .{ .key = "tiles", .value = .{ .array = tile_values[0..] } },
        .{ .key = "attribution", .value = .{ .string = "Example attribution" } },
    };
    try map.addStyleSourceJson(testing.allocator, "vector-meta", .{ .object = vector_members[0..] });

    const vector_info = (try map.getStyleSourceInfo(testing.allocator, "vector-meta")).?;
    try testing.expectEqual(maplibre.StyleSourceType.vector, vector_info.source_type);
    try testing.expect(vector_info.has_attribution);
    try testing.expectEqual(@as(usize, "Example attribution".len), vector_info.attribution_size);

    var attribution = (try map.copyStyleSourceAttribution(testing.allocator, "vector-meta")).?;
    defer attribution.deinit();
    try testing.expectEqualStrings("Example attribution", attribution.value);
}

test "style source removal reports state and copies missing results" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const empty_features = [_]maplibre.Feature{};
    try map.addGeoJsonSourceData(testing.allocator, "remove-me", .{ .feature_collection = empty_features[0..] });
    try testing.expect(try map.styleSourceExists(testing.allocator, "remove-me"));
    try testing.expect(try map.removeStyleSource(testing.allocator, "remove-me"));
    try testing.expect(!try map.styleSourceExists(testing.allocator, "remove-me"));
    try testing.expect(!try map.removeStyleSource(testing.allocator, "remove-me"));
    try testing.expect((try map.getStyleSourceInfo(testing.allocator, "remove-me")) == null);
    try testing.expect((try map.copyStyleSourceAttribution(testing.allocator, "remove-me")) == null);
}

test "tile source helpers add vector raster and raster DEM sources" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const vector_tiles = [_][]const u8{"https://example.com/vector/{z}/{x}/{y}.mvt"};
    try map.addVectorSourceTiles(testing.allocator, "vector-helper", vector_tiles[0..], .{
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
    try testing.expectEqual(maplibre.StyleSourceType.vector, (try map.getStyleSourceType(testing.allocator, "vector-helper")).?);
    const vector_info = (try map.getStyleSourceInfo(testing.allocator, "vector-helper")).?;
    try testing.expect(vector_info.has_attribution);
    try testing.expectEqual(@as(usize, "Helper attribution".len), vector_info.attribution_size);

    try map.addVectorSourceUrl(testing.allocator, "vector-url-helper", "https://example.com/vector.json", null);
    try testing.expectEqual(maplibre.StyleSourceType.vector, (try map.getStyleSourceType(testing.allocator, "vector-url-helper")).?);

    const raster_tiles = [_][]const u8{"https://example.com/raster/{z}/{x}/{y}.png"};
    try map.addRasterSourceTiles(testing.allocator, "raster-helper", raster_tiles[0..], .{ .tile_size = 256 });
    try testing.expectEqual(maplibre.StyleSourceType.raster, (try map.getStyleSourceType(testing.allocator, "raster-helper")).?);
    try map.addRasterSourceUrl(testing.allocator, "raster-url-helper", "https://example.com/raster.json", .{ .tile_size = 256 });

    const dem_tiles = [_][]const u8{"https://example.com/dem/{z}/{x}/{y}.png"};
    try map.addRasterDemSourceTiles(testing.allocator, "dem", dem_tiles[0..], .{
        .min_zoom = 0.0,
        .max_zoom = 14.0,
        .tile_size = 256,
        .raster_encoding = .terrarium,
    });
    try map.addRasterDemSourceUrl(testing.allocator, "dem-url", "https://example.com/dem.json", .{ .tile_size = 256, .raster_encoding = .mapbox });
    try testing.expectEqual(maplibre.StyleSourceType.raster_dem, (try map.getStyleSourceType(testing.allocator, "dem")).?);

    try map.addHillshadeLayer(testing.allocator, "dem-hillshade", "dem", "point-circle");
    try map.addColorReliefLayer(testing.allocator, "dem-relief", "dem", "");
    var layer_type = (try map.getStyleLayerType(testing.allocator, "dem-hillshade")).?;
    defer layer_type.deinit();
    try testing.expectEqualStrings("hillshade", layer_type.value);
    var relief_type = (try map.getStyleLayerType(testing.allocator, "dem-relief")).?;
    defer relief_type.deinit();
    try testing.expectEqualStrings("color-relief", relief_type.value);

    const linear_values = [_]maplibre.JsonValue{.{ .string = "linear" }};
    const linear = maplibre.JsonValue{ .array = linear_values[0..] };
    const elevation_values = [_]maplibre.JsonValue{.{ .string = "elevation" }};
    const elevation = maplibre.JsonValue{ .array = elevation_values[0..] };
    const color_ramp_values = [_]maplibre.JsonValue{
        .{ .string = "interpolate" }, linear,                 elevation,
        .{ .double = 0.0 },           .{ .string = "black" }, .{ .double = 1000.0 },
        .{ .string = "white" },
    };
    try map.setLayerProperty(testing.allocator, "dem-relief", "color-relief-color", .{ .array = color_ramp_values[0..] });

    const zoom_values = [_]maplibre.JsonValue{.{ .string = "zoom" }};
    const zoom = maplibre.JsonValue{ .array = zoom_values[0..] };
    const invalid_color_ramp_values = [_]maplibre.JsonValue{
        .{ .string = "interpolate" }, linear,                 zoom,
        .{ .double = 0.0 },           .{ .string = "black" }, .{ .double = 1.0 },
        .{ .string = "white" },
    };
    try testing.expectError(
        error.InvalidArgument,
        map.setLayerProperty(testing.allocator, "dem-relief", "color-relief-color", .{ .array = invalid_color_ramp_values[0..] }),
    );

    try testing.expectError(error.InvalidArgument, map.addHillshadeLayer(testing.allocator, "bad-hillshade", "point", ""));
    try testing.expectError(error.InvalidArgument, map.addRasterSourceTiles(testing.allocator, "bad-raster", raster_tiles[0..], .{ .raster_encoding = .mapbox }));
}

test "image source helpers add update and copy coordinates" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const coordinates = [4]maplibre.LatLng{
        .{ .latitude = 38.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -123.0 },
    };
    try map.addImageSourceUrl(testing.allocator, "image-url-source", coordinates, "https://example.com/image.png");
    try testing.expectEqual(maplibre.StyleSourceType.image, (try map.getStyleSourceType(testing.allocator, "image-url-source")).?);

    const copied = (try map.getImageSourceCoordinates(testing.allocator, "image-url-source")).?;
    try testing.expectApproxEqAbs(coordinates[0].latitude, copied[0].latitude, 0.000001);
    try testing.expectApproxEqAbs(coordinates[0].longitude, copied[0].longitude, 0.000001);

    var image_pixels = [_]u8{ 1, 2, 3, 4 };
    try map.addImageSourceImage(testing.allocator, "image-inline-source", coordinates, .{
        .width = 1,
        .height = 1,
        .stride = 4,
        .pixels = image_pixels[0..],
    });
    image_pixels[0] = 9;
    try map.setImageSourceUrl(testing.allocator, "image-inline-source", "https://example.com/replacement.png");
    image_pixels[0] = 5;
    try map.setImageSourceImage(testing.allocator, "image-inline-source", .{
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
    try map.setImageSourceCoordinates(testing.allocator, "image-inline-source", updated_coordinates);
    const updated = (try map.getImageSourceCoordinates(testing.allocator, "image-inline-source")).?;
    try testing.expectApproxEqAbs(updated_coordinates[0].latitude, updated[0].latitude, 0.000001);
    try testing.expectApproxEqAbs(updated_coordinates[0].longitude, updated[0].longitude, 0.000001);

    try testing.expect((try map.getImageSourceCoordinates(testing.allocator, "missing-image-source")) == null);
    try testing.expectError(error.InvalidArgument, map.addImageSourceUrl(testing.allocator, "image-url-source", coordinates, "https://example.com/duplicate.png"));
    try testing.expectError(error.InvalidArgument, map.setImageSourceUrl(testing.allocator, "point", "https://example.com/not-image.png"));
}

test "style source JSON descriptors reject invalid source data and pass explicit-length IDs" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const source_members = [_]maplibre.JsonMember{.{ .key = "type", .value = .{ .string = "definitely-not-a-source-type" } }};
    try testing.expectError(
        error.InvalidArgument,
        map.addStyleSourceJson(testing.allocator, "invalid-json-source", .{ .object = source_members[0..] }),
    );

    const empty_features = [_]maplibre.JsonValue{};
    const data_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "FeatureCollection" } },
        .{ .key = "features", .value = .{ .array = empty_features[0..] } },
    };
    const valid_source_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "geojson" } },
        .{ .key = "data", .value = .{ .object = data_members[0..] } },
    };
    try map.addStyleSourceJson(testing.allocator, "nul\x00source", .{ .object = valid_source_members[0..] });
    try testing.expect(try map.styleSourceExists(testing.allocator, "nul\x00source"));
    const info = (try map.getStyleSourceInfo(testing.allocator, "nul\x00source")).?;
    try testing.expectEqual(@as(usize, "nul\x00source".len), info.id_size);
}

const CustomGeometryState = struct {
    fetch_count: usize = 0,
    cancel_count: usize = 0,
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

test "custom geometry source helpers add sources and accept tile updates" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var state = CustomGeometryState{};
    try map.addCustomGeometrySource(testing.allocator, "custom", .{
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

    try testing.expect(try map.styleSourceExists(testing.allocator, "custom"));
    try testing.expectEqual(maplibre.StyleSourceType.custom_vector, (try map.getStyleSourceType(testing.allocator, "custom")).?);

    const tile_id = maplibre.CanonicalTileId{ .z = 0, .x = 0, .y = 0 };
    const empty_features = [_]maplibre.Feature{};
    try map.setCustomGeometrySourceTileData(testing.allocator, "custom", tile_id, .{ .feature_collection = empty_features[0..] });
    try map.invalidateCustomGeometrySourceTile(testing.allocator, "custom", tile_id);
    try map.invalidateCustomGeometrySourceRegion(testing.allocator, "custom", .{
        .southwest = .{ .latitude = -1.0, .longitude = -1.0 },
        .northeast = .{ .latitude = 1.0, .longitude = 1.0 },
    });

    try testing.expectError(error.InvalidArgument, map.addCustomGeometrySource(testing.allocator, "custom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
    }));
    try testing.expectError(error.InvalidArgument, map.addCustomGeometrySource(testing.allocator, "bad-zoom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
        .max_zoom = 33.0,
    }));
    try testing.expectError(
        error.InvalidArgument,
        map.setCustomGeometrySourceTileData(
            testing.allocator,
            "custom",
            .{ .z = 1, .x = 2, .y = 0 },
            .{ .feature_collection = empty_features[0..] },
        ),
    );
    try testing.expectError(error.InvalidArgument, map.invalidateCustomGeometrySourceTile(testing.allocator, "point", tile_id));
}
