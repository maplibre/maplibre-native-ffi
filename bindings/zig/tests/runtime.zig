const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

fn runRuntimeOnThread(runtime: maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.runOnce() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn pollRuntimeOnThread(runtime: maplibre.RuntimeHandle, out_error: *?anyerror) void {
    _ = runtime.pollEvent() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn closeRuntimeOnThread(runtime: maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.close() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

fn createRuntimeOnThread(out_error: *?anyerror) void {
    const runtime = maplibre.RuntimeHandle.init(null) catch |err| {
        out_error.* = err;
        return;
    };
    runtime.close() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "runtime rejects second runtime on same owner and permits distinct owner" {
    const runtime = try maplibre.RuntimeHandle.init(null);
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

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");

    var run_once_error: ?anyerror = null;
    const run_once_thread = try std.Thread.spawn(.{}, runRuntimeOnThread, .{ runtime, &run_once_error });
    run_once_thread.join();
    try testing.expectEqual(error.WrongThread, run_once_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);

    var poll_error: ?anyerror = null;
    const poll_thread = try std.Thread.spawn(.{}, pollRuntimeOnThread, .{ runtime, &poll_error });
    poll_thread.join();
    try testing.expectEqual(error.WrongThread, poll_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);

    var close_error: ?anyerror = null;
    const close_thread = try std.Thread.spawn(.{}, closeRuntimeOnThread, .{ runtime, &close_error });
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
