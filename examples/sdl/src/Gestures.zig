//! Gestures, as camera deltas. A drag pans, a right or Ctrl drag rotates and
//! pitches, and two fingers or a touchpad pinch pan, zoom, and rotate. The
//! wheel zooms about the pointer, and a double click zooms in, or out with
//! Shift. The arrow keys pan, and plus and minus zoom. A drag or a pinch keeps
//! moving after release, until a press stops it.
//!
//! SDL turns the first touch into mouse events, so one finger drags as the
//! mouse does, and a second finger turns the drag into a pinch.

const Gestures = @This();

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");
const c = @import("sdl");
const MapView = @import("MapView.zig");

const degrees_per_pixel = 0.5;

view: *MapView,
drag: ?Drag = null,
pinch: ?Pinch = null,
touchpad_pinch: ?TouchpadPinch = null,
/// The touches on the window, by finger, in the map's logical pixels.
touches: [2]?Touch = .{ null, null },

const Drag = struct {
    rotating: bool,
    x_rate: Rate,
    y_rate: Rate,
};

/// Two touches, whose span scales and turns about their midpoint, which pans as
/// it moves.
const Pinch = struct {
    center: maplibre.ScreenPoint,
    span: f64,
    angle: f64,
    x_rate: Rate,
    y_rate: Rate,
    zoom_rate: Rate,
    bearing_rate: Rate,
};

const TouchpadPinch = struct {
    anchor: maplibre.ScreenPoint,
    zoom_rate: Rate,
};

const Touch = struct {
    finger: c.SDL_FingerID,
    point: maplibre.ScreenPoint,
};

pub fn handle(self: *Gestures, event: *const c.SDL_Event) void {
    switch (event.type) {
        c.SDL_EVENT_MOUSE_BUTTON_DOWN => self.press(&event.button),
        c.SDL_EVENT_MOUSE_MOTION => self.move(&event.motion),
        c.SDL_EVENT_MOUSE_BUTTON_UP => self.release(&event.button),
        c.SDL_EVENT_MOUSE_WHEEL => self.scroll(&event.wheel),
        c.SDL_EVENT_FINGER_DOWN, c.SDL_EVENT_FINGER_MOTION => self.touch(&event.tfinger),
        c.SDL_EVENT_FINGER_UP, c.SDL_EVENT_FINGER_CANCELED => self.lift(&event.tfinger),
        c.SDL_EVENT_PINCH_BEGIN, c.SDL_EVENT_PINCH_UPDATE, c.SDL_EVENT_PINCH_END => self.pinchTouchpad(&event.pinch),
        c.SDL_EVENT_KEY_DOWN => self.key(&event.key),
        // Another window took the input during a gesture.
        c.SDL_EVENT_WINDOW_FOCUS_LOST, c.SDL_EVENT_WINDOW_MOUSE_LEAVE => if (self.drag != null and !mouseHeld()) self.cancel(),
        else => {},
    }
}

fn press(self: *Gestures, button: *const c.SDL_MouseButtonEvent) void {
    if (button.button != c.SDL_BUTTON_LEFT and button.button != c.SDL_BUTTON_RIGHT) return;
    if (self.pinch != null) return;
    if (button.clicks == 2 and button.button == c.SDL_BUTTON_LEFT) {
        const zoom_out = c.SDL_GetModState() & c.SDL_KMOD_SHIFT != 0;
        self.apply(.{
            .scale = if (zoom_out) 0.5 else 2,
            .anchor = self.point(button.x, button.y),
            .animation = animation(),
        });
        return;
    }
    self.drag = .{
        .rotating = button.button == c.SDL_BUTTON_RIGHT or c.SDL_GetModState() & c.SDL_KMOD_CTRL != 0,
        .x_rate = .init(button.timestamp),
        .y_rate = .init(button.timestamp),
    };
    self.apply(.{ .gesture_phase = .begin });
}

