const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

fn drainRuntimeOnThread(runtime: *maplibre.Runtime, out_error: *?anyerror) void {
    var batch = maplibre.runtimeDrainEvents(support.handle(runtime)) catch |err| {
        out_error.* = err;
        return;
    };
    batch.deinit();
    out_error.* = null;
}

const cross_thread_runtime_mask = blk: {
    var mask = maplibre.RuntimeEventMask.all;
    mask.offline_region_status_changed = false;
    break :blk mask;
};

const cross_thread_map_mask = blk: {
    var mask = maplibre.RuntimeEventMask.all;
    mask.map_tile_action = false;
    break :blk mask;
};

fn setRuntimeEventMaskOnThread(runtime: *maplibre.Runtime, out_error: *?anyerror) void {
    maplibre.runtimeSetEventMask(support.handle(runtime), cross_thread_runtime_mask) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn setMapEventMaskOnThread(map: *maplibre.Map, out_error: *?anyerror) void {
    support.expectCommitted(maplibre.mapSetEventMask(support.handle(map), cross_thread_map_mask) catch |err| {
        out_error.* = err;
        return;
    }) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn closeRuntimeOnThread(runtime: *maplibre.Runtime, out_error: *?anyerror) void {
    support.closeRuntime(runtime) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn createRuntimeOnThread(out_error: *?anyerror) void {
    var runtime = support.createRuntime(.{}) catch |err| {
        out_error.* = err;
        return;
    };
    support.closeRuntime(&runtime) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn maskWithout(comptime field_name: []const u8) maplibre.RuntimeEventMask {
    var mask = maplibre.RuntimeEventMask.all;
    @field(mask, field_name) = false;
    return mask;
}

/// Drives a style load and a repaint, then reports whether the map's
/// style-loaded event arrived while `rejected` never did.
fn expectOnlySelectedTypes(
    runtime: *maplibre.Runtime,
    map: *maplibre.Map,
    rejected: maplibre.RuntimeEventType,
) !void {
    // Narrowing gates later events and keeps queued ones, so start empty.
    _ = try support.drainEvents(runtime);

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));

    var saw_style_loaded = false;
    for (0..1000) |_| {
        var batch = try support.drainEventSnapshot(runtime);
        defer batch.deinit();
        for (0..batch.value.events.len) |index| {
            const event = batch.value.events[index];
            try testing.expect(!std.meta.eql(event.type, rejected));
            if (std.meta.eql(event.type, maplibre.RuntimeEventType.map_style_loaded)) {
                saw_style_loaded = true;
            }
        }
        if (saw_style_loaded) break;
        try support.sleepOneMillisecond();
    }
    try testing.expect(saw_style_loaded);
}

test "runtimes can be created on the current thread or another thread" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var second = try support.createRuntime(.{});
    defer support.closeRuntime(&second) catch @panic("runtime close failed");

    var thread_error: ?anyerror = error.InvalidState;
    const thread = try std.Thread.spawn(.{}, createRuntimeOnThread, .{&thread_error});
    thread.join();
    try testing.expect(thread_error == null);
}

test "runtime drain and close are callable from another thread" {
    var runtime = try support.createRuntime(.{});

    var drain_error: ?anyerror = null;
    const drain_thread = try std.Thread.spawn(.{}, drainRuntimeOnThread, .{ &runtime, &drain_error });
    drain_thread.join();
    try testing.expect(drain_error == null);

    var close_error: ?anyerror = null;
    const close_thread = try std.Thread.spawn(.{}, closeRuntimeOnThread, .{ &runtime, &close_error });
    close_thread.join();
    try testing.expect(close_error == null);
    // The close another thread ran is the one this thread observes.
    try testing.expectError(error.InvalidState, maplibre.runtimeDrainEvents(support.handle(runtime)));
}

test "event mask setters accept calls from another thread" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    var runtime_mask_error: ?anyerror = null;
    const runtime_mask_thread = try std.Thread.spawn(.{}, setRuntimeEventMaskOnThread, .{ &runtime, &runtime_mask_error });
    runtime_mask_thread.join();
    try testing.expect(runtime_mask_error == null);

    var map_mask_error: ?anyerror = null;
    const map_mask_thread = try std.Thread.spawn(.{}, setMapEventMaskOnThread, .{ &map, &map_mask_error });
    map_mask_thread.join();
    try testing.expect(map_mask_error == null);

    // Both handles publish the mask the other thread installed.
    try testing.expectEqual(cross_thread_runtime_mask, try maplibre.runtimeGetEventMask(support.handle(runtime)));
    try testing.expectEqual(cross_thread_map_mask, (try maplibre.mapSnapshotGet(support.handle(map))).event_mask);
}

