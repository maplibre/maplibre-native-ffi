const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");
const values = @import("values.zig");

pub fn copyOwnedBuffer(
    allocator: std.mem.Allocator,
    buffer: c.mln_buffer,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!?values.OwnedString {
    if (buffer == 0) return null;
    defer c.mln_buffer_destroy(buffer);
    var view = c.mln_buffer_view{ .data = null, .size = 0 };
    try status.checkStatus(c.mln_buffer_get(buffer, &view), diagnostic_store);
    if (view.size == 0) {
        return .{ .allocator = allocator, .value = try allocator.dupe(u8, "") };
    }
    const data: [*]const u8 = @ptrCast(view.data orelse return error.NativeError);
    return .{ .allocator = allocator, .value = try allocator.dupe(u8, data[0..view.size]) };
}

pub const TempStorage = struct {
    arena: std.heap.ArenaAllocator,
    diagnostic_store: ?*diagnostics.DiagnosticStore = null,

    pub fn init(allocator: std.mem.Allocator) TempStorage {
        return .{ .arena = std.heap.ArenaAllocator.init(allocator) };
    }

    pub fn initWithDiagnostics(
        allocator: std.mem.Allocator,
        diagnostic_store: ?*diagnostics.DiagnosticStore,
    ) TempStorage {
        return .{
            .arena = std.heap.ArenaAllocator.init(allocator),
            .diagnostic_store = diagnostic_store,
        };
    }

    pub fn deinit(self: *TempStorage) void {
        self.arena.deinit();
    }

    pub fn latLngs(self: *TempStorage, coordinates: []const values.LatLng) std.mem.Allocator.Error![]const c.mln_lat_lng {
        const raw = try self.arena.allocator().alloc(c.mln_lat_lng, coordinates.len);
        for (coordinates, raw) |coordinate, *out| out.* = values.latLngToNative(coordinate);
        return raw;
    }

    pub fn screenPoints(self: *TempStorage, points: []const values.ScreenPoint) std.mem.Allocator.Error![]const c.mln_screen_point {
        const raw = try self.arena.allocator().alloc(c.mln_screen_point, points.len);
        for (points, raw) |point, *out| out.* = values.screenPointToNative(point);
        return raw;
    }

    pub fn stringView(_: *TempStorage, value: []const u8) status.Error!c.mln_buffer_view {
        return .{ .data = if (value.len == 0) null else value.ptr, .size = value.len };
    }

    pub fn stringViews(self: *TempStorage, source: []const []const u8) status.Error![]const c.mln_buffer_view {
        const raw = try self.arena.allocator().alloc(c.mln_buffer_view, source.len);
        for (source, raw) |value, *out| out.* = try self.stringView(value);
        return raw;
    }

    pub fn offlineRegionDefinition(self: *TempStorage, value: anytype) status.Error!*const c.mln_offline_region_definition {
        const raw = try self.arena.allocator().create(c.mln_offline_region_definition);
        raw.size = @sizeOf(c.mln_offline_region_definition);
        switch (value) {
            .tile_pyramid => |definition| {
                raw.type = c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
                raw.data = .{ .tile_pyramid = .{
                    .size = @sizeOf(c.mln_offline_tile_pyramid_region_definition),
                    .style_url = (try self.nulTerminatedString(definition.style_url, "offline region style_url contains embedded NUL")).ptr,
                    .bounds = values.latLngBoundsToNative(definition.bounds),
                    .min_zoom = definition.min_zoom,
                    .max_zoom = definition.max_zoom,
                    .pixel_ratio = definition.pixel_ratio,
                    .include_ideographs = definition.include_ideographs,
                } };
            },
            .geometry => |definition| {
                raw.type = c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
                raw.data = .{ .geometry = .{
                    .size = @sizeOf(c.mln_offline_geometry_region_definition),
                    .style_url = (try self.nulTerminatedString(definition.style_url, "offline region style_url contains embedded NUL")).ptr,
                    .geometry = try self.stringView(definition.geometry),
                    .min_zoom = definition.min_zoom,
                    .max_zoom = definition.max_zoom,
                    .pixel_ratio = definition.pixel_ratio,
                    .include_ideographs = definition.include_ideographs,
                } };
            },
        }
        return raw;
    }

    fn nulTerminatedString(self: *TempStorage, value: []const u8, diagnostic_message: []const u8) status.Error![:0]u8 {
        if (std.mem.indexOfScalar(u8, value, 0) != null) {
            try status.setBindingDiagnostic(self.diagnostic_store, diagnostic_message);
            return error.InvalidString;
        }
        return self.arena.allocator().dupeZ(u8, value);
    }
};
