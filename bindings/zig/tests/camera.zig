const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");

const center = maplibre.LatLng{ .latitude = 37.7749, .longitude = -122.4194 };

const TransitionTally = struct {
    finished_count: usize = 0,
    last_transition_id: ?u64 = null,
    last_change_mode: ?maplibre.CameraChangeMode = null,
    change_followed_finish: bool = false,
};

/// Drains the queued runtime events and tallies what the camera events among
/// them reported.
fn drainedCameraEvents(runtime: *maplibre.RuntimeHandle) !TransitionTally {
    var tally = TransitionTally{};
    while (try runtime.pollEvent(testing.allocator)) |polled| {
        var event = polled;
        defer event.deinit();
        switch (event.event_type) {
            .map_camera_transition_finished => {
                try testing.expect(std.meta.eql(
                    event.payload_type,
                    maplibre.RuntimeEventPayloadType.camera_transition_finished,
                ));
                const payload = switch (event.payload) {
                    .camera_transition_finished => |value| value,
                    else => return error.UnexpectedPayload,
                };
                tally.finished_count += 1;
                tally.last_transition_id = payload.transition_id;
            },
            .map_camera_did_change => {
                tally.last_change_mode = maplibre.CameraChangeMode.fromRaw(event.code);
                if (tally.finished_count > 0) tally.change_followed_finish = true;
            },
            else => {},
        }
    }
    return tally;
}

test "camera jump updates snapshot fields through public binding" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.jumpTo(.{ .center = center, .zoom = 10.0 });

    const snapshot = try map.getCamera();
    try testing.expect(snapshot.center != null);
    try testing.expect(snapshot.zoom != null);
}

test "camera commands accept valid public descriptors" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
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

test "gesture in progress brackets host-driven camera commands" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try testing.expect(!try map.isGestureInProgress());

    try map.setGestureInProgress(true);
    try testing.expect(try map.isGestureInProgress());
    try map.moveBy(8, -4);
    try testing.expect(try map.isGestureInProgress());

    try map.setGestureInProgress(false);
    try testing.expect(!try map.isGestureInProgress());
}

test "zero-duration ease reports one transition finish ahead of an immediate camera change" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.easeTo(.{ .center = center, .zoom = 11.0 }, .{ .duration_ms = 0, .transition_id = 7 });

    const tally = try drainedCameraEvents(&runtime);
    try testing.expectEqual(@as(usize, 1), tally.finished_count);
    try testing.expectEqual(@as(?u64, 7), tally.last_transition_id);
    try testing.expect(tally.change_followed_finish);
    try testing.expect(std.meta.eql(tally.last_change_mode.?, maplibre.CameraChangeMode.immediate));
}

test "a superseded camera transition reports one transition finish with an animated camera change" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.easeTo(.{ .center = center, .zoom = 12.0 }, .{ .duration_ms = 5000, .transition_id = 11 });
    try testing.expectEqual(@as(usize, 0), (try drainedCameraEvents(&runtime)).finished_count);

    try map.easeTo(.{ .center = center, .zoom = 13.0 }, .{ .duration_ms = 5000, .transition_id = 12 });

    const tally = try drainedCameraEvents(&runtime);
    try testing.expectEqual(@as(usize, 1), tally.finished_count);
    try testing.expectEqual(@as(?u64, 11), tally.last_transition_id);
    try testing.expect(std.meta.eql(tally.last_change_mode.?, maplibre.CameraChangeMode.animated));
}

test "cancelling reports one transition finish for the transition that carries an ID" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.easeTo(.{ .center = center, .zoom = 12.0 }, .{ .duration_ms = 5000, .transition_id = 21 });
    try map.cancelTransitions();

    const cancelled = try drainedCameraEvents(&runtime);
    try testing.expectEqual(@as(usize, 1), cancelled.finished_count);
    try testing.expectEqual(@as(?u64, 21), cancelled.last_transition_id);

    try map.easeTo(.{ .center = center, .zoom = 14.0 }, .{ .duration_ms = 5000 });
    try map.cancelTransitions();

    try testing.expectEqual(@as(usize, 0), (try drainedCameraEvents(&runtime)).finished_count);
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

    const geometry_camera = try map.cameraForGeometry(testing.allocator, .{ .line_string = coordinates[0..] }, null);
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

fn jumpedLongitude(map: *maplibre.MapHandle, longitude: f64) !f64 {
    try map.jumpTo(.{ .center = .{ .latitude = 0, .longitude = longitude }, .zoom = 2.0 });
    const snapshot = try map.getCamera();
    return (snapshot.center orelse return error.MissingCameraCenter).longitude;
}

// The unbounded constraint is distinct from world bounds: an unbounded camera center pans across
// the antimeridian, while world bounds clamp longitude to the -180..180 range.
test "camera bounds separate the unbounded constraint from world bounds" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const pristine = try map.getBounds();
    try testing.expectEqual(.unbounded, std.meta.activeTag(pristine.bounds.?));
    try testing.expectApproxEqAbs(@as(f64, -160.0), try jumpedLongitude(&map, 200.0), 1e-6);

    try map.setBounds(.{ .bounds = .{ .bounded = .{
        .southwest = .{ .latitude = -90.0, .longitude = -180.0 },
        .northeast = .{ .latitude = 90.0, .longitude = 180.0 },
    } } });

    const world = try map.getBounds();
    switch (world.bounds.?) {
        .bounded => |bounds| try testing.expectApproxEqAbs(@as(f64, 180.0), bounds.northeast.longitude, 1e-6),
        .unbounded => return error.TestExpectedBoundedConstraint,
    }
    try testing.expectApproxEqAbs(@as(f64, 180.0), try jumpedLongitude(&map, 200.0), 1e-6);

    try map.setBounds(.{ .bounds = .unbounded });
    const released = try map.getBounds();
    try testing.expectEqual(.unbounded, std.meta.activeTag(released.bounds.?));
    try testing.expectApproxEqAbs(@as(f64, -160.0), try jumpedLongitude(&map, 200.0), 1e-6);
}

test "camera public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
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
