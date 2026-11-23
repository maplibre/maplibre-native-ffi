//! Snippets for the docs site's Camera page. They compile with the tests,
//! which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &camera;
}

const allocator = std.heap.smp_allocator;

fn useCamera(_: maplibre.Camera) void {}

fn flyEnded(_: bool) void {}

// #region animate
// Runs on a library thread.
fn onFlyEnd(
    _: ?*anyopaque,
    end: maplibre.CameraTransitionEnd,
) anyerror!void {
    flyEnded(end.outcome == .completed);
}
// #endregion animate

// #region gesture
fn createRecognizer(map: maplibre.Map) !maplibre.GestureRecognizer {
    return map.gestureRecognizerCreate(allocator, null, null);
}

fn scheduleTick(result: maplibre.GestureResult) void {
    _ = result; // On the next frame, or at result.next_tick_ms.
}

fn handleInput(
    recognizer: maplibre.GestureRecognizer,
    input: maplibre.GestureInput,
    time_ms: f64,
) !void {
    var result = try recognizer.handle(allocator, input, time_ms, null);
    defer result.deinit();
    scheduleTick(result.value);
}

fn pointerDown(
    recognizer: maplibre.GestureRecognizer,
    id: u32,
    position: maplibre.ScreenPoint,
    time_ms: f64,
) !void {
    try handleInput(recognizer, .{ .pointer_down = .{
        .id = id,
        .kind = .touch,
        .position = position,
    } }, time_ms);
}

fn pointerMove(
    recognizer: maplibre.GestureRecognizer,
    id: u32,
    position: maplibre.ScreenPoint,
    time_ms: f64,
) !void {
    try handleInput(recognizer, .{ .pointer_move = .{
        .id = id,
        .position = position,
    } }, time_ms);
}

fn tick(recognizer: maplibre.GestureRecognizer, time_ms: f64) !void {
    var result = try recognizer.tick(allocator, time_ms, null);
    defer result.deinit();
    scheduleTick(result.value);
}
// #endregion gesture

fn camera(map: maplibre.Map) !void {
    // #region read
    const snapshot = try map.getSnapshot(null);
    useCamera(snapshot.view.camera);

    const result = try (try map.getCamera(null)).await(null);
    useCamera(result.camera);
    // #endregion read

    // #region jump
    try map.discarding().updateCamera(.{
        .mode = .jump,
        .camera = .{
            .center = .{ .latitude = 48.8566, .longitude = 2.3522 },
            .zoom = 12,
        },
    }, null);
    // #endregion jump

    // #region animate
    try map.discarding().updateCamera(.{
        .mode = .fly,
        .camera = .{
            .center = .{ .latitude = 40.7128, .longitude = -74.0060 },
            .zoom = 10,
        },
        .animation = .{
            .duration_ms = 3000,
            .end_handler = .{ .callback = onFlyEnd },
        },
    }, null);
    // #endregion animate

    // #region fit
    const bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 45.8, .longitude = 5.9 },
        .northeast = .{ .latitude = 47.8, .longitude = 10.5 },
    };
    try map.discarding().fitCameraToLatLngBounds(bounds, .{
        .mode = .ease,
        .animation = .{ .duration_ms = 500 },
    }, null);
    // #endregion fit

    // #region limit
    try map.discarding().setBounds(.{
        .bounds = bounds,
        .min_zoom = 5,
        .max_zoom = 16,
    }, null);
    // #endregion limit

    var recognizer = try createRecognizer(map);
    defer recognizer.release();
    try pointerDown(recognizer, 1, .{ .x = 100, .y = 100 }, 0);
    try pointerMove(recognizer, 1, .{ .x = 112, .y = 96 }, 16);
    try tick(recognizer, 32);

    // #region convert
    const view = (try map.getSnapshot(null)).view;
    var projection = try maplibre.MapProjection.create(view, null);
    defer projection.release();

    const coordinate =
        try projection.latLngForPixel(.{ .x = 100, .y = 200 }, null);
    const point = try projection.pixelForLatLng(coordinate, null);
    // #endregion convert
    _ = point;
}
