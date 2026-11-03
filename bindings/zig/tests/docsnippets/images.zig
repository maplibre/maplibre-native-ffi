//! Snippets for the docs site's Render images page. They compile with the
//! tests, which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &images;
    _ = &imagesTile;
}

fn copyPixels(data: []const u8, stride: u32) void {
    _ = .{ data, stride };
}

fn images(allocator: std.mem.Allocator, runtime: maplibre.Runtime) !void {
    // #region map
    const map_options = maplibre.MapOptions{
        .initial_extent = .{
            .width = 800,
            .height = 600,
            .scale_factor = 2.0,
        },
        .map_mode = .static,
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

    // #region attach
    const attached = try map.attachOffscreen(
        .{ .logical_size = .{ .width = 800, .height = 600 } },
        // The library renders on its own thread.
        .{ .driver = .core_worker },
        // With no device, the session creates its own.
        null,
        null,
    );
    var session = attached.session;
    try attached.ready.await(null);
    // #endregion attach

    // #region image
    try map.discarding().updateCamera(.{
        .mode = .jump,
        .camera = .{
            .center = .{ .latitude = 48.8566, .longitude = 2.3522 },
            .zoom = 4,
        },
    }, null);

    try (try map.requestStillImage(null)).await(null);
    // #endregion image

    // #region read
    var image = try (try session.readTexture(allocator, null)).await(null);
    defer image.deinit();
    copyPixels(image.value.data, image.value.info.stride);
    // #endregion read

    try session.dispose(null);

    try (try map.release(null)).await(null);
}

fn imagesTile(runtime: maplibre.Runtime, z: u32, x: u32, y: u32) !void {
    // #region tile
    const created = try runtime.createMap(.{
        .initial_extent = .{
            .width = 512,
            .height = 512,
            .scale_factor = 1.0,
        },
        .map_mode = .tile,
    }, null);
    var map = created.map;
    try created.ready.await(null);

    try map.discarding().updateCamera(.{
        .mode = .jump,
        .camera = try maplibre.cameraForTile(
            .{ .z = z, .x = x, .y = y },
            .{ .width = 512, .height = 512 },
            null,
        ),
    }, null);
    // #endregion tile

    try (try map.release(null)).await(null);
}
