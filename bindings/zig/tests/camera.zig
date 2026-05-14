const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

const center = maplibre.LatLng{ .latitude = 37.7749, .longitude = -122.4194 };

fn createRuntimeAndMap() !struct { runtime: maplibre.RuntimeHandle, map: maplibre.MapHandle } {
    const runtime = try maplibre.RuntimeHandle.init(null);
    errdefer runtime.close() catch {};
    const map = try maplibre.MapHandle.create(runtime, .{});
    return .{ .runtime = runtime, .map = map };
}

test "camera jump updates snapshot fields through public binding" {
    const handles = try createRuntimeAndMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    try handles.map.jumpTo(.{ .center = center, .zoom = 10.0 });

    const snapshot = try handles.map.getCamera();
    try testing.expect(snapshot.center != null);
    try testing.expect(snapshot.zoom != null);
}

test "camera commands accept valid public descriptors" {
    const handles = try createRuntimeAndMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const anchor = maplibre.ScreenPoint{ .x = 256, .y = 256 };
    const rotate_start = maplibre.ScreenPoint{ .x = 200, .y = 200 };
    const rotate_end = maplibre.ScreenPoint{ .x = 220, .y = 210 };
    const animation = maplibre.AnimationOptions{
        .duration_ms = 0,
        .easing = .{ .x1 = 0.0, .y1 = 0.0, .x2 = 0.25, .y2 = 1.0 },
    };

    try handles.map.moveBy(4, -2);
    try handles.map.scaleBy(1.1, anchor);
    try handles.map.scaleBy(0.95, null);
    try handles.map.rotateBy(rotate_start, rotate_end);
    try handles.map.pitchBy(3);
    try handles.map.easeTo(.{ .center = center, .zoom = 12.0 }, animation);
    try handles.map.flyTo(.{ .center = center, .zoom = 10.0 }, animation);
    try handles.map.cancelTransitions();
}

test "camera fitting computes camera and visible bounds" {
    const handles = try createRuntimeAndMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    const bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 35.0, .longitude = -125.0 },
        .northeast = .{ .latitude = 39.0, .longitude = -120.0 },
    };
    const camera = try handles.map.cameraForLatLngBounds(bounds, .{
        .padding = .{ .top = 8, .left = 12, .bottom = 8, .right = 12 },
        .bearing = 5,
        .pitch = 15,
    });
    try testing.expect(camera.center != null);
    try testing.expect(camera.zoom != null);
    try testing.expect(camera.padding != null);
    try testing.expect(camera.bearing != null);
    try testing.expect(camera.pitch != null);

    const visible_bounds = try handles.map.latLngBoundsForCamera(camera);
    try testing.expect(visible_bounds.southwest.latitude <= visible_bounds.northeast.latitude);
}

test "camera public descriptors report invalid native arguments" {
    const handles = try createRuntimeAndMap();
    defer handles.runtime.close() catch @panic("runtime close failed");
    defer handles.map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidArgument, handles.map.jumpTo(.{ .center = .{ .latitude = std.math.inf(f64), .longitude = 0 } }));
    try testing.expectError(error.InvalidArgument, handles.map.moveBy(std.math.nan(f64), 0));
    try testing.expectError(error.InvalidArgument, handles.map.scaleBy(0, null));
    try testing.expectError(error.InvalidArgument, handles.map.rotateBy(.{ .x = std.math.inf(f64), .y = 0 }, .{ .x = 0, .y = 0 }));
    try testing.expectError(error.InvalidArgument, handles.map.pitchBy(std.math.nan(f64)));
}
