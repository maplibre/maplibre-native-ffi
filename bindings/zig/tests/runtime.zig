const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

fn runRuntimeOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.pump(0) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn drainRuntimeOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    var batch = runtime.drainEvents(0) catch |err| {
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
    map.setEventMask(maplibre.RuntimeEventMask.all) catch |err| {
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
    try runtime.pump(0);
    _ = try support.drainEvents(runtime);

    try map.setStyleJson(testing.allocator, support.style_json);
    try map.requestRepaint();

    var saw_style_loaded = false;
    for (0..1000) |_| {
        try runtime.pump(0);
        var batch = try runtime.drainEvents(0);
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

test "runtime rejects second runtime on same owner and permits distinct owner" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    try testing.expectError(error.InvalidState, maplibre.RuntimeHandle.create(testing.allocator, .{}, null));

    var thread_error: ?anyerror = error.InvalidState;
    const thread = try std.Thread.spawn(.{}, createRuntimeOnThread, .{&thread_error});
    thread.join();
    try testing.expect(thread_error == null);
}

test "wrong-thread runtime failures propagate diagnostics" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, &diagnostics);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    var run_once_error: ?anyerror = null;
    const run_once_thread = try std.Thread.spawn(.{}, runRuntimeOnThread, .{ &runtime, &run_once_error });
    run_once_thread.join();
    try testing.expectEqual(error.WrongThread, run_once_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);

    var drain_error: ?anyerror = null;
    const drain_thread = try std.Thread.spawn(.{}, drainRuntimeOnThread, .{ &runtime, &drain_error });
    drain_thread.join();
    try testing.expectEqual(error.WrongThread, drain_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);

    var close_error: ?anyerror = null;
    const close_thread = try std.Thread.spawn(.{}, closeRuntimeOnThread, .{ &runtime, &close_error });
    close_thread.join();
    try testing.expectEqual(error.WrongThread, close_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);

    try runtime.pump(0);
    try runtime.close();
    runtime_open = false;
}

test "event mask setters report wrong thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var runtime_mask_error: ?anyerror = null;
    const runtime_mask_thread = try std.Thread.spawn(.{}, setRuntimeEventMaskOnThread, .{ &runtime, &runtime_mask_error });
    runtime_mask_thread.join();
    try testing.expectEqual(error.WrongThread, runtime_mask_error.?);

    var map_mask_error: ?anyerror = null;
    const map_mask_thread = try std.Thread.spawn(.{}, setMapEventMaskOnThread, .{ &map, &map_mask_error });
    map_mask_thread.join();
    try testing.expectEqual(error.WrongThread, map_mask_error.?);

    // The owner thread still installs both masks.
    try runtime.setEventMask(maplibre.RuntimeEventMask.all);
    try map.setEventMask(maplibre.RuntimeEventMask.all);
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

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleJson(testing.allocator, support.style_json);

    var largest_batch: usize = 0;
    var saw_style_loaded = false;
    for (0..1000) |_| {
        try runtime.pump(0);
        var batch = try runtime.drainEvents(0);
        defer batch.deinit();
        // An unbounded drain takes the whole queue.
        try testing.expectEqual(@as(usize, 0), batch.remaining());
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

test "a bounded drain reports one event at a time in queue order" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    // A malformed inline style is reported twice: as the error here and as a
    // queued loading-failed event carrying the same text.
    try testing.expectError(error.NativeError, map.setStyleJson(testing.allocator, "{"));
    try map.requestRepaint();

    var message_length: usize = 0;
    var count: usize = 0;
    var saw_remaining = false;
    var last = maplibre.RuntimeEventType.map_idle;
    while (true) {
        var batch = try runtime.drainEvents(1);
        defer batch.deinit();
        if (batch.len() == 0) {
            try testing.expectEqual(@as(usize, 0), batch.remaining());
            break;
        }
        try testing.expectEqual(@as(usize, 1), batch.len());
        // The bound leaves the rest of the queue for the next drain.
        if (batch.remaining() > 0) saw_remaining = true;
        const event = try batch.at(0);
        if (std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_loading_failed)) {
            message_length = event.message.len;
        }
        last = event.event_type;
        count += 1;
    }
    try testing.expect(count > 1);
    try testing.expect(saw_remaining);
    // The loading failure carries native's own text.
    try testing.expect(message_length > 0);
    // The repaint queued its invalidation behind the load failure.
    try testing.expect(std.meta.eql(last, maplibre.RuntimeEventType.map_render_update_available));
}

