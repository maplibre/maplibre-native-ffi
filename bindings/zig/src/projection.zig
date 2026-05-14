const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map_module = @import("map.zig");
const MapHandle = map_module.MapHandle;
const status = @import("status.zig");
const std = @import("std");

const MapProjectionStateHandle = opaque {};
const MapProjectionState = struct {
    native: ?*c.mln_map_projection,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

pub const MapProjectionHandle = struct {
    state: *MapProjectionStateHandle,

    pub fn create(map: MapHandle) status.Error!MapProjectionHandle {
        var projection: ?*c.mln_map_projection = null;
        const diagnostic_store = map_module.diagnosticStore(map);
        try status.checkStatus(
            c.mln_map_projection_create(try map_module.native(map), &projection),
            diagnostic_store,
        );
        errdefer {
            if (projection) |handle| _ = c.mln_map_projection_destroy(handle);
        }

        const projection_state = try std.heap.smp_allocator.create(MapProjectionState);
        projection_state.* = .{ .native = projection.?, .diagnostic_store = diagnostic_store };
        return .{ .state = @ptrCast(projection_state) };
    }

    pub fn close(self: MapProjectionHandle) status.Error!void {
        const projection_state = state(self);
        const projection = projection_state.native orelse return;
        try status.checkStatus(c.mln_map_projection_destroy(projection), projection_state.diagnostic_store);
        projection_state.native = null;
    }
};

fn state(handle: MapProjectionHandle) *MapProjectionState {
    return @ptrCast(@alignCast(handle.state));
}
