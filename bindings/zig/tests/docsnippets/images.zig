//! Snippets for the docs site's Render images page. They compile with the
//! tests, which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &images;
}

/// The center of tile z/x/y. The snippet leaves the tile math to the reader.
fn tileCenter(z: u32, x: u32, y: u32) maplibre.LatLng {
    _ = .{ z, x, y };
    return .{};
}

fn copyPixels(data: []const u8, stride: u32) void {
    _ = .{ data, stride };
}

fn images(allocator: std.mem.Allocator, runtime: maplibre.Runtime, z: u32, x: u32, y: u32) !void {
    // #region map
    const map_options = maplibre.MapOptions{
        .initial_extent = .{ .width = 512, .height = 512, .scale_factor = 1.0 },
        .map_mode = .tile,
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
    // null options select the default adapter.
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

    // #region image
    try map.discarding().updateCamera(.{
        .mode = .jump,
        .camera = .{ .center = tileCenter(z, x, y), .zoom = @floatFromInt(z) },
    }, null);

    var still = try map.requestStillImage(null);
    defer still.deinit();
    try still.wait(null);
    // #endregion image

    // #region read
    var read = try session.readTexture(allocator, null);
    defer read.deinit();
    var image = try read.wait(null);
    defer image.deinit();
    copyPixels(image.value.data, image.value.info.stride);
    // #endregion read

    // #region close
    try session.dispose(null);
    device.release();

    var map_released = try map.release(null);
    defer map_released.deinit();
    try map_released.wait(null);
    // #endregion close
}