test "a drained event copies out and outlives the batch that carried it" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    var map_open = true;
    defer if (map_open) map.close() catch @panic("map close failed");
    const map_id = try map.id();

    try map.setStyleUrl(testing.allocator, "unsupported://style.json");

    var owned: maplibre.OwnedRuntimeEvent = undefined;
    var stale: maplibre.EventBatch = undefined;
    var copied = false;
    for (0..1000) |_| {
        try runtime.pump(0);
        var batch = try runtime.drainEvents(0);
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (!std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_loading_failed)) continue;
            owned = try event.toOwned(testing.allocator);
            copied = true;
            break;
        }
        if (copied) {
            // The batch stays live to prove what a later drain does to it.
            stale = batch;
            break;
        }
        batch.deinit();
        try sleepOneMillisecond();
    }
    try testing.expect(copied);
    defer owned.deinit();

    try testing.expectEqual(map_id, owned.source_id.?);
    // A real drain reports the native identity beside the resolved map.
    try testing.expect(owned.source != .none);
    try testing.expect(std.meta.eql(owned.payload, maplibre.RuntimeEventPayload.none));
    try testing.expect(owned.message.len > 0);
    const copied_message = try testing.allocator.dupe(u8, owned.message);
    defer testing.allocator.free(copied_message);

    // A live batch is a borrow on the runtime.
    try testing.expectError(error.ActiveBorrow, runtime.close());

    var next = try runtime.drainEvents(0);
    try testing.expectError(error.InvalidState, stale.at(0));
    stale.deinit();
    try testing.expectError(error.InvalidState, stale.at(0));
    try testing.expectEqualSlices(u8, copied_message, owned.message);

    // Every batch is deinited, so the runtime closes.
    next.deinit();
    try map.close();
    map_open = false;
    try runtime.close();
    runtime_open = false;
}

test "deiniting an event batch invalidates every copy of it" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    var map_open = true;
    defer if (map_open) map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "unsupported://style.json");

    var batch: maplibre.EventBatch = undefined;
    var drained = false;
    for (0..1000) |_| {
        try runtime.pump(0);
        var pending = try runtime.drainEvents(0);
        if (pending.len() > 0) {
            batch = pending;
            drained = true;
            break;
        }
        pending.deinit();
        try sleepOneMillisecond();
    }
    try testing.expect(drained);

    // A batch is a value, so nothing stops a copy from outliving the original.
    const copy = batch;
    try testing.expect(copy.len() > 0);
    _ = try copy.at(0);

    batch.deinit();

    // The runtime owns the epoch, so the deinit invalidated the copy too instead
    // of leaving it reading storage no lease protects.
    try testing.expectEqual(@as(usize, 0), copy.len());
    try testing.expectEqual(@as(usize, 0), copy.remaining());
    try testing.expectError(error.InvalidState, copy.at(0));

    // Deiniting the copy releases nothing a second time, so the runtime closes.
    var second = copy;
    second.deinit();
    try map.close();
    map_open = false;
    try runtime.close();
    runtime_open = false;

    // A copy that outlived its runtime reports the same failure rather than
    // reading the state the close freed.
    try testing.expectError(error.InvalidState, copy.at(0));
}

test "closing a map discards its queued runtime events" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    try testing.expectError(error.NativeError, map.setStyleJson(testing.allocator, "{"));
    try map.close();

    try testing.expectEqual(@as(usize, 0), try support.drainEvents(&runtime));
}

test "event masks round-trip through both handles" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var runtime_mask = try runtime.eventMask();
    runtime_mask.offline_operation_completed = false;
    try runtime.setEventMask(runtime_mask);
    const read_runtime_mask = try runtime.eventMask();
    try testing.expectEqual(runtime_mask, read_runtime_mask);
    // A runtime ignores the map bits and still reports them back.
    try testing.expect(read_runtime_mask.map_style_loaded);

    var map_mask = try map.eventMask();
    map_mask.map_tile_action = false;
    try map.setEventMask(map_mask);
    const read_map_mask = try map.eventMask();
    try testing.expectEqual(map_mask, read_map_mask);
    try testing.expect(read_map_mask.offline_operation_completed);
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

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const narrowed = maskWithout("map_render_update_available");
    try map.setEventMask(narrowed);
    try testing.expectEqual(narrowed, try map.eventMask());

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);

    // Restoring the bit lets the map's only invalidation report arrive again.
    try map.setEventMask(maplibre.RuntimeEventMask.all);
    try map.requestRepaint();
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
}

