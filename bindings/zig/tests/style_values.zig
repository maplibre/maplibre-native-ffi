const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

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

test "style ID lists are copied into owned Zig output" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var source_ids = try support.listStyleSourceIds(&map);
    defer source_ids.deinit();
    try expectListContains(source_ids, "point");

    var layer_ids = try support.listStyleLayerIds(&map);
    defer layer_ids.deinit();
    try expectListContains(layer_ids, "background");
    try expectListContains(layer_ids, "point-circle");
}

test "style layer JSON helpers manage lifecycle and order" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    _ = try map.addGeoJsonSourceData(testing.allocator, "empty-layer-source", "{\"type\":\"FeatureCollection\",\"features\":[]}", null);
    _ = try map.addStyleLayerJson(testing.allocator, "{\"id\":\"empty-circle\",\"type\":\"circle\",\"source\":\"empty-layer-source\"}", "point-circle");
    try testing.expect(try support.styleLayerExists(&map, "empty-circle"));

    var before_move = try support.listStyleLayerIds(&map);
    defer before_move.deinit();
    try testing.expect((try listIndexOf(before_move, "empty-circle")) < (try listIndexOf(before_move, "point-circle")));

    try testing.expectEqualStrings("circle", (try support.styleLayerType(&map, "empty-circle")).?);

    var layer_json = (try support.styleLayerJson(&map, "empty-circle")).?;
    defer layer_json.deinit();
    try testing.expect(std.mem.indexOf(u8, layer_json.value, "\"id\":\"empty-circle\"") != null);

    _ = try map.moveStyleLayer("empty-circle", "");
    // A source a layer still uses fails its removal with INVALID_STATE.
    try testing.expectError(error.InvalidState, support.removeStyleSource(&runtime, &map, "empty-layer-source"));
    try testing.expect(try support.removeStyleLayer(&runtime, &map, "empty-circle"));
    try testing.expect(!try support.styleLayerExists(&map, "empty-circle"));
    try testing.expect(try support.removeStyleSource(&runtime, &map, "empty-layer-source"));
    // A removal of a missing layer is accepted, then fails with NOT_FOUND.
    try testing.expect(!try support.removeStyleLayer(&runtime, &map, "empty-circle"));
    try testing.expect((try support.styleLayerJson(&map, "empty-circle")) == null);
    try testing.expect((try support.styleLayerInfo(&map, "empty-circle")) == null);
}

test "nine-patch style images round-trip stretch, content, and text fit" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
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
    _ = try map.setStyleImage(testing.allocator, "patch", image, .{
        .stretch_x = stretch_x[0..],
        .stretch_y = stretch_y[0..],
        .content = .{ .left = 0.5, .top = 0.5, .right = 1.5, .bottom = 1.5 },
        .text_fit_height = .proportional,
    });

    const info = (try support.styleImageInfo(&map, "patch")).?;
    try testing.expectEqual(@as(usize, 1), info.stretch_x_count);
    try testing.expectEqual(@as(usize, 2), info.stretch_y_count);
    try testing.expectEqual(@as(f32, 1.5), info.content.?.right);
    // An absent text fit stays distinguishable from a present default.
    try testing.expect(info.text_fit_width == null);
    try testing.expectEqual(maplibre.StyleImageTextFit.proportional, info.text_fit_height.?);

    var stretches = (try support.styleImageStretches(&map, "patch")).?;
    defer stretches.deinit();
    try testing.expectEqual(@as(usize, 1), stretches.stretch_x.len);
    try testing.expectEqual(@as(f32, 1.0), stretches.stretch_x[0].to);
    try testing.expectEqual(@as(usize, 2), stretches.stretch_y.len);
    try testing.expectEqual(@as(f32, 2.0), stretches.stretch_y[1].to);

    try testing.expect((try support.styleImageStretches(&map, "missing")) == null);

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
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    {
        var empty = try support.layerSourceLayer(&map, "point-circle");
        defer empty.deinit();
        try testing.expectEqualStrings("", empty.value);
    }
    _ = try map.setLayerSourceLayer(testing.allocator, "point-circle", "roads");
    {
        var source_layer = try support.layerSourceLayer(&map, "point-circle");
        defer source_layer.deinit();
        try testing.expectEqualStrings("roads", source_layer.value);
    }
    {
        var source_id = try support.layerSourceId(&map, "point-circle");
        defer source_id.deinit();
        try testing.expectEqualStrings("point", source_id.value);
    }

    // Semantic rejection is reported asynchronously after command acceptance.
    try testing.expect((try map.setLayerSourceLayer(testing.allocator, "background", "roads")) != 0);
    {
        var background_source = try support.layerSourceId(&map, "background");
        defer background_source.deinit();
        try testing.expectEqualStrings("", background_source.value);
    }

    // The layer-info aggregate carries the type, zoom range, visibility, and
    // string sizes for the copy operations. An unset zoom range crosses the
    // boundary as infinities.
    const unset = (try support.styleLayerInfo(&map, "point-circle")).?;
    try testing.expectEqualStrings("circle", unset.layer_type);
    try testing.expectEqual(-std.math.inf(f64), unset.min_zoom);
    try testing.expectEqual(std.math.inf(f64), unset.max_zoom);
    try testing.expectEqual(maplibre.StyleLayerVisibility.visible, unset.visibility);
    // The reported sizes fit the strings the copy operations return.
    try testing.expectEqual(@as(?usize, "point".len), unset.source_id_size);
    try testing.expectEqual(@as(?usize, "roads".len), unset.source_layer_size);
    {
        var copied_source_id = try support.layerSourceId(&map, "point-circle");
        defer copied_source_id.deinit();
        try testing.expectEqual(unset.source_id_size.?, copied_source_id.value.len);
    }

    _ = try map.setLayerMinZoom(testing.allocator, "point-circle", 4.0);
    _ = try map.setLayerMaxZoom(testing.allocator, "point-circle", 12.5);
    _ = try map.setLayerVisibility(testing.allocator, "point-circle", .none);
    const tuned = (try support.styleLayerInfo(&map, "point-circle")).?;
    try testing.expectEqual(@as(f64, 4.0), tuned.min_zoom);
    try testing.expectEqual(@as(f64, 12.5), tuned.max_zoom);
    try testing.expectEqual(maplibre.StyleLayerVisibility.none, tuned.visibility);

    // A layer that names no source reports absent sizes.
    const background = (try support.styleLayerInfo(&map, "background")).?;
    try testing.expectEqualStrings("background", background.layer_type);
    try testing.expectEqual(@as(?usize, null), background.source_layer_size);

    // An unknown raw visibility is accepted into the ordered queue, then fails.
    const rejected_visibility =
        try map.setLayerVisibility(testing.allocator, "point-circle", .{ .unknown = 900 });
    try testing.expect(std.meta.eql(
        try support.waitForCommandDisposition(&runtime, rejected_visibility),
        maplibre.CommandDisposition.failed,
    ));
    // A missing layer reports not-found through the info getter's found flag.
    try testing.expect((try support.styleLayerInfo(&map, "missing")) == null);
}

