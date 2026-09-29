const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

const LogState = struct {
    count: usize = 0,
    saw_parse_style: bool = false,
    saw_message: bool = false,
};

fn recordLog(context: ?*anyopaque, severity: maplibre.LogSeverity, event: maplibre.LogEvent, _: i64, message: []const u8) maplibre.Error!u32 {
    const state: *LogState = @ptrCast(@alignCast(context.?));
    state.count += 1;
    if (std.meta.eql(severity, maplibre.LogSeverity.@"error") and
        std.meta.eql(event, maplibre.LogEvent.parse_style))
    {
        state.saw_parse_style = true;
    }
    if (message.len > 0) state.saw_message = true;
    return 1;
}

test "log callback receives and consumes native logs" {
    var state = LogState{};
    try maplibre.logSetAsyncSeverityMask(.{}, null);
    try maplibre.logSetCallback(.{ .call = recordLog, .context = &state }, null);
    defer {
        maplibre.logClearCallback(null) catch @panic("log callback clear failed");
        maplibre.logSetAsyncSeverityMask(.{ .info = true, .warning = true, .@"error" = true }, null) catch @panic("log severity restore failed");
    }

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    // An unparseable style fails the command; the logs and events it produces
    // are what these tests read.
    try support.expectCommandError(try maplibre.mapSetStyleJson(support.handle(map), "{"), error.NativeError);
    var barrier = try maplibre.runtimeBarrier(support.handle(runtime));
    defer barrier.deinit();
    _ = try barrier.wait(null);
    try testing.expect(state.count > 0);
    try testing.expect(state.saw_parse_style);
    try testing.expect(state.saw_message);
}

test "log callback can be cleared" {
    var state = LogState{};
    try maplibre.logSetAsyncSeverityMask(.{}, null);
    defer maplibre.logSetAsyncSeverityMask(.{ .info = true, .warning = true, .@"error" = true }, null) catch @panic("log severity restore failed");
    try maplibre.logSetCallback(.{ .call = recordLog, .context = &state }, null);
    try maplibre.logClearCallback(null);

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    // An unparseable style fails the command; the logs and events it produces
    // are what these tests read.
    try support.expectCommandError(try maplibre.mapSetStyleJson(support.handle(map), "{"), error.NativeError);
    var barrier = try maplibre.runtimeBarrier(support.handle(runtime));
    defer barrier.deinit();
    _ = try barrier.wait(null);
    try testing.expectEqual(@as(usize, 0), state.count);
}

test "log callback replacement invokes only the replacement" {
    var first_state = LogState{};
    var replacement_state = LogState{};
    try maplibre.logSetAsyncSeverityMask(.{}, null);
    defer {
        maplibre.logClearCallback(null) catch @panic("log callback clear failed");
        maplibre.logSetAsyncSeverityMask(.{ .info = true, .warning = true, .@"error" = true }, null) catch @panic("log severity restore failed");
    }
    try maplibre.logSetCallback(.{ .call = recordLog, .context = &first_state }, null);
    try maplibre.logSetCallback(.{ .call = recordLog, .context = &replacement_state }, null);

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");

    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    // An unparseable style fails the command; the logs and events it produces
    // are what these tests read.
    try support.expectCommandError(try maplibre.mapSetStyleJson(support.handle(map), "{"), error.NativeError);
    var barrier = try maplibre.runtimeBarrier(support.handle(runtime));
    defer barrier.deinit();
    _ = try barrier.wait(null);
    try testing.expectEqual(@as(usize, 0), first_state.count);
    try testing.expect(replacement_state.count > 0);
    try testing.expect(replacement_state.saw_parse_style);
    try testing.expect(replacement_state.saw_message);
}
