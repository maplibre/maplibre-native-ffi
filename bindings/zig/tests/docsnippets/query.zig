//! Snippets for the docs site's Query page. They compile with the tests, which
//! never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &query;
}

fn useFeature(_: []const u8) void {}

fn query(allocator: std.mem.Allocator, session: maplibre.RenderSession, point: maplibre.ScreenPoint) !void {
    // #region rendered
    var rendered = try session.queryRenderedFeatures(allocator, .{ .box = .{
        .min = .{ .x = point.x - 5, .y = point.y - 5 },
        .max = .{ .x = point.x + 5, .y = point.y + 5 },
    } }, .{ .layer_ids = &.{"poi"} }, null);
    defer rendered.deinit();
    var hits = try rendered.wait(null);
    defer hits.deinit();

    for (hits.value) |hit| {
        // hit.feature is the GeoJSON feature, as JSON.
        useFeature(hit.feature);
    }
    // #endregion rendered

    // #region source
    var source = try session.querySourceFeatures(allocator, "openmaptiles", .{
        .source_layer_ids = &.{"transportation"},
        // A filter from the MapLibre style spec, as JSON.
        .filter = "[\"==\", [\"get\", \"class\"], \"motorway\"]",
    }, null);
    defer source.deinit();
    var features = try source.wait(null);
    defer features.deinit();

    for (features.value) |feature| {
        useFeature(feature.feature);
    }
    // #endregion source
}
