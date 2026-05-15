const std = @import("std");
const build_options = @import("build_options");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const support = @import("support.zig");

extern "c" fn MTLCreateSystemDefaultDevice() ?*anyopaque;

const cluster_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-cluster-query-test",
    \\  "sources": {
    \\    "cluster-source": {
    \\      "type": "geojson",
    \\      "cluster": true,
    \\      "data": {
    \\        "type": "FeatureCollection",
    \\        "features": [
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one"}},
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two"}},
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three"}}
    \\        ]
    \\      }
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#ffffff"}},
    \\    {"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-color":"#2563eb","circle-radius":20}}
    \\  ]
    \\}
;

test "supported render backend is exposed semantically" {
    const support_mask = maplibre.supportedRenderBackends();
    try testing.expect(support_mask.metal or support_mask.vulkan);
    if (build_options.supports_metal) try testing.expect(support_mask.metal);
    if (build_options.supports_vulkan) try testing.expect(support_mask.vulkan);
}

fn expectFeaturePropertyString(feature: *const maplibre.QueriedFeature, key: []const u8, expected: []const u8) !void {
    for (feature.feature.properties) |property| {
        if (std.mem.eql(u8, property.key, key)) {
            const actual = switch (property.value) {
                .string => |value| value,
                else => return error.ExpectedString,
            };
            try testing.expectEqualStrings(expected, actual);
            return;
        }
    }
    return error.MissingFeatureProperty;
}

fn waitForRenderedFeatureQuery(
    runtime: maplibre.RuntimeHandle,
    session: maplibre.RenderSessionHandle,
    geometry: maplibre.RenderedQueryGeometry,
    options: maplibre.RenderedFeatureQueryOptions,
) !maplibre.FeatureQueryResult {
    for (0..1000) |_| {
        var result = try session.queryRenderedFeatures(testing.allocator, geometry, options);
        if (result.features.len > 0) return result;
        result.deinit();
        try runtime.runOnce();
        try session.renderUpdate();
        try std.Thread.yield();
    }
    return error.RenderedFeatureNotQueryable;
}

fn waitForSourceFeatureQuery(
    runtime: maplibre.RuntimeHandle,
    session: maplibre.RenderSessionHandle,
) !maplibre.FeatureQueryResult {
    for (0..1000) |_| {
        var result = try session.querySourceFeatures(testing.allocator, "point", .{
            .filter = .{ .array = &.{
                .{ .string = "==" },
                .{ .array = &.{ .{ .string = "get" }, .{ .string = "kind" } } },
                .{ .string = "capital" },
            } },
        });
        if (result.features.len > 0) return result;
        result.deinit();
        try runtime.runOnce();
        try session.renderUpdate();
        try std.Thread.yield();
    }
    return error.SourceFeatureNotQueryable;
}

fn ownedGeometryAsBorrowed(geometry: maplibre.OwnedGeometry) maplibre.Geometry {
    return switch (geometry) {
        .empty => .empty,
        .point => |point| .{ .point = point },
        .line_string => |coordinates| .{ .line_string = coordinates },
        .polygon => |rings| .{ .polygon = rings },
        .multi_point => |coordinates| .{ .multi_point = coordinates },
        .multi_line_string => |lines| .{ .multi_line_string = lines },
        .multi_polygon => |polygons| .{ .multi_polygon = polygons },
        .collection => .empty,
    };
}

fn ownedJsonAsBorrowed(value: maplibre.OwnedJsonValue) maplibre.JsonValue {
    return switch (value) {
        .null => .null,
        .bool => |item| .{ .bool = item },
        .uint => |item| .{ .uint = item },
        .int => |item| .{ .int = item },
        .double => |item| .{ .double = item },
        .string => |item| .{ .string = item },
        .array, .object => .null,
    };
}

fn queriedFeatureAsBorrowed(allocator: std.mem.Allocator, queried: *const maplibre.QueriedFeature) !struct { feature: maplibre.Feature, properties: []maplibre.JsonMember } {
    const properties = try allocator.alloc(maplibre.JsonMember, queried.feature.properties.len);
    for (queried.feature.properties, properties) |property, *out| {
        out.* = .{ .key = property.key, .value = ownedJsonAsBorrowed(property.value) };
    }
    return .{
        .feature = .{
            .geometry = ownedGeometryAsBorrowed(queried.feature.geometry),
            .properties = properties,
            .identifier = queried.feature.identifier,
        },
        .properties = properties,
    };
}

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

