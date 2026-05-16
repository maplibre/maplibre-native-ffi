const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

const center = maplibre.LatLng{ .latitude = 37.7749, .longitude = -122.4194 };

fn expectCenterPoint(point: maplibre.ScreenPoint) !void {
    try testing.expectApproxEqAbs(@as(f64, 256.0), point.x, 0.001);
    try testing.expectApproxEqAbs(@as(f64, 256.0), point.y, 0.001);
}

fn expectLatLngApprox(expected: maplibre.LatLng, actual: maplibre.LatLng) !void {
    try testing.expectApproxEqAbs(expected.latitude, actual.latitude, 0.000001);
    try testing.expectApproxEqAbs(expected.longitude, actual.longitude, 0.000001);
}

test "map projection mode updates snapshot fields through public binding" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setProjectionMode(.{ .axonometric = true, .x_skew = 0.25, .y_skew = -0.125 });

    const snapshot = try map.getProjectionMode();
    try testing.expectEqual(true, snapshot.axonometric.?);
    try testing.expectApproxEqAbs(@as(f64, 0.25), snapshot.x_skew.?, 0.000001);
    try testing.expectApproxEqAbs(@as(f64, -0.125), snapshot.y_skew.?, 0.000001);
}

test "map converts between lat lngs and screen points" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.jumpTo(.{ .center = center, .zoom = 10.0 });

    const point = try map.pixelForLatLng(center);
    try expectCenterPoint(point);

    const coordinate = try map.latLngForPixel(point);
    try expectLatLngApprox(center, coordinate);

    const coordinates = [_]maplibre.LatLng{
        center,
        .{ .latitude = 0.0, .longitude = 0.0 },
    };
    var points: [coordinates.len]maplibre.ScreenPoint = undefined;
    try map.pixelsForLatLngs(testing.allocator, coordinates[0..], points[0..]);
    try expectCenterPoint(points[0]);

    var roundtrip: [points.len]maplibre.LatLng = undefined;
    try map.latLngsForPixels(testing.allocator, points[0..], roundtrip[0..]);
    try expectLatLngApprox(coordinates[0], roundtrip[0]);
    try expectLatLngApprox(coordinates[1], roundtrip[1]);

    try map.pixelsForLatLngs(testing.allocator, &.{}, &.{});
    try map.latLngsForPixels(testing.allocator, &.{}, &.{});
}

test "standalone projection converts and updates camera" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.jumpTo(.{ .center = center, .zoom = 10.0 });
    var projection = try maplibre.MapProjectionHandle.create(&map);
    defer projection.close() catch @panic("projection close failed");

    const point = try projection.pixelForLatLng(center);
    try expectCenterPoint(point);

    const coordinate = try projection.latLngForPixel(point);
    try expectLatLngApprox(center, coordinate);

    const helper_camera = maplibre.CameraOptions{ .center = .{ .latitude = 0.0, .longitude = 0.0 }, .zoom = 3.0 };
    try projection.setCamera(helper_camera);
    const snapshot = try projection.getCamera();
    try expectLatLngApprox(helper_camera.center.?, snapshot.center.?);
    try testing.expectApproxEqAbs(helper_camera.zoom.?, snapshot.zoom.?, 0.000001);

    var visible = [_]maplibre.LatLng{
        .{ .latitude = -10.0, .longitude = -10.0 },
        .{ .latitude = 10.0, .longitude = 10.0 },
    };
    try projection.setVisibleCoordinates(testing.allocator, visible[0..], .{ .top = 10.0, .left = 20.0, .bottom = 10.0, .right = 20.0 });
    const fitted = try projection.getCamera();
    try testing.expect(fitted.center != null);
    try testing.expect(fitted.zoom != null);

    try projection.setVisibleGeometry(testing.allocator, .{ .line_string = visible[0..] }, .{ .top = 0.0, .left = 0.0, .bottom = 0.0, .right = 0.0 });
    const geometry_fitted = try projection.getCamera();
    try testing.expect(geometry_fitted.center != null);
    try testing.expect(geometry_fitted.zoom != null);
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
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, map.setProjectionMode(.{ .x_skew = std.math.inf(f64) }));
    try testing.expectError(error.InvalidArgument, map.pixelForLatLng(.{ .latitude = 91.0, .longitude = 0.0 }));
    try testing.expectError(error.InvalidArgument, map.latLngForPixel(.{ .x = std.math.inf(f64), .y = 0.0 }));

    var projection = try maplibre.MapProjectionHandle.create(&map);
    defer projection.close() catch @panic("projection close failed");

    try testing.expectError(error.InvalidArgument, projection.setCamera(.{ .center = .{ .latitude = std.math.inf(f64), .longitude = 0.0 } }));
    try testing.expectError(error.InvalidArgument, projection.setVisibleCoordinates(testing.allocator, &.{}, .{}));
    try testing.expectError(error.InvalidArgument, projection.setVisibleCoordinates(testing.allocator, &.{center}, .{ .top = -1.0 }));
    try testing.expectError(error.InvalidArgument, projection.setVisibleGeometry(testing.allocator, .empty, .{}));
    try testing.expectError(error.InvalidArgument, projection.setVisibleGeometry(testing.allocator, .{ .point = .{ .latitude = std.math.inf(f64), .longitude = 0.0 } }, .{}));
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
