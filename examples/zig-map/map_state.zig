const std = @import("std");
const maplibre = @import("maplibre_native");

const diagnostics = @import("diagnostics.zig");
const render_target = @import("render_target.zig");
const types = @import("types.zig");

pub const MapState = struct {
    runtime: maplibre.RuntimeHandle,
    map: maplibre.MapHandle,
    target: render_target.Session,

    pub fn init(allocator: std.mem.Allocator, viewport: types.Viewport, backend: anytype) !MapState {
        var runtime = maplibre.RuntimeHandle.create(allocator, .{ .cache_path = ":memory:" }, null) catch |err| {
            diagnostics.logError("runtime create failed", err);
            return types.AppError.RuntimeCreateFailed;
        };
        errdefer runtime.close() catch {};

        var map = maplibre.MapHandle.create(&runtime, .{
            .width = viewport.logical_width,
            .height = viewport.logical_height,
            .scale_factor = viewport.scale_factor,
            .mode = .continuous,
        }) catch |err| {
            diagnostics.logError("map create failed", err);
            return types.AppError.MapCreateFailed;
        };
        errdefer map.close() catch {};

        try loadStyle(allocator, &map);
        try setCamera(&map);

        var target = try backend.attachRenderTarget(&map, viewport);
        errdefer target.deinit();
        return .{ .runtime = runtime, .map = map, .target = target };
    }

    pub fn deinit(self: *MapState) void {
        self.target.deinit();
        self.map.close() catch {};
        self.runtime.close() catch {};
    }

    pub fn resize(self: *MapState, viewport: types.Viewport) !void {
        try self.target.resize(viewport);
    }

    pub fn resizeWithReattachedTarget(
        self: *MapState,
        viewport: types.Viewport,
        backend: anytype,
    ) !void {
        self.target.deinit();
        try backend.resize(viewport);
        self.target = try backend.attachRenderTarget(&self.map, viewport);
    }
};

pub fn drainEvents(runtime: *maplibre.RuntimeHandle, map: *maplibre.MapHandle) !bool {
    const map_id = try map.id();
    var render_update_available = false;
    while (try runtime.pollEvent()) |event| {
        if (event.source_type == .map and event.source_id != null and std.meta.eql(event.source_id.?, map_id) and
            event.event_type == .map_render_update_available)
        {
            render_update_available = true;
        }
    }
    return render_update_available;
}

fn loadStyle(allocator: std.mem.Allocator, map: *maplibre.MapHandle) !void {
    map.setStyleUrl(allocator, "https://tiles.openfreemap.org/styles/bright") catch |err| {
        diagnostics.logError("style load failed", err);
        return types.AppError.StyleLoadFailed;
    };
}

fn setCamera(map: *maplibre.MapHandle) !void {
    map.jumpTo(.{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    }) catch |err| {
        diagnostics.logError("camera jump failed", err);
        return types.AppError.CameraJumpFailed;
    };
}
