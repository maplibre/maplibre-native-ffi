const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

fn requestRepaintOnThread(map: *maplibre.Map, out_error: *?anyerror) void {
    support.expectCommitted(maplibre.mapRequestRepaint(support.handle(map)) catch |err| {
        out_error.* = err;
        return;
    }) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn createRuntimeAndMap() !struct { runtime: maplibre.Runtime, map: maplibre.Map } {
    var runtime = try support.createRuntime(.{});
    errdefer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    const map = try support.createMap(&runtime, .{});
    return .{ .runtime = runtime, .map = map };
}

test "discarded creation future releases its map" {
    var runtime = try support.createRuntime(.{});
    var creation = try maplibre.mapCreate(testing.allocator, runtime, try maplibre.mapOptionsDefault());
    try support.resolve(void, try maplibre.runtimeBarrier(support.handle(runtime)));
    creation.deinit();
    try support.closeRuntime(&runtime);
}

test "runtime and map vertical slice" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    try testing.expectEqual(@as(usize, 0), try support.drainEvents(&runtime));

    var map = try support.createMap(&runtime, .{});

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));

    try support.closeMap(&map);

    var map_after_close = try support.createMap(&runtime, .{});
    defer support.closeMap(&map_after_close) catch @panic("map close failed");
    var projection_future = try maplibre.mapProjectionCreate(map_after_close);
    defer projection_future.deinit();
    const projection = try projection_future.wait(null);
    try maplibre.mapProjectionClose(support.handle(projection));
}

test "loaded style document and URL read back what was loaded" {
    var handles = try createRuntimeAndMap();
    defer support.closeRuntime(&handles.runtime) catch @panic("runtime close failed");
    defer support.closeMap(&handles.map) catch @panic("map close failed");

    var empty_json = try support.loadedStyleJson(&handles.map);
    defer empty_json.deinit();
    try testing.expectEqualStrings("", empty_json.value);
    var empty_url = try support.styleUrl(&handles.map);
    defer empty_url.deinit();
    try testing.expectEqualStrings("", empty_url.value);

    // The document reads back byte-for-byte, so it can be reloaded unchanged.
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(handles.map), support.style_json));
    var loaded = try support.loadedStyleJson(&handles.map);
    defer loaded.deinit();
    try testing.expectEqualStrings(support.style_json, loaded.value);

    var cleared_url = try support.styleUrl(&handles.map);
    defer cleared_url.deinit();
    try testing.expectEqualStrings("", cleared_url.value);

    // The URL is request state, recorded before the load can succeed, while the
    // document still reports the style that last parsed.
    try support.expectCommitted(try maplibre.mapSetStyleUrl(testing.allocator, support.handle(handles.map), "https://example.com/style.json"));
    var requested_url = try support.styleUrl(&handles.map);
    defer requested_url.deinit();
    try testing.expectEqualStrings("https://example.com/style.json", requested_url.value);
    var still_loaded = try support.loadedStyleJson(&handles.map);
    defer still_loaded.deinit();
    try testing.expectEqualStrings(support.style_json, still_loaded.value);
}

test "map can close after moving with its runtime" {
    var handles = try createRuntimeAndMap();
    try support.closeMap(&handles.map);
    try support.closeRuntime(&handles.runtime);
}

test "copied runtime and map handles share closed state" {
    var runtime = try support.createRuntime(.{});
    var runtime_alias = runtime;
    var map = try support.createMap(&runtime, .{});
    var map_alias = map;
    var projection_future = try maplibre.mapProjectionCreate(map);
    defer projection_future.deinit();
    const projection = try projection_future.wait(null);
    const projection_alias = projection;

    try maplibre.mapProjectionClose(support.handle(projection));
    try maplibre.mapProjectionClose(support.handle(projection_alias));
    try testing.expectError(error.InvalidState, maplibre.mapProjectionGetCamera(support.handle(projection_alias)));

    try support.closeMap(&map);
    try support.closeMap(&map_alias);
    try testing.expectError(error.InvalidState, maplibre.mapSnapshotGet(map_alias));

    try support.closeRuntime(&runtime);
    try support.closeRuntime(&runtime_alias);
    try testing.expectError(error.InvalidState, maplibre.runtimeDrainEvents(support.handle(runtime_alias)));
}

