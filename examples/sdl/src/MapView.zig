//! A continuous map on an SDL window. The view attaches a render session to the
//! window's native surface, and the session renders on a library thread with
//! a graphics context that the library creates. Everything else matches the
//! browser example in examples/browser.
//!
//! Every method runs on the main thread. The library calls the wakes and the
//! event handler on its own threads, which only set flags and wake the event
//! loop, and `update` acts on them.

const MapView = @This();

const std = @import("std");
const builtin = @import("builtin");
const maplibre = @import("maplibre_native_ffi");
const c = @import("sdl");
const Pacer = @import("pacer.zig").Pacer;

const log = std.log.scoped(.map);
const allocator = std.heap.smp_allocator;
const style_url = "https://tiles.openfreemap.org/styles/liberty";

/// The library's pacer demands the frames on macOS, and the view's own
/// elsewhere.
const library_paced = builtin.os.tag == .macos;

pub const Status = union(enum) {
    loading,
    loaded,
    failed: [:0]const u8,
};

window: *c.SDL_Window,
wake: *const fn () void,
runtime: maplibre.Runtime,
map: maplibre.Map,
session: maplibre.RenderSession,
pacer: Pacer,
/// The scale factor of the window's display, and the window's size in logical
/// pixels, as the session last got them.
extent: maplibre.LogicalExtent,
/// SDL's view of the window on macOS, which holds the Metal layer.
metal_view: if (builtin.os.tag == .macos) c.SDL_MetalView else void,

// Set on the library's threads.
redraw_woken: std.atomic.Value(bool) = .init(false),
frames_woken: std.atomic.Value(bool) = .init(false),
style_loaded: std.atomic.Value(bool) = .init(false),
/// The first failure's message. Later ones don't replace it.
failure: std.atomic.Value(?[*:0]u8) = .init(null),

/// The session has something new to draw.
frame_wanted: bool = false,
/// The next frame draws even when nothing changed.
force: bool = false,
/// A demand that has no frame result yet.
frame_pending: bool = false,
/// The window can't take frames, as while it's minimized on Windows.
hidden: bool = false,

/// Creates the map and attaches its session to the window. `wake` wakes the
/// event loop, which then calls `update`. The view must stay where it is
/// until `deinit`, as the library's threads use it.
pub fn init(self: *MapView, window: *c.SDL_Window, wake: *const fn () void) !void {
    var diagnostic: maplibre.Diagnostic = .{};
    errdefer log.err("couldn't start the map: {s}", .{diagnostic.message()});

    self.* = .{
        .window = window,
        .wake = wake,
        .runtime = undefined,
        .map = undefined,
        .session = undefined,
        .pacer = undefined,
        .extent = try windowExtent(window),
        .metal_view = undefined,
    };

    self.runtime = try maplibre.Runtime.create(null, &diagnostic);
    errdefer self.runtime.dispose(null) catch {};
    const created = try self.runtime.createMap(.{
        .initial_extent = self.extent,
        .map_mode = .continuous,
        .event_mask = .{
            .map_style_loaded = true,
            .map_loading_failed = true,
            .map_render_error = true,
        },
    }, &diagnostic);
    self.map = created.map;
    errdefer self.map.dispose(null) catch {};
    try created.ready.await(&diagnostic);
    try self.runtime.discarding().setEventHandler(.init(self, onEvents, null), &diagnostic);
    try self.map.discarding().updateCamera(.{
        .camera = .{
            .center = .{ .latitude = 48.8584, .longitude = 2.2945 },
            .zoom = 15.5,
            .bearing = -30,
            .pitch = 55,
        },
    }, &diagnostic);
    try self.map.discarding().setStyleUrl(style_url, &diagnostic);

    try self.pacer.init(window, wake);
    errdefer self.pacer.deinit();
    const attached = try self.attach(&diagnostic);
    self.session = attached.session;
    errdefer self.session.dispose(null) catch {};
    try attached.ready.await(&diagnostic);
    if (library_paced) try self.session.setFramePacer(self.pacer.pacer, &diagnostic);
}

