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

fn expectListContains(list: maplibre.StringList, expected: []const u8) !void {
    for (list.items) |item| {
        if (std.mem.eql(u8, item, expected)) return;
    }
    return error.MissingListEntry;
}

fn listIndexOf(list: maplibre.StringList, expected: []const u8) !usize {
    for (list.items, 0..) |item, index| {
        if (std.mem.eql(u8, item, expected)) return index;
    }
    return error.MissingListEntry;
}

fn expectObjectString(value: maplibre.OwnedJsonValue, key: []const u8, expected: []const u8) !void {
    for (value.object) |member| {
        if (std.mem.eql(u8, member.key, key)) {
            try testing.expectEqualStrings(expected, member.value.string);
            return;
        }
    }
    return error.MissingObjectMember;
}

test "style ID lists are copied into owned Zig output" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    var source_ids = try handles.map.listStyleSourceIds(testing.allocator);
    defer source_ids.deinit();
    try expectListContains(source_ids, "point");

    var layer_ids = try handles.map.listStyleLayerIds(testing.allocator);
    defer layer_ids.deinit();
    try expectListContains(layer_ids, "background");
    try expectListContains(layer_ids, "point-circle");
}

test "style layer JSON helpers manage lifecycle and order" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const empty_features = [_]maplibre.Feature{};
    try handles.map.addGeoJsonSourceData(testing.allocator, "empty-layer-source", .{ .feature_collection = empty_features[0..] });

    const layer_members = [_]maplibre.JsonMember{
        .{ .key = "id", .value = .{ .string = "empty-circle" } },
        .{ .key = "type", .value = .{ .string = "circle" } },
        .{ .key = "source", .value = .{ .string = "empty-layer-source" } },
    };
    try handles.map.addStyleLayerJson(testing.allocator, .{ .object = layer_members[0..] }, "point-circle");
    try testing.expect(try handles.map.styleLayerExists("empty-circle"));

    var before_move = try handles.map.listStyleLayerIds(testing.allocator);
    defer before_move.deinit();
    try testing.expect((try listIndexOf(before_move, "empty-circle")) < (try listIndexOf(before_move, "point-circle")));

    var layer_type = (try handles.map.getStyleLayerType(testing.allocator, "empty-circle")).?;
    defer layer_type.deinit();
    try testing.expectEqualStrings("circle", layer_type.value);

    var layer_json = (try handles.map.getStyleLayerJson(testing.allocator, "empty-circle")).?;
    defer layer_json.deinit(testing.allocator);
    try expectObjectString(layer_json, "id", "empty-circle");

    try handles.map.moveStyleLayer("empty-circle", "");
    try testing.expectError(error.InvalidState, handles.map.removeStyleSource(testing.allocator, "empty-layer-source"));
    try testing.expect(try handles.map.removeStyleLayer("empty-circle"));
    try testing.expect(!try handles.map.styleLayerExists("empty-circle"));
    try testing.expect(try handles.map.removeStyleSource(testing.allocator, "empty-layer-source"));
    try testing.expect((try handles.map.getStyleLayerJson(testing.allocator, "empty-circle")) == null);
}

test "layer properties accept semantic JSON values and return owned snapshots" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    try handles.map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", .{ .double = 18.0 });

    var snapshot = (try handles.map.getLayerProperty(testing.allocator, "point-circle", "circle-radius")).?;
    defer snapshot.deinit(testing.allocator);
    try testing.expectEqual(@as(f64, 18.0), snapshot.double);
}

test "layer filters accept nested semantic JSON arrays and return owned snapshots" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const get_values = [_]maplibre.JsonValue{ .{ .string = "get" }, .{ .string = "visible" } };
    const get_expr = maplibre.JsonValue{ .array = get_values[0..] };
    const filter_values = [_]maplibre.JsonValue{ .{ .string = "==" }, get_expr, .{ .bool = true } };
    const filter = maplibre.JsonValue{ .array = filter_values[0..] };

    try handles.map.setLayerFilter(testing.allocator, "point-circle", filter);

    var snapshot = (try handles.map.getLayerFilter(testing.allocator, "point-circle")).?;
    defer snapshot.deinit(testing.allocator);
    const array = snapshot.array;
    try testing.expectEqual(@as(usize, 3), array.len);
    try testing.expectEqualStrings("==", array[0].string);

    try handles.map.setLayerFilter(testing.allocator, "point-circle", null);
    var cleared = try handles.map.getLayerFilter(testing.allocator, "point-circle");
    if (cleared) |*value| value.deinit(testing.allocator);
}

test "style light accepts full JSON and property updates" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const position_values = [_]maplibre.JsonValue{ .{ .double = 1.0 }, .{ .double = 2.0 }, .{ .double = 3.0 } };
    const light_members = [_]maplibre.JsonMember{
        .{ .key = "color", .value = .{ .string = "blue" } },
        .{ .key = "intensity", .value = .{ .double = 0.3 } },
        .{ .key = "position", .value = .{ .array = position_values[0..] } },
    };
    try handles.map.setStyleLightJson(testing.allocator, .{ .object = light_members[0..] });

    var snapshot = (try handles.map.getStyleLightProperty(testing.allocator, "intensity")).?;
    defer snapshot.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 0.3), snapshot.double, 0.000001);

    try handles.map.setStyleLightProperty(testing.allocator, "intensity", .{ .double = 0.75 });
    var updated = (try handles.map.getStyleLightProperty(testing.allocator, "intensity")).?;
    defer updated.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 0.75), updated.double, 0.000001);

    try testing.expect((try handles.map.getStyleLightProperty(testing.allocator, "unknown-light-property")) == null);
    try testing.expectError(error.InvalidArgument, handles.map.setStyleLightProperty(testing.allocator, "intensity", .{ .bool = false }));
}