test "layer properties accept semantic JSON values and return owned snapshots" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    _ = try map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", "18");

    var snapshot = (try support.layerProperty(&map, "point-circle", "circle-radius")).?;
    defer snapshot.deinit();
    try testing.expectEqualStrings("18.0", snapshot.value);
}

test "layer filters accept nested semantic JSON arrays and return owned snapshots" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    _ = try map.setLayerFilter(testing.allocator, "point-circle", "[\"==\",[\"get\",\"visible\"],true]");

    var snapshot = (try support.layerFilter(&map, "point-circle")).?;
    defer snapshot.deinit();
    try testing.expectEqualStrings("[\"==\",[\"get\",\"visible\"],true]", snapshot.value);

    _ = try map.setLayerFilter(testing.allocator, "point-circle", null);
    try testing.expect((try support.layerFilter(&map, "point-circle")) == null);
}

test "style light accepts full JSON and property updates" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    _ = try map.setStyleLightJson(testing.allocator, "{\"color\":\"blue\",\"intensity\":0.3,\"position\":[1,2,3]}");

    var snapshot = (try support.styleLightProperty(&map, "intensity")).?;
    defer snapshot.deinit();
    try testing.expectEqualStrings("0.30000001192092896", snapshot.value);

    _ = try map.setStyleLightProperty(testing.allocator, "intensity", "0.75");
    var updated = (try support.styleLightProperty(&map, "intensity")).?;
    defer updated.deinit();
    try testing.expectEqualStrings("0.75", updated.value);

    try testing.expect((try support.styleLightProperty(&map, "unknown-light-property")) == null);
    try testing.expect((try map.setStyleLightProperty(testing.allocator, "intensity", "false")) != 0);
}

test "runtime style images copy premultiplied RGBA8 pixels" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    var pixels = [_]u8{
        10,  20,  30,  255, 40, 50, 60, 255,
        0,   0,   0,   0,   70, 80, 90, 255,
        100, 110, 120, 255, 0,  0,  0,  0,
    };
    _ = try map.setStyleImage(testing.allocator, "runtime-icon", .{
        .width = 2,
        .height = 2,
        .stride = 12,
        .pixels = pixels[0..],
    }, .{ .pixel_ratio = 2.0, .sdf = true });
    pixels[0] = 200;

    try testing.expect(try support.styleImageExists(&map, "runtime-icon"));
    const info = (try support.styleImageInfo(&map, "runtime-icon")).?;
    try testing.expectEqual(@as(u32, 2), info.width);
    try testing.expectEqual(@as(u32, 2), info.height);
    try testing.expectEqual(@as(u32, 8), info.stride);
    try testing.expectEqual(@as(usize, 16), info.byte_length);
    try testing.expectApproxEqAbs(@as(f32, 2.0), info.pixel_ratio, 0.000001);
    try testing.expect(info.sdf);

    var copied = (try support.styleImagePixels(&map, "runtime-icon")).?;
    defer copied.deinit();
    try testing.expectEqualSlices(u8, &[_]u8{
        10, 20, 30, 255, 40,  50,  60,  255,
        70, 80, 90, 255, 100, 110, 120, 255,
    }, copied.value);

    var replacement_pixels = [_]u8{ 1, 2, 3, 4 };
    _ = try map.setStyleImage(testing.allocator, "runtime-icon", .{ .width = 1, .height = 1, .stride = 4, .pixels = replacement_pixels[0..] }, null);
    const replacement_info = (try support.styleImageInfo(&map, "runtime-icon")).?;
    try testing.expectEqual(@as(u32, 1), replacement_info.width);
    try testing.expectEqual(@as(u32, 1), replacement_info.height);
    try testing.expectApproxEqAbs(@as(f32, 1.0), replacement_info.pixel_ratio, 0.000001);
    try testing.expect(!replacement_info.sdf);

    try testing.expect(try support.removeStyleImage(&runtime, &map, "runtime-icon"));
    try testing.expect(!try support.styleImageExists(&map, "runtime-icon"));
    // A removal of a missing image is accepted, then fails with NOT_FOUND.
    try testing.expect(!try support.removeStyleImage(&runtime, &map, "runtime-icon"));
}

