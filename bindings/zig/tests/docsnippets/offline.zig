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
fn onEvents(
    _: ?*anyopaque,
    events: []const maplibre.RuntimeEvent,
) anyerror!void {
    for (events) |event| {
        switch (event.payload) {
            .offline_region_status => |payload| recordProgress(
                payload.region_id,
                payload.status.completed_resource_count,
                payload.status.required_resource_count,
                payload.status.complete,
            ),
            .offline_region_response_error => |payload| recordError(
                payload.region_id,
                event.message,
            ),
            else => {},
        }
    }
}
// #endregion progress

fn offline(allocator: std.mem.Allocator) !void {
    // #region cache-file
    var runtime = try maplibre.Runtime.create(
        .{ .cache_path = "maplibre-cache.db" },
        null,
    );
    // #endregion cache-file

    // #region progress
    try runtime.discarding().setEventHandler(
        .{ .callback = onEvents },
        null,
    );
    // #endregion progress

    // #region ambient
    // 50 MiB
    const resized = try runtime.setMaximumAmbientCacheSize(
        50 * 1024 * 1024,
        null,
    );
    try resized.await(null);

    try (try runtime.runCacheDatabaseOperation(.clear, null)).await(null);
    // #endregion ambient

    // #region download
    const bounds = maplibre.LatLngBounds{
        .southwest = .{ .latitude = 47.5, .longitude = -122.5 },
        .northeast = .{ .latitude = 47.8, .longitude = -122.2 },
    };
    const created = try runtime.createOfflineRegion(allocator, .{
        .tile_pyramid = .{
            .style_url = "https://demotiles.maplibre.org/style.json",
            .bounds = bounds,
            .min_zoom = 0,
            .max_zoom = 6,
            .pixel_ratio = 1,
        },
    }, "Seattle", null);
    var region = try created.await(null);
    defer region.deinit();

    const activated = try runtime.setOfflineRegionDownloadState(
        region.value.id,
        .active,
        null,
    );
    try activated.await(null);
    // #endregion download

    // #region progress
    const observed = try runtime.setOfflineRegionObserved(
        region.value.id,
        maplibre.RuntimeEventFlag.all_runtime_events,
        null,
    );
    try observed.await(null);
    // #endregion progress

    // #region manage
    var regions = try (try runtime.listOfflineRegions(allocator, null)).await(null);
    defer regions.deinit();
    for (regions.value) |stored| {
        recordRegion(stored.id, stored.metadata);
    }

    // Resume a download that stopped when the runtime was released.
    const resumed = try runtime.setOfflineRegionDownloadState(
        region.value.id,
        .active,
        null,
    );
    try resumed.await(null);

    const invalidated =
        try runtime.invalidateOfflineRegion(region.value.id, null);
    try invalidated.await(null);

    const deleted =
        try runtime.deleteOfflineRegion(region.value.id, null);
    try deleted.await(null);
    // #endregion manage

    try (try runtime.release(null)).await(null);
}
