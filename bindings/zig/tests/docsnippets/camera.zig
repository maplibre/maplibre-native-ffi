//! Snippets for the docs site's Camera page. They compile with the tests,
//! which never run them.

const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &camera;
}

fn useCamera(_: maplibre.Camera) void {}

fn flyEnded(_: bool) void {}

// #region animate
// Runs on a library thread.
fn onFlyEnd(_: ?*anyopaque, end: maplibre.CameraTransitionEnd) anyerror!void {
    flyEnded(end.outcome == .completed);
}
// #endregion animate

// #region gesture
fn beginGesture(map: maplibre.Map) !void {
    try map.discarding().cancelTransitions(null);
    try map.discarding().applyCameraDelta(.{ .gesture_phase = .begin }, null);
}

fn applyDrag(map: maplibre.Map, movement: maplibre.ScreenPoint) !void {
    try map.discarding().applyCameraDelta(.{
        .offset = movement,
        .gesture_phase = .update,
    }, null);
}

fn applyPinch(map: maplibre.Map, scale: f64, center: maplibre.ScreenPoint) !void {
    try map.discarding().applyCameraDelta(.{
        .scale = scale,
        .anchor = center,
        .gesture_phase = .update,
    }, null);
}

fn endGesture(map: maplibre.Map) !void {
    try map.discarding().applyCameraDelta(.{ .gesture_phase = .end }, null);
}
// #endregion gesture

fn camera(map: maplibre.Map) !void {
    // #region read
    const snapshot = try map.getSnapshot(null);
    useCamera(snapshot.view.camera);

    var query = try map.getCamera(null);
    defer query.deinit();
    const result = try query.wait(null);
    useCamera(result.camera);
    // #endregion read

    // #region jump
    try map.discarding().updateCamera(.{
        .mode = .jump,
        .camera = .{ .center = .{ .latitude = 48.8566, .longitude = 2.3522 }, .zoom = 12 },
    }, null);
    // #endregion jump

    // #region animate
    try map.discarding().updateCamera(.{
        .mode = .fly,
        .camera = .{ .center = .{ .latitude = 40.7128, .longitude = -74.0060 }, .zoom = 10 },
        .animation = .{ .duration_ms = 3000, .end_handler = .{ .callback = onFlyEnd } },
    }, null);
    // #endregion animate

    // #region fit
    const bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 45.8, .longitude = 5.9 },
        .northeast = .{ .latitude = 47.8, .longitude = 10.5 },
    };
    // null fit options use the defaults.
    var fit = try map.cameraForLatLngBounds(bounds, null, null);
    defer fit.deinit();
    const fitted = try fit.wait(null);

    try map.discarding().updateCamera(.{
        .mode = .ease,
        .camera = fitted,
        .animation = .{ .duration_ms = 500 },
    }, null);
    // #endregion fit

    // #region limit
    try map.discarding().setBounds(.{ .bounds = bounds, .min_zoom = 5, .max_zoom = 16 }, null);
    // #endregion limit

    try beginGesture(map);
    try applyDrag(map, .{ .x = 12, .y = -4 });
    try applyPinch(map, 1.1, .{ .x = 200, .y = 150 });
    try endGesture(map);

    // #region convert
    var projection = try maplibre.MapProjection.create((try map.getSnapshot(null)).view, null);
    defer projection.release();

    const coordinate = try projection.latLngForPixel(.{ .x = 100, .y = 200 }, null);
    const point = try projection.pixelForLatLng(coordinate, null);
    // #endregion convert
    _ = point;
}
