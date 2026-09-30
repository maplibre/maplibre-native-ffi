const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

test "style ID lists are copied into owned Zig output" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    var source_ids = try support.listStyleSourceIds(&map);
    defer source_ids.deinit();
    try support.expectListContains(source_ids, "point");

    var layer_ids = try support.listStyleLayerIds(&map);
    defer layer_ids.deinit();
    try support.expectListContains(layer_ids, "background");
    try support.expectListContains(layer_ids, "point-circle");
}

// style layer listing copies the layer stack in style order.
test "style layer lists are copied into owned Zig output in style order" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.resolve(maplibre.Map, try maplibre.mapCreate(testing.allocator, runtime, try maplibre.mapOptionsDefault(), null));
    defer support.closeMap(&map) catch @panic("map close failed");
    try support.expectCommitted(try maplibre.mapSetStyleJson(
        support.handle(map),
        \\{"version":8,"sources":{"tiles":{"type":"vector","tiles":["http://example.invalid/{z}/{x}/{y}.pbf"]}},
        \\"layers":[{"id":"roads","type":"line","source":"tiles","source-layer":"transportation"},
        \\{"id":"sky","type":"background"}]}
    ,
        null,
    ));
    try testing.expect(try support.waitForEvent(&runtime, .map_style_loaded));

    var layers = try support.resolve(maplibre.generated.OwnedValue([]const maplibre.StyleLayerEntry), try maplibre.mapListStyleLayers(testing.allocator, support.handle(map), null));
    defer layers.deinit();
    try testing.expectEqual(@as(usize, 2), layers.value.len);

    const roads = layers.value[0];
    try testing.expectEqualStrings("roads", roads.id);
    try testing.expectEqualStrings("line", roads.type);
    try testing.expectEqualStrings("tiles", roads.source_id.?);
    try testing.expectEqualStrings("transportation", roads.source_layer.?);

    const sky = layers.value[1];
    try testing.expectEqualStrings("sky", sky.id);
    try testing.expectEqualStrings("background", sky.type);
    try testing.expect(sky.source_id == null);
    try testing.expect(sky.source_layer == null);
}

test "style layer JSON helpers manage lifecycle and order" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    const empty_data = try maplibre.geojsonSourceDataCreate(testing.allocator, "{\"type\":\"FeatureCollection\",\"features\":[]}", null, null);
    defer maplibre.geojsonSourceDataDestroy(support.handle(empty_data)) catch @panic("prepared data destroy failed");
    try support.expectCommitted(try maplibre.mapAddGeojsonSourceData(support.handle(map), "empty-layer-source", empty_data, null));
    try support.expectCommitted(try maplibre.mapAddStyleLayerJson(support.handle(map), "{\"id\":\"empty-circle\",\"type\":\"circle\",\"source\":\"empty-layer-source\"}", "point-circle", null));
    try testing.expect(try support.styleLayerExists(&map, "empty-circle"));

    var before_move = try support.listStyleLayerIds(&map);
    defer before_move.deinit();
    try testing.expect(support.listIndexOf(before_move, "empty-circle").? < support.listIndexOf(before_move, "point-circle").?);

    try support.expectStyleLayerType(&map, "empty-circle", "circle");

    var layer_json = (try support.styleLayerJson(&map, "empty-circle")).?;
    defer layer_json.deinit();
    try testing.expect(std.mem.indexOf(u8, layer_json.value, "\"id\":\"empty-circle\"") != null);

    try support.expectCommitted(try maplibre.mapMoveStyleLayer(support.handle(map), "empty-circle", "", null));
    // A source a layer still uses fails its removal with INVALID_STATE.
    try testing.expectError(error.InvalidState, support.removeStyleSource(&map, "empty-layer-source"));
    try testing.expect(try support.removeStyleLayer(&map, "empty-circle"));
    try testing.expect(!try support.styleLayerExists(&map, "empty-circle"));
    try testing.expect(try support.removeStyleSource(&map, "empty-layer-source"));
    // A removal of a missing layer is accepted, then fails with NOT_FOUND.
    try testing.expect(!try support.removeStyleLayer(&map, "empty-circle"));
    try testing.expect((try support.styleLayerJson(&map, "empty-circle")) == null);
    try testing.expect((try support.styleLayerInfo(&map, "empty-circle")) == null);
}

