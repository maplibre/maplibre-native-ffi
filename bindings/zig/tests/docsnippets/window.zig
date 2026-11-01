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
// Set while the window has no surface, and when it closes.
var paused = false;

fn requestFrame() !void {
    try session.requestFrame(.{ .flags = .{ .present = true, .wait_for_update = true } }, null);
}

fn drainFrameResults() !void {
    var results = try session.drainFrameResults(allocator, null);
    defer results.deinit();
    if (results.value.len == 0) return; // No results yet.

    for (results.value) |result| {
        if (result.disposition == .failed) {
            return replaceSession(); // Dispose the session and attach a new one.
        }
    }
    if (!paused) runAtNextRefresh(requestFrame);
}
// #endregion frame-loop

// #region frame-wake
// Runs on a library thread.
fn onFrameWake(_: ?*anyopaque) anyerror!void {
    runOnEventLoop(drainFrameResults);
}
// #endregion frame-wake

// #region resize
fn onWindowResized(width: u32, height: u32) !void {
    try session.discarding().resize(.{
        .width = width,
        .height = height,
        .scale_factor = 2.0, // The scale factor the session has now.
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
    paused = true;

    var released = try session.releaseTarget(null);
    defer released.deinit();
    try released.wait(null);
    // #endregion lost-surface
}

fn surfaceRestored(vulkan: maplibre.VulkanContextDescriptor, new_surface: u64) !void {
    // #region lost-surface
    var replaced = try session.setVulkanSurfaceTarget(.{
        .extent = .{ .width = 800, .height = 600, .scale_factor = 2.0 },
        .context = vulkan,
        .surface = new_surface,
    }, null);
    defer replaced.deinit();
    try replaced.wait(null);

    paused = false;
    try requestFrame();
    // #endregion lost-surface
}

fn renderToWindow(map: maplibre.Map, vulkan: maplibre.VulkanContextDescriptor, window_surface: u64) !void {
    // #region attach
    var attached = try map.attachVulkanSurface(.{
        // The window's size in logical pixels, and its scale factor.
        .extent = .{ .width = 800, .height = 600, .scale_factor = 2.0 },
        // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
        .context = vulkan,
        // Your VkSurfaceKHR.
        .surface = window_surface,
    }, .{
        .driver = .core_worker,
        .frame_wake = .{ .callback = onFrameWake },
    }, null);
    defer attached.ready.deinit();
    session = attached.session;
    try attached.ready.wait(null);
    // #endregion attach

    try requestFrame();
    try onWindowResized(1024, 768);

    // #region service
    const graphics_thread_options = maplibre.RenderSessionAttachOptions{
        .driver = .caller_graphics_thread,
        .frame_wake = .{ .callback = onFrameWake },
        .driver_work_wake = .{ .callback = onDriverWorkWake },
    };
    // #endregion service
    _ = graphics_thread_options;

    // #region close
    paused = true;

    var closed = try session.release(null);
    defer closed.deinit();
    try closed.wait(null);

    destroySurface(); // For example, with vkDestroySurfaceKHR.
    // #endregion close
}
