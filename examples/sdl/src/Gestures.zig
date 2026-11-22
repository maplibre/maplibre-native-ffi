//! Input for the library's gesture recognizer, which moves the camera. A drag
//! pans, a right or Ctrl drag rotates and pitches, two fingers on a touch
//! screen pan, zoom, rotate, and pitch, and a touchpad pinch zooms. The wheel
//! zooms about the pointer, a double click zooms in, or out with Shift, the
//! arrow keys pan, and plus and minus zoom. The recognizer keeps a release
//! gliding, and reports taps and notches, which the example doesn't use.
//!
//! Every method runs on the main thread.

const Gestures = @This();

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");
const c = @import("sdl");
const MapView = @import("MapView.zig");

const allocator = std.heap.smp_allocator;
/// The mouse's pointer. A touch's is its finger's, from 1 on.
const mouse = 0;

view: *MapView,
recognizer: maplibre.GestureRecognizer,
/// The button that holds the mouse down.
button: ?u8 = null,
/// The touches that are down.
touches: [10]?u32 = @splat(null),
modes: maplibre.GestureMode = .{},
/// When the recognizer next needs a tick, on SDL's clock in nanoseconds, and
/// the timer that wakes the event loop for it.
tick_at: ?u64 = null,
timer: c.SDL_TimerID = 0,

/// Creates the recognizer for the view's map. The view's wake wakes the event
/// loop for each tick, which then calls `update`.
pub fn init(view: *MapView) !Gestures {
    var diagnostic: maplibre.Diagnostic = .{};
    errdefer std.log.err("couldn't create the gesture recognizer: {s}", .{diagnostic.message()});
    return .{ .view = view, .recognizer = try view.map.gestureRecognizerCreate(allocator, null, &diagnostic) };
}

pub fn deinit(self: *Gestures) void {
    if (self.timer != 0) _ = c.SDL_RemoveTimer(self.timer);
    self.recognizer.release();
}

/// Ticks the recognizer once it asked for a tick.
pub fn update(self: *Gestures) void {
    const at = self.tick_at orelse return;
    const now = c.SDL_GetTicksNS();
    if (now < at) return;
    var diagnostic: maplibre.Diagnostic = .{};
    var result = self.recognizer.tick(allocator, milliseconds(now), &diagnostic) catch
        return self.view.fail(diagnostic.message());
    defer result.deinit();
    self.schedule(result.value);
}

pub fn handle(self: *Gestures, event: *const c.SDL_Event) void {
    switch (event.type) {
        c.SDL_EVENT_MOUSE_BUTTON_DOWN, c.SDL_EVENT_MOUSE_BUTTON_UP => self.click(&event.button),
        c.SDL_EVENT_MOUSE_MOTION => if (event.motion.which != c.SDL_TOUCH_MOUSEID and self.button != null)
            self.input(event.motion.timestamp, .{ .pointer_move = .{
                .id = mouse,
                .position = self.point(event.motion.x, event.motion.y),
            } }),
        c.SDL_EVENT_MOUSE_WHEEL => self.scroll(&event.wheel),
        c.SDL_EVENT_FINGER_DOWN, c.SDL_EVENT_FINGER_MOTION, c.SDL_EVENT_FINGER_UP, c.SDL_EVENT_FINGER_CANCELED => self.touch(&event.tfinger),
        c.SDL_EVENT_PINCH_BEGIN, c.SDL_EVENT_PINCH_UPDATE, c.SDL_EVENT_PINCH_END => self.pinch(&event.pinch),
        c.SDL_EVENT_KEY_DOWN, c.SDL_EVENT_KEY_UP => self.key(&event.key),
        // The window can't see the end of a gesture or a held key from here.
        c.SDL_EVENT_WINDOW_FOCUS_LOST => self.cancel(event.window.timestamp),
        else => {},
    }
}