test "nine-patch style images round-trip stretch, content, and text fit" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

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
    try support.expectCommitted(try maplibre.mapSetStyleImage(testing.allocator, support.handle(map), "patch", image, .{
        .stretch_x = stretch_x[0..],
        .stretch_y = stretch_y[0..],
        .content = .{ .left = 0.5, .top = 0.5, .right = 1.5, .bottom = 1.5 },
        .text_fit_height = .proportional,
    }, null));

    var info = (try support.styleImageInfo(&map, "patch")).?;
    defer info.deinit();
    try testing.expectEqual(@as(usize, 1), info.value.info.stretch_x_count);
    try testing.expectEqual(@as(usize, 2), info.value.info.stretch_y_count);
    try testing.expectEqual(@as(f32, 1.5), info.value.info.content.?.right);
    // An absent text fit stays distinguishable from a present default.
    try testing.expect(info.value.info.text_fit_width == null);
    try testing.expectEqual(maplibre.StyleImageTextFit.proportional, info.value.info.text_fit_height.?);

    var stretches = (try support.styleImageStretches(&map, "patch")).?;
    defer stretches.deinit();
    try testing.expectEqual(@as(usize, 1), stretches.value.stretch_x.len);
    try testing.expectEqual(@as(f32, 1.0), stretches.value.stretch_x[0].to);
    try testing.expectEqual(@as(usize, 2), stretches.value.stretch_y.len);
    try testing.expectEqual(@as(f32, 2.0), stretches.value.stretch_y[1].to);

    try testing.expect((try support.styleImageStretches(&map, "missing")) == null);

    // A backwards interval is rejected by C.
    const backwards = [_]maplibre.ImageStretch{.{ .from = 2.0, .to = 1.0 }};
    try testing.expectError(
        error.InvalidArgument,
        maplibre.mapSetStyleImage(testing.allocator, support.handle(map), "bad", image, .{ .stretch_x = backwards[0..] }, null),
    );
}

test "layer base accessors round-trip source, zoom range, and visibility" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    {
        try testing.expectEqual(null, try support.layerSourceLayer(&map, "point-circle"));
    }
    try support.expectCommitted(try maplibre.mapSetLayerSourceLayer(support.handle(map), "point-circle", "roads", null));
    {
        var source_layer = (try support.layerSourceLayer(&map, "point-circle")).?;
        defer source_layer.deinit();
        try testing.expectEqualStrings("roads", source_layer.value);
    }
    {
        var source_id = (try support.layerSourceId(&map, "point-circle")).?;
        defer source_id.deinit();
        try testing.expectEqualStrings("point", source_id.value);
    }

    // Semantic rejection is reported asynchronously after command acceptance.
    try support.expectCommandError(try maplibre.mapSetLayerSourceLayer(support.handle(map), "background", "roads", null), error.InvalidArgument);
    {
        try testing.expectEqual(null, try support.layerSourceId(&map, "background"));
    }

    // The layer-info aggregate carries the type, zoom range, visibility, and
    // the layer's source IDs. An unset zoom range crosses the boundary as
    // infinities.
    var unset = (try support.styleLayerInfo(&map, "point-circle")).?;
    defer unset.deinit();
    try testing.expectEqualStrings("circle", unset.value.info.type);
    try testing.expectEqual(-std.math.inf(f64), unset.value.info.min_zoom);
    try testing.expectEqual(std.math.inf(f64), unset.value.info.max_zoom);
    try testing.expectEqual(maplibre.StyleLayerVisibility.visible, unset.value.info.visibility);
    try testing.expectEqualStrings("point", unset.value.source_id.?);
    try testing.expectEqualStrings("roads", unset.value.source_layer.?);
    {
        // The aggregate and the dedicated copy report the same source ID.
        var copied_source_id = (try support.layerSourceId(&map, "point-circle")).?;
        defer copied_source_id.deinit();
        try testing.expectEqualStrings(unset.value.source_id.?, copied_source_id.value);
    }

    try support.expectCommitted(try maplibre.mapSetLayerMinZoom(support.handle(map), "point-circle", 4.0, null));
    try support.expectCommitted(try maplibre.mapSetLayerMaxZoom(support.handle(map), "point-circle", 12.5, null));
    try support.expectCommitted(try maplibre.mapSetLayerVisibility(support.handle(map), "point-circle", .none, null));
    var tuned = (try support.styleLayerInfo(&map, "point-circle")).?;
    defer tuned.deinit();
    try testing.expectEqual(@as(f64, 4.0), tuned.value.info.min_zoom);
    try testing.expectEqual(@as(f64, 12.5), tuned.value.info.max_zoom);
    try testing.expectEqual(maplibre.StyleLayerVisibility.none, tuned.value.info.visibility);

    // A layer that names no source reports both IDs absent.
    var background = (try support.styleLayerInfo(&map, "background")).?;
    defer background.deinit();
    try testing.expectEqualStrings("background", background.value.info.type);
    try testing.expectEqual(@as(?[]const u8, null), background.value.source_id);
    try testing.expectEqual(@as(?[]const u8, null), background.value.source_layer);

    // An unknown raw visibility is accepted into the ordered queue, then fails.
    const rejected_visibility =
        try maplibre.mapSetLayerVisibility(support.handle(map), "point-circle", @enumFromInt(900), null);
    try support.expectCommandError(rejected_visibility, error.InvalidArgument);
    // A missing layer reports not-found through the info getter's found flag.
    try testing.expect((try support.styleLayerInfo(&map, "missing")) == null);
}