fn move(self: *Gestures, motion: *const c.SDL_MouseMotionEvent) void {
    const drag = &(self.drag orelse return);
    const dx = motion.xrel * self.view.logicalPerPoint();
    const dy = motion.yrel * self.view.logicalPerPoint();
    drag.x_rate.add(dx, motion.timestamp);
    drag.y_rate.add(dy, motion.timestamp);
    self.apply(if (drag.rotating) .{
        .bearing = dx * degrees_per_pixel,
        .pitch = -dy * degrees_per_pixel,
        .gesture_phase = .update,
    } else .{
        .offset = .{ .x = dx, .y = dy },
        .gesture_phase = .update,
    });
}

fn release(self: *Gestures, button: *const c.SDL_MouseButtonEvent) void {
    const drag = self.drag orelse return;
    self.drag = null;
    const vx = drag.x_rate.at(button.timestamp);
    const vy = drag.y_rate.at(button.timestamp);
    self.apply(.{
        .momentum = if (drag.rotating) .{
            .bearing = .{ .velocity = vx * degrees_per_pixel },
            .pitch = .{ .velocity = -vy * degrees_per_pixel },
        } else .{
            .pan = .{ .velocity = .{ .x = vx, .y = vy } },
        },
        .gesture_phase = .end,
    });
}

fn cancel(self: *Gestures) void {
    self.drag = null;
    self.pinch = null;
    self.apply(.{ .gesture_phase = .cancel });
}

fn scroll(self: *Gestures, wheel: *const c.SDL_MouseWheelEvent) void {
    const notches = if (wheel.direction == c.SDL_MOUSEWHEEL_FLIPPED) -wheel.y else wheel.y;
    // A wheel zooms about the pointer, by steps of 40 pixels.
    self.apply(.{
        .scale = std.math.pow(f64, 2, notches * 40 / 300),
        .anchor = self.point(wheel.mouse_x, wheel.mouse_y),
    });
}

fn touch(self: *Gestures, finger: *const c.SDL_TouchFingerEvent) void {
    const position = self.touchPoint(finger);
    for (&self.touches) |*slot| {
        if (slot.*) |*existing| if (existing.finger == finger.fingerID) {
            existing.point = position;
            break;
        };
    } else for (&self.touches) |*slot| {
        if (slot.* == null) {
            slot.* = .{ .finger = finger.fingerID, .point = position };
            break;
        }
    } else return;
    const first = self.touches[0] orelse return;
    const second = self.touches[1] orelse return;
    const center: maplibre.ScreenPoint = .{
        .x = (first.point.x + second.point.x) / 2,
        .y = (first.point.y + second.point.y) / 2,
    };
    const span = @max(1, std.math.hypot(second.point.x - first.point.x, second.point.y - first.point.y));
    const angle = std.math.radiansToDegrees(std.math.atan2(second.point.y - first.point.y, second.point.x - first.point.x));
    const pinch = &(self.pinch orelse {
        // The second touch turns a drag into a pinch.
        if (self.drag != null) self.cancel();
        self.pinch = .{
            .center = center,
            .span = span,
            .angle = angle,
            .x_rate = .init(finger.timestamp),
            .y_rate = .init(finger.timestamp),
            .zoom_rate = .init(finger.timestamp),
            .bearing_rate = .init(finger.timestamp),
        };
        self.apply(.{ .gesture_phase = .begin });
        return;
    });
    const dx = center.x - pinch.center.x;
    const dy = center.y - pinch.center.y;
    const scale = span / pinch.span;
    // Screen angles turn clockwise, which turns the bearing the other way.
    const bearing = -wrapDegrees(angle - pinch.angle);
    pinch.x_rate.add(dx, finger.timestamp);
    pinch.y_rate.add(dy, finger.timestamp);
    pinch.zoom_rate.add(std.math.log2(scale), finger.timestamp);
    pinch.bearing_rate.add(bearing, finger.timestamp);
    pinch.center = center;
    pinch.span = span;
    pinch.angle = angle;
    self.apply(.{
        .offset = .{ .x = dx, .y = dy },
        .scale = scale,
        .bearing = bearing,
        .anchor = center,
        .gesture_phase = .update,
    });
}

