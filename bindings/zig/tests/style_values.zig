const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");
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
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var source_ids = try map.listStyleSourceIds(testing.allocator);
    defer source_ids.deinit();
    try expectListContains(source_ids, "point");

    var layer_ids = try map.listStyleLayerIds(testing.allocator);
    defer layer_ids.deinit();
    try expectListContains(layer_ids, "background");
    try expectListContains(layer_ids, "point-circle");
}

test "style layer JSON helpers manage lifecycle and order" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const empty_features = [_]maplibre.Feature{};
    try map.addGeoJsonSourceData(testing.allocator, "empty-layer-source", .{ .feature_collection = empty_features[0..] }, null);

    const layer_members = [_]maplibre.JsonMember{
        .{ .key = "id", .value = .{ .string = "empty-circle" } },
        .{ .key = "type", .value = .{ .string = "circle" } },
        .{ .key = "source", .value = .{ .string = "empty-layer-source" } },
    };
    try map.addStyleLayerJson(testing.allocator, .{ .object = layer_members[0..] }, "point-circle");
    try testing.expect(try map.styleLayerExists("empty-circle"));

    var before_move = try map.listStyleLayerIds(testing.allocator);
    defer before_move.deinit();
    try testing.expect((try listIndexOf(before_move, "empty-circle")) < (try listIndexOf(before_move, "point-circle")));

    var layer_type = (try map.getStyleLayerType(testing.allocator, "empty-circle")).?;
    defer layer_type.deinit();
    try testing.expectEqualStrings("circle", layer_type.value);

    var layer_json = (try map.getStyleLayerJson(testing.allocator, "empty-circle")).?;
    defer layer_json.deinit(testing.allocator);
    try expectObjectString(layer_json, "id", "empty-circle");

    try map.moveStyleLayer("empty-circle", "");
    try testing.expectError(error.InvalidState, map.removeStyleSource(testing.allocator, "empty-layer-source"));
    try testing.expect(try map.removeStyleLayer("empty-circle"));
    try testing.expect(!try map.styleLayerExists("empty-circle"));
    try testing.expect(try map.removeStyleSource(testing.allocator, "empty-layer-source"));
    try testing.expect((try map.getStyleLayerJson(testing.allocator, "empty-circle")) == null);
}

test "nine-patch style images round-trip stretch, content, and text fit" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const pixels = [_]u8{0} ** 16;
    const image = maplibre.PremultipliedRgba8Image{
        .width = 2,
        .height = 2,
        .stride = 8,
        .pixels = pixels[0..],
    };
    const stretch_x = [_]maplibre.ImageStretch{.{ .from = 0.0, .to = 1.0 }};
    const stretch_y = [_]maplibre.ImageStretch{
        .{ .from = 0.0, .to = 1.0 },
        .{ .from = 1.0, .to = 2.0 },
    };
    try map.setStyleImage(testing.allocator, "patch", image, .{
        .stretch_x = stretch_x[0..],
        .stretch_y = stretch_y[0..],
        .content = .{ .left = 0.5, .top = 0.5, .right = 1.5, .bottom = 1.5 },
        .text_fit_height = .proportional,
    });

    const info = (try map.getStyleImageInfo(testing.allocator, "patch")).?;
    try testing.expectEqual(@as(usize, 1), info.stretch_x_count);
    try testing.expectEqual(@as(usize, 2), info.stretch_y_count);
    try testing.expectEqual(@as(f32, 1.5), info.content.?.right);
    // An absent text fit stays distinguishable from a present default.
    try testing.expect(info.text_fit_width == null);
    try testing.expectEqual(maplibre.StyleImageTextFit.proportional, info.text_fit_height.?);

    var stretches = (try map.copyStyleImageStretches(testing.allocator, "patch")).?;
    defer stretches.deinit();
    try testing.expectEqual(@as(usize, 1), stretches.stretch_x.len);
    try testing.expectEqual(@as(f32, 1.0), stretches.stretch_x[0].to);
    try testing.expectEqual(@as(usize, 2), stretches.stretch_y.len);
    try testing.expectEqual(@as(f32, 2.0), stretches.stretch_y[1].to);

    try testing.expect((try map.copyStyleImageStretches(testing.allocator, "missing")) == null);

    // A backwards interval is rejected by C.
    const backwards = [_]maplibre.ImageStretch{.{ .from = 2.0, .to = 1.0 }};
    try testing.expectError(
        error.InvalidArgument,
        map.setStyleImage(testing.allocator, "bad", image, .{ .stretch_x = backwards[0..] }),
    );
}

