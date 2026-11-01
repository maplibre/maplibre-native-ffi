//! Snippets for the docs site's Events and logs page. They compile with the
//! tests, which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &eventsHandler;
    _ = &eventsQueue;
    _ = &eventsLogs;
}

const allocator = std.heap.smp_allocator;

var map: maplibre.Map = undefined;

fn onStyleLoaded() void {}

fn onLoadingFailed(_: []const u8) void {}

fn postToEventLoop(_: maplibre.RuntimeEventType, _: []const u8) !void {}

fn writeLog(_: maplibre.LogSeverity, _: []const u8) void {}

// #region handler
fn onEvents(_: ?*anyopaque, events: []const maplibre.RuntimeEvent) anyerror!void {
    for (events) |event| {
        if (event.source != map.id()) continue;

        switch (event.type) {
            .map_style_loaded => onStyleLoaded(),
            // The message is valid only during this callback.
            .map_loading_failed => onLoadingFailed(event.message),
            else => {},
        }
    }
}
// #endregion handler

fn eventsHandler(runtime: maplibre.Runtime) !void {
    // #region handler
    try runtime.discarding().setEventHandler(.{ .callback = onEvents }, null);
    // #endregion handler

    // #region mask
    var created = try runtime.createMap(.{
        .initial_extent = .{ .width = 512, .height = 512, .scale_factor = 1.0 },
        .event_mask = .{ .map_style_loaded = true, .map_loading_failed = true },
    }, null);
    defer created.ready.deinit();
    map = created.map;
    // #endregion mask
}

// #region queue
// Runs on a library thread. The binding has no delivery queue, so copy what
// your event loop needs, post it there, and return.
fn postEvents(_: ?*anyopaque, events: []const maplibre.RuntimeEvent) anyerror!void {
    for (events) |event| {
        if (event.source != map.id()) continue;
        try postToEventLoop(event.type, try allocator.dupe(u8, event.message));
    }
}
// #endregion queue

fn eventsQueue(runtime: maplibre.Runtime) !void {
    // #region queue
    try runtime.discarding().setEventHandler(.{ .callback = postEvents }, null);
    // #endregion queue
}

// #region logs
fn onLog(_: ?*anyopaque, severity: maplibre.LogSeverity, _: maplibre.LogEvent, _: i64, message: []const u8) anyerror!bool {
    // The message is valid only during this callback.
    writeLog(severity, message);
    // Consume the record. Return false to let the platform logger print it too.
    return true;
}
// #endregion logs

fn eventsLogs() !void {
    // #region logs
    try maplibre.setLogCallback(.{ .callback = onLog }, null);
    // #endregion logs
}