fn lift(self: *Gestures, finger: *const c.SDL_TouchFingerEvent) void {
    for (&self.touches) |*slot| {
        if (slot.*) |existing| if (existing.finger == finger.fingerID) {
            slot.* = null;
        };
    }
    const pinch = self.pinch orelse return;
    self.pinch = null;
    if (finger.type == c.SDL_EVENT_FINGER_CANCELED) {
        self.apply(.{ .gesture_phase = .cancel });
        return;
    }
    const time = finger.timestamp;
    self.apply(.{
        .momentum = .{
            .pan = .{ .velocity = .{ .x = pinch.x_rate.at(time), .y = pinch.y_rate.at(time) } },
            .zoom = .{ .velocity = pinch.zoom_rate.at(time), .anchor = pinch.center },
            .bearing = .{ .velocity = pinch.bearing_rate.at(time), .anchor = pinch.center },
        },
        .gesture_phase = .end,
    });
}

/// A touchpad pinch zooms about the pointer.
fn pinchTouchpad(self: *Gestures, event: *const c.SDL_PinchFingerEvent) void {
    switch (event.type) {
        c.SDL_EVENT_PINCH_BEGIN => {
            var x: f32 = 0;
            var y: f32 = 0;
            _ = c.SDL_GetMouseState(&x, &y);
            self.touchpad_pinch = .{ .anchor = self.point(x, y), .zoom_rate = .init(event.timestamp) };
            self.apply(.{ .gesture_phase = .begin });
        },
        c.SDL_EVENT_PINCH_UPDATE => {
            const pinch = &(self.touchpad_pinch orelse return);
            pinch.zoom_rate.add(std.math.log2(event.scale), event.timestamp);
            self.apply(.{ .scale = event.scale, .anchor = pinch.anchor, .gesture_phase = .update });
        },
        else => {
            const pinch = self.touchpad_pinch orelse return;
            self.touchpad_pinch = null;
            self.apply(.{
                .momentum = .{ .zoom = .{ .velocity = pinch.zoom_rate.at(event.timestamp), .anchor = pinch.anchor } },
                .gesture_phase = .end,
            });
        },
    }
}

fn key(self: *Gestures, event: *const c.SDL_KeyboardEvent) void {
    const step = 100;
    var delta: maplibre.CameraDelta = switch (event.key) {
        c.SDLK_LEFT => .{ .offset = .{ .x = step } },
        c.SDLK_RIGHT => .{ .offset = .{ .x = -step } },
        c.SDLK_UP => .{ .offset = .{ .y = step } },
        c.SDLK_DOWN => .{ .offset = .{ .y = -step } },
        c.SDLK_PLUS, c.SDLK_EQUALS, c.SDLK_KP_PLUS => .{ .scale = 2 },
        c.SDLK_MINUS, c.SDLK_KP_MINUS => .{ .scale = 0.5 },
        else => return,
    };
    delta.animation = animation();
    self.apply(delta);
}

fn apply(self: *Gestures, delta: maplibre.CameraDelta) void {
    self.view.applyCameraDelta(delta);
}

fn animation() maplibre.AnimationOptions {
    return .{ .duration_ms = 300 };
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

fn mouseHeld() bool {
    return c.SDL_GetMouseState(null, null) != 0;
}

/// An angle in degrees, wrapped to [-180, 180).
fn wrapDegrees(degrees: f64) f64 {
    return @mod(degrees + 180, 360) - 180;
}

/// How fast a gesture changes, per second. Each change's rate averages with
/// the earlier ones, and a pause before the release stops it.
const Rate = struct {
    per_second: f64 = 0,
    /// SDL's timestamp of the last change, in nanoseconds.
    time: u64,

    fn init(time: u64) Rate {
        return .{ .time = time };
    }

    fn add(self: *Rate, change: f64, time: u64) void {
        if (time <= self.time) return;
        const seconds = @as(f64, @floatFromInt(time - self.time)) / std.time.ns_per_s;
        self.per_second = (self.per_second + change / seconds) / 2;
        self.time = time;
    }

    fn at(self: Rate, time: u64) f64 {
        return if (time -| self.time < 50 * std.time.ns_per_ms) self.per_second else 0;
    }
};