test "runtime style images copy premultiplied RGBA8 pixels" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    var pixels = [_]u8{
        10,  20,  30,  255, 40, 50, 60, 255,
        0,   0,   0,   0,   70, 80, 90, 255,
        100, 110, 120, 255, 0,  0,  0,  0,
    };
    try handles.map.setStyleImage(testing.allocator, "runtime-icon", .{
        .width = 2,
        .height = 2,
        .stride = 12,
        .pixels = pixels[0..],
    }, .{ .pixel_ratio = 2.0, .sdf = true });
    pixels[0] = 200;

    try testing.expect(try handles.map.styleImageExists(testing.allocator, "runtime-icon"));
    const info = (try handles.map.getStyleImageInfo(testing.allocator, "runtime-icon")).?;
    try testing.expectEqual(@as(u32, 2), info.width);
    try testing.expectEqual(@as(u32, 2), info.height);
    try testing.expectEqual(@as(u32, 8), info.stride);
    try testing.expectEqual(@as(usize, 16), info.byte_length);
    try testing.expectApproxEqAbs(@as(f32, 2.0), info.pixel_ratio, 0.000001);
    try testing.expect(info.sdf);

    var copied = (try handles.map.copyStyleImagePremultipliedRgba8(testing.allocator, "runtime-icon")).?;
    defer copied.deinit();
    try testing.expectEqualSlices(u8, &[_]u8{
        10, 20, 30, 255, 40,  50,  60,  255,
        70, 80, 90, 255, 100, 110, 120, 255,
    }, copied.pixels);

    var replacement_pixels = [_]u8{ 1, 2, 3, 4 };
    try handles.map.setStyleImage(testing.allocator, "runtime-icon", .{ .width = 1, .height = 1, .stride = 4, .pixels = replacement_pixels[0..] }, null);
    const replacement_info = (try handles.map.getStyleImageInfo(testing.allocator, "runtime-icon")).?;
    try testing.expectEqual(@as(u32, 1), replacement_info.width);
    try testing.expectEqual(@as(u32, 1), replacement_info.height);
    try testing.expectApproxEqAbs(@as(f32, 1.0), replacement_info.pixel_ratio, 0.000001);
    try testing.expect(!replacement_info.sdf);

    try testing.expect(try handles.map.removeStyleImage(testing.allocator, "runtime-icon"));
    try testing.expect(!try handles.map.styleImageExists(testing.allocator, "runtime-icon"));
    try testing.expect(!try handles.map.removeStyleImage(testing.allocator, "runtime-icon"));
}

test "location indicator helpers set focused properties" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    try handles.map.addLocationIndicatorLayer(testing.allocator, "location", "point-circle");
    var layer_type = (try handles.map.getStyleLayerType(testing.allocator, "location")).?;
    defer layer_type.deinit();
    try testing.expectEqualStrings("location-indicator", layer_type.value);

    try handles.map.setLocationIndicatorLocation(testing.allocator, "location", .{ .latitude = 37.7749, .longitude = -122.4194 }, 12.0);
    var location = (try handles.map.getLayerProperty(testing.allocator, "location", "location")).?;
    defer location.deinit(testing.allocator);
    const location_values = location.array;
    try testing.expectEqual(@as(usize, 3), location_values.len);
    try testing.expectApproxEqAbs(@as(f64, -122.4194), location_values[0].double, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 37.7749), location_values[1].double, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 12.0), location_values[2].double, 0.000001);

    try handles.map.setLocationIndicatorBearing(testing.allocator, "location", 45.0);
    var bearing = (try handles.map.getLayerProperty(testing.allocator, "location", "bearing")).?;
    defer bearing.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 45.0), bearing.double, 0.000001);

    try handles.map.setLocationIndicatorAccuracyRadius(testing.allocator, "location", 33.0);
    var radius = (try handles.map.getLayerProperty(testing.allocator, "location", "accuracy-radius")).?;
    defer radius.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 33.0), radius.double, 0.000001);

    try handles.map.setLocationIndicatorImageName(testing.allocator, "location", .top, "top-icon");
    var top_image = (try handles.map.getLayerProperty(testing.allocator, "location", "top-image")).?;
    defer top_image.deinit(testing.allocator);
    switch (top_image) {
        .null => return error.MissingTopImage,
        else => {},
    }
    try handles.map.setLocationIndicatorImageName(testing.allocator, "location", .bearing, "bearing-icon");
    try handles.map.setLocationIndicatorImageName(testing.allocator, "location", .shadow, "shadow-icon");

    try testing.expectError(error.InvalidArgument, handles.map.setLocationIndicatorAccuracyRadius(testing.allocator, "location", -1.0));
    try testing.expectError(error.InvalidArgument, handles.map.setLocationIndicatorBearing(testing.allocator, "point-circle", 1.0));
}

test "style JSON descriptors reject invalid values" {
    const handles = try createLoadedMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    try testing.expectError(
        error.InvalidArgument,
        handles.map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", .{ .double = std.math.inf(f64) }),
    );
    try testing.expectError(
        error.InvalidArgument,
        handles.map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", .{ .string = "not a radius" }),
    );
}
