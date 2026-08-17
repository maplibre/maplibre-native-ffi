const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

pub const CameraCommand = union(enum) {
    cancel_transitions,
    gesture: struct { phase: maplibre.GesturePhase, id: u64 },
    move_by: struct { dx: f64, dy: f64 },
    move_by_animated: struct { dx: f64, dy: f64, duration_ms: f64 },
    scale_by: struct { scale: f64, anchor: maplibre.ScreenPoint },
    scale_by_animated: struct { scale: f64, anchor: maplibre.ScreenPoint, duration_ms: f64 },
    pitch_by: struct { delta: f64 },
    adjust_bearing: struct { delta: f64 },
    adjust_bearing_animated: struct { delta: f64, duration_ms: f64 },
    adjust_pitch_animated: struct { delta: f64, duration_ms: f64 },
    reset_orientation: struct { duration_ms: f64 },
};

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

        var map = maplibre.MapHandle.create(&runtime, .{ .mode = .continuous }) catch |err| {
            diagnostics.logError("map create failed", err, diagnostic_store);
            return types.AppError.MapCreateFailed;
        };
        errdefer map.close() catch {};

        try selectEvents(&map, diagnostic_store);
        _ = map.resize(viewport.logical_width, viewport.logical_height, viewport.scale_factor) catch |err| {
            diagnostics.logError("map resize failed", err, diagnostic_store);
            return types.AppError.MapCreateFailed;
        };
        try loadStyle(allocator, &map, diagnostic_store);
        try setCamera(&map, diagnostic_store);
        try waitForBarrier(&runtime);

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
};

pub fn applyCameraCommand(
    map: *maplibre.MapHandle,
    command: CameraCommand,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    const update: maplibre.CameraUpdate = switch (command) {
        .cancel_transitions => .{ .camera = (try orderedCamera(map, diagnostic_store)).camera },
        .gesture => |gesture| .{
            .gesture_phase = gesture.phase,
            .gesture_id = gesture.id,
        },
        .move_by => |move| .{ .camera = .{
            .center = try movedCenter(map, move.dx, move.dy, diagnostic_store),
        } },
        .move_by_animated => |move| .{
            .mode = .ease,
            .camera = .{ .center = try movedCenter(map, move.dx, move.dy, diagnostic_store) },
            .animation = .{ .duration_ms = move.duration_ms },
        },
        .scale_by => |zoom| .{ .camera = .{
            .zoom = ((try orderedCamera(map, diagnostic_store)).camera.zoom orelse 0) + @log2(zoom.scale),
            .anchor = zoom.anchor,
        } },
        .scale_by_animated => |zoom| .{
            .mode = .ease,
            .camera = .{
                .zoom = ((try orderedCamera(map, diagnostic_store)).camera.zoom orelse 0) + @log2(zoom.scale),
                .anchor = zoom.anchor,
            },
            .animation = .{ .duration_ms = zoom.duration_ms },
        },
        .pitch_by => |pitch| .{ .camera = .{
            .pitch = clamp(((try orderedCamera(map, diagnostic_store)).camera.pitch orelse 0) + pitch.delta, 0.0, 60.0),
        } },
        .adjust_bearing => |bearing| .{ .camera = .{
            .bearing = ((try orderedCamera(map, diagnostic_store)).camera.bearing orelse 0) + bearing.delta,
        } },
        .adjust_bearing_animated => |bearing| .{
            .mode = .ease,
            .camera = .{ .bearing = ((try orderedCamera(map, diagnostic_store)).camera.bearing orelse 0) + bearing.delta },
            .animation = .{ .duration_ms = bearing.duration_ms },
        },
        .adjust_pitch_animated => |pitch| .{
            .mode = .ease,
            .camera = .{
                .pitch = clamp(((try orderedCamera(map, diagnostic_store)).camera.pitch orelse 0) + pitch.delta, 0.0, 60.0),
            },
            .animation = .{ .duration_ms = pitch.duration_ms },
        },
        .reset_orientation => |reset| .{
            .mode = .ease,
            .camera = .{ .bearing = 0, .pitch = 0 },
            .animation = .{ .duration_ms = reset.duration_ms },
        },
    };
    _ = map.updateCamera(update) catch |err| {
        diagnostics.logError("camera update failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
}

fn orderedCamera(
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !maplibre.CameraSnapshot {
    const operation = map.cameraQueryStart() catch |err| {
        diagnostics.logError("camera query failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
    defer operation.release();
    if (!(operation.wait(-1) catch false) or (operation.resultStatus() catch -1) != 0) {
        return types.AppError.CameraCommandFailed;
    }
    return map.cameraQueryTakeResult(operation) catch |err| {
        diagnostics.logError("camera query take failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
}

fn movedCenter(
    map: *maplibre.MapHandle,
    dx: f64,
    dy: f64,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !maplibre.LatLng {
    _ = try orderedCamera(map, diagnostic_store);
    const size = try map.getSize();
    return map.latLngForPixel(.{
        .x = @as(f64, @floatFromInt(size.width)) / 2.0 - dx,
        .y = @as(f64, @floatFromInt(size.height)) / 2.0 - dy,
    });
}

fn waitForBarrier(runtime: *maplibre.RuntimeHandle) !void {
    const operation = try runtime.barrierStart();
    defer operation.release();
    if (!try operation.wait(-1) or try operation.resultStatus() != 0) return error.RuntimeBarrierFailed;
}
/// Drains receiver readiness and runtime events, reporting whether the map
/// requested another frame.
pub fn drainNotifications(self: *MapState) !bool {
    var ready = try self.runtime.drainReady(self.allocator);
    defer ready.deinit();

    const map_id = try self.map.id();
    var render_update_available = false;
    var batch = try self.runtime.drainEvents(self.allocator);
    defer batch.deinit();
    for (0..batch.len()) |index| {
        const event = try batch.at(index);
        if (event.source_type != .map or event.source_id == null or
            !std.meta.eql(event.source_id.?, map_id)) continue;
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

fn clamp(value: f64, min: f64, max: f64) f64 {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/// Selects the two event types that the host drains. This runs before the style
/// load because the map retains events that were already queued.
fn selectEvents(
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    _ = map.setEventMask(.{
        .map_render_update_available = true,
        .map_render_frame_finished = true,
    }) catch |err| {
        diagnostics.logError("event mask select failed", err, diagnostic_store);
        return types.AppError.EventMaskFailed;
    };
}

fn loadStyle(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    _ = map.setStyleUrl(allocator, "https://tiles.openfreemap.org/styles/bright") catch |err| {
        diagnostics.logError("style load failed", err, diagnostic_store);
        return types.AppError.StyleLoadFailed;
    };
}

fn setCamera(
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    _ = map.updateCamera(.{ .camera = .{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    } }) catch |err| {
        diagnostics.logError("camera jump failed", err, diagnostic_store);
        return types.AppError.CameraJumpFailed;
    };
}
