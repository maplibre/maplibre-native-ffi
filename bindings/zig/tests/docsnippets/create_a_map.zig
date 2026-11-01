//! Snippets for the docs site's Create a map page. They compile with the
//! tests, which never run them.

const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &createAMap;
}

/// Attaches a render target, as the Rendering guides show.
fn attachTarget(map: maplibre.Map) !maplibre.RenderSession {
    _ = map;
    return error.Unimplemented;
}

fn createAMap() !void {
    // #region runtime
    var runtime = try maplibre.Runtime.create(.{}, null);
    // #endregion runtime

    // #region map
    const map_options = maplibre.MapOptions{
        .initial_extent = .{ .width = 800, .height = 600, .scale_factor = 2.0 },
        .map_mode = .continuous,
    };

    var created = try runtime.createMap(map_options, null);
    defer created.ready.deinit();
    var map = created.map;
    try created.ready.wait(null);
    // #endregion map

    var session = try attachTarget(map);

    // #region close
    try session.dispose(null);

    var map_released = try map.release(null);
    defer map_released.deinit();
    try map_released.wait(null);

    var runtime_released = try runtime.release(null);
    defer runtime_released.deinit();
    try runtime_released.wait(null);
    // #endregion close
}
