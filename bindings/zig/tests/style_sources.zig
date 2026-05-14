const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const support = @import("support.zig");

fn waitForEvent(runtime: maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent()) |event| {
            if (std.meta.eql(event.event_type, event_type)) return true;
        }
        try std.Thread.yield();
    }
    return false;
}

fn createLoadedMap() !struct { runtime: maplibre.RuntimeHandle, map: maplibre.MapHandle } {
    const runtime = try maplibre.RuntimeHandle.init(null);
    errdefer runtime.close() catch {};
    const map = try maplibre.MapHandle.create(runtime, .{});
    errdefer map.close() catch {};
    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
    return .{ .runtime = runtime, .map = map };
}

test "style source JSON descriptors expose type info and copied attribution" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const empty_features = [_]maplibre.JsonValue{};
    const data_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "FeatureCollection" } },
        .{ .key = "features", .value = .{ .array = empty_features[0..] } },
    };
    const source_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "geojson" } },
        .{ .key = "data", .value = .{ .object = data_members[0..] } },
    };
    try handles.map.addStyleSourceJson(testing.allocator, "empty-json", .{ .object = source_members[0..] });

    try testing.expect(try handles.map.styleSourceExists(testing.allocator, "empty-json"));
    try testing.expectEqual(maplibre.StyleSourceType.geojson, (try handles.map.getStyleSourceType(testing.allocator, "empty-json")).?);
    const info = (try handles.map.getStyleSourceInfo(testing.allocator, "empty-json")).?;
    try testing.expectEqual(maplibre.StyleSourceType.geojson, info.source_type);
    try testing.expectEqual(@as(usize, "empty-json".len), info.id_size);
    try testing.expect(!info.has_attribution);

    const tile_values = [_]maplibre.JsonValue{.{ .string = "https://example.com/{z}/{x}/{y}.pbf" }};
    const vector_members = [_]maplibre.JsonMember{
        .{ .key = "type", .value = .{ .string = "vector" } },
        .{ .key = "tiles", .value = .{ .array = tile_values[0..] } },
        .{ .key = "attribution", .value = .{ .string = "Example attribution" } },
    };
    try handles.map.addStyleSourceJson(testing.allocator, "vector-meta", .{ .object = vector_members[0..] });

    const vector_info = (try handles.map.getStyleSourceInfo(testing.allocator, "vector-meta")).?;
    try testing.expectEqual(maplibre.StyleSourceType.vector, vector_info.source_type);
    try testing.expect(vector_info.has_attribution);
    try testing.expectEqual(@as(usize, "Example attribution".len), vector_info.attribution_size);

    var attribution = (try handles.map.copyStyleSourceAttribution(testing.allocator, "vector-meta")).?;
    defer attribution.deinit();
    try testing.expectEqualStrings("Example attribution", attribution.value);
}

test "style source removal reports state and copies missing results" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const empty_features = [_]maplibre.Feature{};
    try handles.map.addGeoJsonSourceData(testing.allocator, "remove-me", .{ .feature_collection = empty_features[0..] });
    try testing.expect(try handles.map.styleSourceExists(testing.allocator, "remove-me"));
    try testing.expect(try handles.map.removeStyleSource(testing.allocator, "remove-me"));
    try testing.expect(!try handles.map.styleSourceExists(testing.allocator, "remove-me"));
    try testing.expect(!try handles.map.removeStyleSource(testing.allocator, "remove-me"));
    try testing.expect((try handles.map.getStyleSourceInfo(testing.allocator, "remove-me")) == null);
    try testing.expect((try handles.map.copyStyleSourceAttribution(testing.allocator, "remove-me")) == null);
}

test "style source JSON descriptors reject invalid source data" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const source_members = [_]maplibre.JsonMember{.{ .key = "type", .value = .{ .string = "definitely-not-a-source-type" } }};
    try testing.expectError(
        error.InvalidArgument,
        handles.map.addStyleSourceJson(testing.allocator, "invalid-json-source", .{ .object = source_members[0..] }),
    );
    try testing.expectError(
        error.InvalidString,
        handles.map.addStyleSourceJson(testing.allocator, "bad\x00source", .{ .object = source_members[0..] }),
    );
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
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    var state = CustomGeometryState{};
    try handles.map.addCustomGeometrySource(testing.allocator, "custom", .{
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

    try testing.expect(try handles.map.styleSourceExists(testing.allocator, "custom"));
    try testing.expectEqual(maplibre.StyleSourceType.custom_vector, (try handles.map.getStyleSourceType(testing.allocator, "custom")).?);

    const tile_id = maplibre.CanonicalTileId{ .z = 0, .x = 0, .y = 0 };
    const empty_features = [_]maplibre.Feature{};
    try handles.map.setCustomGeometrySourceTileData(testing.allocator, "custom", tile_id, .{ .feature_collection = empty_features[0..] });
    try handles.map.invalidateCustomGeometrySourceTile(testing.allocator, "custom", tile_id);
    try handles.map.invalidateCustomGeometrySourceRegion(testing.allocator, "custom", .{
        .southwest = .{ .latitude = -1.0, .longitude = -1.0 },
        .northeast = .{ .latitude = 1.0, .longitude = 1.0 },
    });

    try testing.expectError(error.InvalidArgument, handles.map.addCustomGeometrySource(testing.allocator, "custom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
    }));
    try testing.expectError(error.InvalidArgument, handles.map.addCustomGeometrySource(testing.allocator, "bad-zoom", .{
        .fetch_tile = fetchCustomGeometryTile,
        .context = &state,
        .max_zoom = 33.0,
    }));
    try testing.expectError(
        error.InvalidArgument,
        handles.map.setCustomGeometrySourceTileData(
            testing.allocator,
            "custom",
            .{ .z = 1, .x = 2, .y = 0 },
            .{ .feature_collection = empty_features[0..] },
        ),
    );
    try testing.expectError(error.InvalidArgument, handles.map.invalidateCustomGeometrySourceTile(testing.allocator, "point", tile_id));
}
