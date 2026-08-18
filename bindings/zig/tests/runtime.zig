const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

fn drainRuntimeOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    var batch = runtime.drainEvents(testing.allocator) catch |err| {
        out_error.* = err;
        return;
    };
    batch.deinit();
    out_error.* = null;
}

fn setRuntimeEventMaskOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.setEventMask(maplibre.RuntimeEventMask.all) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn setMapEventMaskOnThread(map: *maplibre.MapHandle, out_error: *?anyerror) void {
    _ = map.setEventMask(maplibre.RuntimeEventMask.all) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn closeRuntimeOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.close() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn createRuntimeOnThread(out_error: *?anyerror) void {
    var runtime = maplibre.RuntimeHandle.create(testing.allocator, .{}, null) catch |err| {
        out_error.* = err;
        return;
    };
    runtime.close() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

fn maskWithout(comptime field_name: []const u8) maplibre.RuntimeEventMask {
    var mask = maplibre.RuntimeEventMask.all;
    @field(mask, field_name) = false;
    return mask;
}

/// Drives a style load and a repaint, then reports whether the map's
/// style-loaded event arrived while `rejected` never did.
fn expectOnlySelectedTypes(
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
    rejected: maplibre.RuntimeEventType,
) !void {
    // Narrowing gates later events and keeps queued ones, so start empty.
    _ = try support.drainEvents(runtime);

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    _ = try map.requestRepaint();

    var saw_style_loaded = false;
    for (0..1000) |_| {
        var batch = try runtime.drainEvents(testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            try testing.expect(!std.meta.eql(event.event_type, rejected));
            if (std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_style_loaded)) {
                saw_style_loaded = true;
            }
        }
        if (saw_style_loaded) break;
        try sleepOneMillisecond();
    }
    try testing.expect(saw_style_loaded);
}

test "runtimes can be created on the current thread or another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var second = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer second.close() catch @panic("runtime close failed");

    var thread_error: ?anyerror = error.InvalidState;
    const thread = try std.Thread.spawn(.{}, createRuntimeOnThread, .{&thread_error});
    thread.join();
    try testing.expect(thread_error == null);
}

test "runtime drain and close are callable from another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);

    var drain_error: ?anyerror = null;
    const drain_thread = try std.Thread.spawn(.{}, drainRuntimeOnThread, .{ &runtime, &drain_error });
    drain_thread.join();
    try testing.expect(drain_error == null);

    var close_error: ?anyerror = null;
    const close_thread = try std.Thread.spawn(.{}, closeRuntimeOnThread, .{ &runtime, &close_error });
    close_thread.join();
    try testing.expect(close_error == null);
}

test "event mask setters accept calls from another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    var runtime_mask_error: ?anyerror = null;
    const runtime_mask_thread = try std.Thread.spawn(.{}, setRuntimeEventMaskOnThread, .{ &runtime, &runtime_mask_error });
    runtime_mask_thread.join();
    try testing.expect(runtime_mask_error == null);

    var map_mask_error: ?anyerror = null;
    const map_mask_thread = try std.Thread.spawn(.{}, setMapEventMaskOnThread, .{ &map, &map_mask_error });
    map_mask_thread.join();
    try testing.expect(map_mask_error == null);
}

test "runtime option strings reject embedded NUL before C calls" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();
    try diagnostics.set(-5, "stale native diagnostic");

    try testing.expectError(
        error.InvalidString,
        maplibre.RuntimeHandle.create(testing.allocator, .{ .asset_path = "asset\x00path" }, &diagnostics),
    );

    const diagnostic = diagnostics.get().?;
    try testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    try testing.expectEqualStrings("runtime asset_path contains embedded NUL", diagnostic.message);
}

