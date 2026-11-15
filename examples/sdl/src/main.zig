//! An SDL app that shows a continuous map with the Zig binding. SDL's main
//! callbacks run the app, and iterate only after events arrive, so an idle map
//! costs nothing.

const std = @import("std");
const builtin = @import("builtin");
const c = @import("sdl");
const Gestures = @import("Gestures.zig");
const MapView = @import("MapView.zig");

const log = std.log.scoped(.app);
const title = "MapLibre Native";

var window: ?*c.SDL_Window = null;
var view: MapView = undefined;
var view_started = false;
var gestures: Gestures = undefined;
/// The status that the window's title shows.
var shown_status: ?std.meta.Tag(MapView.Status) = null;

/// The event that wakes the loop, and whether one is on its way.
var wake_event: u32 = 0;
var woken: std.atomic.Value(bool) = .init(false);

pub fn main() u8 {
    var arguments = [_:null]?[*:0]u8{};
    return @intCast(c.SDL_EnterAppMainCallbacks(0, @ptrCast(&arguments), init, iterate, handle, quit));
}

fn init(_: [*c]?*anyopaque, _: c_int, _: [*c][*c]u8) callconv(.c) c.SDL_AppResult {
    start() catch |err| {
        var buffer: [256]u8 = undefined;
        const message = std.fmt.bufPrintSentinel(&buffer, "The map couldn't start: {t}. {s}", .{ err, c.SDL_GetError() }, 0) catch "The map couldn't start.";
        log.err("{s}", .{message});
        _ = c.SDL_ShowSimpleMessageBox(c.SDL_MESSAGEBOX_ERROR, title, message, window);
        return c.SDL_APP_FAILURE;
    };
    return c.SDL_APP_CONTINUE;
}

fn start() !void {
    _ = c.SDL_SetAppMetadata(title, null, "org.maplibre.nativeffi.example.sdl");
    // On Linux and FreeBSD, the view presents to a Wayland surface, and to an
    // X11 window where there is no Wayland compositor.
    if (builtin.os.tag == .linux or builtin.os.tag == .freebsd) _ = c.SDL_SetHint(c.SDL_HINT_VIDEO_DRIVER, "wayland,x11");
    _ = c.SDL_SetHint(c.SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");
    if (!c.SDL_Init(c.SDL_INIT_VIDEO)) return error.SdlFailed;
    wake_event = c.SDL_RegisterEvents(1);
    if (wake_event == 0) return error.SdlFailed;

    const properties = c.SDL_CreateProperties();
    defer c.SDL_DestroyProperties(properties);
    _ = c.SDL_SetStringProperty(properties, c.SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    _ = c.SDL_SetNumberProperty(properties, c.SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1024);
    _ = c.SDL_SetNumberProperty(properties, c.SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 768);
    // The window's pixels match the display's, and the library draws them all.
    var flags = c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (builtin.os.tag == .macos) {
        flags |= c.SDL_WINDOW_METAL;
    } else {
        _ = c.SDL_SetBooleanProperty(properties, c.SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
    }
    _ = c.SDL_SetNumberProperty(properties, c.SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, @intCast(flags));
    window = c.SDL_CreateWindowWithProperties(properties) orelse return error.SdlFailed;

    try view.init(window.?, wake);
    view_started = true;
    gestures = .{ .view = &view };
    showStatus();
}

fn iterate(_: ?*anyopaque) callconv(.c) c.SDL_AppResult {
    // A wake after this point sends another event.
    woken.store(false, .release);
    view.update();
    showStatus();
    return c.SDL_APP_CONTINUE;
}

fn handle(_: ?*anyopaque, event: [*c]c.SDL_Event) callconv(.c) c.SDL_AppResult {
    switch (event.*.type) {
        c.SDL_EVENT_QUIT => return c.SDL_APP_SUCCESS,
        c.SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, c.SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED => view.resize(),
        // The window shows again, or, on Wayland, the compositor waits for a
        // frame of the window's new size before it resizes it again.
        c.SDL_EVENT_WINDOW_EXPOSED, c.SDL_EVENT_WINDOW_RESTORED => view.redraw(),
        else => gestures.handle(event),
    }
    return c.SDL_APP_CONTINUE;
}

fn quit(_: ?*anyopaque, _: c.SDL_AppResult) callconv(.c) void {
    // The session lets go of the window before it goes.
    if (view_started) view.deinit();
    if (window) |w| c.SDL_DestroyWindow(w);
}

/// Wakes the event loop, from any thread.
fn wake() void {
    if (woken.swap(true, .acq_rel)) return;
    var event = std.mem.zeroes(c.SDL_Event);
    event.type = wake_event;
    _ = c.SDL_PushEvent(&event);
}

fn showStatus() void {
    const status = view.status();
    if (shown_status == std.meta.activeTag(status)) return;
    shown_status = std.meta.activeTag(status);
    var buffer: [512]u8 = undefined;
    const text = switch (status) {
        .loading => title ++ " — Loading the style…",
        .loaded => title,
        .failed => |message| std.fmt.bufPrintSentinel(&buffer, title ++ " — {s}", .{message}, 0) catch title,
    };
    _ = c.SDL_SetWindowTitle(window, text);
}