/// Releases the session, the map, and the runtime. The session lets go of the
/// window before this returns, so the window can go next.
pub fn deinit(self: *MapView) void {
    var diagnostic: maplibre.Diagnostic = .{};
    release(&self.session, &diagnostic) catch log.warn("couldn't release the session: {s}", .{diagnostic.message()});
    self.pacer.deinit();
    if (builtin.os.tag == .macos) c.SDL_Metal_DestroyView(self.metal_view);
    release(&self.map, &diagnostic) catch log.warn("couldn't release the map: {s}", .{diagnostic.message()});
    release(&self.runtime, &diagnostic) catch log.warn("couldn't release the runtime: {s}", .{diagnostic.message()});
    if (self.failure.load(.acquire)) |message| allocator.free(std.mem.span(message));
}

fn release(owner: anytype, diagnostic: *maplibre.Diagnostic) !void {
    try (try owner.release(diagnostic)).await(diagnostic);
}

/// Acts on what the library's threads reported since the last update.
pub fn update(self: *MapView) void {
    if (self.frames_woken.swap(false, .acq_rel)) self.drainFrameResults();
    if (self.redraw_woken.swap(false, .acq_rel)) self.frame_wanted = true;
    self.demandFrame();
}

pub fn status(self: *const MapView) Status {
    if (self.failure.load(.acquire)) |message| return .{ .failed = std.mem.span(message) };
    return if (self.style_loaded.load(.acquire)) .loaded else .loading;
}

/// Draws a frame even if nothing changed, such as when the window shows again
/// or the compositor waits for a frame of a new size.
pub fn redraw(self: *MapView) void {
    self.hidden = false;
    self.frame_wanted = true;
    self.force = true;
    self.demandFrame();
}

/// Gives the session the window's new size and scale factor, after the window
/// changes size or moves to a display with another scale factor.
pub fn resize(self: *MapView) void {
    const extent = windowExtent(self.window) catch return;
    if (std.meta.eql(extent, self.extent)) return;
    self.extent = extent;
    var diagnostic: maplibre.Diagnostic = .{};
    self.session.discarding().resize(extent, &diagnostic) catch self.fail(diagnostic.message());
}

/// Moves the camera, as a gesture asks.
pub fn applyCameraDelta(self: *MapView, delta: maplibre.CameraDelta) void {
    var diagnostic: maplibre.Diagnostic = .{};
    self.map.discarding().applyCameraDelta(delta, &diagnostic) catch self.fail(diagnostic.message());
}

/// How many of the map's logical pixels a point of SDL's window coordinates
/// spans. Window coordinates are pixels on some platforms, and scaled ones on
/// others.
pub fn logicalPerPoint(self: *const MapView) f64 {
    return c.SDL_GetWindowPixelDensity(self.window) / self.extent.scale_factor;
}

fn attach(self: *MapView, diagnostic: *maplibre.Diagnostic) !maplibre.NewRenderSession {
    const options: maplibre.RenderSessionAttachOptions = .{
        .driver = .core_worker,
        .redraw_wake = if (library_paced) .{} else .init(self, onRedrawWake, null),
        .frame_wake = .init(self, onFrameWake, null),
    };
    const properties = c.SDL_GetWindowProperties(self.window);
    const size = self.logicalSize();
    const target: maplibre.RenderTarget = switch (builtin.os.tag) {
        .linux, .freebsd => if (usesWayland()) .{ .wayland_surface = .{
            .logical_size = size,
            .display = c.SDL_GetPointerProperty(properties, c.SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, null),
            .surface = c.SDL_GetPointerProperty(properties, c.SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, null),
        } } else .{ .xlib_window = .{
            .logical_size = size,
            .display = c.SDL_GetPointerProperty(properties, c.SDL_PROP_WINDOW_X11_DISPLAY_POINTER, null),
            .window = @bitCast(c.SDL_GetNumberProperty(properties, c.SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0)),
        } },
        .windows => .{ .win32_window = .{
            .logical_size = size,
            .window = c.SDL_GetPointerProperty(properties, c.SDL_PROP_WINDOW_WIN32_HWND_POINTER, null),
        } },
        .macos => target: {
            self.metal_view = c.SDL_Metal_CreateView(self.window) orelse return error.SdlFailed;
            break :target .{ .metal_layer = .{
                .logical_size = size,
                .layer = c.SDL_Metal_GetLayer(self.metal_view),
            } };
        },
        else => @compileError("unsupported platform"),
    };
    errdefer if (builtin.os.tag == .macos) c.SDL_Metal_DestroyView(self.metal_view);
    // With no device, the session creates one on the platform's own backend.
    return self.map.attach(allocator, target, null, options, diagnostic);
}

