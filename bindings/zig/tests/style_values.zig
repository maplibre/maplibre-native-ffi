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

test "style JSON descriptors reject invalid values and embedded NUL strings" {
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
    try testing.expectError(
        error.InvalidString,
        handles.map.setLayerProperty(testing.allocator, "point-circle", "circle-radius", .{ .string = "bad\x00value" }),
    );
}