/// The left button drags, and the right one rotates and pitches. The others
/// mean nothing to the map. SDL turns touches into clicks too, which `touch`
/// handles instead.
fn click(self: *Gestures, button: *const c.SDL_MouseButtonEvent) void {
    if (button.which == c.SDL_TOUCH_MOUSEID) return;
    if (button.button != c.SDL_BUTTON_LEFT and button.button != c.SDL_BUTTON_RIGHT) return;
    const position = self.point(button.x, button.y);
    if (button.down and self.button == null) {
        self.button = button.button;
        self.input(button.timestamp, .{ .pointer_down = .{
            .id = mouse,
            .kind = .mouse,
            .position = position,
            .modes = .{ .rotate = button.button == c.SDL_BUTTON_RIGHT },
        } });
    } else if (!button.down and self.button == button.button) {
        self.button = null;
        self.input(button.timestamp, .{ .pointer_up = .{ .id = mouse, .position = position } });
    }
}

/// A wheel's detent zooms a third of a level about the pointer. SDL can't
/// tell a touchpad's scrolling from a wheel's, so both zoom.
fn scroll(self: *Gestures, wheel: *const c.SDL_MouseWheelEvent) void {
    const detents = if (wheel.direction == c.SDL_MOUSEWHEEL_FLIPPED) -wheel.y else wheel.y;
    self.input(wheel.timestamp, .{ .delta = .{
        .position = self.point(wheel.mouse_x, wheel.mouse_y),
        .discrete = true,
        .zoom = detents / 3,
    } });
}

/// A touch screen's touches. A touchpad's are on the touchpad rather than the
/// window, and its pinches come from `pinch`.
fn touch(self: *Gestures, finger: *const c.SDL_TouchFingerEvent) void {
    if (c.SDL_GetTouchDeviceType(finger.touchID) != c.SDL_TOUCH_DEVICE_DIRECT) return;
    const id: u32 = @intCast(finger.fingerID % std.math.maxInt(u32) + 1);
    const slot = for (&self.touches) |*held| {
        if (held.* == id) break held;
    } else null;
    const position = self.touchPoint(finger);
    switch (finger.type) {
        c.SDL_EVENT_FINGER_DOWN => {
            const free = for (&self.touches) |*free| {
                if (free.* == null) break free;
            } else return;
            free.* = id;
            self.input(finger.timestamp, .{ .pointer_down = .{ .id = id, .kind = .touch, .position = position } });
        },
        c.SDL_EVENT_FINGER_MOTION => if (slot != null)
            self.input(finger.timestamp, .{ .pointer_move = .{ .id = id, .position = position } }),
        c.SDL_EVENT_FINGER_UP => if (slot) |down| {
            down.* = null;
            self.input(finger.timestamp, .{ .pointer_up = .{ .id = id, .position = position } });
        },
        else => if (slot) |down| {
            down.* = null;
            self.input(finger.timestamp, .{ .pointer_cancel = .{ .id = id } });
        },
    }
}

/// A touchpad's pinch zooms about the pointer.
fn pinch(self: *Gestures, event: *const c.SDL_PinchFingerEvent) void {
    var x: f32 = 0;
    var y: f32 = 0;
    _ = c.SDL_GetMouseState(&x, &y);
    self.input(event.timestamp, .{ .pinch = .{
        .center = self.point(x, y),
        .scale = if (event.type == c.SDL_EVENT_PINCH_UPDATE) event.scale else 1,
        .phase = switch (event.type) {
            c.SDL_EVENT_PINCH_BEGIN => .begin,
            c.SDL_EVENT_PINCH_UPDATE => .update,
            else => .end,
        },
    } });
}

