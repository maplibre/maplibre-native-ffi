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
    const rendered = try session.queryRenderedFeatures(
        allocator,
        .{ .box = .{
            .min = .{ .x = point.x - 5, .y = point.y - 5 },
            .max = .{ .x = point.x + 5, .y = point.y + 5 },
        } },
        .{ .layer_ids = &.{"poi"} },
        null,
    );
    var hits = try rendered.await(null);
    defer hits.deinit();

    for (hits.value) |hit| {
        // hit.feature is the GeoJSON feature, as JSON.
        useFeature(hit.feature);
    }
    // #endregion rendered

    // #region source
    const source = try session.querySourceFeatures(
        allocator,
        "openmaptiles",
        .{
            .source_layer_ids = &.{"transportation"},
            // A filter from the MapLibre style spec, as JSON.
            .filter = "[\"==\", [\"get\", \"class\"], \"motorway\"]",
        },
        null,
    );
    var features = try source.await(null);
    defer features.deinit();

    for (features.value) |feature| {
        useFeature(feature.feature);
    }
    // #endregion source
}
