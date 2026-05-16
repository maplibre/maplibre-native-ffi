const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

test "map debug options round trip and diagnostics toggles" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const debug = maplibre.DebugOptions{
        .tile_borders = true,
        .collision = true,
        .depth_buffer = true,
    };
    try map.setDebugOptions(debug);

    const snapshot = try map.getDebugOptions();
    try testing.expect(snapshot.tile_borders);
    try testing.expect(snapshot.collision);
    try testing.expect(snapshot.depth_buffer);
    try testing.expect(!snapshot.overdraw);

    try testing.expect(!try map.getRenderingStatsViewEnabled());
    try map.setRenderingStatsViewEnabled(true);
    try testing.expect(try map.getRenderingStatsViewEnabled());

    _ = try map.isFullyLoaded();
    try map.dumpDebugLogs();
}

test "map viewport options update selected fields through public descriptors" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setViewportOptions(.{
        .north_orientation = .right,
        .constrain_mode = .width_and_height,
        .viewport_mode = .flipped_y,
        .frustum_offset = .{ .top = 1.0, .left = 2.0, .bottom = 3.0, .right = 4.0 },
    });

    var snapshot = try map.getViewportOptions();
    try testing.expectEqual(maplibre.NorthOrientation.right, snapshot.north_orientation.?);
    try testing.expectEqual(maplibre.ConstrainMode.width_and_height, snapshot.constrain_mode.?);
    try testing.expectEqual(maplibre.ViewportMode.flipped_y, snapshot.viewport_mode.?);
    try testing.expectApproxEqAbs(@as(f64, 1.0), snapshot.frustum_offset.?.top, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 2.0), snapshot.frustum_offset.?.left, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 3.0), snapshot.frustum_offset.?.bottom, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 4.0), snapshot.frustum_offset.?.right, 0.000001);

    try map.setViewportOptions(.{ .north_orientation = .down });
    snapshot = try map.getViewportOptions();
    try testing.expectEqual(maplibre.NorthOrientation.down, snapshot.north_orientation.?);
    try testing.expectEqual(maplibre.ConstrainMode.width_and_height, snapshot.constrain_mode.?);
}

test "map tile options update selected fields through public descriptors" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setTileOptions(.{
        .prefetch_zoom_delta = 2,
        .lod_min_radius = 1.5,
        .lod_scale = 2.5,
        .lod_pitch_threshold = 0.75,
        .lod_zoom_shift = -1.0,
        .lod_mode = .distance,
    });

    var snapshot = try map.getTileOptions();
    try testing.expectEqual(@as(u32, 2), snapshot.prefetch_zoom_delta.?);
    try testing.expectApproxEqAbs(@as(f64, 1.5), snapshot.lod_min_radius.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 2.5), snapshot.lod_scale.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 0.75), snapshot.lod_pitch_threshold.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, -1.0), snapshot.lod_zoom_shift.?, 0.000001);
    try testing.expectEqual(maplibre.TileLodMode.distance, snapshot.lod_mode.?);

    try map.setTileOptions(.{ .prefetch_zoom_delta = 7 });
    snapshot = try map.getTileOptions();
    try testing.expectEqual(@as(u32, 7), snapshot.prefetch_zoom_delta.?);
    try testing.expectEqual(maplibre.TileLodMode.distance, snapshot.lod_mode.?);
}

test "map tuning public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, map.setViewportOptions(.{ .frustum_offset = .{ .top = std.math.inf(f64) } }));
    try testing.expectError(error.InvalidArgument, map.setViewportOptions(.{ .frustum_offset = .{ .left = -1.0 } }));
    try testing.expectError(error.InvalidArgument, map.setTileOptions(.{ .prefetch_zoom_delta = 256 }));
    try testing.expectError(error.InvalidArgument, map.setTileOptions(.{ .lod_scale = std.math.nan(f64) }));
}
