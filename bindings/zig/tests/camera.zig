const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

const center = maplibre.LatLng{ .latitude = 37.7749, .longitude = -122.4194 };

fn orderedCamera(map: *maplibre.MapHandle) !maplibre.CameraSnapshot {
    var future = try map.cameraQuery();
    defer future.deinit();
    return future.wait(null);
}

fn updateCameraOnThread(map: *maplibre.MapHandle, out_generation: *u64, out_error: *?anyerror) void {
    var future = map.updateCamera(.{
        .camera = .{ .center = center, .zoom = 9.0 },
    }) catch |err| {
        out_error.* = err;
        return;
    };
    defer future.deinit();
    const result = future.wait(null) catch |err| {
        out_error.* = err;
        return;
    };
    out_generation.* = result.generation;
    out_error.* = null;
}

test "camera jump updates snapshot fields through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    var update = try map.updateCamera(.{ .camera = .{ .center = center, .zoom = 10.0 } });
    defer update.deinit();
    try testing.expect((try update.wait(null)).generation != 0);

    const snapshot = try orderedCamera(&map);
    try testing.expect(snapshot.camera.center != null);
    try testing.expect(snapshot.camera.zoom != null);
}

test "camera commands accept valid public descriptors" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    var jump = try map.updateCamera(.{
        .mode = .jump,
        .camera = .{ .center = center, .zoom = 10.0 },
    });
    defer jump.deinit();
    const jump_result = try jump.wait(null);
    var ease = try map.updateCamera(.{
        .mode = .ease,
        .camera = .{ .center = center, .zoom = 12.0 },
        .animation = .{ .duration_ms = 0, .easing = .{ .x1 = 0.0, .y1 = 0.0, .x2 = 0.25, .y2 = 1.0 }, .transition_id = 7 },
    });
    defer ease.deinit();
    const ease_result = try ease.wait(null);
    var fly = try map.updateCamera(.{
        .mode = .fly,
        .camera = .{ .center = center, .zoom = 11.0 },
        .animation = .{ .duration_ms = 0 },
    });
    defer fly.deinit();
    const fly_result = try fly.wait(null);
    try testing.expect(jump_result.generation < ease_result.generation);
    try testing.expect(ease_result.generation < fly_result.generation);
}

test "camera commands are accepted from another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    var generation: u64 = 0;
    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, updateCameraOnThread, .{ &map, &generation, &thread_error });
    thread.join();
    try testing.expect(thread_error == null);
    try testing.expect(generation != 0);

    const snapshot = try orderedCamera(&map);
    try testing.expectApproxEqAbs(@as(f64, 9.0), snapshot.camera.zoom.?, 0.000001);
}

test "camera fitting computes camera and visible bounds" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 35.0, .longitude = -125.0 },
        .northeast = .{ .latitude = 39.0, .longitude = -120.0 },
    };
    var camera_future = try map.cameraForLatLngBounds(bounds, .{
        .padding = .{ .top = 8, .left = 12, .bottom = 8, .right = 12 },
        .bearing = 5,
        .pitch = 15,
    });
    defer camera_future.deinit();
    const camera = try camera_future.wait(null);
    try testing.expect(camera.center != null);
    try testing.expect(camera.zoom != null);
    try testing.expect(camera.padding != null);
    try testing.expect(camera.bearing != null);
    try testing.expect(camera.pitch != null);

    const coordinates = [_]maplibre.LatLng{ bounds.southwest, bounds.northeast };
    var coordinate_future = try map.cameraForLatLngs(testing.allocator, coordinates[0..], null);
    defer coordinate_future.deinit();
    const coordinate_camera = try coordinate_future.wait(null);
    try testing.expect(coordinate_camera.center != null);
    try testing.expect(coordinate_camera.zoom != null);

    var geometry_future = try map.cameraForGeometry(testing.allocator, "{\"type\":\"LineString\",\"coordinates\":[[-125,35],[-120,39]]}", null);
    defer geometry_future.deinit();
    const geometry_camera = try geometry_future.wait(null);
    try testing.expect(geometry_camera.center != null);
    try testing.expect(geometry_camera.zoom != null);

    var bounds_future = try map.latLngBoundsForCamera(camera);
    defer bounds_future.deinit();
    const visible_bounds = try bounds_future.wait(null);
    try testing.expect(visible_bounds.southwest.latitude <= visible_bounds.northeast.latitude);
    var unwrapped_future = try map.latLngBoundsForCameraUnwrapped(camera);
    defer unwrapped_future.deinit();
    const unwrapped_bounds = try unwrapped_future.wait(null);
    try testing.expect(unwrapped_bounds.southwest.latitude <= unwrapped_bounds.northeast.latitude);
}

