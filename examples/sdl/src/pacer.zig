//! Paces the map's frames to the display, so that the window gets at most one
//! frame per refresh. The library presents each frame without waiting for the
//! display, and SDL has no frame clock for a window that it doesn't draw.
//!
//! On Wayland, a frame callback on the window's surface tells when the
//! compositor wants the next frame. On X11, a thread waits out the display's
//! refresh period. On Windows, `DwmFlush` waits for the desktop compositor's
//! next frame. On macOS, the library's own frame pacer follows the display,
//! and demands the frames itself.

const std = @import("std");
const builtin = @import("builtin");
const maplibre = @import("maplibre_native_ffi");
const c = @import("sdl");
const MapView = @import("MapView.zig");

/// The pacer of this platform. Each one, other than the library's, is ready
/// for a demand at most once per refresh: `demanding` makes it wait for the
/// next one, and it calls `wake` once it's ready again.
pub const Pacer = switch (builtin.os.tag) {
    .linux, .freebsd => UnixPacer,
    .windows => ThreadPacer,
    .macos => LibraryPacer,
    else => @compileError("unsupported platform"),
};

/// A function that wakes the event loop. Any thread can call it.
pub const Wake = *const fn () void;

/// A pacer for the window system that SDL drives the window through.
const UnixPacer = union(enum) {
    wayland: WaylandPacer,
    x11: ThreadPacer,

    pub fn init(self: *UnixPacer, window: *c.SDL_Window, wake: Wake) !void {
        if (MapView.usesWayland()) {
            self.* = .{ .wayland = undefined };
            try self.wayland.init(window, wake);
        } else {
            self.* = .{ .x11 = undefined };
            try self.x11.init(window, wake);
        }
    }

    pub fn deinit(self: *UnixPacer) void {
        switch (self.*) {
            inline else => |*pacer| pacer.deinit(),
        }
    }

    pub fn ready(self: *const UnixPacer) bool {
        return switch (self.*) {
            inline else => |*pacer| pacer.ready(),
        };
    }

    pub fn demanding(self: *UnixPacer) void {
        switch (self.*) {
            inline else => |*pacer| pacer.demanding(),
        }
    }

    pub fn skipped(self: *UnixPacer) void {
        switch (self.*) {
            inline else => |*pacer| pacer.skipped(),
        }
    }
};

/// A pacer that follows the compositor's frame callbacks.
///
/// SDL loads libwayland-client at run time and dispatches the window's events
/// as it pumps its own, so the frame callback comes on the main thread. This
/// loads the same library, so the example builds without Wayland's development
/// files.
const WaylandPacer = struct {
    library: *c.SDL_SharedObject,
    marshal: *const fn (proxy: *anyopaque, opcode: u32, interface: *const anyopaque, version: u32, flags: u32, arguments: [*]Argument) callconv(.c) ?*anyopaque,
    add_listener: *const fn (proxy: *anyopaque, implementation: *const anyopaque, data: ?*anyopaque) callconv(.c) c_int,
    destroy: *const fn (proxy: *anyopaque) callconv(.c) void,
    get_version: *const fn (proxy: *anyopaque) callconv(.c) u32,
    callback_interface: *const anyopaque,
    surface: *anyopaque,
    wake: Wake,
    /// The frame callback that a demanded frame commits, until it's done.
    callback: ?*anyopaque = null,

    /// A request's argument, as `union wl_argument`.
    const Argument = extern union {
        i: i32,
        u: u32,
        o: ?*anyopaque,
    };

    /// The listener of a `wl_callback`.
    const Listener = extern struct {
        done: *const fn (data: ?*anyopaque, callback: ?*anyopaque, time: u32) callconv(.c) void,
    };

    /// The opcode of `wl_surface.frame`.
    const surface_frame = 3;

    const listener: Listener = .{ .done = done };

    pub fn init(self: *WaylandPacer, window: *c.SDL_Window, wake: Wake) !void {
        const surface = c.SDL_GetPointerProperty(c.SDL_GetWindowProperties(window), c.SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, null) orelse
            return error.NoWaylandSurface;
        const library = c.SDL_LoadObject("libwayland-client.so.0") orelse return error.NoWaylandLibrary;
        errdefer c.SDL_UnloadObject(library);
        self.* = .{
            .library = library,
            .marshal = try loadFunction(library, @FieldType(WaylandPacer, "marshal"), "wl_proxy_marshal_array_flags"),
            .add_listener = try loadFunction(library, @FieldType(WaylandPacer, "add_listener"), "wl_proxy_add_listener"),
            .destroy = try loadFunction(library, @FieldType(WaylandPacer, "destroy"), "wl_proxy_destroy"),
            .get_version = try loadFunction(library, @FieldType(WaylandPacer, "get_version"), "wl_proxy_get_version"),
            .callback_interface = try load(library, "wl_callback_interface"),
            .surface = surface,
            .wake = wake,
        };
    }

    pub fn deinit(self: *WaylandPacer) void {
        self.cancel();
        c.SDL_UnloadObject(self.library);
    }

    pub fn ready(self: *const WaylandPacer) bool {
        return self.callback == null;
    }

    /// Asks for a frame callback, which the demanded frame commits with its
    /// buffer. The compositor calls it when it's time for the next frame.
    pub fn demanding(self: *WaylandPacer) void {
        // libwayland writes the new callback into its argument.
        var arguments = [_]Argument{.{ .o = null }};
        const callback = self.marshal(self.surface, surface_frame, self.callback_interface, self.get_version(self.surface), 0, &arguments) orelse
            return;
        _ = self.add_listener(callback, &listener, self);
        self.callback = callback;
    }

    /// The demanded frame didn't present, so it committed no frame callback.
    pub fn skipped(self: *WaylandPacer) void {
        self.cancel();
    }

    fn cancel(self: *WaylandPacer) void {
        if (self.callback) |callback| self.destroy(callback);
        self.callback = null;
    }

    fn done(data: ?*anyopaque, callback: ?*anyopaque, _: u32) callconv(.c) void {
        const self: *WaylandPacer = @ptrCast(@alignCast(data.?));
        self.destroy(callback.?);
        self.callback = null;
        self.wake();
    }

    fn load(library: *c.SDL_SharedObject, comptime name: [:0]const u8) !*const anyopaque {
        return @ptrCast(c.SDL_LoadFunction(library, name) orelse return error.NoWaylandLibrary);
    }

    fn loadFunction(library: *c.SDL_SharedObject, comptime T: type, comptime name: [:0]const u8) !T {
        return @ptrCast(try load(library, name));
    }
};

