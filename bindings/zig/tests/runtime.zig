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

fn setRuntimeEventMaskOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.setEventMask(cross_thread_runtime_mask) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn setMapEventMaskOnThread(map: *maplibre.MapHandle, out_error: *?anyerror) void {
    support.expectCommitted(map.setEventMask(cross_thread_map_mask) catch |err| {
        out_error.* = err;
        return;
    }) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn closeRuntimeOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    support.closeRuntime(runtime) catch |err| {
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
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
    rejected: maplibre.RuntimeEventType,
) !void {
    // Narrowing gates later events and keeps queued ones, so start empty.
    _ = try support.drainEvents(runtime);

    try support.expectCommitted(try map.setStyleJson(support.style_json));
    try support.expectCommitted(try map.requestRepaint());

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
        try support.sleepOneMillisecond();
    }
    try testing.expect(saw_style_loaded);
}

test "runtimes can be created on the current thread or another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var second = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&second) catch @panic("runtime close failed");

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
    // The close another thread ran is the one this thread observes.
    try testing.expectError(error.ClosedHandle, runtime.drainEvents(testing.allocator));
}

test "event mask setters accept calls from another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
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
    try testing.expectEqual(cross_thread_runtime_mask, try runtime.eventMask());
    try testing.expectEqual(cross_thread_map_mask, (try map.snapshot()).event_mask);
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
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try map.setStyleJson(support.style_json));

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
        try support.sleepOneMillisecond();
    }
    try testing.expect(saw_style_loaded);
    try testing.expect(largest_batch > 1);
}
test "a drained batch outlives its runtime" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    var runtime_open = true;
    defer if (runtime_open) support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    var map_open = true;
    defer if (map_open) support.closeMap(&map) catch @panic("map close failed");
    const map_id = try map.id();

    try support.expectCommitted(try map.setStyleUrl(testing.allocator, "unsupported://style.json"));

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
        try support.sleepOneMillisecond();
    }
    try testing.expect(found);
    defer kept.deinit();

    const before_close = try kept.at(kept_index);
    try testing.expectEqual(map_id, before_close.source_id.?);
    try testing.expect(before_close.source != .none);
    try testing.expect(before_close.message.len > 0);

    try support.closeMap(&map);
    map_open = false;
    try support.closeRuntime(&runtime);
    runtime_open = false;

    const after_close = try kept.at(kept_index);
    try testing.expectEqual(map_id, after_close.source_id.?);
    try testing.expect(after_close.message.len > 0);
}

test "closing a map leaves its queued runtime events unchanged" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    // An unparseable style fails the command; the logs and events it produces
    // are what these tests read.
    try support.expectCommandError(try map.setStyleJson("{"), error.NativeError);
    try support.waitForBarrier(&runtime);
    try support.closeMap(&map);

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
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    var runtime_mask = try runtime.eventMask();
    runtime_mask.offline_region_status_changed = false;
    try runtime.setEventMask(runtime_mask);
    const read_runtime_mask = try runtime.eventMask();
    try testing.expectEqual(runtime_mask, read_runtime_mask);
    // A runtime ignores the map bits and still reports them back.
    try testing.expect(read_runtime_mask.map_style_loaded);

    var map_mask = (try map.snapshot()).event_mask;
    map_mask.map_tile_action = false;
    try support.expectCommitted(try map.setEventMask(map_mask));
    try support.waitForBarrier(&runtime);
    const read_map_mask = (try map.snapshot()).event_mask;
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
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    const narrowed = maskWithout("map_render_update_available");
    try support.expectCommitted(try map.setEventMask(narrowed));
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(narrowed, (try map.snapshot()).event_mask);

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);

    // Restoring the bit lets the map's only invalidation report arrive again.
    try support.expectCommitted(try map.setEventMask(maplibre.RuntimeEventMask.all));
    try support.waitForBarrier(&runtime);
    try support.expectCommitted(try map.requestRepaint());
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
}

test "masks passed as create options narrow both handles" {
    const narrowed_runtime_mask = maskWithout("offline_region_status_changed");
    var runtime = try maplibre.RuntimeHandle.create(
        testing.allocator,
        .{ .event_mask = narrowed_runtime_mask },
        null,
    );
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    try testing.expectEqual(narrowed_runtime_mask, try runtime.eventMask());

    const narrowed_map_mask = maskWithout("map_render_update_available");
    var map = try support.createMap(&runtime, .{ .event_mask = narrowed_map_mask });
    defer support.closeMap(&map) catch @panic("map close failed");
    try testing.expectEqual(narrowed_map_mask, (try map.snapshot()).event_mask);

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);
}

// Events reach the queue in the order the map committed the commands that
// produced them, and a drain hands them out in that order.
test "drained events keep the order the map committed their commands in" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");
    _ = try support.drainEvents(&runtime);

    try support.expectCommitted(try map.requestRepaint());
    try support.expectCommitted(try map.updateCamera(.{
        .mode = .ease,
        .camera = .{ .zoom = 4.0 },
        .animation = .{ .duration_ms = 60_000, .transition_id = 41 },
    }));
    try support.expectCommitted(try map.updateCamera(.{ .mode = .jump, .camera = .{ .zoom = 8.0 } }));

    var saw_update = false;
    var saw_transition_finished = false;
    for (0..1000) |_| {
        var batch = try runtime.drainEvents(testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_render_update_available)) {
                saw_update = true;
            }
            if (std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_camera_transition_finished)) {
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

    fn onWake(user_data: ?*anyopaque) callconv(.c) void {
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

// The wake tells a host receiver that a drain has something to take, and
// clearing it returns only once no invocation is running, so the host may free
// the state the callback read.
test "the event wake runs until the host clears it" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    var counter = WakeCounter{};
    try runtime.setEventWakeCallback(WakeCounter.onWake, &counter);

    // The wake fires when the queue goes from empty to holding an event, so
    // the drain is what makes the next event wake the receiver.
    _ = try support.drainEvents(&runtime);
    try support.expectCommitted(try map.requestRepaint());
    try counter.waitForWake();

    try runtime.clearEventWakeCallback();
    const after_clear = counter.calls.load(.seq_cst);
    _ = try support.drainEvents(&runtime);
    try support.expectCommitted(try map.requestRepaint());
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(after_clear, counter.calls.load(.seq_cst));
}

// A wake outlives close on the native side: the runtime keeps the descriptor
// until teardown finishes, so close is what retires the host's callback
// (BND-089), and a host may free its wake state as soon as close returns.
test "closing the runtime retires the event wake (BND-089)" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    var map = try support.createMap(&runtime, .{});

    var counter = WakeCounter{};
    try runtime.setEventWakeCallback(WakeCounter.onWake, &counter);
    _ = try support.drainEvents(&runtime);
    try support.expectCommitted(try map.requestRepaint());
    try counter.waitForWake();

    // The map's teardown outlives its close call, so the runtime tears down
    // with work left that could still queue events, and the drained queue lets
    // one of them wake the receiver.
    var map_teardown = try map.close();
    map_teardown.deinit();
    _ = try support.drainEvents(&runtime);

    var teardown = try runtime.close();
    defer teardown.deinit();
    const calls_at_close = counter.calls.load(.seq_cst);
    try teardown.wait(null);
    try testing.expectEqual(calls_at_close, counter.calls.load(.seq_cst));
}
