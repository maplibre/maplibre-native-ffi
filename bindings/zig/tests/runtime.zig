const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

fn runRuntimeOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.runOnce() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn pollRuntimeOnThread(runtime: *maplibre.RuntimeHandle, out_error: *?anyerror) void {
    _ = runtime.pollEvent(testing.allocator) catch |err| {
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

    var poll_error: ?anyerror = null;
    const poll_thread = try std.Thread.spawn(.{}, pollRuntimeOnThread, .{ &runtime, &poll_error });
    poll_thread.join();
    try testing.expectEqual(error.WrongThread, poll_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);

    var close_error: ?anyerror = null;
    const close_thread = try std.Thread.spawn(.{}, closeRuntimeOnThread, .{ &runtime, &close_error });
    close_thread.join();
    try testing.expectEqual(error.WrongThread, close_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);

    try runtime.runOnce();
    try runtime.close();
    runtime_open = false;
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

test "owned runtime events copy message and resolve map identity" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");
    const map_id = try map.id();

    try map.setStyleUrl(testing.allocator, "unsupported://style.json");

    var found: ?maplibre.OwnedRuntimeEvent = null;
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent(testing.allocator)) |event| {
            if (std.meta.eql(event.event_type, maplibre.RuntimeEventType.map_loading_failed)) {
                found = event;
                break;
            }
            var discard = event;
            discard.deinit();
        }
        if (found != null) break;
        try sleepOneMillisecond();
    }

    var event = found orelse return error.EventNotFound;
    defer event.deinit();
    const source_id = event.source_id orelse return error.MissingSourceId;
    try testing.expectEqual(map_id, source_id);
    try testing.expect(std.meta.eql(event.payload, maplibre.RuntimeEventPayload.none));
    try testing.expect(event.message.len > 0);
    const copied_message = try testing.allocator.dupe(u8, event.message);
    defer testing.allocator.free(copied_message);

    if (try runtime.pollEvent(testing.allocator)) |later_event| {
        var discard = later_event;
        discard.deinit();
    }
    try testing.expectEqualSlices(u8, copied_message, event.message);
}

test "closing a map discards queued runtime events" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    try testing.expectError(error.NativeError, map.setStyleJson(testing.allocator, "{"));
    try map.close();

    try testing.expectEqual(@as(?maplibre.OwnedRuntimeEvent, null), try runtime.pollEvent(testing.allocator));
}

// Leaves the runtime idle with no latched signal, so a following park can only
// be released by the signal the test raises.
fn drainLatchedWakes(runtime: *maplibre.RuntimeHandle) !void {
    for (0..100) |_| {
        if (!try runtime.wait(0)) return;
        try runtime.runOnce();
        while (try runtime.pollEvent(testing.allocator)) |event| {
            var owned_event = event;
            owned_event.deinit();
        }
    }
    return error.RuntimeKeptLatchingWakes;
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
    try drainLatchedWakes(&runtime);

    // The style is malformed, so native reports the failure from its own
    // threads. What matters here is that the failure reaches a parked owner
    // thread at all.
    try map.setStyleUrl(testing.allocator, "unsupported://style.json");
    var loading_failed = false;
    for (0..20) |_| {
        if (!try runtime.wait(10_000)) return error.ParkTimedOut;
        try runtime.runOnce();
        while (try runtime.pollEvent(testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            if (std.meta.eql(owned_event.event_type, maplibre.RuntimeEventType.map_loading_failed)) {
                loading_failed = true;
            }
        }
        if (loading_failed) break;
    }
    try testing.expect(loading_failed);

    // A source used from another thread is what a host's submission path holds,
    // and the park it releases has no other work to end it.
    const source = try runtime.wakeSource();
    try drainLatchedWakes(&runtime);
    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, signalWakeSourceOnThread, .{ source, &thread_error });
    try testing.expect(try runtime.wait(10_000));
    thread.join();
    try testing.expect(thread_error == null);

    // A wake source stays usable once its runtime is gone, so host teardown
    // ordering is free.
    try map.close();
    map_open = false;
    try runtime.close();
    runtime_open = false;
    try source.signal();
    source.release();
    try testing.expectError(error.ClosedHandle, source.signal());
}

test "a wait consumes one latched signal at a time" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    const source = try runtime.wakeSource();
    defer source.release();
    try drainLatchedWakes(&runtime);

    try source.signal();
    try testing.expect(try runtime.wait(0));
    // The latch is consumed, so an idle runtime reports the timeout instead.
    try testing.expect(!try runtime.wait(0));
}

test "runtime event polling reports empty queues" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    for (0..100) |_| {
        try runtime.runOnce();
        var drained = false;
        while (try runtime.pollEvent(testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            drained = true;
        }
        if (!drained) break;
    }

    try testing.expectEqual(@as(?maplibre.OwnedRuntimeEvent, null), try runtime.pollEvent(testing.allocator));
}
