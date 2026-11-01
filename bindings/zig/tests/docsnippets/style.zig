//! Snippets for the docs site's Style page. They compile with the tests, which
//! never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &style;
    _ = &styleEvents;
}

const allocator = std.heap.smp_allocator;

var map: maplibre.Map = undefined;

fn drawImage(_: []const u8) maplibre.PremultipliedRgba8Image {
    return .{ .width = 1, .height = 1, .stride = 4, .pixels = &.{ 255, 0, 0, 255 } };
}

// pixels holds 16 x 16 premultiplied RGBA pixels.
fn style(pixels: []const u8) !void {
    // #region load
    try map.discarding().setStyleUrl("https://demotiles.maplibre.org/style.json", null);
    // #endregion load

    // #region add
    try map.discarding().addStyleSourceJson(
        "satellite",
        "{\"type\":\"raster\",\"tileSize\":256," ++
            "\"tiles\":[\"https://tiles.example.com/{z}/{x}/{y}.png\"]}",
        null,
    );

    // Draw the layer under the "countries-label" layer.
    try map.discarding().addStyleLayerJson(
        "{\"id\":\"satellite\",\"type\":\"raster\",\"source\":\"satellite\"}",
        "countries-label",
        null,
    );
    // #endregion add

    // #region geojson
    var data = try maplibre.GeoJsonSourceData.create(
        "{\"type\":\"Point\",\"coordinates\":[-122.4,37.8]}",
        null,
        null,
    );
    try map.discarding().addGeoJsonSourceData("points", data, null);
    // The source keeps its own reference to the data.
    data.release();

    // Later, prepare new data with the same options, and set it.
    var next = try maplibre.GeoJsonSourceData.create(
        "{\"type\":\"Point\",\"coordinates\":[2.35,48.86]}",
        null,
        null,
    );
    try map.discarding().setGeoJsonSourceData("points", next, null);
    next.release();
    // #endregion geojson

    // #region set-property
    // Show both changes in the same frame.
    try map.discarding().beginCommandGroup(null);
    try map.discarding().setStyleLayerProperty("satellite", "visibility", "\"none\"", null);
    try map.discarding().setStyleLayerProperty(
        "countries-boundary",
        "line-color",
        "[\"interpolate\",[\"linear\"],[\"zoom\"],5,\"#9cf\",12,\"#06c\"]",
        null,
    );
    try map.discarding().endCommandGroup(null);
    // #endregion set-property

    // #region image
    try map.discarding().setStyleImage(allocator, "marker", .{
        .width = 16,
        .height = 16,
        .stride = 16 * 4,
        .pixels = pixels,
    }, null, null);

    // An empty layer ID draws the layer on top.
    try map.discarding().addStyleLayerJson(
        "{\"id\":\"markers\",\"type\":\"symbol\",\"source\":\"points\"," ++
            "\"layout\":{\"icon-image\":\"marker\"}}",
        "",
        null,
    );
    // #endregion image
}

// #region missing-image
// Runs on a library thread.
fn onEvents(_: ?*anyopaque, events: []const maplibre.RuntimeEvent) anyerror!void {
    for (events) |event| {
        if (event.source != map.id() or event.type != .map_style_image_missing) continue;
        // The message holds the image's ID.
        try map.discarding().setStyleImage(allocator, event.message, drawImage(event.message), null, null);
    }
}
// #endregion missing-image

fn styleEvents(runtime: maplibre.Runtime) !void {
    try runtime.discarding().setEventHandler(.{ .callback = onEvents }, null);
}
