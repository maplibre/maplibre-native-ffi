const std = @import("std");
const maplibre = @import("maplibre_native");

const c = @import("c.zig").c;
const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

const DragMode = enum {
    none,
    pan,
    rotate,
    pitch,
};

const keyboard_animation_ms = 160.0;
const reset_animation_ms = 220.0;

pub const Result = struct {
    handled: bool = false,
    camera_changed: bool = false,
};

pub const Controller = struct {
    drag_mode: DragMode = .none,
    last_x: f64 = 0,
    last_y: f64 = 0,

    pub fn handleEvent(
        self: *Controller,
        event: *const c.SDL_Event,
        map: *maplibre.MapHandle,
        current_viewport: types.Viewport,
    ) !Result {
        return switch (event.type) {
            c.SDL_EVENT_MOUSE_BUTTON_DOWN => self.handleMouseButtonDown(event.button, map),
            c.SDL_EVENT_MOUSE_BUTTON_UP => self.handleMouseButtonUp(event.button),
            c.SDL_EVENT_MOUSE_MOTION => self.handleMouseMotion(event.motion, map),
            c.SDL_EVENT_MOUSE_WHEEL => handleMouseWheel(event.wheel, map),
            c.SDL_EVENT_KEY_DOWN => handleKeyDown(event.key, map, current_viewport),
            else => .{},
        };
    }

    fn handleMouseButtonDown(
        self: *Controller,
        button: c.SDL_MouseButtonEvent,
        map: *maplibre.MapHandle,
    ) !Result {
        self.last_x = button.x;
        self.last_y = button.y;

        const mode = dragModeForButton(button.button);
        if (mode == .none) return .{};

        try expectCameraStatus(map.cancelTransitions(), "cancel camera transitions failed");
        self.drag_mode = mode;
        return .{ .handled = true };
    }

    fn handleMouseButtonUp(self: *Controller, button: c.SDL_MouseButtonEvent) Result {
        if (button.button != c.SDL_BUTTON_LEFT and button.button != c.SDL_BUTTON_RIGHT) {
            return .{};
        }
        self.drag_mode = .none;
        self.last_x = button.x;
        self.last_y = button.y;
        return .{ .handled = true };
    }

    fn handleMouseMotion(
        self: *Controller,
        motion: c.SDL_MouseMotionEvent,
        map: *maplibre.MapHandle,
    ) !Result {
        const x: f64 = motion.x;
        const y: f64 = motion.y;
        defer {
            self.last_x = x;
            self.last_y = y;
        }

        switch (self.drag_mode) {
            .none => return .{},
            .pan => {
                const dx = x - self.last_x;
                const dy = y - self.last_y;
                if (dx == 0 and dy == 0) return .{ .handled = true };
                try expectCameraStatus(map.moveBy(dx, dy), "camera pan failed");
            },
            .rotate => {
                const dx = x - self.last_x;
                const dy = y - self.last_y;
                if (dx == 0 and dy == 0) return .{ .handled = true };
                try adjustBearing(map, dx * 0.5);
                try expectCameraStatus(map.pitchBy(dy / 2.0), "camera pitch failed");
            },
            .pitch => {
                const dy = y - self.last_y;
                if (dy == 0) return .{ .handled = true };
                try expectCameraStatus(map.pitchBy(dy / 2.0), "camera pitch failed");
            },
        }
        return .{ .handled = true, .camera_changed = true };
    }
};

pub fn logControls() void {
    std.debug.print(
        \\Controls:
        \\  left drag: pan
        \\  right drag or Ctrl+left drag: rotate with X, pitch with Y
        \\  scroll: zoom at cursor
        \\  arrows or WASD: pan
        \\  + / -: zoom at center
        \\  Q / E: rotate
        \\  PageUp / PageDown or [ / ]: pitch
        \\  0: reset pitch and bearing
        \\
    , .{});
}

fn handleMouseWheel(wheel: c.SDL_MouseWheelEvent, map: *maplibre.MapHandle) !Result {
    const delta: f64 = -wheel.y;
    if (delta == 0) return .{ .handled = true };

    const anchor = point(wheel.mouse_x, wheel.mouse_y);
    const scale = std.math.pow(f64, 2.0, delta * 0.25);
    try expectCameraStatus(map.scaleBy(scale, anchor), "camera zoom failed");
    return .{ .handled = true, .camera_changed = true };
}