test "owned texture render session lifecycle and readback" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOwnedTexture(map, .{
        .extent = .{ .width = 32, .height = 16, .scale_factor = 1.0 },
    });
    const session_copy = session;
    defer session.close() catch {};

    try testing.expectError(error.InvalidState, session.readPremultipliedRgba8(testing.allocator));

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(runtime, .map_render_update_available));
    try session.renderUpdate();
    try session.reduceMemoryUse();
    try session.dumpDebugLogs();
    try session.clearData();

    var small: [4]u8 = .{ 0, 0, 0, 0 };
    try testing.expectError(error.InvalidArgument, session.readPremultipliedRgba8Into(small[0..]));

    const probed_info = try session.textureImageInfo();
    try testing.expectEqual(@as(u32, 32), probed_info.width);
    try testing.expectEqual(@as(u32, 16), probed_info.height);
    try testing.expectEqual(@as(usize, 32 * 16 * 4), probed_info.byte_length);

    var image = try session.readPremultipliedRgba8(testing.allocator);
    defer image.deinit();
    try testing.expectEqual(@as(u32, 32), image.info.width);
    try testing.expectEqual(@as(u32, 16), image.info.height);
    try testing.expectEqual(@as(u32, 32 * 4), image.info.stride);
    try testing.expectEqual(@as(usize, 32 * 16 * 4), image.info.byte_length);
    try testing.expectEqual(image.info.byte_length, image.data.len);

    try session.resize(.{ .width = 64, .height = 64, .scale_factor = 1.0 });
    try session.detach();
    try testing.expectError(error.InvalidState, session.renderUpdate());
    try session.close();
    try session.close();
    try testing.expectError(error.ClosedHandle, session.renderUpdate());
    try testing.expectError(error.ClosedHandle, session_copy.renderUpdate());
}

test "static map still-image requests drive owned texture rendering" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const map = try maplibre.MapHandle.create(runtime, .{ .mode = .static });
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOwnedTexture(map, .{ .extent = .{ .width = 32, .height = 32 } });
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try map.requestStillImage();
    try testing.expectError(error.InvalidState, map.requestStillImage());
}

test "owned texture attachment validates public descriptors" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, maplibre.attachOwnedTexture(map, .{ .extent = .{ .width = 0 } }));
    try testing.expectError(error.InvalidArgument, maplibre.attachOwnedTexture(map, .{ .extent = .{ .height = 0 } }));
    try testing.expectError(error.InvalidArgument, maplibre.attachOwnedTexture(map, .{ .extent = .{ .scale_factor = 0 } }));

    var first = try maplibre.attachOwnedTexture(map, .{});
    defer first.close() catch {};
    try testing.expectError(error.InvalidState, maplibre.attachOwnedTexture(map, .{}));
}

test "render session feature state set get and remove" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOwnedTexture(map, .{ .extent = .{ .width = 64, .height = 64 } });
    defer session.close() catch {};

    const selector = maplibre.FeatureStateSelector{ .source_id = "point", .feature_id = "feature-1" };
    const state_members = [_]maplibre.JsonMember{
        .{ .key = "hover", .value = .{ .bool = true } },
        .{ .key = "radius", .value = .{ .uint = 20 } },
    };
    try testing.expectError(error.InvalidState, session.setFeatureState(testing.allocator, selector, .{ .object = state_members[0..] }));

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(runtime, .map_render_update_available));
    try session.renderUpdate();

    try session.setFeatureState(testing.allocator, selector, .{ .object = state_members[0..] });
    var snapshot = try session.getFeatureState(testing.allocator, selector);
    defer snapshot.deinit(testing.allocator);
    const members = switch (snapshot) {
        .object => |items| items,
        else => return error.ExpectedObject,
    };
    try testing.expectEqual(@as(usize, 2), members.len);
    var saw_hover = false;
    var saw_radius = false;
    for (members) |member| {
        if (std.mem.eql(u8, member.key, "hover")) {
            try testing.expectEqual(true, member.value.bool);
            saw_hover = true;
        } else if (std.mem.eql(u8, member.key, "radius")) {
            try testing.expectEqual(@as(u64, 20), member.value.uint);
            saw_radius = true;
        }
    }
    try testing.expect(saw_hover);
    try testing.expect(saw_radius);

    try session.removeFeatureState(testing.allocator, .{ .source_id = "point", .feature_id = "feature-1", .state_key = "hover" });
    try testing.expect(try waitForEvent(runtime, .map_render_update_available));
    try session.renderUpdate();

    var after_remove = try session.getFeatureState(testing.allocator, selector);
    defer after_remove.deinit(testing.allocator);
    const after_members = switch (after_remove) {
        .object => |items| items,
        else => return error.ExpectedObject,
    };
    try testing.expectEqual(@as(usize, 1), after_members.len);
    try testing.expectEqualStrings("radius", after_members[0].key);
    try testing.expectEqual(@as(u64, 20), after_members[0].value.uint);

    try testing.expectError(error.InvalidArgument, session.removeFeatureState(testing.allocator, .{ .source_id = "point", .state_key = "hover" }));
}

