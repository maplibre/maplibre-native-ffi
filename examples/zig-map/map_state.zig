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
        errdefer if (runtime.close()) |future| {
            var teardown = future;
            _ = teardown.wait(null) catch {};
            teardown.deinit();
        } else |_| {};

        // Selecting the event mask at creation puts it ahead of the style
        // load, so the map queues render updates from the first tile response.
        // The render loop re-arms from the frame result's repaint flag, so the
        // map only has to report updates that arrive between frames.
        var map_future = maplibre.MapHandle.create(&runtime, .{
            .width = viewport.logical_width,
            .height = viewport.logical_height,
            .scale_factor = viewport.scale_factor,
            .mode = .continuous,
            .event_mask = .{ .map_render_update_available = true },
        }) catch |err| {
            diagnostics.logError("map create failed", err, diagnostic_store);
            return types.AppError.MapCreateFailed;
        };
        defer map_future.deinit();
        var map = map_future.wait(diagnostic_store) catch |err| {
            diagnostics.logError("map create failed", err, diagnostic_store);
            return types.AppError.MapCreateFailed;
        };
        errdefer if (map.close()) |future| {
            var teardown = future;
            teardown.deinit();
        } else |_| {};

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
        // Awaiting both release completions keeps process exit ordered after
        // native teardown.
        if (self.map.close()) |future| {
            var teardown = future;
            _ = teardown.wait(null) catch {};
            teardown.deinit();
        } else |_| {}
        if (self.runtime.close()) |future| {
            var teardown = future;
            _ = teardown.wait(null) catch {};
            teardown.deinit();
        } else |_| {}
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
        try self.cameraMutation(self.map.updateCamera(update));
    }

    /// Ends any running camera transition, so a starting gesture takes over
    /// from it rather than fighting it.
    pub fn cancelTransitions(self: *MapState) !void {
        try self.cameraMutation(self.map.cancelTransitions());
    }

    /// Drains runtime events, reporting whether the map requested another
    /// frame.
    pub fn drainEvents(self: *MapState) !bool {
        const map_id = try self.map.id();
        var batch = try self.runtime.drainEvents(self.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (event.source_type != .map or event.source_id == null or
                !std.meta.eql(event.source_id.?, map_id)) continue;
            if (event.event_type == .map_render_update_available) return true;
        }
        return false;
    }

    fn cameraMutation(self: *MapState, result: anytype) !void {
        var completion = result catch |err| {
            diagnostics.logError("camera update failed", err, self.diagnostic_store);
            return types.AppError.CameraUpdateFailed;
        };
        completion.deinit();
    }
};

fn loadStyle(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    var completion = map.setStyleUrl(allocator, "https://tiles.openfreemap.org/styles/bright") catch |err| {
        diagnostics.logError("style load failed", err, diagnostic_store);
        return types.AppError.StyleLoadFailed;
    };
    completion.deinit();
}

fn setCamera(
    map: *maplibre.MapHandle,
    diagnostic_store: *const maplibre.DiagnosticStore,
) !void {
    var completion = map.updateCamera(.{ .camera = .{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    } }) catch |err| {
        diagnostics.logError("camera jump failed", err, diagnostic_store);
        return types.AppError.CameraJumpFailed;
    };
    completion.deinit();
}
