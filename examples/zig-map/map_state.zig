const std = @import("std");
const maplibre = @import("maplibre_native");

const channel = @import("channel.zig");
const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

/// Runtime and map, owned for their whole lifetime by the runtime loop thread.
///
/// The render target is not here: it belongs to the render loop thread, which
/// owns the window and the graphics context.
pub const MapState = struct {
    allocator: std.mem.Allocator,
    diagnostic_store: *maplibre.DiagnosticStore,
    runtime: maplibre.RuntimeHandle,
    map: maplibre.MapHandle,

    pub fn init(allocator: std.mem.Allocator, viewport: types.Viewport) !MapState {
        const diagnostic_store = try allocator.create(maplibre.DiagnosticStore);
        diagnostic_store.* = maplibre.DiagnosticStore.init(allocator);
        errdefer {
            diagnostic_store.deinit();
            allocator.destroy(diagnostic_store);
        }

        var runtime = maplibre.RuntimeHandle.create(allocator, .{ .cache_path = ":memory:" }, diagnostic_store) catch |err| {
            diagnostics.logError("runtime create failed", err, diagnostic_store);
            return types.AppError.RuntimeCreateFailed;
        };
        errdefer runtime.close() catch {};

        var map = maplibre.MapHandle.create(&runtime, .{
            .width = viewport.logical_width,
            .height = viewport.logical_height,
            .scale_factor = viewport.scale_factor,
            .mode = .continuous,
        }) catch |err| {
            diagnostics.logError("map create failed", err, diagnostic_store);
            return types.AppError.MapCreateFailed;
        };
        errdefer map.close() catch {};

        try loadStyle(allocator, &map, diagnostic_store);
        try setCamera(&map, diagnostic_store);

        return .{
            .allocator = allocator,
            .diagnostic_store = diagnostic_store,
            .runtime = runtime,
            .map = map,
        };
    }

    pub fn deinit(self: *MapState) void {
        self.map.close() catch {};
        self.runtime.close() catch {};
        self.diagnostic_store.deinit();
        self.allocator.destroy(self.diagnostic_store);
    }

    /// Applies every queued camera command on the map's owner thread.
    pub fn applyCommands(self: *MapState, commands: *channel.CommandQueue) !void {
        var batch: [64]channel.CameraCommand = undefined;
        while (true) {
            const count = commands.drain(&batch);
            if (count == 0) return;
            for (batch[0..count]) |command| {
                try applyCameraCommand(&self.map, command, self.diagnostic_store);
            }
        }
    }
};

/// Applies one decoded camera command. Runs on the map's owner thread, which is
/// why the read-modify-write commands read the current camera here rather than
/// on the render loop that produced them.
pub fn applyCameraCommand(
    map: *maplibre.MapHandle,
    command: channel.CameraCommand,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    switch (command) {
        .cancel_transitions => try expectCameraStatus(
            map.cancelTransitions(),
            "cancel camera transitions failed",
            diagnostic_store,
        ),
        .set_gesture_in_progress => |gesture| try expectCameraStatus(
            map.setGestureInProgress(gesture.in_progress),
            "set gesture in progress failed",
            diagnostic_store,
        ),
        .move_by => |move| try expectCameraStatus(
            map.moveBy(move.dx, move.dy),
            "camera pan failed",
            diagnostic_store,
        ),
        .move_by_animated => |move| try expectCameraStatus(
            map.moveByAnimated(move.dx, move.dy, .{ .duration_ms = move.duration_ms }),
            "keyboard pan failed",
            diagnostic_store,
        ),
        .scale_by => |zoom| try expectCameraStatus(
            map.scaleBy(zoom.scale, zoom.anchor),
            "camera zoom failed",
            diagnostic_store,
        ),
        .scale_by_animated => |zoom| try expectCameraStatus(
            map.scaleByAnimated(zoom.scale, zoom.anchor, .{ .duration_ms = zoom.duration_ms }),
            "keyboard zoom failed",
            diagnostic_store,
        ),
        .pitch_by => |pitch| try expectCameraStatus(
            map.pitchBy(pitch.delta),
            "camera pitch failed",
            diagnostic_store,
        ),
        .adjust_bearing => |bearing| {
            const camera = try currentCamera(map, diagnostic_store);
            try expectCameraStatus(
                map.jumpTo(.{ .bearing = (camera.bearing orelse 0) + bearing.delta }),
                "camera rotate failed",
                diagnostic_store,
            );
        },
        .adjust_bearing_animated => |bearing| {
            const camera = try currentCamera(map, diagnostic_store);
            try expectCameraStatus(
                map.easeTo(
                    .{ .bearing = (camera.bearing orelse 0) + bearing.delta },
                    .{ .duration_ms = bearing.duration_ms },
                ),
                "keyboard rotate failed",
                diagnostic_store,
            );
        },
        .adjust_pitch_animated => |pitch| {
            const camera = try currentCamera(map, diagnostic_store);
            try expectCameraStatus(
                map.easeTo(
                    .{ .pitch = clamp((camera.pitch orelse 0) + pitch.delta, 0.0, 60.0) },
                    .{ .duration_ms = pitch.duration_ms },
                ),
                "keyboard pitch failed",
                diagnostic_store,
            );
        },
        .reset_orientation => |reset| try expectCameraStatus(
            map.easeTo(.{ .bearing = 0, .pitch = 0 }, .{ .duration_ms = reset.duration_ms }),
            "camera reset failed",
            diagnostic_store,
        ),
    }
}

/// Drains runtime events, reporting whether the map wants another frame.
pub fn drainEvents(
    allocator: std.mem.Allocator,
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
) !bool {
    const map_id = try map.id();
    var render_update_available = false;
    while (try runtime.pollEvent(allocator)) |event_value| {
        var event = event_value;
        defer event.deinit();
        if (event.source_type != .map or event.source_id == null or !std.meta.eql(event.source_id.?, map_id)) continue;
        switch (event.event_type) {
            .map_render_update_available => render_update_available = true,
            .map_render_frame_finished => switch (event.payload) {
                .render_frame => |frame| render_update_available = render_update_available or frame.needs_repaint,
                else => {},
            },
            else => {},
        }
    }
    return render_update_available;
}

fn currentCamera(
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !maplibre.CameraOptions {
    return map.getCamera() catch |err| {
        diagnostics.logError("camera snapshot failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
}

fn expectCameraStatus(
    result: maplibre.Error!void,
    message: []const u8,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    result catch |err| {
        diagnostics.logError(message, err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
}

fn clamp(value: f64, min: f64, max: f64) f64 {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

fn loadStyle(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    map.setStyleUrl(allocator, "https://tiles.openfreemap.org/styles/bright") catch |err| {
        diagnostics.logError("style load failed", err, diagnostic_store);
        return types.AppError.StyleLoadFailed;
    };
}

fn setCamera(
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    map.jumpTo(.{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    }) catch |err| {
        diagnostics.logError("camera jump failed", err, diagnostic_store);
        return types.AppError.CameraJumpFailed;
    };
}