test "render session queries rendered and source features" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOwnedTexture(map, .{});
    defer session.close() catch {};

    try testing.expectError(error.InvalidState, session.queryRenderedFeatures(testing.allocator, .{ .point = .{ .x = 256, .y = 256 } }, null));

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(runtime, .map_render_update_available));
    try session.renderUpdate();

    const query_point = try map.pixelForLatLng(.{ .latitude = 37.7749, .longitude = -122.4194 });
    var rendered = try waitForRenderedFeatureQuery(runtime, session, .{ .box = .{
        .min = .{ .x = query_point.x - 20, .y = query_point.y - 20 },
        .max = .{ .x = query_point.x + 20, .y = query_point.y + 20 },
    } }, .{
        .layer_ids = &.{"point-circle"},
        .filter = .{ .array = &.{
            .{ .string = "==" },
            .{ .array = &.{ .{ .string = "get" }, .{ .string = "kind" } } },
            .{ .string = "capital" },
        } },
    });
    defer rendered.deinit();
    try testing.expect(rendered.features[0].source_id != null);
    try testing.expectEqualStrings("point", rendered.features[0].source_id.?);
    try expectFeaturePropertyString(&rendered.features[0], "kind", "capital");

    var source = try waitForSourceFeatureQuery(runtime, session);
    defer source.deinit();
    try testing.expect(source.features[0].source_id != null);
    try testing.expectEqualStrings("point", source.features[0].source_id.?);
    try expectFeaturePropertyString(&source.features[0], "kind", "capital");

    try testing.expectError(error.InvalidArgument, session.querySourceFeatures(testing.allocator, "", null));
}

test "render session queries cluster feature extensions" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOwnedTexture(map, .{});
    defer session.close() catch {};

    try map.jumpTo(.{ .center = .{ .latitude = 0, .longitude = 0 }, .zoom = 0 });
    try map.setStyleJson(testing.allocator, cluster_style_json);
    for (0..5) |_| {
        if (!try waitForEvent(runtime, .map_render_update_available)) break;
        try session.renderUpdate();
    }

    const query_point = try map.pixelForLatLng(.{ .latitude = 0, .longitude = 0 });
    var clusters = try waitForRenderedFeatureQuery(runtime, session, .{ .box = .{
        .min = .{ .x = query_point.x - 30, .y = query_point.y - 30 },
        .max = .{ .x = query_point.x + 30, .y = query_point.y + 30 },
    } }, .{ .layer_ids = &.{"cluster-circle"} });
    defer clusters.deinit();

    const borrowed = try queriedFeatureAsBorrowed(testing.allocator, &clusters.features[0]);
    defer testing.allocator.free(borrowed.properties);

    var children = try session.queryFeatureExtension(testing.allocator, "cluster-source", borrowed.feature, "supercluster", "children", null);
    defer children.deinit(testing.allocator);
    const child_collection = switch (children) {
        .feature_collection => |collection| collection,
        else => return error.ExpectedFeatureCollection,
    };
    try testing.expect(child_collection.features.len > 0);

    var expansion_zoom = try session.queryFeatureExtension(testing.allocator, "cluster-source", borrowed.feature, "supercluster", "expansion-zoom", null);
    defer expansion_zoom.deinit(testing.allocator);
    const zoom_value = switch (expansion_zoom) {
        .value => |value| value,
        else => return error.ExpectedValue,
    };
    try testing.expect(zoom_value == .uint);

    const args_members = [_]maplibre.JsonMember{
        .{ .key = "limit", .value = .{ .uint = 1 } },
        .{ .key = "offset", .value = .{ .uint = 0 } },
    };
    var leaves = try session.queryFeatureExtension(testing.allocator, "cluster-source", borrowed.feature, "supercluster", "leaves", .{ .object = args_members[0..] });
    defer leaves.deinit(testing.allocator);
    const leaf_collection = switch (leaves) {
        .feature_collection => |collection| collection,
        else => return error.ExpectedFeatureCollection,
    };
    try testing.expectEqual(@as(usize, 1), leaf_collection.features.len);
}

test "Metal owned texture frame handle scopes native pointers" {
    if (!build_options.supports_metal) return error.SkipZigTest;
    const device = MTLCreateSystemDefaultDevice() orelse return error.SkipZigTest;

    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachMetalOwnedTexture(map, .{
        .extent = .{ .width = 32, .height = 32, .scale_factor = 1.0 },
        .context = .{ .device = .{ .ptr = device } },
    });
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(runtime, .map_render_update_available));
    try session.renderUpdate();

    const frame = try session.acquireMetalOwnedTextureFrame();
    const info = try frame.info();
    try testing.expectEqual(@as(u32, 32), info.width);
    try testing.expectEqual(@as(u32, 32), info.height);
    try testing.expectEqual(@as(u64, 1), info.generation);
    try testing.expect(info.texture.ptr != info.device.ptr);

    try testing.expectError(error.ActiveBorrow, session.resize(.{ .width = 16, .height = 16, .scale_factor = 1.0 }));
    try testing.expectError(error.ActiveBorrow, session.renderUpdate());
    try testing.expectError(error.ActiveBorrow, session.detach());
    try testing.expectError(error.ActiveBorrow, session.acquireMetalOwnedTextureFrame());
    try testing.expectError(error.ActiveBorrow, session.close());

    try frame.release();
    try frame.release();
    try testing.expectError(error.ClosedHandle, frame.info());
    try session.close();
}