test "one drain reports the events a style load queued together" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    _ = try map.setStyleJson(testing.allocator, support.style_json);

    var largest_batch: usize = 0;
    var saw_style_loaded = false;
    for (0..1000) |_| {
        var batch = try runtime.drainEvents(testing.allocator);
        defer batch.deinit();
        largest_batch = @max(largest_batch, batch.len());
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_style_loaded)) {
                saw_style_loaded = true;
            }
        }
        if (saw_style_loaded and largest_batch > 1) break;
        try sleepOneMillisecond();
    }
    try testing.expect(saw_style_loaded);
    try testing.expect(largest_batch > 1);
}
test "a drained batch outlives its runtime" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);
    var map_open = true;
    defer if (map_open) map.close() catch @panic("map close failed");
    const map_id = try map.id();

    _ = try map.setStyleUrl(testing.allocator, "unsupported://style.json");

    var kept: maplibre.EventBatch = undefined;
    var kept_index: usize = 0;
    var found = false;
    for (0..1000) |_| {
        var batch = try runtime.drainEvents(testing.allocator);
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (!std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_loading_failed)) continue;
            kept = batch;
            kept_index = index;
            found = true;
            break;
        }
        if (found) break;
        batch.deinit();
        try sleepOneMillisecond();
    }
    try testing.expect(found);
    defer kept.deinit();

    const before_close = try kept.at(kept_index);
    try testing.expectEqual(map_id, before_close.source_id.?);
    try testing.expect(before_close.source != .none);
    try testing.expect(before_close.message.len > 0);

    try map.close();
    map_open = false;
    try runtime.close();
    runtime_open = false;

    const after_close = try kept.at(kept_index);
    try testing.expectEqual(map_id, after_close.source_id.?);
    try testing.expect(after_close.message.len > 0);
}

test "closing a map leaves its queued runtime events unchanged" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);
    _ = try map.setStyleJson(testing.allocator, "{");
    try support.waitForBarrier(&runtime);
    try map.close();

    var batch = try runtime.drainEvents(testing.allocator);
    defer batch.deinit();
    try testing.expect(batch.len() > 0);
    var found_source = false;
    for (0..batch.len()) |index| {
        const event = try batch.at(index);
        if (event.source_type == .map and event.source != .none) found_source = true;
    }
    try testing.expect(found_source);
}

test "event masks round-trip through both handles" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    var runtime_mask = try runtime.eventMask();
    runtime_mask.offline_region_status_changed = false;
    try runtime.setEventMask(runtime_mask);
    const read_runtime_mask = try runtime.eventMask();
    try testing.expectEqual(runtime_mask, read_runtime_mask);
    // A runtime ignores the map bits and still reports them back.
    try testing.expect(read_runtime_mask.map_style_loaded);

    var map_mask = try map.eventMask();
    map_mask.map_tile_action = false;
    _ = try map.setEventMask(map_mask);
    try support.waitForBarrier(&runtime);
    const read_map_mask = try map.eventMask();
    try testing.expectEqual(map_mask, read_map_mask);
    try testing.expect(read_map_mask.offline_region_status_changed);
}

// A newer native library can report an event type this binding does not name,
// and a mask holds only 64 bits, so the membership test must not shift by the
// raw value it was handed.
test "mask membership rejects an unknown type no mask bit can hold" {
    try testing.expect(!maplibre.RuntimeEventMask.all.contains(.{ .unknown = 0xfeed }));
}

test "a narrowed map mask drops the type it clears and keeps the rest" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map_future = try maplibre.MapHandle.create(&runtime, .{});

    defer map_future.deinit();

    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const narrowed = maskWithout("map_render_update_available");
    _ = try map.setEventMask(narrowed);
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(narrowed, try map.eventMask());

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);

    // Restoring the bit lets the map's only invalidation report arrive again.
    _ = try map.setEventMask(maplibre.RuntimeEventMask.all);
    try support.waitForBarrier(&runtime);
    _ = try map.requestRepaint();
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
}

test "masks passed as create options narrow both handles" {
    const narrowed_runtime_mask = maskWithout("offline_region_status_changed");
    var runtime = try maplibre.RuntimeHandle.create(
        testing.allocator,
        .{ .event_mask = narrowed_runtime_mask },
        null,
    );
    defer runtime.close() catch @panic("runtime close failed");
    try testing.expectEqual(narrowed_runtime_mask, try runtime.eventMask());

    const narrowed_map_mask = maskWithout("map_render_update_available");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .event_mask = narrowed_map_mask });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    try testing.expectEqual(narrowed_map_mask, try map.eventMask());

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);
}