/// Held Ctrl rotates and pitches on a drag, and held Shift zooms out on a
/// double click. The arrow keys pan, and plus and minus zoom.
fn key(self: *Gestures, event: *const c.SDL_KeyboardEvent) void {
    const modes: maplibre.GestureMode = .{
        .rotate = event.mod & c.SDL_KMOD_CTRL != 0,
        .zoom_out = event.mod & c.SDL_KMOD_SHIFT != 0,
    };
    if (!std.meta.eql(modes, self.modes)) {
        self.modes = modes;
        self.input(event.timestamp, .{ .modes = modes });
    }
    if (!event.down) return;
    const step = 100;
    var delta: maplibre.GestureDelta = .{
        .position = .{ .x = @as(f64, @floatFromInt(self.view.extent.width)) / 2, .y = @as(f64, @floatFromInt(self.view.extent.height)) / 2 },
        .discrete = true,
    };
    switch (event.key) {
        c.SDLK_LEFT => delta.pan.x = step,
        c.SDLK_RIGHT => delta.pan.x = -step,
        c.SDLK_UP => delta.pan.y = step,
        c.SDLK_DOWN => delta.pan.y = -step,
        c.SDLK_PLUS, c.SDLK_EQUALS, c.SDLK_KP_PLUS => delta.zoom = 1,
        c.SDLK_MINUS, c.SDLK_KP_MINUS => delta.zoom = -1,
        else => return,
    }
    self.input(event.timestamp, .{ .delta = delta });
}

/// Cancels the pointers that are down and lets go of the held keys.
fn cancel(self: *Gestures, timestamp: u64) void {
    if (self.button != null) {
        self.button = null;
        self.input(timestamp, .{ .pointer_cancel = .{ .id = mouse } });
    }
    for (&self.touches) |*slot| if (slot.*) |id| {
        slot.* = null;
        self.input(timestamp, .{ .pointer_cancel = .{ .id = id } });
    };
    self.modes = .{};
    self.input(timestamp, .{ .modes = .{} });
}

fn input(self: *Gestures, timestamp: u64, gesture_input: maplibre.GestureInput) void {
    var diagnostic: maplibre.Diagnostic = .{};
    var result = self.recognizer.handle(allocator, gesture_input, milliseconds(timestamp), &diagnostic) catch
        return self.view.fail(diagnostic.message());
    defer result.deinit();
    self.schedule(result.value);
}

/// Wakes the event loop when the recognizer next needs a tick: at the next
/// refresh of the window's display, or at the time it asks for.
fn schedule(self: *Gestures, result: maplibre.GestureResult) void {
    const now = c.SDL_GetTicksNS();
    self.tick_at = switch (result.next_tick) {
        .frame => now + refreshPeriod(self.view.window),
        .at => @intFromFloat(@max(0, result.next_tick_ms) * std.time.ns_per_ms),
        else => null,
    };
    if (self.timer != 0) _ = c.SDL_RemoveTimer(self.timer);
    self.timer = 0;
    const at = self.tick_at orelse return;
    self.timer = c.SDL_AddTimerNS(at -| now, onTimer, self);
}

/// Runs on SDL's timer thread, and reads only what never changes.
fn onTimer(data: ?*anyopaque, _: c.SDL_TimerID, _: u64) callconv(.c) u64 {
    const self: *Gestures = @ptrCast(@alignCast(data.?));
    self.view.wake();
    return 0;
}

/// A point in window coordinates, in the map's logical pixels.
fn point(self: *const Gestures, x: f32, y: f32) maplibre.ScreenPoint {
    const scale = self.view.logicalPerPoint();
    return .{ .x = x * scale, .y = y * scale };
}

/// A touch's point, which SDL reports as a fraction of the window's size.
fn touchPoint(self: *const Gestures, finger: *const c.SDL_TouchFingerEvent) maplibre.ScreenPoint {
    var width: c_int = 0;
    var height: c_int = 0;
    _ = c.SDL_GetWindowSize(self.view.window, &width, &height);
    return self.point(finger.x * @as(f32, @floatFromInt(width)), finger.y * @as(f32, @floatFromInt(height)));
}

/// SDL's timestamps and ticks, in nanoseconds, in the recognizer's
/// milliseconds.
fn milliseconds(nanoseconds: u64) f64 {
    return @as(f64, @floatFromInt(nanoseconds)) / std.time.ns_per_ms;
}

fn refreshPeriod(window: *c.SDL_Window) u64 {
    const mode = c.SDL_GetCurrentDisplayMode(c.SDL_GetDisplayForWindow(window));
    const refresh_rate: f32 = if (mode != null and mode.*.refresh_rate > 0) mode.*.refresh_rate else 60;
    return @intFromFloat(std.time.ns_per_s / refresh_rate);
}
