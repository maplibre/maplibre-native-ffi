const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const runtime_module = @import("runtime.zig");
const RuntimeHandle = runtime_module.RuntimeHandle;
const status = @import("status.zig");

const MapStateHandle = opaque {};
const MapState = struct {
    native: ?*c.mln_map,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

pub const MapMode = enum {
    continuous,
    static,
    tile,

    fn toRaw(self: MapMode) u32 {
        return switch (self) {
            .continuous => c.MLN_MAP_MODE_CONTINUOUS,
            .static => c.MLN_MAP_MODE_STATIC,
            .tile => c.MLN_MAP_MODE_TILE,
        };
    }
};

pub const MapOptions = struct {
    width: u32 = 512,
    height: u32 = 512,
    scale_factor: f64 = 1.0,
    mode: MapMode = .continuous,
};

pub const MapHandle = struct {
    state: *MapStateHandle,

    pub fn create(runtime: RuntimeHandle, options: MapOptions) status.Error!MapHandle {
        var native_options = c.mln_map_options_default();
        native_options.width = options.width;
        native_options.height = options.height;
        native_options.scale_factor = options.scale_factor;
        native_options.map_mode = options.mode.toRaw();

        var map: ?*c.mln_map = null;
        const diagnostic_store = runtime_module.diagnosticStore(runtime);
        try status.checkStatus(
            c.mln_map_create(try runtime_module.native(runtime), &native_options, &map),
            diagnostic_store,
        );
        errdefer {
            if (map) |handle| _ = c.mln_map_destroy(handle);
        }

        const map_state = try std.heap.smp_allocator.create(MapState);
        map_state.* = .{ .native = map.?, .diagnostic_store = diagnostic_store };
        return .{ .state = @ptrCast(map_state) };
    }

    pub fn setStyleJson(
        self: MapHandle,
        allocator: std.mem.Allocator,
        json: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const json_z = try nulTerminated(allocator, json);
        defer allocator.free(json_z);
        try status.checkStatus(c.mln_map_set_style_json(native_map, json_z.ptr), state(self).diagnostic_store);
    }

    pub fn setStyleUrl(
        self: MapHandle,
        allocator: std.mem.Allocator,
        url: []const u8,
    ) status.Error!void {
        const native_map = try native(self);
        const url_z = try nulTerminated(allocator, url);
        defer allocator.free(url_z);
        try status.checkStatus(c.mln_map_set_style_url(native_map, url_z.ptr), state(self).diagnostic_store);
    }

    pub fn requestRepaint(self: MapHandle) status.Error!void {
        try status.checkStatus(c.mln_map_request_repaint(try native(self)), state(self).diagnostic_store);
    }

    pub fn close(self: MapHandle) status.Error!void {
        const map_state = state(self);
        const map = map_state.native orelse return;
        try status.checkStatus(c.mln_map_destroy(map), map_state.diagnostic_store);
        map_state.native = null;
    }
};

fn state(handle: MapHandle) *MapState {
    return @ptrCast(@alignCast(handle.state));
}

pub fn native(handle: MapHandle) status.BindingError!*c.mln_map {
    return state(handle).native orelse error.ClosedHandle;
}

pub fn diagnosticStore(handle: MapHandle) ?*diagnostics.DiagnosticStore {
    return state(handle).diagnostic_store;
}

fn nulTerminated(allocator: std.mem.Allocator, value: []const u8) status.Error![:0]u8 {
    if (std.mem.indexOfScalar(u8, value, 0) != null) return error.InvalidString;
    return allocator.dupeZ(u8, value);
}
