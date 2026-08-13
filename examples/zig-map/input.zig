const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

const c = @import("c.zig").c;
const map_state = @import("map_state.zig");
const types = @import("types.zig");

const DragMode = enum {
    none,
    pan,
    rotate,
};

const keyboard_animation_ms = 160.0;
const reset_animation_ms = 220.0;

pub const Result = struct {
    handled: bool = false,
    camera_changed: bool = false,
};

const CommandSink = struct {
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,

    fn push(self: CommandSink, command: map_state.CameraCommand) !void {
        try map_state.applyCameraCommand(self.map, command, self.diagnostic_store);
    }
};

/// Decodes host input and submits camera updates directly to the any-thread map.
pub const Controller = struct {
    drag_mode: DragMode = .none,
    drag_button: u8 = 0,
    next_gesture_id: u64 = 1,
    active_gesture_id: u64 = 0,
    last_x: f64 = 0,
    last_y: f64 = 0,

    pub fn handleEvent(
        self: *Controller,
        event: *const c.SDL_Event,
        map: *maplibre.MapHandle,
        diagnostic_store: *const maplibre.DiagnosticStore,
        current_viewport: types.Viewport,
    ) !Result {
        const commands = CommandSink{ .map = map, .diagnostic_store = diagnostic_store };
        return switch (event.type) {
            c.SDL_EVENT_MOUSE_BUTTON_DOWN => try self.handleMouseButtonDown(event.button, commands, current_viewport),
            c.SDL_EVENT_MOUSE_BUTTON_UP => try self.handleMouseButtonUp(event.button, commands),
            c.SDL_EVENT_MOUSE_MOTION => try self.handleMouseMotion(event.motion, commands, current_viewport),
            c.SDL_EVENT_MOUSE_WHEEL => try handleMouseWheel(event.wheel, commands, current_viewport),
            c.SDL_EVENT_KEY_DOWN => try handleKeyDown(event.key, commands, current_viewport),
            else => .{},
        };
    }

    fn handleMouseButtonDown(
        self: *Controller,
        button: c.SDL_MouseButtonEvent,
        commands: CommandSink,
        current_viewport: types.Viewport,
    ) !Result {
        if (self.drag_mode != .none) return .{ .handled = true };
        const mode = dragModeForButton(button.button);
        if (mode == .none) return .{};

        const cursor = logicalPoint(button.x, button.y, current_viewport);
        self.last_x = cursor.x;
        self.last_y = cursor.y;
        const gesture_id = self.next_gesture_id;
        self.next_gesture_id +%= 1;
        try commands.push(.cancel_transitions);
        try commands.push(.{ .gesture = .{ .phase = .begin, .id = gesture_id } });
        self.active_gesture_id = gesture_id;
        self.drag_mode = mode;
        self.drag_button = button.button;
        return .{ .handled = true };
    }

    fn handleMouseButtonUp(
        self: *Controller,
        button: c.SDL_MouseButtonEvent,
        commands: CommandSink,
    ) !Result {
        if (button.button != c.SDL_BUTTON_LEFT and button.button != c.SDL_BUTTON_RIGHT) return .{};
        if (button.button != self.drag_button) return .{ .handled = true };
        try self.endDrag(commands);
        self.last_x = button.x;
        self.last_y = button.y;
        return .{ .handled = true };
    }

    fn endDrag(self: *Controller, commands: CommandSink) !void {
        if (self.drag_mode == .none) return;
        self.drag_mode = .none;
        self.drag_button = 0;
        const gesture_id = self.active_gesture_id;
        self.active_gesture_id = 0;
        try commands.push(.{ .gesture = .{ .phase = .end, .id = gesture_id } });
    }

    fn handleMouseMotion(
        self: *Controller,
        motion: c.SDL_MouseMotionEvent,
        commands: CommandSink,
        current_viewport: types.Viewport,
    ) !Result {
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
                try commands.push(.{ .move_by = .{ .dx = dx, .dy = dy } });
            },
            .rotate => {
                const dx = x - self.last_x;
                const dy = y - self.last_y;
                if (dx == 0 and dy == 0) return .{ .handled = true };
                try commands.push(.{ .adjust_bearing = .{ .delta = dx * 0.5 } });
                try commands.push(.{ .pitch_by = .{ .delta = dy / 2.0 } });
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
    commands: CommandSink,
    current_viewport: types.Viewport,
) !Result {
    const delta: f64 = wheel.y;
    if (delta == 0) return .{ .handled = true };

    const anchor = logicalPoint(wheel.mouse_x, wheel.mouse_y, current_viewport);
    const scale = std.math.pow(f64, 2.0, delta * 0.25);
    try commands.push(.{ .scale_by = .{ .scale = scale, .anchor = anchor } });
    return .{ .handled = true, .camera_changed = true };
}

fn handleKeyDown(
    key: c.SDL_KeyboardEvent,
    commands: CommandSink,
    current_viewport: types.Viewport,
) !Result {
    const pan_step = 120.0;
    const zoom_step = 1.25;
    const bearing_step = 10.0;
    const pitch_step = 5.0;
    const center = point(
        @as(f64, @floatFromInt(current_viewport.logical_width)) / 2.0,
        @as(f64, @floatFromInt(current_viewport.logical_height)) / 2.0,
    );

    switch (key.scancode) {
        scancode(c.SDL_SCANCODE_LEFT), scancode(c.SDL_SCANCODE_A) => try commands.push(.{ .move_by_animated = .{ .dx = pan_step, .dy = 0, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_RIGHT), scancode(c.SDL_SCANCODE_D) => try commands.push(.{ .move_by_animated = .{ .dx = -pan_step, .dy = 0, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_UP), scancode(c.SDL_SCANCODE_W) => try commands.push(.{ .move_by_animated = .{ .dx = 0, .dy = pan_step, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_DOWN), scancode(c.SDL_SCANCODE_S) => try commands.push(.{ .move_by_animated = .{ .dx = 0, .dy = -pan_step, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_EQUALS), scancode(c.SDL_SCANCODE_KP_PLUS) => try commands.push(.{ .scale_by_animated = .{ .scale = zoom_step, .anchor = center, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_MINUS), scancode(c.SDL_SCANCODE_KP_MINUS) => try commands.push(.{ .scale_by_animated = .{ .scale = 1.0 / zoom_step, .anchor = center, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_Q) => try commands.push(.{ .adjust_bearing_animated = .{ .delta = -bearing_step, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_E) => try commands.push(.{ .adjust_bearing_animated = .{ .delta = bearing_step, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_RIGHTBRACKET) => try commands.push(.{ .adjust_pitch_animated = .{ .delta = pitch_step, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_LEFTBRACKET) => try commands.push(.{ .adjust_pitch_animated = .{ .delta = -pitch_step, .duration_ms = keyboard_animation_ms } }),
        scancode(c.SDL_SCANCODE_0) => try commands.push(.{ .reset_orientation = .{ .duration_ms = reset_animation_ms } }),
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
