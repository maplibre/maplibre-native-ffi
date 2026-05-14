const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

fn stringView(value: []const u8) c.mln_string_view {
    return .{ .data = value.ptr, .size = value.len };
}

fn jsonString(value: []const u8) c.mln_json_value {
    return .{
        .size = @sizeOf(c.mln_json_value),
        .type = c.MLN_JSON_VALUE_TYPE_STRING,
        .data = .{ .string_value = stringView(value) },
    };
}

fn jsonDouble(value: f64) c.mln_json_value {
    return .{
        .size = @sizeOf(c.mln_json_value),
        .type = c.MLN_JSON_VALUE_TYPE_DOUBLE,
        .data = .{ .double_value = value },
    };
}

fn jsonBool(value: bool) c.mln_json_value {
    return .{
        .size = @sizeOf(c.mln_json_value),
        .type = c.MLN_JSON_VALUE_TYPE_BOOL,
        .data = .{ .bool_value = value },
    };
}

fn jsonArray(values: []const c.mln_json_value) c.mln_json_value {
    return .{
        .size = @sizeOf(c.mln_json_value),
        .type = c.MLN_JSON_VALUE_TYPE_ARRAY,
        .data = .{ .array_value = .{ .values = values.ptr, .value_count = values.len } },
    };
}

fn jsonObject(members: []const c.mln_json_member) c.mln_json_value {
    return .{
        .size = @sizeOf(c.mln_json_value),
        .type = c.MLN_JSON_VALUE_TYPE_OBJECT,
        .data = .{ .object_value = .{ .members = members.ptr, .member_count = members.len } },
    };
}

fn jsonMember(key: []const u8, value: *const c.mln_json_value) c.mln_json_member {
    return .{ .key = stringView(key), .value = value };
}

fn viewBytes(view: c.mln_string_view) []const u8 {
    return view.data[0..view.size];
}

fn listId(list: *c.mln_style_id_list, index: usize) ![]const u8 {
    var id: c.mln_string_view = .{ .data = null, .size = 0 };
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_style_id_list_get(list, index, &id));
    return viewBytes(id);
}

fn expectListContains(list: *c.mln_style_id_list, expected: []const u8) !void {
    var count: usize = 0;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_style_id_list_count(list, &count));
    for (0..count) |index| {
        if (std.mem.eql(u8, try listId(list, index), expected)) return;
    }
    return error.MissingListEntry;
}

fn listIndexOf(list: *c.mln_style_id_list, expected: []const u8) !usize {
    var count: usize = 0;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_style_id_list_count(list, &count));
    for (0..count) |index| {
        if (std.mem.eql(u8, try listId(list, index), expected)) return index;
    }
    return error.MissingListEntry;
}

fn expectObjectString(root: *const c.mln_json_value, key: []const u8, expected: []const u8) !void {
    try testing.expectEqual(c.MLN_JSON_VALUE_TYPE_OBJECT, root.type);
    const members = root.data.object_value.members[0..root.data.object_value.member_count];
    for (members) |member| {
        if (std.mem.eql(u8, viewBytes(member.key), key)) {
            try testing.expectEqual(c.MLN_JSON_VALUE_TYPE_STRING, member.value.*.type);
            try testing.expect(std.mem.eql(u8, viewBytes(member.value.*.data.string_value), expected));
            return;
        }
    }
    return error.MissingObjectMember;
}

fn snapshotRoot(snapshot: *c.mln_json_snapshot) !*const c.mln_json_value {
    var root: ?*const c.mln_json_value = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_json_snapshot_get(snapshot, &root));
    return root orelse error.MissingSnapshotRoot;
}

const CustomGeometryCallbackState = struct {
    fetch_count: usize = 0,
    cancel_count: usize = 0,
    last_tile: c.mln_canonical_tile_id = .{ .z = 0, .x = 0, .y = 0 },
};

fn customGeometryFetch(user_data: ?*anyopaque, tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    if (user_data == null) return;
    const state: *CustomGeometryCallbackState = @ptrCast(@alignCast(user_data.?));
    state.fetch_count += 1;
    state.last_tile = tile_id;
}

