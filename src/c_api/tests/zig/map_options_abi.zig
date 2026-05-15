// Raw C ABI coverage: map option structs use null pointers, undersized structs, unknown raw masks/enums, and non-null out handles hidden by the Zig binding.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

fn testCamera() c.mln_camera_options {
    var camera = c.mln_camera_options_default();
    camera.fields = c.MLN_CAMERA_OPTION_CENTER | c.MLN_CAMERA_OPTION_ZOOM | c.MLN_CAMERA_OPTION_BEARING | c.MLN_CAMERA_OPTION_PITCH;
    camera.latitude = 37.7749;
    camera.longitude = -122.4194;
    camera.zoom = 11.0;
    camera.bearing = 12.0;
    camera.pitch = 30.0;
    return camera;
}

test "camera rejects invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_camera(map, null));

    var snapshot = c.mln_camera_options_default();
    snapshot.size = @sizeOf(c.mln_camera_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_camera(map, &snapshot));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_jump_to(map, null));

    var camera = testCamera();
    camera.size = @sizeOf(c.mln_camera_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_jump_to(map, &camera));

    camera = testCamera();
    camera.fields |= @as(u32, 1) << 31;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_jump_to(map, &camera));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_ease_to(map, null, null));

    camera = testCamera();
    var animation = c.mln_animation_options_default();
    animation.size = @sizeOf(c.mln_animation_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_ease_to(map, &camera, &animation));

    animation = c.mln_animation_options_default();
    animation.fields = @as(u32, 1) << 31;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_fly_to(map, &camera, &animation));
}

test "camera fitting rejects invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var camera = c.mln_camera_options_default();
    var fit = c.mln_camera_fit_options_default();
    fit.size = @sizeOf(c.mln_camera_fit_options) - 1;
    const valid_bounds = c.mln_lat_lng_bounds{
        .southwest = .{ .latitude = -10.0, .longitude = -10.0 },
        .northeast = .{ .latitude = 10.0, .longitude = 10.0 },
    };
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_camera_for_lat_lng_bounds(map, valid_bounds, &fit, &camera));

    const coordinate = c.mln_lat_lng{ .latitude = 0.0, .longitude = 0.0 };
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_camera_for_lat_lngs(map, null, 1, null, &camera));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_camera_for_lat_lngs(map, &coordinate, 1, null, null));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_camera_for_geometry(map, null, null, &camera));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_lat_lng_bounds_for_camera(map, null, null));
}

test "camera bounds constraints reject invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_bounds(map, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_bounds(map, null));

    var options = c.mln_bound_options_default();
    options.size = @sizeOf(c.mln_bound_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_bounds(map, &options));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_bounds(map, &options));

    options = c.mln_bound_options_default();
    options.fields = @as(u32, 1) << 31;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_bounds(map, &options));
}

test "free camera options reject raw invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_free_camera_options(map, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_free_camera_options(map, null));

    var options = c.mln_free_camera_options_default();
    options.size = @sizeOf(c.mln_free_camera_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_free_camera_options(map, &options));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_free_camera_options(map, &options));

    options = c.mln_free_camera_options_default();
    options.fields = @as(u32, 1) << 31;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_free_camera_options(map, &options));
}

const center = c.mln_lat_lng{ .latitude = 37.7749, .longitude = -122.4194 };

test "map projection mode rejects invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_projection_mode(map, null));

    var snapshot = c.mln_projection_mode_default();
    snapshot.size = @sizeOf(c.mln_projection_mode) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_projection_mode(map, &snapshot));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_projection_mode(map, null));

    var mode = c.mln_projection_mode_default();
    mode.size = @sizeOf(c.mln_projection_mode) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_projection_mode(map, &mode));

    mode = c.mln_projection_mode_default();
    mode.fields = @as(u32, 1) << 31;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_projection_mode(map, &mode));
}

test "map coordinate conversion rejects invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var point: c.mln_screen_point = undefined;
    var coordinate: c.mln_lat_lng = undefined;

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_pixel_for_lat_lng(map, center, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_lat_lng_for_pixel(map, .{ .x = 0.0, .y = 0.0 }, null));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_pixels_for_lat_lngs(map, null, 1, &point));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_lat_lngs_for_pixels(map, null, 1, &coordinate));
}

test "standalone projection rejects invalid arguments" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_create(map, null));

    var non_null_projection: ?*c.mln_map_projection = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_create(map, &non_null_projection));

    var projection: ?*c.mln_map_projection = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_projection_create(map, &projection));
    const helper = projection orelse return error.ProjectionCreateFailed;
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_projection_destroy(helper)) catch @panic("projection destroy failed");

    var camera = c.mln_camera_options_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_get_camera(helper, null));
    camera.size = @sizeOf(c.mln_camera_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_get_camera(helper, &camera));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_set_camera(helper, null));

    const padding = c.mln_edge_insets{ .top = 0.0, .left = 0.0, .bottom = 0.0, .right = 0.0 };
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_set_visible_coordinates(helper, null, 1, padding));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_set_visible_geometry(helper, null, padding));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_pixel_for_lat_lng(helper, center, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_projection_lat_lng_for_pixel(helper, .{ .x = 0.0, .y = 0.0 }, null));
}

test "projected meters reject invalid arguments" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_projected_meters_for_lat_lng(center, null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_lat_lng_for_projected_meters(.{ .northing = 0.0, .easting = 0.0 }, null));
}

test "map debug options reject raw invalid arguments" {
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
    options.fields = c.MLN_MAP_TILE_OPTION_LOD_MODE;
    options.lod_mode = 99;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_tile_options(map, &options));
}