test "masks passed as create options narrow both handles" {
    const narrowed_runtime_mask = maskWithout("offline_operation_completed");
    var runtime = try maplibre.RuntimeHandle.create(
        testing.allocator,
        .{ .event_mask = narrowed_runtime_mask },
        null,
    );
    defer runtime.close() catch @panic("runtime close failed");
    try testing.expectEqual(narrowed_runtime_mask, try runtime.eventMask());

    const narrowed_map_mask = maskWithout("map_render_update_available");
    var map = try maplibre.MapHandle.create(&runtime, .{ .event_mask = narrowed_map_mask });
    defer map.close() catch @panic("map close failed");
    try testing.expectEqual(narrowed_map_mask, try map.eventMask());

    try expectOnlySelectedTypes(&runtime, &map, .map_render_update_available);
}

// Pumps until the runtime is idle, so a park that follows is released by the
// signal the test raises.
fn quiesce(runtime: *maplibre.RuntimeHandle) !void {
    for (0..100) |_| {
        try runtime.pump(0);
        if ((try support.drainEvents(runtime)) == 0) return;
    }
    return error.RuntimeKeptProducingEvents;
}

fn elapsedMilliseconds(started: std.Io.Timestamp) u64 {
    const elapsed = started.durationTo(std.Io.Clock.awake.now(testing.io));
    return @intCast(@divTrunc(elapsed.toNanoseconds(), std.time.ns_per_ms));
}

fn signalWakeSourceOnThread(source: maplibre.WakeSourceHandle, out_error: *?anyerror) void {
    testing.io.sleep(.fromMilliseconds(20), .awake) catch |err| {
        out_error.* = err;
        return;
    };
    source.signal() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "a parked owner thread wakes for native work and for a wake source" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    var map_open = true;
    defer if (map_open) map.close() catch @panic("map close failed");
    try quiesce(&runtime);

    // A malformed style is reported from native's own threads, so the failure
    // reaches the parked owner thread.
    try map.setStyleUrl(testing.allocator, "unsupported://style.json");
    var loading_failed = false;
    const load_started = std.Io.Clock.awake.now(testing.io);
    for (0..20) |_| {
        try runtime.pump(10_000);
        if (elapsedMilliseconds(load_started) > 5_000) return error.ParkTimedOut;
        var batch = try runtime.drainEvents(0);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_loading_failed)) {
                loading_failed = true;
            }
        }
        if (loading_failed) break;
    }
    try testing.expect(loading_failed);

    // The park this signal releases has no other work to end it.
    const source = try runtime.wakeSource();
    try quiesce(&runtime);
    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, signalWakeSourceOnThread, .{ source, &thread_error });
    const park_started = std.Io.Clock.awake.now(testing.io);
    try runtime.pump(10_000);
    try testing.expect(elapsedMilliseconds(park_started) < 5_000);
    thread.join();
    try testing.expect(thread_error == null);

    // A wake source stays usable after its runtime closes.
    try map.close();
    map_open = false;
    try runtime.close();
    runtime_open = false;
    try source.signal();
    source.release();
    try testing.expectError(error.ClosedHandle, source.signal());
}

test "a pump clears the wake flag it returns on" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    const source = try runtime.wakeSource();
    defer source.release();
    try quiesce(&runtime);

    try source.signal();
    const signalled_started = std.Io.Clock.awake.now(testing.io);
    try runtime.pump(10_000);
    try testing.expect(elapsedMilliseconds(signalled_started) < 5_000);

    // The pump above cleared the wake flag, so this one waits its full timeout.
    const idle_started = std.Io.Clock.awake.now(testing.io);
    try runtime.pump(200);
    try testing.expect(elapsedMilliseconds(idle_started) >= 100);
}

test "an idle map drains empty batches" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try quiesce(&runtime);
    var batch = try runtime.drainEvents(0);
    defer batch.deinit();
    try testing.expectEqual(@as(usize, 0), batch.len());
    try testing.expectEqual(@as(usize, 0), batch.remaining());
    // An empty batch has no event to report at any index.
    try testing.expectError(error.InvalidArgument, batch.at(0));
}