fn customGeometryCancel(user_data: ?*anyopaque, tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    if (user_data == null) return;
    const state: *CustomGeometryCallbackState = @ptrCast(@alignCast(user_data.?));
    state.cancel_count += 1;
    state.last_tile = tile_id;
}

fn emptyFeatureCollectionGeoJSON() c.mln_geojson {
    return .{
        .size = @sizeOf(c.mln_geojson),
        .type = c.MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
        .data = .{ .feature_collection = .{ .features = null, .feature_count = 0 } },
    };
}

test "style registry exposes primary source and layer ID APIs" {
    try support.suppressLogs();
    defer support.restoreLogs();

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, support.style_json));
    _ = try support.waitForEvent(runtime, map, c.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED);

    var source_ids: ?*c.mln_style_id_list = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_list_style_source_ids(map, &source_ids));
    defer c.mln_style_id_list_destroy(source_ids.?);
    var source_count: usize = 0;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_style_id_list_count(source_ids.?, &source_count));
    try testing.expect(source_count >= 1);
    try expectListContains(source_ids.?, "point");

    const feature_collection_type = jsonString("FeatureCollection");
    const empty_features = [_]c.mln_json_value{};
    const features = jsonArray(&empty_features);
    const data_members = [_]c.mln_json_member{
        jsonMember("type", &feature_collection_type),
        jsonMember("features", &features),
    };
    const data = jsonObject(&data_members);
    const source_type = jsonString("geojson");
    const source_members = [_]c.mln_json_member{
        jsonMember("type", &source_type),
        jsonMember("data", &data),
    };
    const source = jsonObject(&source_members);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_add_style_source_json(map, stringView("empty"), &source));

    var exists = false;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_style_source_exists(map, stringView("empty"), &exists));
    try testing.expect(exists);

    var found = false;
    var source_type_value: u32 = c.MLN_STYLE_SOURCE_TYPE_UNKNOWN;
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_get_style_source_type(map, stringView("empty"), &source_type_value, &found),
    );
    try testing.expect(found);
    try testing.expectEqual(c.MLN_STYLE_SOURCE_TYPE_GEOJSON, source_type_value);

    var info: c.mln_style_source_info = .{
        .size = @sizeOf(c.mln_style_source_info),
        .type = c.MLN_STYLE_SOURCE_TYPE_UNKNOWN,
        .id_size = 0,
        .is_volatile = false,
        .has_attribution = false,
        .attribution_size = 0,
    };
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_get_style_source_info(map, stringView("empty"), &info, &found),
    );
    try testing.expect(found);
    try testing.expectEqual(c.MLN_STYLE_SOURCE_TYPE_GEOJSON, info.type);
    try testing.expectEqual(@as(usize, "empty".len), info.id_size);
    try testing.expect(!info.has_attribution);

    const vector_type = jsonString("vector");
    const tile_url = jsonString("https://example.com/{z}/{x}/{y}.pbf");
    const tile_values = [_]c.mln_json_value{tile_url};
    const tiles = jsonArray(&tile_values);
    const attribution = jsonString("Example attribution");
    const vector_members = [_]c.mln_json_member{
        jsonMember("type", &vector_type),
        jsonMember("tiles", &tiles),
        jsonMember("attribution", &attribution),
    };
    const vector_source = jsonObject(&vector_members);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_add_style_source_json(map, stringView("vector-meta"), &vector_source));
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_get_style_source_info(map, stringView("vector-meta"), &info, &found),
    );
    try testing.expect(found);
    try testing.expectEqual(c.MLN_STYLE_SOURCE_TYPE_VECTOR, info.type);
    try testing.expect(info.has_attribution);
    try testing.expectEqual(@as(usize, "Example attribution".len), info.attribution_size);

    var attribution_buffer: [64]u8 = undefined;
    var attribution_size: usize = 0;
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_copy_style_source_attribution(
            map,
            stringView("vector-meta"),
            &attribution_buffer,
            attribution_buffer.len,
            &attribution_size,
            &found,
        ),
    );
    try testing.expect(found);
    try testing.expect(std.mem.eql(u8, attribution_buffer[0..attribution_size], "Example attribution"));

    const layer_id = jsonString("empty-circle");
    const layer_type = jsonString("circle");
    const layer_source = jsonString("empty");
    const layer_members = [_]c.mln_json_member{
        jsonMember("id", &layer_id),
        jsonMember("type", &layer_type),
        jsonMember("source", &layer_source),
    };
    const layer = jsonObject(&layer_members);

    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_add_style_layer_json(map, &layer, stringView("point-circle")),
    );

    var layer_ids: ?*c.mln_style_id_list = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_list_style_layer_ids(map, &layer_ids));
    defer c.mln_style_id_list_destroy(layer_ids.?);
    var layer_count: usize = 0;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_style_id_list_count(layer_ids.?, &layer_count));
    try testing.expect(layer_count >= 3);
    try testing.expect((try listIndexOf(layer_ids.?, "empty-circle")) < (try listIndexOf(layer_ids.?, "point-circle")));

    var layer_type_view: c.mln_string_view = .{ .data = null, .size = 0 };
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_get_style_layer_type(map, stringView("empty-circle"), &layer_type_view, &found),
    );
    try testing.expect(found);
    try testing.expect(std.mem.eql(u8, viewBytes(layer_type_view), "circle"));

    var layer_snapshot: ?*c.mln_json_snapshot = null;
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_get_style_layer_json(map, stringView("empty-circle"), &layer_snapshot, &found),
    );
    defer c.mln_json_snapshot_destroy(layer_snapshot.?);
    try testing.expect(found);
    try expectObjectString(try snapshotRoot(layer_snapshot.?), "id", "empty-circle");

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_move_style_layer(map, stringView("empty-circle"), stringView("")));

    var used_source_removed = false;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_STATE,
        c.mln_map_remove_style_source(map, stringView("empty"), &used_source_removed),
    );

    var layer_removed = false;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_remove_style_layer(map, stringView("empty-circle"), &layer_removed));
    try testing.expect(layer_removed);
    var source_removed = false;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_remove_style_source(map, stringView("empty"), &source_removed));
    try testing.expect(source_removed);
    var vector_source_removed = false;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_remove_style_source(map, stringView("vector-meta"), &vector_source_removed));
    try testing.expect(vector_source_removed);
}