test "location indicator helpers set focused properties" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    _ = try map.addLocationIndicatorLayer(testing.allocator, "location", "point-circle");
    try testing.expectEqualStrings("location-indicator", (try support.styleLayerType(&map, "location")).?);

    _ = try map.setLocationIndicatorLocation(testing.allocator, "location", .{ .latitude = 37.7749, .longitude = -122.4194 }, 12.0);
    var location = (try support.layerProperty(&map, "location", "location")).?;
    defer location.deinit();
    try testing.expectEqualStrings("[37.7749,-122.4194,12.0]", location.value);

    _ = try map.setLocationIndicatorBearing(testing.allocator, "location", 45.0);
    var bearing = (try support.layerProperty(&map, "location", "bearing")).?;
    defer bearing.deinit();
    try testing.expectEqualStrings("45.0", bearing.value);

    _ = try map.setLocationIndicatorAccuracyRadius(testing.allocator, "location", 33.0);
    var radius = (try support.layerProperty(&map, "location", "accuracy-radius")).?;
    defer radius.deinit();
    try testing.expectEqualStrings("33.0", radius.value);

    _ = try map.setLocationIndicatorImageName(testing.allocator, "location", .top, "top-icon");
    var top_image = (try support.layerProperty(&map, "location", "top-image")).?;
    defer top_image.deinit();
    try testing.expect(!std.mem.eql(u8, top_image.value, "null"));
    _ = try map.setLocationIndicatorImageName(testing.allocator, "location", .bearing, "bearing-icon");
    _ = try map.setLocationIndicatorImageName(testing.allocator, "location", .shadow, "shadow-icon");

    try testing.expect((try map.setLocationIndicatorAccuracyRadius(testing.allocator, "location", -1.0)) != 0);
    try testing.expect((try map.setLocationIndicatorBearing(testing.allocator, "point-circle", 1.0)) != 0);
}

test "style JSON buffers reject invalid values" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    try testing.expect((try map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", "1e999")) != 0);
    try testing.expect((try map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", "\"not a radius\"")) != 0);
}

const transition_style_json =
    \\{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}
;

test "style transition options round trip through the C API" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer map.close() catch @panic("map close failed");

    // The style parser fills in a 300ms duration when a style declares none.
    const parsed = try support.styleTransitionOptions(&map);
    try testing.expectEqual(@as(?f64, 300.0), parsed.duration_ms);
    try testing.expectEqual(@as(?f64, null), parsed.delay_ms);

    _ = try map.setStyleJson(testing.allocator, transition_style_json);
    const declared = try support.styleTransitionOptions(&map);
    try testing.expectEqual(@as(?f64, 750.0), declared.duration_ms);
    try testing.expectEqual(@as(?f64, 100.0), declared.delay_ms);
    try testing.expectEqual(@as(?bool, true), declared.enable_placement_transitions);

    // A present zero stays distinguishable from an absent field, and an absent
    // field clears what the style declared.
    const options = maplibre.StyleTransitionOptions{
        .duration_ms = 0.0,
        .enable_placement_transitions = false,
    };
    _ = try map.setStyleTransitionOptions(options);
    try testing.expectEqual(options, try support.styleTransitionOptions(&map));

    // Omitting the flag leaves the cross-fade on rather than clearing it.
    _ = try map.setStyleTransitionOptions(.{ .duration_ms = 250.0 });
    try testing.expectEqual(
        @as(?bool, true),
        (try support.styleTransitionOptions(&map)).enable_placement_transitions,
    );

    // Loading a style replaces the override with what that style declares.
    _ = try map.setStyleJson(testing.allocator, transition_style_json);
    try testing.expectEqual(declared, try support.styleTransitionOptions(&map));

    try testing.expect((try map.setStyleTransitionOptions(.{ .delay_ms = -1.0 })) != 0);
}
