const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

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

    pub fn setGesture(self: *MapState, phase: maplibre.GesturePhase) !void {
        try self.updateCamera(.{ .gesture_phase = phase });
    }

    pub fn moveBy(self: *MapState, dx: f64, dy: f64) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{ .offset = .{ .x = dx, .y = dy } }));
    }

    pub fn moveByAnimated(self: *MapState, dx: f64, dy: f64, duration_ms: f64) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{
            .offset = .{ .x = dx, .y = dy },
            .animation = .{ .duration_ms = duration_ms },
        }));
    }

    pub fn scaleBy(self: *MapState, scale: f64, anchor: maplibre.ScreenPoint) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{ .kind = .scale, .amount = scale, .anchor = anchor }));
    }

    pub fn scaleByAnimated(self: *MapState, scale: f64, anchor: maplibre.ScreenPoint, duration_ms: f64) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{
            .kind = .scale,
            .amount = scale,
            .anchor = anchor,
            .animation = .{ .duration_ms = duration_ms },
        }));
    }

    pub fn pitchBy(self: *MapState, delta: f64) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{ .kind = .pitch, .amount = delta }));
    }

    pub fn adjustBearing(self: *MapState, delta: f64) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{ .kind = .bearing, .amount = delta }));
    }

    pub fn adjustBearingAnimated(self: *MapState, delta: f64, duration_ms: f64) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{
            .kind = .bearing,
            .amount = delta,
            .animation = .{ .duration_ms = duration_ms },
        }));
    }

    pub fn adjustPitchAnimated(self: *MapState, delta: f64, duration_ms: f64) !void {
        try self.cameraMutation(self.map.applyCameraDelta(.{
            .kind = .pitch,
            .amount = delta,
            .animation = .{ .duration_ms = duration_ms },
        }));
    }

    pub fn resetOrientation(self: *MapState, duration_ms: f64) !void {
        try self.updateCamera(.{
            .mode = .ease,
            .camera = .{ .bearing = 0, .pitch = 0 },
            .animation = .{ .duration_ms = duration_ms },
        });
    }

    fn updateCamera(self: *MapState, update: maplibre.CameraUpdate) !void {
        _ = self.map.updateCamera(update) catch |err| {
            diagnostics.logError("camera update failed", err, self.diagnostic_store);
            return types.AppError.CameraUpdateFailed;
        };
    }

    fn cameraMutation(self: *MapState, result: anyerror!u64) !void {
        _ = result catch |err| {
            diagnostics.logError("camera update failed", err, self.diagnostic_store);
            return types.AppError.CameraUpdateFailed;
        };
    }
};
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