test "style helper C ABI-only invalid descriptors remain covered" {
    try support.suppressLogs();
    defer support.restoreLogs();

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, support.style_json));
    _ = try support.waitForEvent(runtime, map, c.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED);

    var image_pixels = [_]u8{ 1, 2, 3, 4 };
    var image = c.mln_premultiplied_rgba8_image_default();
    try testing.expectEqual(@as(u32, 0), image.width);
    image.width = 1;
    image.height = 1;
    image.stride = 4;
    image.pixels = &image_pixels;
    image.byte_length = image_pixels.len;

    var image_options = c.mln_style_image_options_default();
    image_options.fields = c.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO | c.MLN_STYLE_IMAGE_OPTION_SDF;
    image_options.pixel_ratio = 2.0;
    image_options.sdf = true;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_image(map, stringView("runtime-icon"), &image, &image_options));

    var required: usize = 0;
    var found = false;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_copy_style_image_premultiplied_rgba8(map, stringView("runtime-icon"), null, 0, &required, &found),
    );
    try testing.expect(found);
    try testing.expectEqual(@as(usize, 4), required);

    var coordinates = [_]c.mln_lat_lng{
        .{ .latitude = 38.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -123.0 },
    };
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_add_image_source_url(map, stringView("image-url-source"), &coordinates, coordinates.len, stringView("https://example.com/image.png")),
    );

    var required_coordinates: usize = 0;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_get_image_source_coordinates(map, stringView("image-url-source"), null, 0, &required_coordinates, &found),
    );
    try testing.expect(found);
    try testing.expectEqual(@as(usize, 4), required_coordinates);
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_set_image_source_coordinates(map, stringView("image-url-source"), &coordinates, 3),
    );

    var options = c.mln_style_tile_source_options_default();
    options.fields = c.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
    options.raster_encoding = 99;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_add_raster_dem_source_url(map, stringView("bad-dem"), stringView("https://example.com/bad.json"), &options),
    );

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_add_location_indicator_layer(map, stringView("location"), stringView("point-circle")));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_location_indicator_image_name(map, stringView("location"), 99, stringView("bad")));
}
