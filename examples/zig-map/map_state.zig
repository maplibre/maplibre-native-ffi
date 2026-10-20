const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

const diagnostics = @import("diagnostics.zig");
const render_target = @import("render_target.zig");
const types = @import("types.zig");

pub const MapState = struct {
    allocator: std.mem.Allocator,
    // The details of the latest failed camera call. The example drives the
    // map from one thread, so one diagnostic serves every call.
    diagnostic: maplibre.Diagnostic = .{},
    runtime: maplibre.Runtime,
    map: maplibre.Map,

    /// Creates the runtime and map. A smoke run loads an inline style, so it
    /// needs no network.
    pub fn init(allocator: std.mem.Allocator, viewport: types.Viewport, smoke: bool) !MapState {
        var diagnostic: maplibre.Diagnostic = .{};
        var runtime = maplibre.runtimeCreate(allocator, .{ .cache_path = ":memory:" }, &diagnostic) catch |err| {
            diagnostics.logError("runtime create failed", err, &diagnostic);
            return types.AppError.RuntimeCreateFailed;
        };
        errdefer runtime.deinit();
        errdefer if (maplibre.runtimeRelease(runtime, null)) |future| {
            var teardown = future;
            _ = teardown.wait(null) catch {};
            teardown.deinit();
        } else |_| {};

        var map_future = maplibre.runtimeCreateMap(allocator, runtime, .{
            .initial_extent = render_target.extent(viewport),
            .map_mode = .continuous,
            .event_mask = .{},
        }, &diagnostic) catch |err| {
            diagnostics.logError("map create failed", err, &diagnostic);
            return types.AppError.MapCreateFailed;
        };
        defer map_future.deinit();
        var map = map_future.wait(&diagnostic) catch |err| {
            diagnostics.logError("map create failed", err, &diagnostic);
            return types.AppError.MapCreateFailed;
        };
        errdefer map.deinit();
        errdefer if (maplibre.mapRelease(map, null)) |future| {
            var teardown = future;
            teardown.deinit();
        } else |_| {};

        try loadStyle(&map, &diagnostic, smoke);
        try setCamera(allocator, &map, &diagnostic);
        return .{
            .allocator = allocator,
            .runtime = runtime,
            .map = map,
        };
    }

    pub fn deinit(self: *MapState) void {
        // Awaiting both release completions lets native teardown finish before
        // the app tears down state that the callbacks use.
        if (maplibre.mapRelease(self.map, null)) |future| {
            var teardown = future;
            _ = teardown.wait(null) catch {};
            teardown.deinit();
        } else |_| {}
        if (maplibre.runtimeRelease(self.runtime, null)) |future| {
            var teardown = future;
            _ = teardown.wait(null) catch {};
            teardown.deinit();
        } else |_| {}
        self.map.deinit();
        self.runtime.deinit();
    }

    pub fn setGesture(self: *MapState, phase: maplibre.GesturePhase) !void {
        try self.updateCamera(.{ .gesture_phase = phase });
    }

    pub fn moveBy(self: *MapState, dx: f64, dy: f64) !void {
        try self.cameraMutation(maplibre.mapApplyCameraDelta(self.allocator, self.map, .{ .offset = .{ .x = dx, .y = dy } }, &self.diagnostic));
    }

    pub fn moveByAnimated(self: *MapState, dx: f64, dy: f64, duration_ms: f64) !void {
        try self.cameraMutation(maplibre.mapApplyCameraDelta(self.allocator, self.map, .{
            .offset = .{ .x = dx, .y = dy },
            .animation = .{ .duration_ms = duration_ms },
        }, &self.diagnostic));
    }

    pub fn scaleBy(self: *MapState, scale: f64, anchor: maplibre.ScreenPoint) !void {
        try self.cameraMutation(maplibre.mapApplyCameraDelta(self.allocator, self.map, .{ .scale = scale, .anchor = anchor }, &self.diagnostic));
    }

    pub fn scaleByAnimated(self: *MapState, scale: f64, anchor: maplibre.ScreenPoint, duration_ms: f64) !void {
        try self.cameraMutation(maplibre.mapApplyCameraDelta(self.allocator, self.map, .{
            .scale = scale,
            .anchor = anchor,
            .animation = .{ .duration_ms = duration_ms },
        }, &self.diagnostic));
    }

    pub fn adjustOrientation(self: *MapState, bearing: f64, pitch: f64) !void {
        try self.cameraMutation(maplibre.mapApplyCameraDelta(self.allocator, self.map, .{ .bearing = bearing, .pitch = pitch }, &self.diagnostic));
    }

    pub fn adjustBearingAnimated(self: *MapState, delta: f64, duration_ms: f64) !void {
        try self.cameraMutation(maplibre.mapApplyCameraDelta(self.allocator, self.map, .{
            .bearing = delta,
            .animation = .{ .duration_ms = duration_ms },
        }, &self.diagnostic));
    }

    pub fn adjustPitchAnimated(self: *MapState, delta: f64, duration_ms: f64) !void {
        try self.cameraMutation(maplibre.mapApplyCameraDelta(self.allocator, self.map, .{
            .pitch = delta,
            .animation = .{ .duration_ms = duration_ms },
        }, &self.diagnostic));
    }

    pub fn resetOrientation(self: *MapState, duration_ms: f64) !void {
        try self.updateCamera(.{
            .mode = .ease,
            .camera = .{ .bearing = 0, .pitch = 0 },
            .animation = .{ .duration_ms = duration_ms },
        });
    }

    fn updateCamera(self: *MapState, update: maplibre.CameraUpdate) !void {
        try self.cameraMutation(maplibre.mapUpdateCamera(self.allocator, self.map, update, &self.diagnostic));
    }

    /// Ends any running camera transition, so a starting gesture takes over
    /// from it rather than fighting it.
    pub fn cancelTransitions(self: *MapState) !void {
        try self.cameraMutation(maplibre.mapCancelTransitions(self.map, &self.diagnostic));
    }

    fn cameraMutation(self: *MapState, result: anytype) !void {
        var completion = result catch |err| {
            diagnostics.logError("camera update failed", err, &self.diagnostic);
            return types.AppError.CameraUpdateFailed;
        };
        completion.deinit();
    }
};

/// The style a smoke run renders, which needs no network.
const smoke_style_json =
    \\{"version":8,"sources":{},"layers":[{"id":"background","type":"background",
    \\"paint":{"background-color":"#d8f1ff"}}]}
;

fn loadStyle(
    map: *maplibre.Map,
    diagnostic: *maplibre.Diagnostic,
    smoke: bool,
) !void {
    const submitted = if (smoke)
        maplibre.mapSetStyleJson(map.*, smoke_style_json, diagnostic)
    else
        maplibre.mapSetStyleUrl(map.*, "https://tiles.openfreemap.org/styles/bright", diagnostic);
    var completion = submitted catch |err| {
        diagnostics.logError("style load failed", err, diagnostic);
        return types.AppError.StyleLoadFailed;
    };
    completion.deinit();
}

fn setCamera(
    allocator: std.mem.Allocator,
    map: *maplibre.Map,
    diagnostic: *maplibre.Diagnostic,
) !void {
    var completion = maplibre.mapUpdateCamera(allocator, map.*, .{ .camera = .{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    } }, diagnostic) catch |err| {
        diagnostics.logError("camera jump failed", err, diagnostic);
        return types.AppError.CameraJumpFailed;
    };
    completion.deinit();
}
