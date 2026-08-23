const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

fn requestRepaintOnThread(map: *maplibre.MapHandle, out_error: *?anyerror) void {
    _ = map.requestRepaint() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn createRuntimeAndMap() !struct { runtime: maplibre.RuntimeHandle, map: maplibre.MapHandle } {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    errdefer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    const map = try map_future.wait(null);
    return .{ .runtime = runtime, .map = map };
}

test "runtime and map vertical slice" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, &diagnostics);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    try testing.expectEqual(@as(usize, 0), try support.drainEvents(&runtime));

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);

    _ = try map.setStyleJson(testing.allocator, support.style_json);

    try map.close();

    var map_after_close_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_after_close_future.deinit();

    var map_after_close = try map_after_close_future.wait(null);
    defer map_after_close.close() catch @panic("map close failed");
    var projection_future = try maplibre.MapProjectionHandle.create(&map_after_close);
    defer projection_future.deinit();
    var projection = try projection_future.wait(null);
    try projection.close();
}

test "loaded style document and URL read back what was loaded" {
    var handles = try createRuntimeAndMap();
    defer support.closeRuntime(&handles.runtime) catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    var empty_json = try support.loadedStyleJson(&handles.map);
    defer empty_json.deinit();
    try testing.expectEqualStrings("", empty_json.value);
    var empty_url = try support.styleUrl(&handles.map);
    defer empty_url.deinit();
    try testing.expectEqualStrings("", empty_url.value);

    // The document reads back byte-for-byte, so it can be reloaded unchanged.
    _ = try handles.map.setStyleJson(testing.allocator, support.style_json);
    var loaded = try support.loadedStyleJson(&handles.map);
    defer loaded.deinit();
    try testing.expectEqualStrings(support.style_json, loaded.value);

    var cleared_url = try support.styleUrl(&handles.map);
    defer cleared_url.deinit();
    try testing.expectEqualStrings("", cleared_url.value);

    // The URL is request state, recorded before the load can succeed, while the
    // document still reports the style that last parsed.
    _ = try handles.map.setStyleUrl(testing.allocator, "https://example.com/style.json");
    var requested_url = try support.styleUrl(&handles.map);
    defer requested_url.deinit();
    try testing.expectEqualStrings("https://example.com/style.json", requested_url.value);
    var still_loaded = try support.loadedStyleJson(&handles.map);
    defer still_loaded.deinit();
    try testing.expectEqualStrings(support.style_json, still_loaded.value);
}

test "map can close after moving with its runtime" {
    var handles = try createRuntimeAndMap();
    try handles.map.close();
    try support.closeRuntime(&handles.runtime);
}

test "copied runtime and map handles share closed state" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    var runtime_alias = runtime;
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    var map_alias = map;
    var projection_future = try maplibre.MapProjectionHandle.create(&map);
    defer projection_future.deinit();
    var projection = try projection_future.wait(null);
    var projection_alias = projection;

    try projection.close();
    try projection_alias.close();
    try testing.expectError(error.ClosedHandle, projection_alias.getCamera());

    try map.close();
    try map_alias.close();
    try testing.expectError(error.ClosedHandle, map_alias.id());

    try support.closeRuntime(&runtime);
    try support.closeRuntime(&runtime_alias);
    try testing.expectError(error.ClosedHandle, runtime_alias.drainEvents(testing.allocator));
}

test "successful close releases lifecycle handles" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, &diagnostics);
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    var projection_future = try maplibre.MapProjectionHandle.create(&map);
    defer projection_future.deinit();
    var projection = try projection_future.wait(null);

    try map.close();
    try map.close();
    try support.closeRuntime(&runtime);
    try support.closeRuntime(&runtime);
    _ = try projection.getCamera();
    try projection.close();
    try projection.close();
}

test "failed close remains retryable" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, &diagnostics);
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);

    try testing.expectError(error.InvalidState, support.closeRuntime(&runtime));
    try map.close();
    try support.closeRuntime(&runtime);
}

test "map options validate through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    try testing.expectError(error.InvalidArgument, maplibre.MapHandle.create(&runtime, .{ .width = 0 }));
    try testing.expectError(error.InvalidArgument, maplibre.MapHandle.create(&runtime, .{ .height = 0 }));
    try testing.expectError(error.InvalidArgument, maplibre.MapHandle.create(&runtime, .{ .scale_factor = 0 }));
}

test "map creation accepts FastPFOR decoding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{ .fast_pfor_enabled = true });

    defer map_future.deinit();

    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
}

test "unset map options take the C creation defaults" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const size = try map.getSize();
    try testing.expectEqual(@as(u32, 256), size.width);
    try testing.expectEqual(@as(u32, 256), size.height);
    try testing.expectEqual(@as(f64, 1.0), size.scale_factor);
}

test "continuous repaint request makes render update available" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    _ = try map.requestRepaint();
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
}

test "map commands are accepted from another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, requestRepaintOnThread, .{ &map, &thread_error });
    thread.join();
    try testing.expect(thread_error == null);
}

test "runtime supports multiple maps" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var first_future = try maplibre.MapHandle.create(&runtime, .{});

    defer first_future.deinit();

    var first = try first_future.wait(null);
    defer first.close() catch @panic("first map close failed");
    var second_future = try maplibre.MapHandle.create(&runtime, .{});
    defer second_future.deinit();
    var second = try second_future.wait(null);
    defer second.close() catch @panic("second map close failed");
}

test "style JSON buffers preserve embedded NUL for native validation" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();
    try diagnostics.set(-5, "stale native diagnostic");

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, &diagnostics);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    try support.expectCommandError(&runtime, try map.setStyleJson(testing.allocator, "{\x00}"), error.InvalidArgument);
}