test "layer base accessors round-trip source, zoom range, and visibility" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    {
        var empty = try map.copyLayerSourceLayer(testing.allocator, "point-circle");
        defer empty.deinit();
        try testing.expectEqualStrings("", empty.value);
    }
    try map.setLayerSourceLayer(testing.allocator, "point-circle", "roads");
    {
        var source_layer = try map.copyLayerSourceLayer(testing.allocator, "point-circle");
        defer source_layer.deinit();
        try testing.expectEqualStrings("roads", source_layer.value);
    }
    {
        var source_id = try map.copyLayerSourceId(testing.allocator, "point-circle");
        defer source_id.deinit();
        try testing.expectEqualStrings("point", source_id.value);
    }

    // A layer type that takes no source is rejected rather than silently ignored.
    try testing.expectError(
        error.InvalidArgument,
        map.setLayerSourceLayer(testing.allocator, "background", "roads"),
    );
    {
        var background_source = try map.copyLayerSourceId(testing.allocator, "background");
        defer background_source.deinit();
        try testing.expectEqualStrings("", background_source.value);
    }

    // An unset zoom range crosses the boundary as infinities.
    try testing.expectEqual(
        -std.math.inf(f64),
        try map.getLayerMinZoom(testing.allocator, "point-circle"),
    );
    try testing.expectEqual(
        std.math.inf(f64),
        try map.getLayerMaxZoom(testing.allocator, "point-circle"),
    );
    try map.setLayerMinZoom(testing.allocator, "point-circle", 4.0);
    try map.setLayerMaxZoom(testing.allocator, "point-circle", 12.5);
    try testing.expectEqual(@as(f64, 4.0), try map.getLayerMinZoom(testing.allocator, "point-circle"));
    try testing.expectEqual(@as(f64, 12.5), try map.getLayerMaxZoom(testing.allocator, "point-circle"));

    try testing.expectEqual(
        maplibre.StyleLayerVisibility.visible,
        try map.getLayerVisibility(testing.allocator, "point-circle"),
    );
    try map.setLayerVisibility(testing.allocator, "point-circle", .none);
    try testing.expectEqual(
        maplibre.StyleLayerVisibility.none,
        try map.getLayerVisibility(testing.allocator, "point-circle"),
    );

    // An unknown raw visibility passes through to C, which rejects it.
    try testing.expectError(
        error.InvalidArgument,
        map.setLayerVisibility(testing.allocator, "point-circle", .{ .unknown = 900 }),
    );
    try testing.expectError(
        error.InvalidArgument,
        map.getLayerMinZoom(testing.allocator, "missing"),
    );
}

test "layer properties accept semantic JSON values and return owned snapshots" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", .{ .double = 18.0 });

    var snapshot = (try map.getLayerProperty(testing.allocator, "point-circle", "circle-radius")).?;
    defer snapshot.deinit(testing.allocator);
    try testing.expectEqual(@as(f64, 18.0), snapshot.double);
}

test "layer filters accept nested semantic JSON arrays and return owned snapshots" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const get_values = [_]maplibre.JsonValue{ .{ .string = "get" }, .{ .string = "visible" } };
    const get_expr = maplibre.JsonValue{ .array = get_values[0..] };
    const filter_values = [_]maplibre.JsonValue{ .{ .string = "==" }, get_expr, .{ .bool = true } };
    const filter = maplibre.JsonValue{ .array = filter_values[0..] };

    try map.setLayerFilter(testing.allocator, "point-circle", filter);

    var snapshot = (try map.getLayerFilter(testing.allocator, "point-circle")).?;
    defer snapshot.deinit(testing.allocator);
    const array = snapshot.array;
    try testing.expectEqual(@as(usize, 3), array.len);
    try testing.expectEqualStrings("==", array[0].string);

    try map.setLayerFilter(testing.allocator, "point-circle", null);
    var cleared = try map.getLayerFilter(testing.allocator, "point-circle");
    if (cleared) |*value| value.deinit(testing.allocator);
}

