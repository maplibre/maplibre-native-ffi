const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

const center = maplibre.LatLng{ .latitude = 37.7749, .longitude = -122.4194 };

test "camera jump updates snapshot fields through public binding" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.jumpTo(.{ .center = center, .zoom = 10.0 });

    const snapshot = try map.getCamera();
    try testing.expect(snapshot.center != null);
    try testing.expect(snapshot.zoom != null);
}

test "camera commands accept valid public descriptors" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const anchor = maplibre.ScreenPoint{ .x = 256, .y = 256 };
    const rotate_start = maplibre.ScreenPoint{ .x = 200, .y = 200 };
    const rotate_end = maplibre.ScreenPoint{ .x = 220, .y = 210 };
    const animation = maplibre.AnimationOptions{
        .duration_ms = 0,
        .easing = .{ .x1 = 0.0, .y1 = 0.0, .x2 = 0.25, .y2 = 1.0 },
    };

    try map.moveBy(4, -2);
    try map.moveByAnimated(1, 1, animation);
    try map.scaleBy(1.1, anchor);
    try map.scaleBy(0.95, null);
    try map.scaleByAnimated(1.05, anchor, animation);
    try map.rotateBy(rotate_start, rotate_end);
    try map.rotateByAnimated(rotate_start, rotate_end, animation);
    try map.pitchBy(3);
    try map.pitchByAnimated(-1, animation);
    try map.easeTo(.{ .center = center, .zoom = 12.0 }, animation);
    try map.flyTo(.{ .center = center, .zoom = 10.0 }, animation);
    try map.cancelTransitions();
}

test "camera fitting computes camera and visible bounds" {
    var runtime = try maplibre.RuntimeHandle.init(null);
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

    const geometry_camera = try map.cameraForGeometry(testing.allocator, .{ .line_string = coordinates[0..] }, null);
    try testing.expect(geometry_camera.center != null);
    try testing.expect(geometry_camera.zoom != null);

    const visible_bounds = try map.latLngBoundsForCamera(camera);
    try testing.expect(visible_bounds.southwest.latitude <= visible_bounds.northeast.latitude);
    const unwrapped_bounds = try map.latLngBoundsForCameraUnwrapped(camera);
    try testing.expect(unwrapped_bounds.southwest.latitude <= unwrapped_bounds.northeast.latitude);
}

test "camera constraints and free camera options round-trip public values" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const constraints = maplibre.BoundOptions{
        .bounds = .{
            .southwest = .{ .latitude = -45.0, .longitude = -120.0 },
            .northeast = .{ .latitude = 45.0, .longitude = 120.0 },
        },
        .min_zoom = 1.0,
        .max_zoom = 12.0,
        .min_pitch = 0.0,
        .max_pitch = 45.0,
    };
    try map.setBounds(constraints);
    const copied_constraints = try map.getBounds();
    try testing.expect(copied_constraints.bounds != null);
    try testing.expectApproxEqAbs(constraints.min_zoom.?, copied_constraints.min_zoom.?, 0.000001);
    try testing.expectApproxEqAbs(constraints.max_pitch.?, copied_constraints.max_pitch.?, 0.000001);

    const free_camera = try map.getFreeCameraOptions();
    try testing.expect(free_camera.position != null);
    try testing.expect(free_camera.orientation != null);
    try map.setFreeCameraOptions(.{ .orientation = free_camera.orientation });
}

test "camera public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, map.jumpTo(.{ .center = .{ .latitude = std.math.inf(f64), .longitude = 0 } }));
    try testing.expectError(error.InvalidArgument, map.easeTo(.{ .center = center }, .{ .duration_ms = -1 }));
    try testing.expectError(error.InvalidArgument, map.flyTo(.{ .center = center }, .{ .easing = .{ .x1 = 2, .y1 = 0, .x2 = 1, .y2 = 1 } }));
    try testing.expectError(error.InvalidArgument, map.moveBy(std.math.nan(f64), 0));
    try testing.expectError(error.InvalidArgument, map.scaleBy(0, null));
    try testing.expectError(error.InvalidArgument, map.rotateBy(.{ .x = std.math.inf(f64), .y = 0 }, .{ .x = 0, .y = 0 }));
    try testing.expectError(error.InvalidArgument, map.pitchBy(std.math.nan(f64)));

    const inverted_bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 10.0, .longitude = 10.0 },
        .northeast = .{ .latitude = -10.0, .longitude = 20.0 },
    };
    try testing.expectError(error.InvalidArgument, map.cameraForLatLngBounds(inverted_bounds, null));
    try testing.expectError(error.InvalidArgument, map.cameraForLatLngs(testing.allocator, &.{}, null));
    try testing.expectError(error.InvalidArgument, map.cameraForGeometry(testing.allocator, .empty, null));

    try testing.expectError(error.InvalidArgument, map.setBounds(.{ .min_zoom = 10, .max_zoom = 1 }));
    try testing.expectError(error.InvalidArgument, map.setFreeCameraOptions(.{ .position = .{ .x = std.math.inf(f64), .y = 0, .z = 0 } }));
    try testing.expectError(error.InvalidArgument, map.setFreeCameraOptions(.{ .orientation = .{ .x = 0, .y = 0, .z = 0, .w = 0 } }));
}
