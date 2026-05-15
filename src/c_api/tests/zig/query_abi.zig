// Raw C ABI coverage: undersized query descriptors, unknown option masks, and invalid string-view pointers are hidden by the Zig binding.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

fn attachTextureSession(map: *c.mln_map) !*c.mln_render_session {
    var descriptor = c.mln_owned_texture_descriptor_default();
    descriptor.extent.width = 64;
    descriptor.extent.height = 64;

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_owned_texture_attach(map, &descriptor, &session));
    return session orelse error.SessionAttachFailed;
}

test "feature query validation rejects raw descriptor shapes" {
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

    var rendered_options = c.mln_rendered_feature_query_options_default();
    rendered_options.fields = @as(u32, 1) << 31;
    geometry = c.mln_rendered_query_geometry_point(.{ .x = 256.0, .y = 256.0 });
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_query_rendered_features(session, &geometry, &rendered_options, &result));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_query_source_features(session, .{ .data = null, .size = 1 }, null, &result));
}
