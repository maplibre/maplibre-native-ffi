const std = @import("std");
const maplibre = @import("maplibre_native");

const c = @import("c.zig").c;
const channel = @import("channel.zig");
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

/// Decodes host input into camera commands.
///
/// This runs on the render loop, which does not own the map, so it only
/// produces commands; the runtime loop applies them on the map's owner thread.
/// Anything needing the current viewport is converted to logical map
/// coordinates here, where the viewport lives.
pub const Controller = struct {
    drag_mode: DragMode = .none,
    last_x: f64 = 0,
    last_y: f64 = 0,

    pub fn handleEvent(
        self: *Controller,
        event: *const c.SDL_Event,
        commands: *channel.CommandQueue,
        current_viewport: types.Viewport,
    ) Result {
        return switch (event.type) {
            c.SDL_EVENT_MOUSE_BUTTON_DOWN => self.handleMouseButtonDown(event.button, commands, current_viewport),
            c.SDL_EVENT_MOUSE_BUTTON_UP => self.handleMouseButtonUp(event.button),
            c.SDL_EVENT_MOUSE_MOTION => self.handleMouseMotion(event.motion, commands, current_viewport),
            c.SDL_EVENT_MOUSE_WHEEL => handleMouseWheel(event.wheel, commands, current_viewport),
            c.SDL_EVENT_KEY_DOWN => handleKeyDown(event.key, commands, current_viewport),
            else => .{},
        };
    }

    fn handleMouseButtonDown(
        self: *Controller,
        button: c.SDL_MouseButtonEvent,
        commands: *channel.CommandQueue,
        current_viewport: types.Viewport,
    ) Result {
        const cursor = logicalPoint(button.x, button.y, current_viewport);
        self.last_x = cursor.x;
        self.last_y = cursor.y;

        const mode = dragModeForButton(button.button);
        if (mode == .none) return .{};

        // Queued ahead of the drag's own commands, so the transition stops
        // before the first delta lands.
        commands.push(.cancel_transitions);
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
        commands: *channel.CommandQueue,
        current_viewport: types.Viewport,
    ) Result {
        const cursor = logicalPoint(motion.x, motion.y, current_viewport);
        const x = cursor.x;
        const y = cursor.y;
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
                commands.push(.{ .move_by = .{ .dx = dx, .dy = dy } });
            },
            .rotate => {
                const dx = x - self.last_x;
                const dy = y - self.last_y;
                if (dx == 0 and dy == 0) return .{ .handled = true };
                commands.push(.{ .adjust_bearing = .{ .delta = dx * 0.5 } });
                commands.push(.{ .pitch_by = .{ .delta = dy / 2.0 } });
            },
            .pitch => {
                const dy = y - self.last_y;
                if (dy == 0) return .{ .handled = true };
                commands.push(.{ .pitch_by = .{ .delta = dy / 2.0 } });
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
        \\  ] / [: pitch
        \\  0: reset pitch and bearing
        \\
    , .{});
}

fn handleMouseWheel(
    wheel: c.SDL_MouseWheelEvent,
    commands: *channel.CommandQueue,
    current_viewport: types.Viewport,
) Result {
    const delta: f64 = wheel.y;
    if (delta == 0) return .{ .handled = true };

    const anchor = logicalPoint(wheel.mouse_x, wheel.mouse_y, current_viewport);
    const scale = std.math.pow(f64, 2.0, delta * 0.25);
    commands.push(.{ .scale_by = .{ .scale = scale, .anchor = anchor } });
    return .{ .handled = true, .camera_changed = true };
}

fn handleKeyDown(
    key: c.SDL_KeyboardEvent,
    commands: *channel.CommandQueue,
    current_viewport: types.Viewport,
) Result {
    const pan_step = 120.0;
    const zoom_step = 1.25;
    const bearing_step = 10.0;
    const pitch_step = 5.0;
    const center = point(
        @as(f64, @floatFromInt(current_viewport.logical_width)) / 2.0,
        @as(f64, @floatFromInt(current_viewport.logical_height)) / 2.0,
    );

    switch (key.scancode) {
        scancode(c.SDL_SCANCODE_LEFT), scancode(c.SDL_SCANCODE_A) => {
            commands.push(.{ .move_by_animated = .{ .dx = pan_step, .dy = 0, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_RIGHT), scancode(c.SDL_SCANCODE_D) => {
            commands.push(.{ .move_by_animated = .{ .dx = -pan_step, .dy = 0, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_UP), scancode(c.SDL_SCANCODE_W) => {
            commands.push(.{ .move_by_animated = .{ .dx = 0, .dy = pan_step, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_DOWN), scancode(c.SDL_SCANCODE_S) => {
            commands.push(.{ .move_by_animated = .{ .dx = 0, .dy = -pan_step, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_EQUALS), scancode(c.SDL_SCANCODE_KP_PLUS) => {
            commands.push(.{ .scale_by_animated = .{ .scale = zoom_step, .anchor = center, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_MINUS), scancode(c.SDL_SCANCODE_KP_MINUS) => {
            commands.push(.{ .scale_by_animated = .{ .scale = 1.0 / zoom_step, .anchor = center, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_Q) => {
            commands.push(.{ .adjust_bearing_animated = .{ .delta = -bearing_step, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_E) => {
            commands.push(.{ .adjust_bearing_animated = .{ .delta = bearing_step, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_RIGHTBRACKET) => {
            commands.push(.{ .adjust_pitch_animated = .{ .delta = pitch_step, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_LEFTBRACKET) => {
            commands.push(.{ .adjust_pitch_animated = .{ .delta = -pitch_step, .duration_ms = keyboard_animation_ms } });
        },
        scancode(c.SDL_SCANCODE_0) => {
            commands.push(.{ .reset_orientation = .{ .duration_ms = reset_animation_ms } });
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

fn point(x: f64, y: f64) maplibre.ScreenPoint {
    return .{ .x = x, .y = y };
}

fn logicalPoint(x: f64, y: f64, current_viewport: types.Viewport) maplibre.ScreenPoint {
    return point(
        logicalCoordinate(x, current_viewport.window_width, current_viewport.logical_width),
        logicalCoordinate(y, current_viewport.window_height, current_viewport.logical_height),
    );
}

fn logicalCoordinate(value: f64, window_size: u32, logical_size: u32) f64 {
    if (window_size == 0) return value;
    return value * @as(f64, @floatFromInt(logical_size)) /
        @as(f64, @floatFromInt(window_size));
}

fn scancode(value: c_int) c.SDL_Scancode {
    return @intCast(value);
}
