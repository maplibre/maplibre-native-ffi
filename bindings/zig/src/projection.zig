const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map_module = @import("map.zig");
const MapHandle = map_module.MapHandle;
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const status = @import("status.zig");
const std = @import("std");
const values = @import("values.zig");

const ProjectionState = struct {
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    active_leases: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    closing: bool = false,
};

const ProjectionLease = struct {
    state: *ProjectionState,
    native: c.mln_map_projection,
    diagnostic_store: ?*diagnostics.DiagnosticStore,

    fn release(self: ProjectionLease) void {
        _ = self.state.active_leases.fetchSub(1, .seq_cst);
    }
};

// Binding-owned state keyed by the C API's generational projection handle.
var projection_registry_lock = std.atomic.Value(bool).init(false);
var projection_registry: std.AutoHashMapUnmanaged(c.mln_map_projection, *ProjectionState) = .empty;

pub const MapProjectionHandle = enum(c.mln_map_projection) {
    _,

    pub fn create(map: *MapHandle) status.Error!MapProjectionHandle {
        var operation: c.mln_operation = 0;
        const diagnostic_store = map_module.diagnosticStore(map);
        try status.checkStatus(c.mln_map_projection_create_start(try map_module.native(map), &operation), diagnostic_store);
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnostic_store);
        var projection: c.mln_map_projection = 0;
        try status.checkStatus(c.mln_map_projection_create_take_result(operation, &projection), diagnostic_store);

        const projection_state = try std.heap.smp_allocator.create(ProjectionState);
        projection_state.* = .{ .diagnostic_store = diagnostic_store };
        errdefer std.heap.smp_allocator.destroy(projection_state);
        return try registerProjectionState(projection, projection_state);
    }

    pub fn getCamera(self: *MapProjectionHandle) status.Error!values.CameraOptions {
        const lease = try projectionLease(self.*);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_projection_get_camera_start(lease.native, &operation), lease.diagnostic_store);
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, lease.diagnostic_store);
        var camera = c.mln_camera_options_default();
        try status.checkStatus(c.mln_map_projection_get_camera_take_result(operation, &camera), lease.diagnostic_store);
        return values.cameraOptionsFromNative(camera);
    }

    pub fn setCamera(self: *MapProjectionHandle, camera: values.CameraOptions) status.Error!u64 {
        var raw_camera = values.cameraOptionsToNative(camera);
        const lease = try projectionLease(self.*);
        defer lease.release();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_projection_set_camera(lease.native, &raw_camera, &command_id), lease.diagnostic_store);
        return command_id;
    }

    pub fn setVisibleCoordinates(
        self: *MapProjectionHandle,
        allocator: std.mem.Allocator,
        coordinates: []const values.LatLng,
        padding: values.EdgeInsets,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        const coordinate_ptr = if (raw_coordinates.len == 0) null else raw_coordinates.ptr;
        const lease = try projectionLease(self.*);
        defer lease.release();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_projection_set_visible_coordinates(lease.native, coordinate_ptr, raw_coordinates.len, values.edgeInsetsToNative(padding), &command_id),
            lease.diagnostic_store,
        );
        return command_id;
    }

    pub fn setVisibleGeometry(
        self: *MapProjectionHandle,
        allocator: std.mem.Allocator,
        geometry: []const u8,
        padding: values.EdgeInsets,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const lease = try projectionLease(self.*);
        defer lease.release();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_projection_set_visible_geometry(lease.native, try temp.stringView(geometry), values.edgeInsetsToNative(padding), &command_id),
            lease.diagnostic_store,
        );
        return command_id;
    }

    pub fn pixelForLatLng(self: *MapProjectionHandle, coordinate: values.LatLng) status.Error!values.ScreenPoint {
        const lease = try projectionLease(self.*);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_projection_pixel_for_lat_lng_start(lease.native, values.latLngToNative(coordinate), &operation), lease.diagnostic_store);
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, lease.diagnostic_store);
        var point: c.mln_screen_point = undefined;
        try status.checkStatus(c.mln_map_projection_pixel_for_lat_lng_take_result(operation, &point), lease.diagnostic_store);
        return values.screenPointFromNative(point);
    }

    pub fn latLngForPixel(self: *MapProjectionHandle, point: values.ScreenPoint) status.Error!values.LatLng {
        const lease = try projectionLease(self.*);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_projection_lat_lng_for_pixel_start(lease.native, values.screenPointToNative(point), &operation), lease.diagnostic_store);
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, lease.diagnostic_store);
        var coordinate: c.mln_lat_lng = undefined;
        try status.checkStatus(c.mln_map_projection_lat_lng_for_pixel_take_result(operation, &coordinate), lease.diagnostic_store);
        return values.latLngFromNative(coordinate);
    }

    pub fn close(self: *MapProjectionHandle) status.Error!void {
        const projection_close = try beginProjectionClose(self.*) orelse return;
        var operation: c.mln_operation = 0;
        status.checkStatus(c.mln_map_projection_close_start(projection_close.native, &operation), projection_close.diagnostic_store) catch |err| {
            cancelProjectionClose(projection_close.state);
            return err;
        };
        defer c.mln_operation_release(operation);
        runtime_module.waitNativeOperation(operation, projection_close.diagnostic_store) catch |err| {
            cancelProjectionClose(projection_close.state);
            return err;
        };
        const projection_state = finishProjectionClose(self.*) orelse projection_close.state;
        std.heap.smp_allocator.destroy(projection_state);
    }
};