test "layer properties accept semantic JSON values and return owned snapshots" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapSetLayerProperty(support.handle(map), "point-circle", "circle-radius", "18", null));

    var snapshot = (try support.layerProperty(&map, "point-circle", "circle-radius")).?;
    defer snapshot.deinit();
    try testing.expectEqualStrings("18.0", snapshot.value);
}

test "layer filters accept nested semantic JSON arrays and return owned snapshots" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapSetLayerFilter(testing.allocator, support.handle(map), "point-circle", "[\"==\",[\"get\",\"visible\"],true]", null));

    var snapshot = (try support.layerFilter(&map, "point-circle")).?;
    defer snapshot.deinit();
    try testing.expectEqualStrings("[\"==\",[\"get\",\"visible\"],true]", snapshot.value);

    try support.expectCommitted(try maplibre.mapSetLayerFilter(testing.allocator, support.handle(map), "point-circle", null, null));
    try testing.expect((try support.layerFilter(&map, "point-circle")) == null);
}

test "style light accepts full JSON and property updates" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapSetStyleLightJson(support.handle(map), "{\"color\":\"blue\",\"intensity\":0.3,\"position\":[1,2,3]}", null));

    var snapshot = (try support.styleLightProperty(&map, "intensity")).?;
    defer snapshot.deinit();
    try testing.expectEqualStrings("0.30000001192092896", snapshot.value);

    try support.expectCommitted(try maplibre.mapSetStyleLightProperty(support.handle(map), "intensity", "0.75", null));
    var updated = (try support.styleLightProperty(&map, "intensity")).?;
    defer updated.deinit();
    try testing.expectEqualStrings("0.75", updated.value);

    try testing.expect((try support.styleLightProperty(&map, "unknown-light-property")) == null);
    try support.expectCommandError(try maplibre.mapSetStyleLightProperty(support.handle(map), "intensity", "false", null), error.InvalidArgument);
}

