const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map_module = @import("map.zig");
const MapHandle = map_module.MapHandle;
const NativeMapProjection = opaque {};
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const status = @import("status.zig");
const std = @import("std");
const values = @import("values.zig");

const ProjectionState = struct {
    native: ?*NativeMapProjection,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    active_leases: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    closing: bool = false,
};

const ProjectionLease = struct {
    state: *ProjectionState,
    native: *c.mln_map_projection,
    diagnostic_store: ?*diagnostics.DiagnosticStore,

    fn release(self: ProjectionLease) void {
        _ = self.state.active_leases.fetchSub(1, .seq_cst);
    }
};

const ProjectionRegistrySlot = struct {
    state: ?*ProjectionState,
    generation: u64,
};

var projection_registry_lock = std.atomic.Value(bool).init(false);
var projection_registry: std.ArrayList(ProjectionRegistrySlot) = .empty;
var projection_free_list: std.ArrayList(usize) = .empty;

pub const MapProjectionHandle = enum(u128) {
    _,

    pub fn create(map: *MapHandle) status.Error!MapProjectionHandle {
        var projection: ?*c.mln_map_projection = null;
        const diagnostic_store = map_module.diagnosticStore(map);
        try status.checkStatus(
            c.mln_map_projection_create(try map_module.native(map), &projection),
            diagnostic_store,
        );
        errdefer {
            if (projection) |handle| _ = c.mln_map_projection_destroy(handle);
        }

        const projection_state = try std.heap.smp_allocator.create(ProjectionState);
        projection_state.* = .{ .native = @ptrCast(projection.?), .diagnostic_store = diagnostic_store };
        errdefer std.heap.smp_allocator.destroy(projection_state);

        return try registerProjectionState(projection_state);
    }

    pub fn getCamera(self: *MapProjectionHandle) status.Error!values.CameraOptions {
        var camera = c.mln_camera_options_default();
        const lease = try projectionLease(self.*);
        defer lease.release();
        try status.checkStatus(c.mln_map_projection_get_camera(lease.native, &camera), lease.diagnostic_store);
        return values.cameraOptionsFromNative(camera);
    }

    pub fn setCamera(self: *MapProjectionHandle, camera: values.CameraOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        const lease = try projectionLease(self.*);
        defer lease.release();
        try status.checkStatus(c.mln_map_projection_set_camera(lease.native, &raw_camera), lease.diagnostic_store);
    }

    pub fn setVisibleCoordinates(
        self: *MapProjectionHandle,
        allocator: std.mem.Allocator,
        coordinates: []const values.LatLng,
        padding: values.EdgeInsets,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        const coordinate_ptr = if (raw_coordinates.len == 0) null else raw_coordinates.ptr;
        const lease = try projectionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_map_projection_set_visible_coordinates(
                lease.native,
                coordinate_ptr,
                raw_coordinates.len,
                values.edgeInsetsToNative(padding),
            ),
            lease.diagnostic_store,
        );
    }

    pub fn setVisibleGeometry(
        self: *MapProjectionHandle,
        allocator: std.mem.Allocator,
        geometry: values.Geometry,
        padding: values.EdgeInsets,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const lease = try projectionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_map_projection_set_visible_geometry(
                lease.native,
                try temp.geometry(geometry),
                values.edgeInsetsToNative(padding),
            ),
            lease.diagnostic_store,
        );
    }

    pub fn pixelForLatLng(self: *MapProjectionHandle, coordinate: values.LatLng) status.Error!values.ScreenPoint {
        var point: c.mln_screen_point = undefined;
        const lease = try projectionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_map_projection_pixel_for_lat_lng(lease.native, values.latLngToNative(coordinate), &point),
            lease.diagnostic_store,
        );
        return values.screenPointFromNative(point);
    }

    pub fn latLngForPixel(self: *MapProjectionHandle, point: values.ScreenPoint) status.Error!values.LatLng {
        var coordinate: c.mln_lat_lng = undefined;
        const lease = try projectionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_map_projection_lat_lng_for_pixel(lease.native, values.screenPointToNative(point), &coordinate),
            lease.diagnostic_store,
        );
        return values.latLngFromNative(coordinate);
    }

    pub fn close(self: *MapProjectionHandle) status.Error!void {
        const projection_close = try beginProjectionClose(self.*) orelse return;
        status.checkStatus(c.mln_map_projection_destroy(projection_close.native), projection_close.diagnostic_store) catch |err| {
            cancelProjectionClose(projection_close.state);
            return err;
        };
        const projection_state = finishProjectionClose(self.*) orelse projection_close.state;
        std.heap.smp_allocator.destroy(projection_state);
    }
};