fn registerProjectionState(
    projection: c.mln_map_projection,
    projection_state: *ProjectionState,
) std.mem.Allocator.Error!MapProjectionHandle {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    try projection_registry.put(std.heap.smp_allocator, projection, projection_state);
    return @enumFromInt(projection);
}

fn projectionLease(handle: MapProjectionHandle) status.BindingError!ProjectionLease {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    const projection_state = projection_registry.get(@intFromEnum(handle)) orelse return error.ClosedHandle;
    if (projection_state.closing) return error.ActiveBorrow;
    _ = projection_state.active_leases.fetchAdd(1, .seq_cst);
    return .{
        .state = projection_state,
        .native = @intFromEnum(handle),
        .diagnostic_store = projection_state.diagnostic_store,
    };
}

const ProjectionClose = struct {
    state: *ProjectionState,
    native: c.mln_map_projection,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

fn beginProjectionClose(handle: MapProjectionHandle) status.BindingError!?ProjectionClose {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    const projection_state = projection_registry.get(@intFromEnum(handle)) orelse return null;
    if (projection_state.closing) return error.ActiveBorrow;
    if (projection_state.active_leases.load(.seq_cst) != 0) return error.ActiveBorrow;
    projection_state.closing = true;
    return .{
        .state = projection_state,
        .native = @intFromEnum(handle),
        .diagnostic_store = projection_state.diagnostic_store,
    };
}

fn cancelProjectionClose(projection_state: *ProjectionState) void {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    projection_state.closing = false;
}

fn finishProjectionClose(handle: MapProjectionHandle) ?*ProjectionState {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    const entry = projection_registry.fetchRemove(@intFromEnum(handle)) orelse return null;
    return entry.value;
}

fn lockProjectionRegistry() void {
    while (projection_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockProjectionRegistry() void {
    projection_registry_lock.store(false, .seq_cst);
}

pub fn projectedMetersForLatLng(
    coordinate: values.LatLng,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!values.ProjectedMeters {
    var meters: c.mln_projected_meters = undefined;
    try status.checkStatus(c.mln_projected_meters_for_lat_lng(values.latLngToNative(coordinate), &meters), diagnostic_store);
    return values.projectedMetersFromNative(meters);
}

pub fn latLngForProjectedMeters(
    meters: values.ProjectedMeters,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!values.LatLng {
    var coordinate: c.mln_lat_lng = undefined;
    try status.checkStatus(c.mln_lat_lng_for_projected_meters(values.projectedMetersToNative(meters), &coordinate), diagnostic_store);
    return values.latLngFromNative(coordinate);
}
