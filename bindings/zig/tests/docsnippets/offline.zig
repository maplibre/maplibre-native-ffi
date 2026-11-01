//! Snippets for the docs site's Offline page. They compile with the tests,
//! which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &offline;
}

fn recordProgress(_: i64, _: u64, _: u64, _: bool) void {}

fn recordError(_: i64, _: []const u8) void {}

fn recordRegion(_: i64, _: []const u8) void {}

// #region progress
fn onEvents(_: ?*anyopaque, events: []const maplibre.RuntimeEvent) anyerror!void {
    for (events) |event| {
        switch (event.payload) {
            .offline_region_status => |payload| recordProgress(
                payload.region_id,
                payload.status.completed_resource_count,
                payload.status.required_resource_count,
                payload.status.complete,
            ),
            .offline_region_response_error => |payload| recordError(payload.region_id, event.message),
            else => {},
        }
    }
}
// #endregion progress

fn offline(allocator: std.mem.Allocator) !void {
    // #region cache-file
    var runtime = try maplibre.Runtime.create(.{ .cache_path = "maplibre-cache.db" }, null);
    // #endregion cache-file

    // #region progress
    try runtime.discarding().setEventHandler(.{ .callback = onEvents }, null);
    // #endregion progress

    // #region ambient
    // 50 MiB
    var resized = try runtime.setMaximumAmbientCacheSize(50 * 1024 * 1024, null);
    defer resized.deinit();
    try resized.wait(null);

    var cleared = try runtime.runAmbientCacheOperation(.clear, null);
    defer cleared.deinit();
    try cleared.wait(null);
    // #endregion ambient

    // #region download
    var created = try runtime.createOfflineRegion(allocator, .{ .tile_pyramid = .{
        .style_url = "https://demotiles.maplibre.org/style.json",
        .bounds = .{
            .southwest = .{ .latitude = 47.5, .longitude = -122.5 },
            .northeast = .{ .latitude = 47.8, .longitude = -122.2 },
        },
        .min_zoom = 0,
        .max_zoom = 6,
        .pixel_ratio = 1,
    } }, "Seattle", null);
    defer created.deinit();
    var region = try created.wait(null);
    defer region.deinit();

    var activated = try runtime.setOfflineRegionDownloadState(region.value.id, .active, null);
    defer activated.deinit();
    try activated.wait(null);
    // #endregion download

    // #region progress
    var observed = try runtime.setOfflineRegionObserved(region.value.id, true, null);
    defer observed.deinit();
    try observed.wait(null);
    // #endregion progress

    // #region manage
    var listed = try runtime.listOfflineRegions(allocator, null);
    defer listed.deinit();
    var regions = try listed.wait(null);
    defer regions.deinit();
    for (regions.value) |stored| recordRegion(stored.id, stored.metadata);

    // Resume a download that stopped when the runtime closed.
    var resumed = try runtime.setOfflineRegionDownloadState(region.value.id, .active, null);
    defer resumed.deinit();
    try resumed.wait(null);

    var invalidated = try runtime.invalidateOfflineRegion(region.value.id, null);
    defer invalidated.deinit();
    try invalidated.wait(null);

    var deleted = try runtime.deleteOfflineRegion(region.value.id, null);
    defer deleted.deinit();
    try deleted.wait(null);
    // #endregion manage

    var released = try runtime.release(null);
    defer released.deinit();
    try released.wait(null);
}