test "runtime style images copy premultiplied RGBA8 pixels" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    var pixels = [_]u8{
        10,  20,  30,  255, 40, 50, 60, 255,
        0,   0,   0,   0,   70, 80, 90, 255,
        100, 110, 120, 255, 0,  0,  0,  0,
    };
    try support.expectCommitted(try maplibre.mapSetStyleImage(testing.allocator, support.handle(map), "runtime-icon", .{
        .width = 2,
        .height = 2,
        .stride = 12,
        .pixels = pixels[0..],
    }, .{ .pixel_ratio = 2.0, .sdf = true }, null));
    pixels[0] = 200;

    try testing.expect(try support.styleImageExists(&map, "runtime-icon"));
    var info = (try support.styleImageInfo(&map, "runtime-icon")).?;
    defer info.deinit();
    try testing.expectEqual(@as(u32, 2), info.value.info.width);
    try testing.expectEqual(@as(u32, 2), info.value.info.height);
    try testing.expectEqual(@as(u32, 8), info.value.info.stride);
    try testing.expectEqual(@as(usize, 16), info.value.info.byte_length);
    try testing.expectApproxEqAbs(@as(f32, 2.0), info.value.info.pixel_ratio, 0.000001);
    try testing.expect(info.value.info.sdf);

    var copied = (try support.styleImagePixels(&map, "runtime-icon")).?;
    defer copied.deinit();
    try testing.expectEqualSlices(u8, &[_]u8{
        10, 20, 30, 255, 40,  50,  60,  255,
        70, 80, 90, 255, 100, 110, 120, 255,
    }, copied.value);

    var replacement_pixels = [_]u8{ 1, 2, 3, 4 };
    try support.expectCommitted(try maplibre.mapSetStyleImage(testing.allocator, support.handle(map), "runtime-icon", .{ .width = 1, .height = 1, .stride = 4, .pixels = replacement_pixels[0..] }, null, null));
    var replacement_info = (try support.styleImageInfo(&map, "runtime-icon")).?;
    defer replacement_info.deinit();
    try testing.expectEqual(@as(u32, 1), replacement_info.value.info.width);
    try testing.expectEqual(@as(u32, 1), replacement_info.value.info.height);
    try testing.expectApproxEqAbs(@as(f32, 1.0), replacement_info.value.info.pixel_ratio, 0.000001);
    try testing.expect(!replacement_info.value.info.sdf);

    try testing.expect(try support.removeStyleImage(&map, "runtime-icon"));
    try testing.expect(!try support.styleImageExists(&map, "runtime-icon"));
    // A removal of a missing image is accepted, then fails with NOT_FOUND.
    try testing.expect(!try support.removeStyleImage(&map, "runtime-icon"));
}

test "location indicator helpers set focused properties" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapAddLocationIndicatorLayer(support.handle(map), "location", "point-circle", null));
    try support.expectStyleLayerType(&map, "location", "location-indicator");

    try support.expectCommitted(try maplibre.mapSetLocationIndicatorLocation(support.handle(map), "location", .{ .latitude = 37.7749, .longitude = -122.4194 }, 12.0, null));
    var location = (try support.layerProperty(&map, "location", "location")).?;
    defer location.deinit();
    try testing.expectEqualStrings("[37.7749,-122.4194,12.0]", location.value);

    try support.expectCommitted(try maplibre.mapSetLocationIndicatorBearing(support.handle(map), "location", 45.0, null));
    var bearing = (try support.layerProperty(&map, "location", "bearing")).?;
    defer bearing.deinit();
    try testing.expectEqualStrings("45.0", bearing.value);

    try support.expectCommitted(try maplibre.mapSetLocationIndicatorAccuracyRadius(support.handle(map), "location", 33.0, null));
    var radius = (try support.layerProperty(&map, "location", "accuracy-radius")).?;
    defer radius.deinit();
    try testing.expectEqualStrings("33.0", radius.value);

    try support.expectCommitted(try maplibre.mapSetLocationIndicatorImageName(support.handle(map), "location", .top, "top-icon", null));
    var top_image = (try support.layerProperty(&map, "location", "top-image")).?;
    defer top_image.deinit();
    try testing.expect(!std.mem.eql(u8, top_image.value, "null"));
    try support.expectCommitted(try maplibre.mapSetLocationIndicatorImageName(support.handle(map), "location", .bearing, "bearing-icon", null));
    try support.expectCommitted(try maplibre.mapSetLocationIndicatorImageName(support.handle(map), "location", .shadow, "shadow-icon", null));

    try testing.expectError(error.InvalidArgument, maplibre.mapSetLocationIndicatorAccuracyRadius(support.handle(map), "location", -1.0, null));
    try support.expectCommandError(try maplibre.mapSetLocationIndicatorBearing(support.handle(map), "point-circle", 1.0, null), error.InvalidArgument);
}

test "style JSON buffers reject invalid values" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommandError(try maplibre.mapSetLayerProperty(support.handle(map), "point-circle", "circle-radius", "1e999", null), error.InvalidArgument);
    try support.expectCommandError(try maplibre.mapSetLayerProperty(support.handle(map), "point-circle", "circle-radius", "\"not a radius\"", null), error.InvalidArgument);
}

