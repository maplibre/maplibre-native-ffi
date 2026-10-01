//! Carries native wakes to the SDL event loop. Every native callback only
//! pushes an SDL event; the render loop does the work when it dispatches it.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

const c = @import("c.zig").c;
const types = @import("types.zig");

/// The work an app event asks the render loop to do.
pub const Code = enum(i32) {
    /// The runtime event queue has events to drain.
    runtime_events,
    /// The render session has frame results to drain.
    frame_results,
    /// The render session has caller-driver work to service.
    driver_work,
    /// A paced retry after a frame that could not reach the screen.
    retry_frame,
    /// A smoke run waited too long for its first frame.
    smoke_timeout,
};

var event_type: u32 = 0;

/// Registers the SDL event type app events use. Call once after SDL_Init.
pub fn init() !void {
    event_type = c.SDL_RegisterEvents(1);
    if (event_type == 0) return types.AppError.EventDrainFailed;
}

/// Returns the code of an app event, or null for any other SDL event.
pub fn codeOf(event: *const c.SDL_Event) ?Code {
    if (event.type != event_type) return null;
    return @enumFromInt(event.user.code);
}

/// A wake that pushes an app event with `code` from any native thread.
pub fn wake(code: Code) maplibre.Wake {
    return .{ .context = contextFor(code), .callback = wakeRenderLoop };
}

/// Pushes an app event with `code` after `delay_ms`.
pub fn pushAfter(code: Code, delay_ms: u32) void {
    if (c.SDL_AddTimer(delay_ms, pushTimed, contextFor(code)) == 0) {
        std.debug.print("SDL_AddTimer failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
    }
}

fn contextFor(code: Code) ?*anyopaque {
    // The code travels in the context pointer, offset so it is never null.
    return @ptrFromInt(@as(usize, @intCast(@intFromEnum(code))) + 1);
}

fn codeFor(context: ?*anyopaque) Code {
    return @enumFromInt(@as(i32, @intCast(@intFromPtr(context) - 1)));
}

fn push(code: Code) void {
    var event = std.mem.zeroes(c.SDL_Event);
    event.user.type = event_type;
    event.user.code = @intFromEnum(code);
    // A native queue wakes again only after its next drain, so a lost push
    // stalls the loop. SDL's queue holds far more events than these wakes post.
    if (!c.SDL_PushEvent(&event)) {
        std.debug.print("SDL_PushEvent failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
    }
}

fn wakeRenderLoop(context: ?*anyopaque) maplibre.Error!void {
    push(codeFor(context));
}

fn pushTimed(context: ?*anyopaque, _: c.SDL_TimerID, _: u32) callconv(.c) u32 {
    push(codeFor(context));
    return 0;
}
