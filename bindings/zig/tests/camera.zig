const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");

const center = maplibre.LatLng{ .latitude = 37.7749, .longitude = -122.4194 };

fn orderedCamera(map: *maplibre.MapHandle) !maplibre.CameraSnapshot {
    const operation = try map.cameraQueryStart();
    defer operation.release();
    try testing.expect(try operation.wait(-1));
    try testing.expectEqual(@as(i32, 0), try operation.resultStatus());
    return map.cameraQueryTakeResult(operation);
}

fn updateCameraOnThread(map: *maplibre.MapHandle, out_command_id: *u64, out_error: *?anyerror) void {
    out_command_id.* = map.updateCamera(.{
        .camera = .{ .center = center, .zoom = 9.0 },
    }) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "camera jump updates snapshot fields through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const command_id = try map.updateCamera(.{ .camera = .{ .center = center, .zoom = 10.0 } });
    try testing.expect(command_id != 0);

    const snapshot = try orderedCamera(&map);
    try testing.expect(snapshot.camera.center != null);
    try testing.expect(snapshot.camera.zoom != null);
}

test "camera commands accept valid public descriptors" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const jump_id = try map.updateCamera(.{
        .mode = .jump,
        .camera = .{ .center = center, .zoom = 10.0 },
    });
    const ease_id = try map.updateCamera(.{
        .mode = .ease,
        .camera = .{ .center = center, .zoom = 12.0 },
        .animation = .{ .duration_ms = 0, .easing = .{ .x1 = 0.0, .y1 = 0.0, .x2 = 0.25, .y2 = 1.0 } },
        .animation_id = 7,
    });
    const fly_id = try map.updateCamera(.{
        .mode = .fly,
        .camera = .{ .center = center, .zoom = 11.0 },
        .animation = .{ .duration_ms = 0 },
    });
    try testing.expect(jump_id < ease_id);
    try testing.expect(ease_id < fly_id);
}

test "camera commands are accepted from another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var command_id: u64 = 0;
    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, updateCameraOnThread, .{ &map, &command_id, &thread_error });
    thread.join();
    try testing.expect(thread_error == null);
    try testing.expect(command_id != 0);

    const snapshot = try orderedCamera(&map);
    try testing.expectApproxEqAbs(@as(f64, 9.0), snapshot.camera.zoom.?, 0.000001);
}

test "camera fitting computes camera and visible bounds" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 35.0, .longitude = -125.0 },
        .northeast = .{ .latitude = 39.0, .longitude = -120.0 },
    };
    const camera = try map.cameraForLatLngBounds(bounds, .{
        .padding = .{ .top = 8, .left = 12, .bottom = 8, .right = 12 },
        .bearing = 5,
        .pitch = 15,
    });
    try testing.expect(camera.center != null);
    try testing.expect(camera.zoom != null);
    try testing.expect(camera.padding != null);
    try testing.expect(camera.bearing != null);
    try testing.expect(camera.pitch != null);

    const coordinates = [_]maplibre.LatLng{ bounds.southwest, bounds.northeast };
    const coordinate_camera = try map.cameraForLatLngs(testing.allocator, coordinates[0..], null);
    try testing.expect(coordinate_camera.center != null);
    try testing.expect(coordinate_camera.zoom != null);

    const geometry_camera = try map.cameraForGeometry(testing.allocator, "{\"type\":\"LineString\",\"coordinates\":[[-125,35],[-120,39]]}", null);
    try testing.expect(geometry_camera.center != null);
    try testing.expect(geometry_camera.zoom != null);

    const visible_bounds = try map.latLngBoundsForCamera(camera);
    try testing.expect(visible_bounds.southwest.latitude <= visible_bounds.northeast.latitude);
    const unwrapped_bounds = try map.latLngBoundsForCameraUnwrapped(camera);
    try testing.expect(unwrapped_bounds.southwest.latitude <= unwrapped_bounds.northeast.latitude);
}

test "camera constraints and free camera options round-trip public values" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
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
    _ = try map.setBounds(constraints);
    const copied_constraints = try map.getBounds();
    try testing.expect(copied_constraints.bounds != null);
    try testing.expectApproxEqAbs(constraints.min_zoom.?, copied_constraints.min_zoom.?, 0.000001);
    try testing.expectApproxEqAbs(constraints.max_pitch.?, copied_constraints.max_pitch.?, 0.000001);

    const free_camera = try map.getFreeCameraOptions();
    try testing.expect(free_camera.position != null);
    try testing.expect(free_camera.orientation != null);
    _ = try map.setFreeCameraOptions(.{ .orientation = free_camera.orientation });
}

fn jumpedLongitude(map: *maplibre.MapHandle, longitude: f64) !f64 {
    _ = try map.updateCamera(.{ .camera = .{ .center = .{ .latitude = 0, .longitude = longitude }, .zoom = 2.0 } });
    const snapshot = try orderedCamera(map);
    return (snapshot.camera.center orelse return error.MissingCameraCenter).longitude;
}

// An unbounded camera center pans across the antimeridian; world bounds clamp
// longitude to -180..180.
test "camera bounds separate the unbounded constraint from world bounds" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const pristine = try map.getBounds();
    try testing.expectEqual(.unbounded, std.meta.activeTag(pristine.bounds.?));
    try testing.expectApproxEqAbs(@as(f64, -160.0), try jumpedLongitude(&map, 200.0), 1e-6);

    _ = try map.setBounds(.{ .bounds = .{ .bounded = .{
        .southwest = .{ .latitude = -90.0, .longitude = -180.0 },
        .northeast = .{ .latitude = 90.0, .longitude = 180.0 },
    } } });

    const world = try map.getBounds();
    switch (world.bounds.?) {
        .bounded => |bounds| try testing.expectApproxEqAbs(@as(f64, 180.0), bounds.northeast.longitude, 1e-6),
        .unbounded => return error.TestExpectedBoundedConstraint,
    }
    try testing.expectApproxEqAbs(@as(f64, 180.0), try jumpedLongitude(&map, 200.0), 1e-6);

    _ = try map.setBounds(.{ .bounds = .unbounded });
    const released = try map.getBounds();
    try testing.expectEqual(.unbounded, std.meta.activeTag(released.bounds.?));
    try testing.expectApproxEqAbs(@as(f64, -160.0), try jumpedLongitude(&map, 200.0), 1e-6);
}

test "camera public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
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
