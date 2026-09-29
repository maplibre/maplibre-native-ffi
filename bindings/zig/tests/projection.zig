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

fn useAndCloseProjectionOnThread(
    projection: *maplibre.MapProjection,
    out_error: *?anyerror,
) void {
    _ = maplibre.mapProjectionGetCamera(support.handle(projection)) catch |err| {
        out_error.* = err;
        return;
    };
    maplibre.mapProjectionClose(support.handle(projection)) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "meters per pixel query observes camera commands and preserves detached scale" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .zoom = 3 } }));
    const projection = try support.resolve(maplibre.MapProjection, try maplibre.mapProjectionCreate(map));
    defer maplibre.mapProjectionClose(support.handle(projection)) catch @panic("projection close failed");
    const meters = try support.resolve(f64, try maplibre.mapMetersPerPixelAtLatitude(support.handle(map), 45));
    try testing.expectApproxEqAbs(meters, try maplibre.mapProjectionMetersPerPixelAtLatitude(support.handle(projection), 45), 1e-10);
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .zoom = 4 } }));
    try testing.expectApproxEqAbs(meters / 2, try support.resolve(f64, try maplibre.mapMetersPerPixelAtLatitude(support.handle(map), 45)), 1e-10);
    try testing.expectApproxEqAbs(meters, try maplibre.mapProjectionMetersPerPixelAtLatitude(support.handle(projection), 45), 1e-10);
}

test "map projection mode updates snapshot fields through public binding" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapSetProjectionMode(testing.allocator, support.handle(map), .{ .axonometric = true, .x_skew = 0.25, .y_skew = -0.125 }));
    try support.waitForBarrier(&runtime);

    const snapshot = (try maplibre.mapSnapshotGet(support.handle(map))).projection_mode;
    try testing.expectEqual(true, snapshot.axonometric.?);
    try testing.expectApproxEqAbs(@as(f64, 0.25), snapshot.x_skew.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, -0.125), snapshot.y_skew.?, 0.000001);
}

test "map converts between lat lngs and screen points" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = viewport_extent, .height = viewport_extent, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .center = center, .zoom = 10.0 } }));
    try support.waitForBarrier(&runtime);

    var point_future = try maplibre.mapPixelForLatLng(support.handle(map), center);
    defer point_future.deinit();
    const point = try point_future.wait(null);
    try expectCenterPoint(point);

    var coordinate_future = try maplibre.mapLatLngForPixel(support.handle(map), point);
    defer coordinate_future.deinit();
    const coordinate = try coordinate_future.wait(null);
    try expectLatLngApprox(center, coordinate);

    const coordinates = [_]maplibre.LatLng{
        center,
        .{ .latitude = 0.0, .longitude = 0.0 },
    };
    var points: [coordinates.len]maplibre.ScreenPoint = undefined;
    var points_future = try maplibre.mapPixelsForLatLngs(testing.allocator, support.handle(map), coordinates[0..]);
    defer points_future.deinit();
    var owned_points = try points_future.wait(null);
    defer owned_points.deinit();
    @memcpy(points[0..], owned_points.value);
    try expectCenterPoint(points[0]);

    var roundtrip: [points.len]maplibre.LatLng = undefined;
    var coordinates_future = try maplibre.mapLatLngsForPixels(testing.allocator, support.handle(map), points[0..]);
    defer coordinates_future.deinit();
    var owned_coordinates = try coordinates_future.wait(null);
    defer owned_coordinates.deinit();
    @memcpy(roundtrip[0..], owned_coordinates.value);
    try expectLatLngApprox(coordinates[0], roundtrip[0]);
    try expectLatLngApprox(coordinates[1], roundtrip[1]);

    var empty_points_future = try maplibre.mapPixelsForLatLngs(testing.allocator, support.handle(map), &.{});
    defer empty_points_future.deinit();
    var empty_points = try empty_points_future.wait(null);
    defer empty_points.deinit();
    try testing.expectEqual(@as(usize, 0), empty_points.value.len);

    var empty_coordinates_future = try maplibre.mapLatLngsForPixels(testing.allocator, support.handle(map), &.{});
    defer empty_coordinates_future.deinit();
    var empty_coordinates = try empty_coordinates_future.wait(null);
    defer empty_coordinates.deinit();
    try testing.expectEqual(@as(usize, 0), empty_coordinates.value.len);
}

