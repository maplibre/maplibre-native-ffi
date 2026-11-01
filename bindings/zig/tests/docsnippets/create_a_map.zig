//! Snippets for the docs site's Create a map page. They compile with the
//! tests, which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &createAMap;
}

fn copyPixels(data: []const u8, stride: u32) void {
    _ = data;
    _ = stride;
}

fn createAMap(allocator: std.mem.Allocator) !void {
    // #region runtime
    var runtime = try maplibre.Runtime.create(.{}, null);
    // #endregion runtime

    // #region map
    const map_options = maplibre.MapOptions{
        .initial_extent = .{ .width = 512, .height = 512, .scale_factor = 1.0 },
        .map_mode = .static,
    };

    var created = try runtime.createMap(map_options, null);
    defer created.ready.deinit();
    var map = created.map;
    try created.ready.wait(null);
    // #endregion map

    // #region style
    try map.discarding().setStyleUrl("https://demotiles.maplibre.org/style.json", null);
    // #endregion style

    // #region attach
    // null selects the default adapter.
    var device = try maplibre.GraphicsDevice.create(null, null);

    var attached = try map.attachGraphicsDeviceTexture(
        device,
        .{ .extent = map_options.initial_extent },
        // The library renders on its own thread.
        .{ .driver = .core_worker },
        null,
    );
    defer attached.ready.deinit();
    var session = attached.session;
    try attached.ready.wait(null);
    // #endregion attach

    // #region render
    var rendered = try map.requestStillImage(null);
    defer rendered.deinit();
    try rendered.wait(null);
    // #endregion render

    // #region render
    var read = try session.readTexture(allocator, null);
    defer read.deinit();
    var image = try read.wait(null);
    defer image.deinit();
    // #endregion render

    // #region render
    copyPixels(image.value.data, image.value.info.stride);
    // #endregion render

    // #region close
    try session.dispose(null);
    device.release();

    var map_released = try map.release(null);
    defer map_released.deinit();
    try map_released.wait(null);

    var runtime_released = try runtime.release(null);
    defer runtime_released.deinit();
    try runtime_released.wait(null);
    // #endregion close
}