fn registerProjectionState(projection_state: *ProjectionState) std.mem.Allocator.Error!MapProjectionHandle {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    if (projection_free_list.items.len > 0) {
        const slot_index = projection_free_list.pop().?;
        projection_registry.items[slot_index].state = projection_state;
        projection_registry.items[slot_index].generation = runtime_module.nextHandleGeneration();
        return projectionHandle(slot_index + 1, projection_registry.items[slot_index].generation);
    }

    const generation = runtime_module.nextHandleGeneration();
    try projection_free_list.ensureTotalCapacity(std.heap.smp_allocator, projection_registry.items.len + 1);
    try projection_registry.append(std.heap.smp_allocator, .{ .state = projection_state, .generation = generation });
    return projectionHandle(projection_registry.items.len, generation);
}

fn projectionHandle(index: usize, generation: u64) MapProjectionHandle {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn projectionHandleIndex(handle: MapProjectionHandle) ?usize {
    const index = @intFromEnum(handle) & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn projectionHandleGeneration(handle: MapProjectionHandle) u64 {
    return @intCast(@intFromEnum(handle) >> 64);
}

fn projectionLease(handle: MapProjectionHandle) status.BindingError!ProjectionLease {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    const index = projectionHandleIndex(handle) orelse return error.ClosedHandle;
    if (index > projection_registry.items.len) return error.ClosedHandle;
    const slot = projection_registry.items[index - 1];
    if (slot.generation != projectionHandleGeneration(handle)) return error.ClosedHandle;
    const projection_state = slot.state orelse return error.ClosedHandle;
    if (projection_state.closing) return error.ActiveBorrow;
    const projection: *c.mln_map_projection = @ptrCast(projection_state.native orelse return error.ClosedHandle);
    _ = projection_state.active_leases.fetchAdd(1, .seq_cst);
    return .{
        .state = projection_state,
        .native = projection,
        .diagnostic_store = projection_state.diagnostic_store,
    };
}

const ProjectionClose = struct {
    state: *ProjectionState,
    native: *c.mln_map_projection,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

fn beginProjectionClose(handle: MapProjectionHandle) status.BindingError!?ProjectionClose {
    lockProjectionRegistry();
    defer unlockProjectionRegistry();

    const index = projectionHandleIndex(handle) orelse return null;
    if (index > projection_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &projection_registry.items[slot_index];
    if (slot.generation != projectionHandleGeneration(handle)) return null;
    const projection_state = slot.state orelse return null;
    if (projection_state.closing) return error.ActiveBorrow;
    if (projection_state.active_leases.load(.seq_cst) != 0) return error.ActiveBorrow;
    const projection: *c.mln_map_projection = @ptrCast(projection_state.native orelse return null);
    projection_state.closing = true;
    return .{
        .state = projection_state,
        .native = projection,
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

    const index = projectionHandleIndex(handle) orelse return null;
    if (index > projection_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &projection_registry.items[slot_index];
    if (slot.generation != projectionHandleGeneration(handle)) return null;
    const projection_state = slot.state orelse return null;
    slot.state = null;
    slot.generation = runtime_module.nextHandleGeneration();
    projection_state.native = null;
    projection_free_list.appendAssumeCapacity(slot_index);
    return projection_state;
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
