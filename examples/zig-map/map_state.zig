const std = @import("std");
const maplibre = @import("maplibre_native");

const diagnostics = @import("diagnostics.zig");
const render = @import("render/mod.zig");
const types = @import("types.zig");

pub const MapState = struct {
    allocator: std.mem.Allocator,
    diagnostic_store: *maplibre.DiagnosticStore,
    runtime: maplibre.RuntimeHandle,
    map: maplibre.MapHandle,
    target: ?render.RenderTarget,

    pub fn init(
        allocator: std.mem.Allocator,
        window: anytype,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !MapState {
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

        var target = render.RenderTarget.init(allocator, window, viewport, mode, &map) catch |err| {
            diagnostics.logError("render target attach failed", err, diagnostic_store);
            return err;
        };
        errdefer target.deinit();
        return .{
            .allocator = allocator,
            .diagnostic_store = diagnostic_store,
            .runtime = runtime,
            .map = map,
            .target = target,
        };
    }

    pub fn deinit(self: *MapState) void {
        if (self.target) |*target| target.deinit();
        self.target = null;
        self.map.close() catch {};
        self.runtime.close() catch {};
        self.diagnostic_store.deinit();
        self.allocator.destroy(self.diagnostic_store);
    }

    pub fn finishFrame(self: *MapState) !void {
        if (self.target) |*target| try target.finishFrame();
    }

    pub fn needsReattachOnResize(self: *const MapState) bool {
        return if (self.target) |*target| target.needsReattachOnResize() else false;
    }

    pub fn renderUpdate(self: *MapState, viewport: types.Viewport) !bool {
        return if (self.target) |*target|
            try target.renderUpdate(self.diagnostic_store, viewport)
        else
            false;
    }

    pub fn resize(self: *MapState, viewport: types.Viewport) !void {
        if (self.target) |*target| {
            target.resize(viewport) catch |err| {
                diagnostics.logError("render target resize failed", err, self.diagnostic_store);
                return err;
            };
        } else {
            return types.AppError.TextureResizeFailed;
        }
    }

    pub fn resizeWithReattachedTarget(
        self: *MapState,
        window: anytype,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !void {
        if (self.target) |*target| target.deinit();
        self.target = null;
        self.target = render.RenderTarget.init(
            self.allocator,
            window,
            viewport,
            mode,
            &self.map,
        ) catch |err| {
            diagnostics.logError("render target reattach failed", err, self.diagnostic_store);
            return err;
        };
    }
};

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
