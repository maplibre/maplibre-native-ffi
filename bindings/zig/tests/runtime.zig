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
    _ = runtime.pollEvent() catch |err| {
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
    var runtime = maplibre.RuntimeHandle.init(null) catch |err| {
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
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    try testing.expectError(error.InvalidState, maplibre.RuntimeHandle.init(null));

    var thread_error: ?anyerror = error.InvalidState;
    const thread = try std.Thread.spawn(.{}, createRuntimeOnThread, .{&thread_error});
    thread.join();
    try testing.expect(thread_error == null);
}

test "wrong-thread runtime failures propagate diagnostics" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");

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
}

test "runtime option strings reject embedded NUL before C calls" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    try testing.expectError(
        error.InvalidString,
        maplibre.RuntimeHandle.create(testing.allocator, .{ .asset_path = "asset\x00path" }, &diagnostics),
    );
}

test "owned runtime events copy message and resolve map identity" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");
    const map_id = try map.id();

    try map.setStyleUrl(testing.allocator, "unsupported://style.json");

    var found: ?maplibre.OwnedRuntimeEvent = null;
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEventOwned(testing.allocator)) |event| {
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

    if (try runtime.pollEventOwned(testing.allocator)) |later_event| {
        var discard = later_event;
        discard.deinit();
    }
    try testing.expectEqualSlices(u8, copied_message, event.message);
}

test "closing a map discards queued runtime events" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    try testing.expectError(error.NativeError, map.setStyleJson(testing.allocator, "{"));
    try map.close();

    try testing.expectEqual(@as(?maplibre.RuntimeEvent, null), try runtime.pollEvent());
}

test "runtime event polling reports empty queues" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    for (0..100) |_| {
        try runtime.runOnce();
        var drained = false;
        while (try runtime.pollEvent()) |_| drained = true;
        if (!drained) break;
    }

    try testing.expectEqual(@as(?maplibre.RuntimeEvent, null), try runtime.pollEvent());
    try testing.expectEqual(@as(?maplibre.OwnedRuntimeEvent, null), try runtime.pollEventOwned(testing.allocator));
}
