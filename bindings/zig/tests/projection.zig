const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

const center = maplibre.LatLng{ .latitude = 37.7749, .longitude = -122.4194 };

// The camera center projects to the middle of the viewport, so these tests
// state the extent rather than lean on the creation default.
const viewport_extent: u32 = 512;

fn expectCenterPoint(point: maplibre.ScreenPoint) !void {
    const middle: f64 = @as(f64, @floatFromInt(viewport_extent)) / 2.0;
    try testing.expectApproxEqAbs(middle, point.x, 0.001);
    try testing.expectApproxEqAbs(middle, point.y, 0.001);
}

fn expectLatLngApprox(expected: maplibre.LatLng, actual: maplibre.LatLng) !void {
    try testing.expectApproxEqAbs(expected.latitude, actual.latitude, 0.000001);
    try testing.expectApproxEqAbs(expected.longitude, actual.longitude, 0.000001);
}

fn waitForCameraCommands(map: *maplibre.MapHandle) !void {
    var future = try map.cameraQuery();
    defer future.deinit();
    _ = try future.wait(null);
}

test "map projection mode updates snapshot fields through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    _ = try map.setProjectionMode(.{ .axonometric = true, .x_skew = 0.25, .y_skew = -0.125 });
    try support.waitForBarrier(&runtime);

    const snapshot = try map.getProjectionMode();
    try testing.expectEqual(true, snapshot.axonometric.?);
    try testing.expectApproxEqAbs(@as(f64, 0.25), snapshot.x_skew.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, -0.125), snapshot.y_skew.?, 0.000001);
}

test "map converts between lat lngs and screen points" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = viewport_extent, .height = viewport_extent });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    _ = try map.updateCamera(.{ .camera = .{ .center = center, .zoom = 10.0 } });
    try waitForCameraCommands(&map);

    var point_future = try map.pixelForLatLng(center);
    defer point_future.deinit();
    const point = try point_future.wait(null);
    try expectCenterPoint(point);

    var coordinate_future = try map.latLngForPixel(point);
    defer coordinate_future.deinit();
    const coordinate = try coordinate_future.wait(null);
    try expectLatLngApprox(center, coordinate);

    const coordinates = [_]maplibre.LatLng{
        center,
        .{ .latitude = 0.0, .longitude = 0.0 },
    };
    var points: [coordinates.len]maplibre.ScreenPoint = undefined;
    var points_future = try map.pixelsForLatLngs(testing.allocator, coordinates[0..]);
    defer points_future.deinit();
    var owned_points = try points_future.wait(null);
    defer owned_points.deinit();
    @memcpy(points[0..], owned_points.items);
    try expectCenterPoint(points[0]);

    var roundtrip: [points.len]maplibre.LatLng = undefined;
    var coordinates_future = try map.latLngsForPixels(testing.allocator, points[0..]);
    defer coordinates_future.deinit();
    var owned_coordinates = try coordinates_future.wait(null);
    defer owned_coordinates.deinit();
    @memcpy(roundtrip[0..], owned_coordinates.items);
    try expectLatLngApprox(coordinates[0], roundtrip[0]);
    try expectLatLngApprox(coordinates[1], roundtrip[1]);

    var empty_points_future = try map.pixelsForLatLngs(testing.allocator, &.{});
    defer empty_points_future.deinit();
    var empty_points = try empty_points_future.wait(null);
    defer empty_points.deinit();
    try testing.expectEqual(@as(usize, 0), empty_points.items.len);

    var empty_coordinates_future = try map.latLngsForPixels(testing.allocator, &.{});
    defer empty_coordinates_future.deinit();
    var empty_coordinates = try empty_coordinates_future.wait(null);
    defer empty_coordinates.deinit();
    try testing.expectEqual(@as(usize, 0), empty_coordinates.items.len);
}

