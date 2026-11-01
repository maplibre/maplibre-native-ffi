//! Snippets for the docs site's Network page. They compile with the tests,
//! which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &network;
}

fn withApiKey(url: []const u8, buffer: []u8) ![]const u8 {
    const separator = if (std.mem.indexOfScalar(u8, url, '?') == null) "?" else "&";
    return std.fmt.bufPrint(buffer, "{s}{s}key=my-api-key", .{ url, separator });
}

fn findTile(_: []const u8) ?[]const u8 {
    return null;
}

fn geojsonForTile(_: maplibre.CanonicalTileId) []const u8 {
    return "{\"type\":\"FeatureCollection\",\"features\":[]}";
}

// #region rewrite
fn addApiKey(_: ?*anyopaque, _: maplibre.ResourceKind, url: []const u8, request: maplibre.ResourceTransformRequest) anyerror!void {
    var owned = request;
    var buffer: [4096]u8 = undefined;
    // Returning an error releases the request, which keeps the old URL.
    try owned.complete(try withApiKey(url, &buffer), null);
}
// #endregion rewrite

// #region provider
fn serveTile(_: ?*anyopaque, request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) anyerror!void {
    var owned = handle;
    if (findTile(request.requested_url)) |tile| {
        try owned.complete(.{ .status = .ok, .bytes = tile }, null);
    } else {
        try owned.complete(.{ .status = .no_content }, null);
    }
}
// #endregion provider

// #region custom-source
fn fetchTile(map: *const maplibre.Map, tile: maplibre.CanonicalTileId) !void {
    try map.discarding().setCustomGeometrySourceTileData("app-data", tile, geojsonForTile(tile), null);
}
// #endregion custom-source

fn network(allocator: std.mem.Allocator, runtime: maplibre.Runtime, map: *const maplibre.Map) !void {
    // #region headers
    try runtime.discarding().setHttpHeaderTransform(allocator, .{ .rules = &.{.{
        .flags = .{ .glob = true },
        .url = "https://tiles.example.com/**",
        .headers = &.{.{ .name = "Authorization", .value = "Bearer my-token" }},
    }} }, null);
    // #endregion headers

    // #region rewrite
    try runtime.discarding().setResourceTransform(.{ .callback = addApiKey }, null);
    // #endregion rewrite

    // #region provider
    try runtime.discarding().setResourceProvider(allocator, .{
        .callback = serveTile,
        .routes = &.{.{
            .kinds = .{ .tile = true },
            .flags = .{ .glob = true },
            .url = "app://tiles/**",
        }},
    }, null);
    // #endregion provider

    // #region custom-source
    // The source calls fetchTile with map, which must outlive the source.
    try map.discarding().addCustomGeometrySource("app-data", .init(map, fetchTile, null, null), null);
    // #endregion custom-source

    // #region status
    try maplibre.setNetworkStatus(.offline, null);

    // Later, when the connection returns:
    try maplibre.setNetworkStatus(.online, null);
    // #endregion status
}