test "camera constraints and free camera options round-trip public values" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const constraints = maplibre.BoundOptions{
        .bounds = .{ .bounded = .{
            .southwest = .{ .latitude = -45.0, .longitude = -120.0 },
            .northeast = .{ .latitude = 45.0, .longitude = 120.0 },
        } },
        .min_zoom = 1.0,
        .max_zoom = 12.0,
        .min_pitch = 0.0,
        .max_pitch = 45.0,
    };
    const bounds_id = try map.setBounds(constraints);
    const bounded = try support.snapshotAfterCommand(&runtime, &map, bounds_id);
    const copied_constraints = bounded.bounds;
    try testing.expect(copied_constraints.bounds != null);
    try testing.expectApproxEqAbs(constraints.min_zoom.?, copied_constraints.min_zoom.?, 0.000001);
    try testing.expectApproxEqAbs(constraints.max_pitch.?, copied_constraints.max_pitch.?, 0.000001);

    const free_camera = bounded.free_camera;
    try testing.expect(free_camera.position != null);
    try testing.expect(free_camera.orientation != null);
    const free_camera_id = try map.setFreeCameraOptions(.{ .orientation = free_camera.orientation });
    const oriented = try support.snapshotAfterCommand(&runtime, &map, free_camera_id);
    try testing.expect(oriented.free_camera.orientation != null);
}

fn jumpedLongitude(map: *maplibre.MapHandle, longitude: f64) !f64 {
    var future = try map.updateCamera(.{ .camera = .{ .center = .{ .latitude = 0, .longitude = longitude }, .zoom = 2.0 } });
    defer future.deinit();
    _ = try future.wait(null);
    const snapshot = try orderedCamera(map);
    return (snapshot.camera.center orelse return error.MissingCameraCenter).longitude;
}

// An unbounded camera center pans across the antimeridian; world bounds clamp
// longitude to -180..180.
test "camera bounds separate the unbounded constraint from world bounds" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const pristine = (try map.snapshot()).bounds;
    try testing.expectEqual(.unbounded, std.meta.activeTag(pristine.bounds.?));
    try testing.expectApproxEqAbs(@as(f64, -160.0), try jumpedLongitude(&map, 200.0), 1e-6);

    const world_id = try map.setBounds(.{ .bounds = .{ .bounded = .{
        .southwest = .{ .latitude = -90.0, .longitude = -180.0 },
        .northeast = .{ .latitude = 90.0, .longitude = 180.0 },
    } } });

    const world = (try support.snapshotAfterCommand(&runtime, &map, world_id)).bounds;
    switch (world.bounds.?) {
        .bounded => |bounds| try testing.expectApproxEqAbs(@as(f64, 180.0), bounds.northeast.longitude, 1e-6),
        .unbounded => return error.TestExpectedBoundedConstraint,
    }
    try testing.expectApproxEqAbs(@as(f64, 180.0), try jumpedLongitude(&map, 200.0), 1e-6);

    const released_id = try map.setBounds(.{ .bounds = .unbounded });
    const released = (try support.snapshotAfterCommand(&runtime, &map, released_id)).bounds;
    try testing.expectEqual(.unbounded, std.meta.activeTag(released.bounds.?));
    try testing.expectApproxEqAbs(@as(f64, -160.0), try jumpedLongitude(&map, 200.0), 1e-6);
}

test "camera public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, map.updateCamera(.{
        .camera = .{ .center = .{ .latitude = std.math.inf(f64), .longitude = 0 } },
    }));
    try testing.expectError(error.InvalidArgument, map.updateCamera(.{
        .mode = .ease,
        .camera = .{ .center = center },
        .animation = .{ .duration_ms = -1 },
    }));
    try testing.expectError(error.InvalidArgument, map.updateCamera(.{
        .mode = .fly,
        .camera = .{ .center = center },
        .animation = .{ .easing = .{ .x1 = 2, .y1 = 0, .x2 = 1, .y2 = 1 } },
    }));

    const inverted_bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 10.0, .longitude = 10.0 },
        .northeast = .{ .latitude = -10.0, .longitude = 20.0 },
    };
    try testing.expectError(error.InvalidArgument, map.cameraForLatLngBounds(inverted_bounds, null));
    try testing.expectError(error.InvalidArgument, map.cameraForLatLngs(testing.allocator, &.{}, null));
    try testing.expectError(error.InvalidArgument, map.cameraForGeometry(testing.allocator, "{", null));

    try testing.expectError(error.InvalidArgument, map.setBounds(.{ .min_zoom = 10, .max_zoom = 1 }));
    try testing.expectError(error.InvalidArgument, map.setFreeCameraOptions(.{ .position = .{ .x = std.math.inf(f64), .y = 0, .z = 0 } }));
    try testing.expectError(error.InvalidArgument, map.setFreeCameraOptions(.{ .orientation = .{ .x = 0, .y = 0, .z = 0, .w = 0 } }));
}
