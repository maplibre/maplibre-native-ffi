//! Snippets for the docs site's Create a map page. They compile with the
//! tests, which never run them.

const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &createAMap;
}

fn reportFailure(message: []const u8) void {
    _ = message;
}

/// Attaches a render target, as the render target guides show.
fn attachTarget(map: maplibre.Map) !maplibre.RenderSession {
    _ = map;
    return error.Unimplemented;
}

fn createAMap() !void {
    // #region runtime
    var runtime = try maplibre.Runtime.create(null, null);
    // #endregion runtime

    // #region map
    const map_options = maplibre.MapOptions{
        .initial_extent = .{
            .width = 800,
            .height = 600,
            .scale_factor = 2.0,
        },
        .map_mode = .continuous,
    };

    const created = try runtime.createMap(map_options, null);
    var map = created.map;
    try created.ready.await(null);
    // #endregion map

    // #region style
    try map.discarding().setStyleUrl(
        "https://demotiles.maplibre.org/style.json",
        null,
    );
    // #endregion style

    const update = maplibre.CameraUpdate{
        .mode = .jump,
        .camera = .{ .zoom = 3 },
    };

    // #region wait
    _ = try (try map.updateCamera(update, null)).await(null);

    // Use the discarding variant to ignore the result.
    try map.discarding().updateCamera(update, null);
    // #endregion wait

    // #region errors
    var diagnostic: maplibre.Diagnostic = .{};
    const jumped = map.updateCamera(update, &diagnostic) catch |err| {
        // The call failed right away.
        reportFailure(diagnostic.message());
        return err;
    };
    _ = jumped.await(&diagnostic) catch |err| {
        // The result reports a failure.
        reportFailure(diagnostic.message());
        return err;
    };
    // #endregion errors

    var session = try attachTarget(map);

    // #region close
    try session.dispose(null);
    try map.dispose(null);
    try (try runtime.release(null)).await(null);
    // #endregion close
}
