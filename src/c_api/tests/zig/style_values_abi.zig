// Raw C ABI coverage: malformed image-source coordinate counts and unknown raw raster enum values that typed bindings cannot construct.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

fn stringView(value: []const u8) c.mln_string_view {
    return .{ .data = value.ptr, .size = value.len };
}

// PRUNING REVIEW: KEEP.
// This verifies malformed coordinate counts and unknown raster encoding that typed bindings prevent before calling C.
test "style value helpers reject unsafe raw descriptors" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var coordinates = [_]c.mln_lat_lng{
        .{ .latitude = 38.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -123.0 },
    };
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_set_image_source_coordinates(map, stringView("image-url-source"), &coordinates, 3),
    );

    var tile_options = c.mln_style_tile_source_options_default();
    tile_options.fields = c.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
    tile_options.raster_encoding = 99;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_add_raster_dem_source_url(map, stringView("bad-dem"), stringView("https://example.com/bad.json"), &tile_options),
    );
}
