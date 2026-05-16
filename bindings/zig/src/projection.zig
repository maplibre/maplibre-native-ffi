const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map_module = @import("map.zig");
const MapHandle = map_module.MapHandle;
const NativeMapProjection = opaque {};
const native_temp = @import("native_temp.zig");
const status = @import("status.zig");
const std = @import("std");
const values = @import("values.zig");

pub const MapProjectionHandle = struct {
    native: ?*NativeMapProjection,
    diagnostic_store: ?*diagnostics.DiagnosticStore,

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

        return .{ .native = @ptrCast(projection.?), .diagnostic_store = diagnostic_store };
    }

    pub fn getCamera(self: *MapProjectionHandle) status.Error!values.CameraOptions {
        var camera = c.mln_camera_options_default();
        try status.checkStatus(c.mln_map_projection_get_camera(try native(self), &camera), self.diagnostic_store);
        return values.cameraOptionsFromNative(camera);
    }

    pub fn setCamera(self: *MapProjectionHandle, camera: values.CameraOptions) status.Error!void {
        var raw_camera = values.cameraOptionsToNative(camera);
        try status.checkStatus(c.mln_map_projection_set_camera(try native(self), &raw_camera), self.diagnostic_store);
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
        try status.checkStatus(
            c.mln_map_projection_set_visible_coordinates(
                try native(self),
                coordinate_ptr,
                raw_coordinates.len,
                values.edgeInsetsToNative(padding),
            ),
            self.diagnostic_store,
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
        try status.checkStatus(
            c.mln_map_projection_set_visible_geometry(
                try native(self),
                try temp.geometry(geometry),
                values.edgeInsetsToNative(padding),
            ),
            self.diagnostic_store,
        );
    }

    pub fn pixelForLatLng(self: *MapProjectionHandle, coordinate: values.LatLng) status.Error!values.ScreenPoint {
        var point: c.mln_screen_point = undefined;
        try status.checkStatus(
            c.mln_map_projection_pixel_for_lat_lng(try native(self), values.latLngToNative(coordinate), &point),
            self.diagnostic_store,
        );
        return values.screenPointFromNative(point);
    }

    pub fn latLngForPixel(self: *MapProjectionHandle, point: values.ScreenPoint) status.Error!values.LatLng {
        var coordinate: c.mln_lat_lng = undefined;
        try status.checkStatus(
            c.mln_map_projection_lat_lng_for_pixel(try native(self), values.screenPointToNative(point), &coordinate),
            self.diagnostic_store,
        );
        return values.latLngFromNative(coordinate);
    }

    pub fn close(self: *MapProjectionHandle) status.Error!void {
        const projection: *c.mln_map_projection = @ptrCast(self.native orelse return);
        try status.checkStatus(c.mln_map_projection_destroy(projection), self.diagnostic_store);
        self.native = null;
    }
};

fn native(handle: *MapProjectionHandle) status.BindingError!*c.mln_map_projection {
    return @ptrCast(handle.native orelse return error.ClosedHandle);
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