/// Whether SDL drives the window through Wayland rather than X11.
pub fn usesWayland() bool {
    const driver = c.SDL_GetCurrentVideoDriver() orelse return false;
    return std.mem.eql(u8, std.mem.span(driver), "wayland");
}

/// Demands a frame when the session has one to draw and the display is ready
/// for it. One demand runs at a time.
fn demandFrame(self: *MapView) void {
    if (library_paced) return;
    if (!self.frame_wanted or self.frame_pending or self.hidden or !self.pacer.ready()) return;
    self.pacer.demanding();
    var diagnostic: maplibre.Diagnostic = .{};
    self.session.demandFrame(.{ .flags = .{ .force = self.force } }, &diagnostic) catch {
        self.pacer.skipped();
        self.fail(diagnostic.message());
        return;
    };
    self.frame_wanted = false;
    self.force = false;
    self.frame_pending = true;
}

fn drainFrameResults(self: *MapView) void {
    var diagnostic: maplibre.Diagnostic = .{};
    var results = self.session.drainFrameResults(allocator, &diagnostic) catch {
        self.fail(diagnostic.message());
        return;
    };
    defer results.deinit();
    for (results.value) |result| {
        self.frame_pending = false;
        switch (result.disposition) {
            .rendered => {},
            // Demand the frame again once the window shows.
            .target_not_ready => {
                if (!library_paced) self.pacer.skipped();
                self.hidden = true;
                self.frame_wanted = true;
            },
            .failed => {
                var failure = self.session.getFailure(allocator, null) catch {
                    self.fail("The map failed to render a frame.");
                    continue;
                };
                defer failure.deinit();
                self.fail(failure.value);
            },
            else => if (!library_paced) self.pacer.skipped(),
        }
    }
}

fn logicalSize(self: *const MapView) maplibre.LogicalSize {
    return .{ .width = self.extent.width, .height = self.extent.height };
}

/// The window's size in logical pixels, from its size in pixels and its
/// display's scale factor.
fn windowExtent(window: *c.SDL_Window) !maplibre.LogicalExtent {
    var width: c_int = 0;
    var height: c_int = 0;
    if (!c.SDL_GetWindowSizeInPixels(window, &width, &height)) return error.SdlFailed;
    const display_scale: f64 = c.SDL_GetWindowDisplayScale(window);
    const scale = if (display_scale > 0) display_scale else 1;
    return .{
        .width = @max(1, @as(u32, @intFromFloat(@round(@as(f64, @floatFromInt(width)) / scale)))),
        .height = @max(1, @as(u32, @intFromFloat(@round(@as(f64, @floatFromInt(height)) / scale)))),
        .scale_factor = scale,
    };
}

/// Shows an error. Once the view shows one, later ones don't replace it.
fn fail(self: *MapView, message: []const u8) void {
    log.err("the map failed: {s}", .{message});
    const copy = allocator.dupeSentinel(u8, message, 0) catch return;
    if (self.failure.cmpxchgStrong(null, copy.ptr, .acq_rel, .acquire) != null) allocator.free(copy);
    self.wake();
}

fn onRedrawWake(self: *MapView) void {
    self.redraw_woken.store(true, .release);
    self.wake();
}

fn onFrameWake(self: *MapView) void {
    self.frames_woken.store(true, .release);
    self.wake();
}

fn onEvents(self: *MapView, events: []const maplibre.RuntimeEvent) void {
    for (events) |event| {
        if (event.source != self.map.id()) continue;
        switch (event.type) {
            .map_style_loaded => {
                self.style_loaded.store(true, .release);
                self.wake();
            },
            .map_loading_failed, .map_render_error => self.fail(event.message),
            else => {},
        }
    }
}