/// A pacer that waits for the next refresh on a thread of its own: on Windows,
/// for the desktop compositor's next frame, and elsewhere for the display's
/// refresh period.
const ThreadPacer = struct {
    const dwm = struct {
        extern "dwmapi" fn DwmFlush() callconv(.winapi) i32;
    };

    thread: *c.SDL_Thread,
    demanded: *c.SDL_Semaphore,
    waiting: std.atomic.Value(bool) = .init(false),
    stopping: std.atomic.Value(bool) = .init(false),
    wake: Wake,
    /// The display's refresh period, the least time between two frames.
    period_ns: u64,

    pub fn init(self: *ThreadPacer, window: *c.SDL_Window, wake: Wake) !void {
        const mode = c.SDL_GetCurrentDisplayMode(c.SDL_GetDisplayForWindow(window));
        const refresh_rate: f32 = if (mode != null and mode.*.refresh_rate > 0) mode.*.refresh_rate else 60;
        self.* = .{
            .thread = undefined,
            .demanded = c.SDL_CreateSemaphore(0) orelse return error.SdlFailed,
            .wake = wake,
            .period_ns = @intFromFloat(std.time.ns_per_s / refresh_rate),
        };
        errdefer c.SDL_DestroySemaphore(self.demanded);
        self.thread = c.SDL_CreateThread(run, "frame pacer", self) orelse return error.SdlFailed;
    }

    pub fn deinit(self: *ThreadPacer) void {
        self.stopping.store(true, .release);
        c.SDL_SignalSemaphore(self.demanded);
        c.SDL_WaitThread(self.thread, null);
        c.SDL_DestroySemaphore(self.demanded);
    }

    pub fn ready(self: *const ThreadPacer) bool {
        return !self.waiting.load(.acquire);
    }

    pub fn demanding(self: *ThreadPacer) void {
        self.waiting.store(true, .release);
        c.SDL_SignalSemaphore(self.demanded);
    }

    pub fn skipped(_: *ThreadPacer) void {}

    fn run(data: ?*anyopaque) callconv(.c) c_int {
        const self: *ThreadPacer = @ptrCast(@alignCast(data.?));
        var last = c.SDL_GetTicksNS();
        while (true) {
            c.SDL_WaitSemaphore(self.demanded);
            if (self.stopping.load(.acquire)) return 0;
            if (builtin.os.tag == .windows) _ = dwm.DwmFlush();
            // DwmFlush returns at once when nothing composes the desktop, as
            // under Wine, so then the pacer waits out the refresh period.
            const now = c.SDL_GetTicksNS();
            if (now < last + self.period_ns / 2) c.SDL_DelayPrecise(last + self.period_ns - now);
            last = c.SDL_GetTicksNS();
            self.waiting.store(false, .release);
            self.wake();
        }
    }
};

/// The library's pacer, which demands the session's frames itself.
const LibraryPacer = struct {
    pacer: maplibre.FramePacer,

    pub fn init(self: *LibraryPacer, _: *c.SDL_Window, _: Wake) !void {
        self.* = .{ .pacer = try maplibre.FramePacer.create(null, null) };
    }

    pub fn deinit(self: *LibraryPacer) void {
        self.pacer.release();
    }
};