test "style light accepts full JSON and property updates" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    const position_values = [_]maplibre.JsonValue{ .{ .double = 1.0 }, .{ .double = 2.0 }, .{ .double = 3.0 } };
    const light_members = [_]maplibre.JsonMember{
        .{ .key = "color", .value = .{ .string = "blue" } },
        .{ .key = "intensity", .value = .{ .double = 0.3 } },
        .{ .key = "position", .value = .{ .array = position_values[0..] } },
    };
    try map.setStyleLightJson(testing.allocator, .{ .object = light_members[0..] });

    var snapshot = (try map.getStyleLightProperty(testing.allocator, "intensity")).?;
    defer snapshot.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 0.3), snapshot.double, 0.000001);

    try map.setStyleLightProperty(testing.allocator, "intensity", .{ .double = 0.75 });
    var updated = (try map.getStyleLightProperty(testing.allocator, "intensity")).?;
    defer updated.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 0.75), updated.double, 0.000001);

    try testing.expect((try map.getStyleLightProperty(testing.allocator, "unknown-light-property")) == null);
    try testing.expectError(error.InvalidArgument, map.setStyleLightProperty(testing.allocator, "intensity", .{ .bool = false }));
}

test "runtime style images copy premultiplied RGBA8 pixels" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var pixels = [_]u8{
        10,  20,  30,  255, 40, 50, 60, 255,
        0,   0,   0,   0,   70, 80, 90, 255,
        100, 110, 120, 255, 0,  0,  0,  0,
    };
    try map.setStyleImage(testing.allocator, "runtime-icon", .{
        .width = 2,
        .height = 2,
        .stride = 12,
        .pixels = pixels[0..],
    }, .{ .pixel_ratio = 2.0, .sdf = true });
    pixels[0] = 200;

    try testing.expect(try map.styleImageExists(testing.allocator, "runtime-icon"));
    const info = (try map.getStyleImageInfo(testing.allocator, "runtime-icon")).?;
    try testing.expectEqual(@as(u32, 2), info.width);
    try testing.expectEqual(@as(u32, 2), info.height);
    try testing.expectEqual(@as(u32, 8), info.stride);
    try testing.expectEqual(@as(usize, 16), info.byte_length);
    try testing.expectApproxEqAbs(@as(f32, 2.0), info.pixel_ratio, 0.000001);
    try testing.expect(info.sdf);

    var copied = (try map.copyStyleImagePremultipliedRgba8(testing.allocator, "runtime-icon")).?;
    defer copied.deinit();
    try testing.expectEqualSlices(u8, &[_]u8{
        10, 20, 30, 255, 40,  50,  60,  255,
        70, 80, 90, 255, 100, 110, 120, 255,
    }, copied.pixels);

    var replacement_pixels = [_]u8{ 1, 2, 3, 4 };
    try map.setStyleImage(testing.allocator, "runtime-icon", .{ .width = 1, .height = 1, .stride = 4, .pixels = replacement_pixels[0..] }, null);
    const replacement_info = (try map.getStyleImageInfo(testing.allocator, "runtime-icon")).?;
    try testing.expectEqual(@as(u32, 1), replacement_info.width);
    try testing.expectEqual(@as(u32, 1), replacement_info.height);
    try testing.expectApproxEqAbs(@as(f32, 1.0), replacement_info.pixel_ratio, 0.000001);
    try testing.expect(!replacement_info.sdf);

    try testing.expect(try map.removeStyleImage(testing.allocator, "runtime-icon"));
    try testing.expect(!try map.styleImageExists(testing.allocator, "runtime-icon"));
    try testing.expect(!try map.removeStyleImage(testing.allocator, "runtime-icon"));
}

