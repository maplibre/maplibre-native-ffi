//! Snippets for the docs site's Render to a window page. They compile with the
//! tests, which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &renderToWindow;
    _ = &surfaceLost;
    _ = &surfaceRestored;
}

const allocator = std.heap.smp_allocator;

// Stand-ins for the host's event loop, display timer, and graphics thread.
fn runOnEventLoop(comptime task: anytype) void {
    _ = &task;
}

fn runAtNextRefresh(comptime task: anytype) void {
    _ = &task;
}

fn runOnGraphicsThread(comptime task: anytype) void {
    _ = &task;
}

fn replaceSession() void {}

fn destroySurface() void {}

// #region attach
var session: maplibre.RenderSession = undefined;
// #endregion attach

// #region frame-loop
// Draws the map if it changed since the last frame.
fn demandFrame() !void {
    try session.demandFrame(.{}, null);
}

fn drainFrameResults() !void {
    var results = try session.drainFrameResults(allocator, null);
    defer results.deinit();
    if (results.value.len == 0) return; // No results yet.

    for (results.value) |result| {
        if (result.disposition == .failed) {
            // Dispose the session and attach a new one.
            return replaceSession();
        }
    }
}
// #endregion frame-loop

// #region frame-wake
// Both run on a library thread.
fn onRedrawWake(_: ?*anyopaque) anyerror!void {
    runAtNextRefresh(demandFrame);
}

fn onFrameWake(_: ?*anyopaque) anyerror!void {
    runOnEventLoop(drainFrameResults);
}
// #endregion frame-wake

// #region resize
// Runs when the window changes size, or moves to a display with another
// scale factor.
fn onWindowResized(width: u32, height: u32, scale_factor: f64) !void {
    // The window's new size in logical pixels, and its scale factor.
    try session.discarding().resize(.{
        .width = width,
        .height = height,
        .scale_factor = scale_factor,
    }, null);
}
// #endregion resize

// #region service
// Your graphics context is current on this thread.
fn serviceSession() !void {
    try session.serviceDriverWork(null);
}

// Runs on a library thread.
fn onDriverWorkWake(_: ?*anyopaque) anyerror!void {
    runOnGraphicsThread(serviceSession);
}
// #endregion service

fn surfaceLost() !void {
    // #region lost-surface
    try (try session.releaseTarget(null)).await(null);
    // #endregion lost-surface
}

fn surfaceRestored(display: ?*anyopaque, new_surface: ?*anyopaque) !void {
    // #region lost-surface
    const replaced = try session.setTarget(allocator, .{ .wayland_surface = .{
        .logical_size = .{ .width = 800, .height = 600 },
        .display = display,
        .surface = new_surface,
    } }, null);
    try replaced.await(null);
    // #endregion lost-surface
}

fn renderToWindow(map: maplibre.Map, display: ?*anyopaque, window_surface: ?*anyopaque) !void {
    // #region attach
    // With no device, the session creates one on the platform's own
    // backend.
    const attached = try map.attach(allocator, .{
        .wayland_surface = .{
            // The window's size in logical pixels. The map's scale factor
            // turns it into pixels.
            .logical_size = .{ .width = 800, .height = 600 },
            // Your wl_display and wl_surface.
            .display = display,
            .surface = window_surface,
        },
    }, null, .{
        .driver = .core_worker,
        .redraw_wake = .{ .callback = onRedrawWake },
        .frame_wake = .{ .callback = onFrameWake },
    }, null);
    session = attached.session;
    try attached.ready.await(null);
    // #endregion attach

    try onWindowResized(1024, 768, 2);

    // #region service
    const graphics_thread_options =
        maplibre.RenderSessionAttachOptions{
            .driver = .caller_graphics_thread,
            .redraw_wake = .{ .callback = onRedrawWake },
            .frame_wake = .{ .callback = onFrameWake },
            .driver_work_wake = .{ .callback = onDriverWorkWake },
        };
    // #endregion service
    _ = graphics_thread_options;

    try (try session.release(null)).await(null);

    destroySurface(); // For example, with wl_surface_destroy.
}