// Creation is ordered after every earlier map command, so a projection created
// right after a camera command observes that command without an explicit wait.
test "standalone projection converts and updates camera" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = viewport_extent, .height = viewport_extent });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    _ = try map.updateCamera(.{ .camera = .{ .center = center, .zoom = 10.0 } });
    var projection_future = try maplibre.MapProjectionHandle.create(&map);
    defer projection_future.deinit();
    var projection = try projection_future.wait(null);
    defer projection.close() catch @panic("projection close failed");

    const point = try projection.pixelForLatLng(center);
    try expectCenterPoint(point);

    const coordinate = try projection.latLngForPixel(point);
    try expectLatLngApprox(center, coordinate);

    // A setter is applied before it returns, so later conversions observe it.
    const helper_camera = maplibre.CameraOptions{ .center = .{ .latitude = 0.0, .longitude = 0.0 }, .zoom = 3.0 };
    try projection.setCamera(helper_camera);
    const snapshot = try projection.getCamera();
    try expectLatLngApprox(helper_camera.center.?, snapshot.center.?);
    try testing.expectApproxEqAbs(helper_camera.zoom.?, snapshot.zoom.?, 0.000001);
    const recentered = try projection.pixelForLatLng(helper_camera.center.?);
    try expectCenterPoint(recentered);

    var visible = [_]maplibre.LatLng{
        .{ .latitude = -10.0, .longitude = -10.0 },
        .{ .latitude = 10.0, .longitude = 10.0 },
    };
    try projection.setVisibleCoordinates(testing.allocator, visible[0..], .{ .top = 10.0, .left = 20.0, .bottom = 10.0, .right = 20.0 });
    const fitted = try projection.getCamera();
    try testing.expect(fitted.center != null);
    try testing.expect(fitted.zoom != null);

    try projection.setVisibleGeometry(testing.allocator, "{\"type\":\"LineString\",\"coordinates\":[[-10,-10],[10,10]]}", .{ .top = 0.0, .left = 0.0, .bottom = 0.0, .right = 0.0 });
    const geometry_fitted = try projection.getCamera();
    try testing.expect(geometry_fitted.center != null);
    try testing.expect(geometry_fitted.zoom != null);

    // A later map camera command never reaches the projection.
    _ = try map.updateCamera(.{ .camera = .{ .center = center, .zoom = 1.0 } });
    try waitForCameraCommands(&map);
    const frozen = try projection.getCamera();
    try testing.expectApproxEqAbs(geometry_fitted.zoom.?, frozen.zoom.?, 0.000001);
}

fn convertOnThread(projection: *maplibre.MapProjectionHandle, out_point: *maplibre.ScreenPoint, out_error: *?anyerror) void {
    out_point.* = projection.pixelForLatLng(center) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "standalone projection conversions run from another thread" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = viewport_extent, .height = viewport_extent });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    _ = try map.updateCamera(.{ .camera = .{ .center = center, .zoom = 10.0 } });
    var projection_future = try maplibre.MapProjectionHandle.create(&map);
    defer projection_future.deinit();
    var projection = try projection_future.wait(null);
    defer projection.close() catch @panic("projection close failed");

    var point = maplibre.ScreenPoint{ .x = 0.0, .y = 0.0 };
    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, convertOnThread, .{ &projection, &point, &thread_error });
    thread.join();
    try testing.expect(thread_error == null);
    try expectCenterPoint(point);
}

test "projected meters convert to and from lat lng" {
    const origin = maplibre.LatLng{ .latitude = 0.0, .longitude = 0.0 };
    const origin_meters = try maplibre.projectedMetersForLatLng(origin, null);
    try testing.expectApproxEqAbs(@as(f64, 0.0), origin_meters.northing, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, 0.0), origin_meters.easting, 0.000001);

    const meters = try maplibre.projectedMetersForLatLng(center, null);
    const roundtrip = try maplibre.latLngForProjectedMeters(meters, null);
    try expectLatLngApprox(center, roundtrip);
}

test "projection public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, map.setProjectionMode(.{ .x_skew = std.math.inf(f64) }));
    try testing.expectError(error.InvalidArgument, map.pixelForLatLng(.{ .latitude = 91.0, .longitude = 0.0 }));
    try testing.expectError(error.InvalidArgument, map.latLngForPixel(.{ .x = std.math.inf(f64), .y = 0.0 }));

    var projection_future = try maplibre.MapProjectionHandle.create(&map);

    defer projection_future.deinit();

    var projection = try projection_future.wait(null);
    defer projection.close() catch @panic("projection close failed");

    try testing.expectError(error.InvalidArgument, projection.setCamera(.{ .center = .{ .latitude = std.math.inf(f64), .longitude = 0.0 } }));
    try testing.expectError(error.InvalidArgument, projection.setVisibleCoordinates(testing.allocator, &.{}, .{}));
    try testing.expectError(error.InvalidArgument, projection.setVisibleCoordinates(testing.allocator, &.{center}, .{ .top = -1.0 }));
    try testing.expectError(error.InvalidArgument, projection.setVisibleGeometry(testing.allocator, "{", .{}));
    try testing.expectError(error.InvalidArgument, projection.setVisibleGeometry(testing.allocator, "{\"type\":\"Point\",\"coordinates\":[0,1e999]}", .{}));
    try testing.expectError(error.InvalidArgument, projection.pixelForLatLng(.{ .latitude = std.math.nan(f64), .longitude = 0.0 }));
    try testing.expectError(error.InvalidArgument, projection.latLngForPixel(.{ .x = 0.0, .y = std.math.inf(f64) }));
}

test "projection free helpers preserve native diagnostics" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    try testing.expectError(
        error.InvalidArgument,
        maplibre.projectedMetersForLatLng(.{ .latitude = std.math.inf(f64), .longitude = 0.0 }, &diagnostics),
    );
    const diagnostic = diagnostics.get().?;
    try testing.expectEqual(@as(?i32, -1), diagnostic.raw_status);
    try testing.expect(diagnostic.message.len > 0);

    try testing.expectError(error.InvalidArgument, maplibre.latLngForProjectedMeters(.{ .northing = std.math.nan(f64), .easting = 0.0 }, null));
}