test "location indicator helpers set focused properties" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try map.addLocationIndicatorLayer(testing.allocator, "location", "point-circle");
    var layer_type = (try map.getStyleLayerType(testing.allocator, "location")).?;
    defer layer_type.deinit();
    try testing.expectEqualStrings("location-indicator", layer_type.value);

    try map.setLocationIndicatorLocation(testing.allocator, "location", .{ .latitude = 37.7749, .longitude = -122.4194 }, 12.0);
    var location = (try map.getLayerProperty(testing.allocator, "location", "location")).?;
    defer location.deinit(testing.allocator);
    const location_values = location.array;
    try testing.expectEqual(@as(usize, 3), location_values.len);
    try testing.expectApproxEqAbs(@as(f64, 37.7749), location_values[0].double, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, -122.4194), location_values[1].double, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 12.0), location_values[2].double, 0.000001);

    try map.setLocationIndicatorBearing(testing.allocator, "location", 45.0);
    var bearing = (try map.getLayerProperty(testing.allocator, "location", "bearing")).?;
    defer bearing.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 45.0), bearing.double, 0.000001);

    try map.setLocationIndicatorAccuracyRadius(testing.allocator, "location", 33.0);
    var radius = (try map.getLayerProperty(testing.allocator, "location", "accuracy-radius")).?;
    defer radius.deinit(testing.allocator);
    try testing.expectApproxEqAbs(@as(f64, 33.0), radius.double, 0.000001);

    try map.setLocationIndicatorImageName(testing.allocator, "location", .top, "top-icon");
    var top_image = (try map.getLayerProperty(testing.allocator, "location", "top-image")).?;
    defer top_image.deinit(testing.allocator);
    switch (top_image) {
        .null => return error.MissingTopImage,
        else => {},
    }
    try map.setLocationIndicatorImageName(testing.allocator, "location", .bearing, "bearing-icon");
    try map.setLocationIndicatorImageName(testing.allocator, "location", .shadow, "shadow-icon");

    try testing.expectError(error.InvalidArgument, map.setLocationIndicatorAccuracyRadius(testing.allocator, "location", -1.0));
    try testing.expectError(error.InvalidArgument, map.setLocationIndicatorBearing(testing.allocator, "point-circle", 1.0));
}

test "style JSON descriptors reject invalid values" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try testing.expectError(
        error.InvalidArgument,
        map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", .{ .double = std.math.inf(f64) }),
    );
    try testing.expectError(
        error.InvalidArgument,
        map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", .{ .string = "not a radius" }),
    );
}

const transition_style_json =
    \\{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}
;

test "style transition options round trip through the C API" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    // The style parser fills in its own 300ms duration for a style that declares no transition.
    const parsed = try map.getStyleTransitionOptions();
    try testing.expectEqual(@as(?f64, 300.0), parsed.duration_ms);
    try testing.expectEqual(@as(?f64, null), parsed.delay_ms);

    try map.setStyleJson(testing.allocator, transition_style_json);
    const declared = try map.getStyleTransitionOptions();
    try testing.expectEqual(@as(?f64, 750.0), declared.duration_ms);
    try testing.expectEqual(@as(?f64, 100.0), declared.delay_ms);
    try testing.expectEqual(@as(?bool, true), declared.enable_placement_transitions);

    // A present zero stays distinguishable from an absent field, and an absent field clears what
    // the style declared rather than merging into it.
    const options = maplibre.StyleTransitionOptions{
        .duration_ms = 0.0,
        .enable_placement_transitions = false,
    };
    try map.setStyleTransitionOptions(options);
    try testing.expectEqual(options, try map.getStyleTransitionOptions());

    // Omitting the flag leaves the cross-fade on rather than clearing it.
    try map.setStyleTransitionOptions(.{ .duration_ms = 250.0 });
    try testing.expectEqual(
        @as(?bool, true),
        (try map.getStyleTransitionOptions()).enable_placement_transitions,
    );

    // Loading a style replaces the override with what that style declares.
    try map.setStyleJson(testing.allocator, transition_style_json);
    try testing.expectEqual(declared, try map.getStyleTransitionOptions());

    try testing.expectError(
        error.InvalidArgument,
        map.setStyleTransitionOptions(.{ .delay_ms = -1.0 }),
    );
}