test "successful close releases lifecycle handles" {
    var runtime = try support.createRuntime(.{});
    var map = try support.createMap(&runtime, .{});
    var projection_future = try maplibre.mapProjectionCreate(map);
    defer projection_future.deinit();
    const projection = try projection_future.wait(null);

    try support.closeMap(&map);
    try support.closeMap(&map);
    try support.closeRuntime(&runtime);
    try support.closeRuntime(&runtime);
    _ = try maplibre.mapProjectionGetCamera(support.handle(projection));
    try maplibre.mapProjectionClose(support.handle(projection));
    try maplibre.mapProjectionClose(support.handle(projection));
}

test "failed close remains retryable" {
    var runtime = try support.createRuntime(.{});
    var map = try support.createMap(&runtime, .{});

    try testing.expectError(error.InvalidState, support.closeRuntime(&runtime));
    try support.closeMap(&map);
    try support.closeRuntime(&runtime);
}

test "map options validate through public binding" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    try testing.expectError(error.InvalidArgument, support.createMap(&runtime, .{ .width = 0 }));
    try testing.expectError(error.InvalidArgument, support.createMap(&runtime, .{ .height = 0 }));
    try testing.expectError(error.InvalidArgument, support.createMap(&runtime, .{ .scale_factor = 0 }));
}

test "map creation accepts FastPFOR decoding" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{ .fast_pfor_enabled = true });
    defer support.closeMap(&map) catch @panic("map close failed");
}

test "unset map options take the C creation defaults" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    const snapshot = try maplibre.mapSnapshotGet(support.handle(map));
    try testing.expectEqual(@as(u32, 256), snapshot.logical_extent.width);
    try testing.expectEqual(@as(u32, 256), snapshot.logical_extent.height);
    try testing.expectEqual(@as(f64, 1.0), snapshot.logical_extent.scale_factor);
}

test "continuous repaint request makes render update available" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
}

test "map commands are accepted from another thread" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, requestRepaintOnThread, .{ &map, &thread_error });
    thread.join();
    try testing.expect(thread_error == null);

    // The repaint another thread committed is observable from this one.
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
}

test "runtime supports multiple maps" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var first = try support.createMap(&runtime, .{});
    defer support.closeMap(&first) catch @panic("map close failed");
    var second = try support.createMap(&runtime, .{});
    defer support.closeMap(&second) catch @panic("map close failed");
}

test "style JSON buffers preserve embedded NUL for native validation" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();
    try diagnostics.set(-5, "stale native diagnostic");

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    var future = try maplibre.mapSetStyleJson(support.handle(map), "{\x00}");
    defer future.deinit();
    const finished = try future.wait(&diagnostics);
    try testing.expectEqual(maplibre.CommandDisposition.failed, finished.disposition);
    try testing.expectError(error.InvalidArgument, finished.statusError());

    // The command's own diagnostic replaces the stale one the store held.
    const diagnostic = diagnostics.get().?;
    try testing.expectEqual(@as(?i32, finished.raw_status), diagnostic.raw_status);
    try testing.expect(!std.mem.eql(u8, "stale native diagnostic", diagnostic.message));
}

// A still image needs a render target, so a static map with none leaves the
// request pending until the map closes and reports it cancelled.
test "closing a map cancels its pending work" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .mode = .static });
    var map_open = true;
    defer if (map_open) support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    var still_image = try maplibre.mapRequestStillImage(support.handle(map));
    defer still_image.deinit();
    try testing.expect(!try still_image.poll());

    try support.closeMap(&map);
    map_open = false;
    try testing.expectError(error.Cancelled, still_image.wait(null));
}

// The scale factor is fixed at map creation, so a resize carries a new width
// and height only.
test "map resize keeps the scale factor the map was created with" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 256, .height = 256, .scale_factor = 2.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    const resized = try support.snapshotAfterCommand(&map, try maplibre.mapResize(support.handle(map), .{ .width = 320, .height = 200, .scale_factor = 2.0 }));
    try testing.expectEqual(@as(u32, 320), resized.logical_extent.width);
    try testing.expectEqual(@as(u32, 200), resized.logical_extent.height);
    try testing.expectEqual(@as(f64, 2.0), resized.logical_extent.scale_factor);

    try testing.expectError(error.InvalidArgument, maplibre.mapResize(support.handle(map), .{ .width = 320, .height = 200, .scale_factor = 1.0 }));
    try testing.expectEqual(@as(f64, 2.0), (try maplibre.mapSnapshotGet(support.handle(map))).logical_extent.scale_factor);
}