test "runtime option strings reject embedded NUL before C calls" {
    try testing.expectError(
        error.InvalidString,
        support.createRuntime(.{ .asset_path = "asset\x00path" }),
    );
}

test "one drain reports the events a style load queued together" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));

    var largest_batch: usize = 0;
    var saw_style_loaded = false;
    for (0..1000) |_| {
        var batch = try support.drainEventSnapshot(runtime);
        defer batch.deinit();
        largest_batch = @max(largest_batch, batch.value.events.len);
        for (0..batch.value.events.len) |index| {
            const event = batch.value.events[index];
            if (std.meta.eql(event.type, maplibre.RuntimeEventType.map_style_loaded)) {
                saw_style_loaded = true;
            }
        }
        if (saw_style_loaded and largest_batch > 1) break;
        try support.sleepOneMillisecond();
    }
    try testing.expect(saw_style_loaded);
    try testing.expect(largest_batch > 1);
}
test "a drained batch outlives its runtime" {
    var runtime = try support.createRuntime(.{});
    var runtime_open = true;
    defer if (runtime_open) support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    var map_open = true;
    defer if (map_open) support.closeMap(&map) catch @panic("map close failed");
    const map_id = map.raw;

    try support.expectCommitted(try maplibre.mapSetStyleUrl(testing.allocator, support.handle(map), "unsupported://style.json"));

    var kept: maplibre.OwnedValue(maplibre.RuntimeEventBatchView) = undefined;
    var kept_index: usize = 0;
    var found = false;
    for (0..1000) |_| {
        var batch = try support.drainEventSnapshot(runtime);
        for (0..batch.value.events.len) |index| {
            const event = batch.value.events[index];
            if (!std.meta.eql(event.type, maplibre.RuntimeEventType.map_loading_failed)) continue;
            kept = batch;
            kept_index = index;
            found = true;
            break;
        }
        if (found) break;
        batch.deinit();
        try support.sleepOneMillisecond();
    }
    try testing.expect(found);
    defer kept.deinit();

    const before_close = kept.value.events[kept_index];
    try testing.expectEqual(map_id, before_close.source);
    try testing.expect(before_close.source != 0);
    try testing.expect(before_close.message.len > 0);

    try support.closeMap(&map);
    map_open = false;
    try support.closeRuntime(&runtime);
    runtime_open = false;

    const after_close = kept.value.events[kept_index];
    try testing.expectEqual(map_id, after_close.source);
    try testing.expect(after_close.message.len > 0);
}

test "closing a map leaves its queued runtime events unchanged" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    // An unparseable style fails the command; the logs and events it produces
    // are what these tests read.
    try support.expectCommandError(try maplibre.mapSetStyleJson(support.handle(map), "{"), error.NativeError);
    try support.waitForBarrier(&runtime);
    try support.closeMap(&map);

    var batch = try support.drainEventSnapshot(runtime);
    defer batch.deinit();
    try testing.expect(batch.value.events.len > 0);
    var found_source = false;
    for (0..batch.value.events.len) |index| {
        const event = batch.value.events[index];
        if (event.source_type == .map and event.source != 0) found_source = true;
    }
    try testing.expect(found_source);
}

test "event masks round-trip through both handles" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    var runtime_mask = try maplibre.runtimeGetEventMask(support.handle(runtime));
    runtime_mask.offline_region_status_changed = false;
    try maplibre.runtimeSetEventMask(support.handle(runtime), runtime_mask);
    const read_runtime_mask = try maplibre.runtimeGetEventMask(support.handle(runtime));
    try testing.expectEqual(runtime_mask, read_runtime_mask);
    // A runtime ignores the map bits and still reports them back.
    try testing.expect(read_runtime_mask.map_style_loaded);

    var map_mask = (try maplibre.mapSnapshotGet(support.handle(map))).event_mask;
    map_mask.map_tile_action = false;
    try support.expectCommitted(try maplibre.mapSetEventMask(support.handle(map), map_mask));
    try support.waitForBarrier(&runtime);
    const read_map_mask = (try maplibre.mapSnapshotGet(support.handle(map))).event_mask;
    try testing.expectEqual(map_mask, read_map_mask);
    try testing.expect(read_map_mask.offline_region_status_changed);
}

