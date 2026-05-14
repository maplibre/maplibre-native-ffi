const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

extern fn usleep(useconds: c_uint) c_int;

const query_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-c-query-test",
    \\  "sources": {
    \\    "point": {
    \\      "type": "geojson",
    \\      "data": {
    \\        "type": "FeatureCollection",
    \\        "features": [
    \\          {
    \\            "type": "Feature",
    \\            "id": "feature-1",
    \\            "geometry": {"type": "Point", "coordinates": [-122.4194, 37.7749]},
    \\            "properties": {"kind": "capital", "visible": true}
    \\          }
    \\        ]
    \\      }
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id": "background", "type": "background", "paint": {"background-color": "#d8f1ff"}},
    \\    {"id": "point-circle", "type": "circle", "source": "point", "paint": {"circle-color": "#f97316", "circle-radius": 12}}
    \\  ]
    \\}
;

const cluster_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-c-cluster-query-test",
    \\  "sources": {
    \\    "cluster-source": {
    \\      "type": "geojson",
    \\      "cluster": true,
    \\      "data": {
    \\        "type": "FeatureCollection",
    \\        "features": [
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one"}},
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two"}},
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three"}}
    \\        ]
    \\      }
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#ffffff"}},
    \\    {"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-color":"#2563eb","circle-radius":20}}
    \\  ]
    \\}
;

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

fn jsonUint(value: u64) c.mln_json_value {
    return .{
        .size = @sizeOf(c.mln_json_value),
        .type = c.MLN_JSON_VALUE_TYPE_UINT,
        .data = .{ .uint_value = value },
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

fn attachTextureSession(map: *c.mln_map) !*c.mln_render_session {
    var descriptor = c.mln_owned_texture_descriptor_default();
    descriptor.extent.width = 64;
    descriptor.extent.height = 64;

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_owned_texture_attach(map, &descriptor, &session));
    return session orelse error.SessionAttachFailed;
}

fn loadStyleAndRender(runtime: *c.mln_runtime, map: *c.mln_map, session: *c.mln_render_session) !void {
    var camera = c.mln_camera_options_default();
    camera.fields = c.MLN_CAMERA_OPTION_CENTER | c.MLN_CAMERA_OPTION_ZOOM;
    camera.latitude = 37.7749;
    camera.longitude = -122.4194;
    camera.zoom = 10.0;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_jump_to(map, &camera));

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, query_style_json));
    for (0..5) |_| {
        if (!try support.waitForEvent(runtime, map, c.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE)) break;
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_render_update(session));
    }
}

fn loadClusterStyleAndRender(runtime: *c.mln_runtime, map: *c.mln_map, session: *c.mln_render_session) !void {
    var camera = c.mln_camera_options_default();
    camera.fields = c.MLN_CAMERA_OPTION_CENTER | c.MLN_CAMERA_OPTION_ZOOM;
    camera.latitude = 0.0;
    camera.longitude = 0.0;
    camera.zoom = 0.0;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_jump_to(map, &camera));

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, cluster_style_json));
    for (0..5) |_| {
        if (!try support.waitForEvent(runtime, map, c.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE)) break;
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_render_update(session));
    }
}

fn renderPendingUpdates(runtime: *c.mln_runtime, map: *c.mln_map, session: *c.mln_render_session) !void {
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_run_once(runtime));
    for (0..100) |_| {
        var event = support.emptyEvent();
        var has_event = false;
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_poll_event(runtime, &event, &has_event));
        if (!has_event) return;
        if (event.type == c.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE and
            event.source_type == c.MLN_RUNTIME_EVENT_SOURCE_MAP and
            event.source == @as(?*anyopaque, @ptrCast(map)))
        {
            const status = c.mln_render_session_render_update(session);
            if (status != c.MLN_STATUS_INVALID_STATE) {
                try testing.expectEqual(c.MLN_STATUS_OK, status);
            }
        }
    }
}

fn waitForRenderedFeature(runtime: *c.mln_runtime, map: *c.mln_map, session: *c.mln_render_session, geometry: *const c.mln_rendered_query_geometry, options: *const c.mln_rendered_feature_query_options) !*c.mln_feature_query_result {
    for (0..1000) |_| {
        var result: ?*c.mln_feature_query_result = null;
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_query_rendered_features(session, geometry, options, &result));

        var count: usize = 0;
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_feature_query_result_count(result.?, &count));
        if (count == 1) return result.?;

        c.mln_feature_query_result_destroy(result.?);
        try renderPendingUpdates(runtime, map, session);
        _ = usleep(1000);
    }
    return error.RenderedFeatureNotQueryable;
}

fn waitForSourceFeature(runtime: *c.mln_runtime, map: *c.mln_map, session: *c.mln_render_session, source_id: c.mln_string_view, options: *const c.mln_source_feature_query_options) !*c.mln_feature_query_result {
    for (0..1000) |_| {
        var result: ?*c.mln_feature_query_result = null;
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_query_source_features(session, source_id, options, &result));

        var count: usize = 0;
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_feature_query_result_count(result.?, &count));
        if (count == 1) return result.?;

        c.mln_feature_query_result_destroy(result.?);
        try renderPendingUpdates(runtime, map, session);
        _ = usleep(1000);
    }
    return error.SourceFeatureNotQueryable;
}

fn expectFeatureKind(feature: *const c.mln_queried_feature, expected: []const u8) !void {
    const properties = feature.feature.properties[0..feature.feature.property_count];
    for (properties) |property| {
        if (std.mem.eql(u8, viewBytes(property.key), "kind")) {
            const value = property.value.?;
            try testing.expectEqual(c.MLN_JSON_VALUE_TYPE_STRING, value.*.type);
            try testing.expect(std.mem.eql(u8, viewBytes(value.*.data.string_value), expected));
            return;
        }
    }
    return error.MissingFeatureKind;
}

test "feature query ABI structs import" {
    const rendered_options = c.mln_rendered_feature_query_options_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_rendered_feature_query_options)), rendered_options.size);
    try testing.expectEqual(@as(u32, 0), rendered_options.fields);

    const source_options = c.mln_source_feature_query_options_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_source_feature_query_options)), source_options.size);
    try testing.expectEqual(@as(u32, 0), source_options.fields);

    const geometry = c.mln_rendered_query_geometry_point(.{ .x = 256.0, .y = 256.0 });
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_rendered_query_geometry)), geometry.size);
    try testing.expectEqual(c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT, geometry.type);
}

test "feature query validation" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);
    const session = try attachTextureSession(map);
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(session)) catch @panic("session destroy failed");

    var result: ?*c.mln_feature_query_result = null;
    var geometry = c.mln_rendered_query_geometry_point(.{ .x = 256.0, .y = 256.0 });
    geometry.size = @sizeOf(c.mln_rendered_query_geometry) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_query_rendered_features(session, &geometry, null, &result));

    geometry = c.mln_rendered_query_geometry_point(.{ .x = std.math.inf(f64), .y = 0.0 });
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_query_rendered_features(session, &geometry, null, &result));

    var rendered_options = c.mln_rendered_feature_query_options_default();
    rendered_options.fields = @as(u32, 1) << 31;
    geometry = c.mln_rendered_query_geometry_point(.{ .x = 256.0, .y = 256.0 });
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_query_rendered_features(session, &geometry, &rendered_options, &result));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_query_source_features(session, .{ .data = null, .size = 1 }, null, &result));
}