test "unwrapped coordinate conversions preserve visible world copies" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 1024, .height = 512, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    var camera_future = try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .center = .{ .latitude = 0.0, .longitude = 180.0 }, .zoom = 0.0 } });
    defer camera_future.deinit();
    _ = try camera_future.wait(null);

    const points = [_]maplibre.ScreenPoint{
        .{ .x = 0.0, .y = 256.0 },
        .{ .x = 1024.0, .y = 256.0 },
    };

    var wrapped_future = try maplibre.mapLatLngsForPixels(testing.allocator, support.handle(map), points[0..]);
    defer wrapped_future.deinit();
    var wrapped = try wrapped_future.wait(null);
    defer wrapped.deinit();
    for (wrapped.value) |coordinate| {
        try testing.expect(coordinate.longitude >= -180.0);
        try testing.expect(coordinate.longitude <= 180.0);
    }

    var unwrapped_future = try maplibre.mapLatLngsForPixelsUnwrapped(testing.allocator, support.handle(map), points[0..]);
    defer unwrapped_future.deinit();
    var unwrapped = try unwrapped_future.wait(null);
    defer unwrapped.deinit();
    try testing.expect(unwrapped.value[1].longitude - unwrapped.value[0].longitude > 360.0);

    var wrapped_right_future = try maplibre.mapLatLngForPixel(support.handle(map), points[1]);
    defer wrapped_right_future.deinit();
    const wrapped_right = try wrapped_right_future.wait(null);
    try testing.expect(wrapped_right.longitude >= -180.0);
    try testing.expect(wrapped_right.longitude <= 180.0);

    var right_future = try maplibre.mapLatLngForPixelUnwrapped(support.handle(map), points[1]);
    defer right_future.deinit();
    const right = try right_future.wait(null);
    try expectLatLngApprox(unwrapped.value[1], right);

    var projection_future = try maplibre.mapProjectionCreate(map);
    defer projection_future.deinit();
    const projection = try projection_future.wait(null);
    defer maplibre.mapProjectionClose(support.handle(projection)) catch @panic("projection close failed");

    const projected_wrapped_right = try maplibre.mapProjectionLatLngForPixel(support.handle(projection), points[1]);
    try testing.expect(projected_wrapped_right.longitude >= -180.0);
    try testing.expect(projected_wrapped_right.longitude <= 180.0);
    const projected_right = try maplibre.mapProjectionLatLngForPixelUnwrapped(support.handle(projection), points[1]);
    try expectLatLngApprox(right, projected_right);
}

// Creation is ordered after every earlier map command, so a projection created
// right after a camera command observes that command without an explicit wait.
test "standalone projection converts and updates camera" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = viewport_extent, .height = viewport_extent, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .center = center, .zoom = 10.0 } }));
    var projection_future = try maplibre.mapProjectionCreate(map);
    defer projection_future.deinit();
    const projection = try projection_future.wait(null);
    defer maplibre.mapProjectionClose(support.handle(projection)) catch @panic("projection close failed");

    const point = try maplibre.mapProjectionPixelForLatLng(support.handle(projection), center);
    try expectCenterPoint(point);

    const coordinate = try maplibre.mapProjectionLatLngForPixel(support.handle(projection), point);
    try expectLatLngApprox(center, coordinate);

    // A setter is applied before it returns, so later conversions observe it.
    const helper_camera = maplibre.CameraOptions{ .center = .{ .latitude = 0.0, .longitude = 0.0 }, .zoom = 3.0 };
    try maplibre.mapProjectionSetCamera(testing.allocator, support.handle(projection), helper_camera);
    const snapshot = try maplibre.mapProjectionGetCamera(support.handle(projection));
    try expectLatLngApprox(helper_camera.center.?, snapshot.center.?);
    try testing.expectApproxEqAbs(helper_camera.zoom.?, snapshot.zoom.?, 0.000001);
    const recentered = try maplibre.mapProjectionPixelForLatLng(support.handle(projection), helper_camera.center.?);
    try expectCenterPoint(recentered);

    var visible = [_]maplibre.LatLng{
        .{ .latitude = -10.0, .longitude = -10.0 },
        .{ .latitude = 10.0, .longitude = 10.0 },
    };
    try maplibre.mapProjectionSetVisibleCoordinates(testing.allocator, support.handle(projection), visible[0..], .{ .top = 10.0, .left = 20.0, .bottom = 10.0, .right = 20.0 });
    const fitted = try maplibre.mapProjectionGetCamera(support.handle(projection));
    try testing.expect(fitted.center != null);
    try testing.expect(fitted.zoom != null);

    try maplibre.mapProjectionSetVisibleGeometry(support.handle(projection), "{\"type\":\"LineString\",\"coordinates\":[[-10,-10],[10,10]]}", .{ .top = 0.0, .left = 0.0, .bottom = 0.0, .right = 0.0 });
    const geometry_fitted = try maplibre.mapProjectionGetCamera(support.handle(projection));
    try testing.expect(geometry_fitted.center != null);
    try testing.expect(geometry_fitted.zoom != null);

    // A later map camera command never reaches the projection.
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .center = center, .zoom = 1.0 } }));
    try support.waitForBarrier(&runtime);
    const frozen = try maplibre.mapProjectionGetCamera(support.handle(projection));
    try testing.expectApproxEqAbs(geometry_fitted.zoom.?, frozen.zoom.?, 0.000001);
}

