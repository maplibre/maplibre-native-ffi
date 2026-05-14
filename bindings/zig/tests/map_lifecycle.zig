const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const support = @import("support.zig");

fn requestRepaintOnThread(map: maplibre.MapHandle, out_error: *?anyerror) void {
    map.requestRepaint() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
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

test "runtime and map vertical slice" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");

    try runtime.runOnce();
    try testing.expectEqual(@as(?maplibre.RuntimeEvent, null), try runtime.pollEvent());

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleJson(testing.allocator, support.style_json);
    try runtime.runOnce();

    try map.close();
    try runtime.runOnce();

    const map_after_close = try maplibre.MapHandle.create(runtime, .{});
    defer map_after_close.close() catch @panic("map close failed");
    const projection = try maplibre.MapProjectionHandle.create(map_after_close);
    try projection.close();
    try projection.close();
}

test "successful close is idempotent and closed handles fail before C calls" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    const runtime_copy = runtime;
    const map = try maplibre.MapHandle.create(runtime, .{});
    const map_copy = map;
    const projection = try maplibre.MapProjectionHandle.create(map);
    const projection_copy = projection;

    try projection.close();
    try projection.close();
    try testing.expectError(error.ClosedHandle, projection_copy.getCamera());

    try map.close();
    try map.close();
    try testing.expectError(error.ClosedHandle, map.requestRepaint());
    try testing.expectError(error.ClosedHandle, map_copy.requestRepaint());

    try runtime.close();
    try runtime.close();
    try testing.expectError(error.ClosedHandle, runtime.runOnce());
    try testing.expectError(error.ClosedHandle, runtime_copy.runOnce());
}

test "failed close remains retryable" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    const map = try maplibre.MapHandle.create(runtime, .{});

    try testing.expectError(error.InvalidState, runtime.close());
    try map.close();
    try runtime.close();
}

test "map options validate through public binding" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    try testing.expectError(error.InvalidArgument, maplibre.MapHandle.create(runtime, .{ .width = 0 }));
    try testing.expectError(error.InvalidArgument, maplibre.MapHandle.create(runtime, .{ .height = 0 }));
    try testing.expectError(error.InvalidArgument, maplibre.MapHandle.create(runtime, .{ .scale_factor = 0 }));
}

test "continuous repaint request makes render update available" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.requestRepaint();
    try testing.expect(try waitForEvent(runtime, .map_render_update_available));
}

test "wrong-thread map failures propagate diagnostics" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");
    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    var thread_error: ?anyerror = null;
    const thread = try std.Thread.spawn(.{}, requestRepaintOnThread, .{ map, &thread_error });
    thread.join();

    try testing.expectEqual(error.WrongThread, thread_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);
}

test "runtime supports multiple maps" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const first = try maplibre.MapHandle.create(runtime, .{});
    defer first.close() catch @panic("first map close failed");
    const second = try maplibre.MapHandle.create(runtime, .{});
    defer second.close() catch @panic("second map close failed");

    try runtime.runOnce();
}

test "map string methods check closed handles before string materialization" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");
    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.close();
    try testing.expectError(error.ClosedHandle, map.setStyleJson(testing.allocator, "{\x00}"));
}

test "live map string methods reject embedded NUL before C calls" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidString, map.setStyleJson(testing.allocator, "{\x00}"));
}
