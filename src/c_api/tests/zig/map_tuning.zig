const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

test "map tuning exposes default options" {
    const viewport = c.mln_map_viewport_options_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_map_viewport_options)), viewport.size);
    try testing.expectEqual(@as(u32, 0), viewport.fields);
    try testing.expectEqual(@as(u32, c.MLN_NORTH_ORIENTATION_UP), viewport.north_orientation);
    try testing.expectEqual(@as(u32, c.MLN_CONSTRAIN_MODE_HEIGHT_ONLY), viewport.constrain_mode);
    try testing.expectEqual(@as(u32, c.MLN_VIEWPORT_MODE_DEFAULT), viewport.viewport_mode);

    const tile = c.mln_map_tile_options_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_map_tile_options)), tile.size);
    try testing.expectEqual(@as(u32, 0), tile.fields);
    try testing.expectEqual(@as(u32, c.MLN_TILE_LOD_MODE_DEFAULT), tile.lod_mode);
}

test "map debug options reject invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var out_options: u32 = 0;
    var out_bool = false;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_debug_options(null, 0));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_debug_options(map, @as(u32, 1) << 31));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_debug_options(map, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_debug_options(null, &out_options));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_rendering_stats_view_enabled(null, true));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_rendering_stats_view_enabled(map, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_rendering_stats_view_enabled(null, &out_bool));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_is_fully_loaded(map, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_is_fully_loaded(null, &out_bool));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_dump_debug_logs(null));
}

test "map viewport options reject invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_viewport_options(map, null));

    var options = c.mln_map_viewport_options_default();
    options.size = @sizeOf(c.mln_map_viewport_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_viewport_options(map, &options));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, &options));

    options = c.mln_map_viewport_options_default();
    options.fields = @as(u32, 1) << 31;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, &options));

    options = c.mln_map_viewport_options_default();
    options.fields = c.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
    options.north_orientation = 99;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, &options));

    options = c.mln_map_viewport_options_default();
    options.fields = c.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
    options.constrain_mode = 99;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, &options));

    options = c.mln_map_viewport_options_default();
    options.fields = c.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
    options.viewport_mode = 99;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, &options));

    options = c.mln_map_viewport_options_default();
    options.fields = c.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
    options.frustum_offset.top = std.math.inf(f64);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, &options));

    options = c.mln_map_viewport_options_default();
    options.fields = c.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
    options.frustum_offset.left = -1.0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_viewport_options(map, &options));
}

test "map tile options reject invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_tile_options(map, null));

    var options = c.mln_map_tile_options_default();
    options.size = @sizeOf(c.mln_map_tile_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_tile_options(map, &options));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_tile_options(map, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_tile_options(map, &options));

    options = c.mln_map_tile_options_default();
    options.fields = @as(u32, 1) << 31;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_tile_options(map, &options));

    options = c.mln_map_tile_options_default();
    options.fields = c.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
    options.prefetch_zoom_delta = 256;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_tile_options(map, &options));

    options = c.mln_map_tile_options_default();
    options.fields = c.MLN_MAP_TILE_OPTION_LOD_SCALE;
    options.lod_scale = std.math.nan(f64);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_tile_options(map, &options));

    options = c.mln_map_tile_options_default();
    options.fields = c.MLN_MAP_TILE_OPTION_LOD_MODE;
    options.lod_mode = 99;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_tile_options(map, &options));
}
