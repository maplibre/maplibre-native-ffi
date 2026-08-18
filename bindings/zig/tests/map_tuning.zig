const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

// The committed event fences the snapshot: a snapshot whose generation is at
// or past the commit's observes the committed value.
test "map debug options fence and round trip through the snapshot" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const debug = maplibre.DebugOptions{
        .tile_borders = true,
        .collision = true,
        .depth_buffer = true,
    };
    const completion = try map.setDebugOptions(debug);
    const snapshot = try support.snapshotAfterCommand(&runtime, &map, completion);
    try testing.expect(snapshot.debug_options.tile_borders);
    try testing.expect(snapshot.debug_options.collision);
    try testing.expect(snapshot.debug_options.depth_buffer);
    try testing.expect(!snapshot.debug_options.overdraw);

    try testing.expect(!snapshot.rendering_stats_view_enabled);
    const stats_id = try map.setRenderingStatsViewEnabled(true);
    const stats_snapshot = try support.snapshotAfterCommand(&runtime, &map, stats_id);
    try testing.expect(stats_snapshot.rendering_stats_view_enabled);

    _ = try map.dumpDebugLogs();
}

test "map viewport options update selected snapshot fields" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const completion = try map.setViewportOptions(.{
        .north_orientation = .right,
        .constrain_mode = .width_and_height,
        .viewport_mode = .flipped_y,
        .frustum_offset = .{ .top = 1.0, .left = 2.0, .bottom = 3.0, .right = 4.0 },
    });

    var snapshot = try support.snapshotAfterCommand(&runtime, &map, completion);
    try testing.expectEqual(maplibre.NorthOrientation.right, snapshot.viewport.north_orientation.?);
    try testing.expectEqual(maplibre.ConstrainMode.width_and_height, snapshot.viewport.constrain_mode.?);
    try testing.expectEqual(maplibre.ViewportMode.flipped_y, snapshot.viewport.viewport_mode.?);
    try testing.expectApproxEqAbs(@as(f64, 1.0), snapshot.viewport.frustum_offset.?.top, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 2.0), snapshot.viewport.frustum_offset.?.left, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 3.0), snapshot.viewport.frustum_offset.?.bottom, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 4.0), snapshot.viewport.frustum_offset.?.right, 0.000001);

    const narrowed_id = try map.setViewportOptions(.{ .north_orientation = .down });
    snapshot = try support.snapshotAfterCommand(&runtime, &map, narrowed_id);
    try testing.expectEqual(maplibre.NorthOrientation.down, snapshot.viewport.north_orientation.?);
    try testing.expectEqual(maplibre.ConstrainMode.width_and_height, snapshot.viewport.constrain_mode.?);
}

test "map tile options update selected snapshot fields" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const completion = try map.setTileOptions(.{
        .prefetch_zoom_delta = 2,
        .lod_min_radius = 1.5,
        .lod_scale = 2.5,
        .lod_pitch_threshold = 0.75,
        .lod_zoom_shift = -1.0,
        .lod_mode = .distance,
    });

    var snapshot = try support.snapshotAfterCommand(&runtime, &map, completion);
    try testing.expectEqual(@as(u32, 2), snapshot.tile.prefetch_zoom_delta.?);
    try testing.expectApproxEqAbs(@as(f64, 1.5), snapshot.tile.lod_min_radius.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 2.5), snapshot.tile.lod_scale.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 0.75), snapshot.tile.lod_pitch_threshold.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, -1.0), snapshot.tile.lod_zoom_shift.?, 0.000001);
    try testing.expectEqual(maplibre.TileLodMode.distance, snapshot.tile.lod_mode.?);

    const narrowed_id = try map.setTileOptions(.{ .prefetch_zoom_delta = 7 });
    snapshot = try support.snapshotAfterCommand(&runtime, &map, narrowed_id);
    try testing.expectEqual(@as(u32, 7), snapshot.tile.prefetch_zoom_delta.?);
    try testing.expectEqual(maplibre.TileLodMode.distance, snapshot.tile.lod_mode.?);
}

test "map tuning public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, map.setViewportOptions(.{ .frustum_offset = .{ .top = std.math.inf(f64) } }));
    try testing.expectError(error.InvalidArgument, map.setViewportOptions(.{ .frustum_offset = .{ .left = -1.0 } }));
    try testing.expectError(error.InvalidArgument, map.setTileOptions(.{ .prefetch_zoom_delta = 256 }));
    try testing.expectError(error.InvalidArgument, map.setTileOptions(.{ .lod_scale = std.math.nan(f64) }));
}