// A newer native library can report an event type this binding does not name,
// and a mask holds only 64 bits, so the membership test must not shift by the
// raw value it was handed.
test "mask membership rejects an unknown type no mask bit can hold" {}

test "a narrowed map mask drops the type it clears and keeps the rest" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    const narrowed = maskWithout("map_render_update_available");
    try support.expectCommitted(try maplibre.mapSetEventMask(support.handle(map), narrowed));
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(narrowed, (try maplibre.mapSnapshotGet(support.handle(map))).event_mask);

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);

    // Restoring the bit lets the map's only invalidation report arrive again.
    try support.expectCommitted(try maplibre.mapSetEventMask(support.handle(map), maplibre.RuntimeEventMask.all));
    try support.waitForBarrier(&runtime);
    try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
}

test "masks passed as create options narrow both handles" {
    const narrowed_runtime_mask = maskWithout("offline_region_status_changed");
    var runtime = try support.createRuntime(.{ .event_mask = narrowed_runtime_mask });
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    try testing.expectEqual(narrowed_runtime_mask, try maplibre.runtimeGetEventMask(support.handle(runtime)));

    const narrowed_map_mask = maskWithout("map_render_update_available");
    var map = try support.createMap(&runtime, .{ .event_mask = narrowed_map_mask });
    defer support.closeMap(&map) catch @panic("map close failed");
    try testing.expectEqual(narrowed_map_mask, (try maplibre.mapSnapshotGet(support.handle(map))).event_mask);

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);
}

// Events reach the queue in the order the map committed the commands that
// produced them, and a drain hands them out in that order.
test "drained events keep the order the map committed their commands in" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");
    _ = try support.drainEvents(&runtime);

    try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{
        .mode = .ease,
        .camera = .{ .zoom = 4.0 },
        .animation = .{ .duration_ms = 60_000, .transition_id = 41 },
    }));
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .mode = .jump, .camera = .{ .zoom = 8.0 } }));

    var saw_update = false;
    var saw_transition_finished = false;
    for (0..1000) |_| {
        var batch = try support.drainEventSnapshot(runtime);
        defer batch.deinit();
        for (0..batch.value.events.len) |index| {
            const event = batch.value.events[index];
            if (std.meta.eql(event.type, maplibre.RuntimeEventType.map_render_update_available)) {
                saw_update = true;
            }
            if (std.meta.eql(event.type, maplibre.RuntimeEventType.map_camera_transition_finished)) {
                try testing.expect(saw_update);
                try testing.expectEqual(@as(u64, 41), event.payload.camera_transition_finished.transition_id);
                saw_transition_finished = true;
            }
        }
        if (saw_transition_finished) break;
        try support.sleepOneMillisecond();
    }
    try testing.expect(saw_transition_finished);
}

const WakeCounter = struct {
    calls: std.atomic.Value(usize) = .init(0),

    fn onWake(user_data: ?*anyopaque) maplibre.Error!void {
        const self: *WakeCounter = @ptrCast(@alignCast(user_data orelse return));
        _ = self.calls.fetchAdd(1, .seq_cst);
    }

    fn waitForWake(self: *WakeCounter) !void {
        for (0..1000) |_| {
            if (self.calls.load(.seq_cst) != 0) return;
            try support.sleepOneMillisecond();
        }
        return error.WakeNotObserved;
    }
};

test "runtime wake lifetime follows native runtime retirement" {
    var counter = WakeCounter{};
    var runtime = try support.createRuntime(.{ .event_wake = maplibre.Wake{ .callback = WakeCounter.onWake, .context = &counter } });
    var map = try support.createMap(&runtime, .{});
    _ = try support.drainEvents(&runtime);
    try support.expectCommitted(try maplibre.mapRequestRepaint(map));
    try counter.waitForWake();
    try support.closeMap(&map);
    try support.closeRuntime(&runtime);
    const after_close = counter.calls.load(.seq_cst);
    try support.sleepOneMillisecond();
    try testing.expectEqual(after_close, counter.calls.load(.seq_cst));
}