fn handleKeyDown(
    key: c.SDL_KeyboardEvent,
    map: *maplibre.MapHandle,
    current_viewport: types.Viewport,
) !Result {
    const pan_step = 120.0;
    const zoom_step = 1.25;
    const bearing_step = 10.0;
    const pitch_step = 5.0;
    const animation = cameraAnimation(keyboard_animation_ms);
    const center = point(
        @as(f64, @floatFromInt(current_viewport.logical_width)) / 2.0,
        @as(f64, @floatFromInt(current_viewport.logical_height)) / 2.0,
    );

    switch (key.scancode) {
        scancode(c.SDL_SCANCODE_LEFT), scancode(c.SDL_SCANCODE_A) => {
            try expectCameraStatus(map.moveBy(-pan_step, 0), "keyboard pan failed");
        },
        scancode(c.SDL_SCANCODE_RIGHT), scancode(c.SDL_SCANCODE_D) => {
            try expectCameraStatus(map.moveBy(pan_step, 0), "keyboard pan failed");
        },
        scancode(c.SDL_SCANCODE_UP), scancode(c.SDL_SCANCODE_W) => {
            try expectCameraStatus(map.moveBy(0, -pan_step), "keyboard pan failed");
        },
        scancode(c.SDL_SCANCODE_DOWN), scancode(c.SDL_SCANCODE_S) => {
            try expectCameraStatus(map.moveBy(0, pan_step), "keyboard pan failed");
        },
        scancode(c.SDL_SCANCODE_EQUALS), scancode(c.SDL_SCANCODE_KP_PLUS) => {
            try expectCameraStatus(map.scaleBy(zoom_step, center), "keyboard zoom failed");
        },
        scancode(c.SDL_SCANCODE_MINUS), scancode(c.SDL_SCANCODE_KP_MINUS) => {
            try expectCameraStatus(map.scaleBy(1.0 / zoom_step, center), "keyboard zoom failed");
        },
        scancode(c.SDL_SCANCODE_Q) => try adjustBearingAnimated(map, -bearing_step, animation),
        scancode(c.SDL_SCANCODE_E) => try adjustBearingAnimated(map, bearing_step, animation),
        scancode(c.SDL_SCANCODE_PAGEUP), scancode(c.SDL_SCANCODE_RIGHTBRACKET) => {
            try adjustPitchAnimated(map, pitch_step, animation);
        },
        scancode(c.SDL_SCANCODE_PAGEDOWN), scancode(c.SDL_SCANCODE_LEFTBRACKET) => {
            try adjustPitchAnimated(map, -pitch_step, animation);
        },
        scancode(c.SDL_SCANCODE_0) => {
            try resetPitchAndBearingAnimated(map, cameraAnimation(reset_animation_ms));
        },
        else => return .{},
    }

    return .{ .handled = true, .camera_changed = true };
}

fn dragModeForButton(button: u8) DragMode {
    if (button == c.SDL_BUTTON_RIGHT) return .rotate;
    if (button != c.SDL_BUTTON_LEFT) return .none;

    const mod_state = @as(c_uint, c.SDL_GetModState());
    if ((mod_state & c.SDL_KMOD_CTRL) != 0) return .rotate;
    return .pan;
}

fn adjustBearing(map: *maplibre.MapHandle, delta: f64) !void {
    const camera = try currentCamera(map);
    try expectCameraStatus(map.jumpTo(.{ .bearing = (camera.bearing orelse 0) + delta }), "keyboard rotate failed");
}

fn adjustBearingAnimated(map: *maplibre.MapHandle, delta: f64, animation: maplibre.AnimationOptions) !void {
    const camera = try currentCamera(map);
    try expectCameraStatus(map.easeTo(.{ .bearing = (camera.bearing orelse 0) + delta }, animation), "keyboard rotate failed");
}

fn adjustPitchAnimated(map: *maplibre.MapHandle, delta: f64, animation: maplibre.AnimationOptions) !void {
    const camera = try currentCamera(map);
    const current_pitch = camera.pitch orelse 0;
    try expectCameraStatus(map.easeTo(.{ .pitch = clamp(current_pitch + delta, 0.0, 60.0) }, animation), "keyboard pitch failed");
}

fn resetPitchAndBearingAnimated(map: *maplibre.MapHandle, animation: maplibre.AnimationOptions) !void {
    try expectCameraStatus(map.easeTo(.{ .bearing = 0, .pitch = 0 }, animation), "camera reset failed");
}

fn currentCamera(map: *maplibre.MapHandle) !maplibre.CameraOptions {
    return map.getCamera() catch |err| {
        diagnostics.logError("camera snapshot failed", err);
        return types.AppError.CameraCommandFailed;
    };
}

fn expectCameraStatus(result: maplibre.Error!void, message: []const u8) !void {
    result catch |err| {
        diagnostics.logError(message, err);
        return types.AppError.CameraCommandFailed;
    };
}

fn point(x: f64, y: f64) maplibre.ScreenPoint {
    return .{ .x = x, .y = y };
}

fn cameraAnimation(duration_ms: f64) maplibre.AnimationOptions {
    return .{ .duration_ms = duration_ms };
}

fn scancode(value: c_int) c.SDL_Scancode {
    return @intCast(value);
}

fn clamp(value: f64, min: f64, max: f64) f64 {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