fn convertOnThread(projection: *maplibre.MapProjection, out_point: *maplibre.ScreenPoint, out_error: *?anyerror) void {
    out_point.* = maplibre.mapProjectionPixelForLatLng(support.handle(projection), center) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "standalone projection conversions run from another thread" {
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = viewport_extent, .height = viewport_extent, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .center = center, .zoom = 10.0 } }));
    var projection_future = try maplibre.mapProjectionCreate(map);
    defer projection_future.deinit();
    var projection = try projection_future.wait(null);
    defer maplibre.mapProjectionClose(support.handle(projection)) catch @panic("projection close failed");

    var point = maplibre.ScreenPoint{ .x = 0.0, .y = 0.0 };
    var thread_error: ?anyerror = error.Unexpected;
    const thread = try std.Thread.spawn(.{}, convertOnThread, .{ &projection, &point, &thread_error });
    thread.join();
    try testing.expect(thread_error == null);
    try expectCenterPoint(point);
}

test "standalone projection remains usable on another thread" {
    var runtime = try support.createRuntime(.{});
    var map = try support.createMap(&runtime, .{});
    var projection_future = try maplibre.mapProjectionCreate(map);
    defer projection_future.deinit();
    var projection = try projection_future.wait(null);
    try support.closeMap(&map);
    try support.closeRuntime(&runtime);

    var thread_error: ?anyerror = null;
    const thread = try std.Thread.spawn(
        .{},
        useAndCloseProjectionOnThread,
        .{ &projection, &thread_error },
    );
    thread.join();
    try testing.expect(thread_error == null);
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
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{});
    defer support.closeMap(&map) catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, maplibre.mapSetProjectionMode(testing.allocator, support.handle(map), .{ .x_skew = std.math.inf(f64) }));
    try testing.expectError(error.InvalidArgument, maplibre.mapPixelForLatLng(support.handle(map), .{ .latitude = 91.0, .longitude = 0.0 }));
    try testing.expectError(error.InvalidArgument, maplibre.mapLatLngForPixel(support.handle(map), .{ .x = std.math.inf(f64), .y = 0.0 }));

    var projection_future = try maplibre.mapProjectionCreate(map);

    defer projection_future.deinit();

    const projection = try projection_future.wait(null);
    defer maplibre.mapProjectionClose(support.handle(projection)) catch @panic("projection close failed");

    try testing.expectError(error.InvalidArgument, maplibre.mapProjectionSetCamera(testing.allocator, support.handle(projection), .{ .center = .{ .latitude = std.math.inf(f64), .longitude = 0.0 } }));
    try testing.expectError(error.InvalidArgument, maplibre.mapProjectionSetVisibleCoordinates(testing.allocator, support.handle(projection), &.{}, .{}));
    try testing.expectError(error.InvalidArgument, maplibre.mapProjectionSetVisibleCoordinates(testing.allocator, support.handle(projection), &.{center}, .{ .top = -1.0 }));
    try testing.expectError(error.InvalidArgument, maplibre.mapProjectionSetVisibleGeometry(support.handle(projection), "{", .{}));
    try testing.expectError(error.InvalidArgument, maplibre.mapProjectionSetVisibleGeometry(support.handle(projection), "{\"type\":\"Point\",\"coordinates\":[0,1e999]}", .{}));
    try testing.expectError(error.InvalidArgument, maplibre.mapProjectionPixelForLatLng(support.handle(projection), .{ .latitude = std.math.nan(f64), .longitude = 0.0 }));
    try testing.expectError(error.InvalidArgument, maplibre.mapProjectionLatLngForPixel(support.handle(projection), .{ .x = 0.0, .y = std.math.inf(f64) }));
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