const transition_style_json =
    \\{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}
;

test "style transition options round trip through the C API" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createLoadedMap(&runtime);
    defer support.closeMap(&map) catch @panic("map close failed");

    // The style parser fills in a 300ms duration when a style declares none.
    const parsed = try support.styleTransitionOptions(&map);
    try testing.expectEqual(@as(?f64, 300.0), parsed.duration_ms);
    try testing.expectEqual(@as(?f64, null), parsed.delay_ms);

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), transition_style_json, null));
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
    try support.expectCommitted(try maplibre.mapSetStyleTransitionOptions(testing.allocator, support.handle(map), options, null));
    try testing.expectEqual(options, try support.styleTransitionOptions(&map));

    // Omitting the flag leaves the cross-fade on rather than clearing it.
    try support.expectCommitted(try maplibre.mapSetStyleTransitionOptions(testing.allocator, support.handle(map), .{ .duration_ms = 250.0 }, null));
    try testing.expectEqual(
        @as(?bool, true),
        (try support.styleTransitionOptions(&map)).enable_placement_transitions,
    );

    // Loading a style replaces the override with what that style declares.
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), transition_style_json, null));
    try testing.expectEqual(declared, try support.styleTransitionOptions(&map));

    try support.expectCommandError(try maplibre.mapSetStyleTransitionOptions(testing.allocator, support.handle(map), .{ .delay_ms = -1.0 }, null), error.InvalidArgument);
}

// global-state lifetime and copied JSON values.
test "global state defaults updates and style replacement" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.resolve(maplibre.Map, try maplibre.mapCreate(testing.allocator, runtime, try maplibre.mapOptionsDefault(), null));
    defer support.closeMap(&map) catch @panic("map close failed");
    try support.expectCommandError(try maplibre.mapSetGlobalStateProperty(support.handle(map), "theme", "true", null), error.InvalidState);
    try support.expectCommitted(try maplibre.mapSetStyleJson(
        support.handle(map),
        \\{"version":8,"sources":{},"layers":[],"state":{"theme":{"default":"light"}}}
    ,
        null,
    ));
    var defaults = try support.resolve(maplibre.OwnedValue([]const u8), try maplibre.mapGetGlobalState(testing.allocator, support.handle(map), null));
    defer defaults.deinit();
    try testing.expectEqualStrings("{\"theme\":\"light\"}", defaults.value);
    try support.expectCommitted(try maplibre.mapSetGlobalStateProperty(support.handle(map), "theme", "[\"dark\",{\"enabled\":true}]", null));
    var snapshot = try support.resolve(maplibre.OwnedValue([]const u8), try maplibre.mapGetGlobalState(testing.allocator, support.handle(map), null));
    defer snapshot.deinit();
    try support.expectCommitted(try maplibre.mapSetGlobalStateProperty(support.handle(map), "theme", "null", null));
    var reset = try support.resolve(maplibre.OwnedValue([]const u8), try maplibre.mapGetGlobalState(testing.allocator, support.handle(map), null));
    defer reset.deinit();
    try testing.expectEqualStrings(defaults.value, reset.value);
    try testing.expectEqualStrings("{\"theme\":[\"dark\",{\"enabled\":true}]}", snapshot.value);
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), "{\"version\":8,\"sources\":{},\"layers\":[]}", null));
    var replaced = try support.resolve(maplibre.OwnedValue([]const u8), try maplibre.mapGetGlobalState(testing.allocator, support.handle(map), null));
    defer replaced.deinit();
    try testing.expectEqualStrings("{}", replaced.value);
    try support.expectCommitted(try maplibre.mapSetGlobalStateProperty(support.handle(map), "theme", "true", null));
    try support.expectCommitted(try maplibre.mapSetGlobalStateProperty(support.handle(map), "theme", "null", null));
    var cleared = try support.resolve(maplibre.OwnedValue([]const u8), try maplibre.mapGetGlobalState(testing.allocator, support.handle(map), null));
    defer cleared.deinit();
    try testing.expectEqualStrings("{\"theme\":null}", cleared.value);
}
