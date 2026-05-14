const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

fn stringView(value: []const u8) c.mln_string_view {
    return .{ .data = value.ptr, .size = value.len };
}

fn featureStateSelector(source_id: []const u8, feature_id: []const u8) c.mln_feature_state_selector {
    return .{
        .size = @sizeOf(c.mln_feature_state_selector),
        .fields = c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
        .source_id = stringView(source_id),
        .source_layer_id = .{ .data = null, .size = 0 },
        .feature_id = stringView(feature_id),
        .state_key = .{ .data = null, .size = 0 },
    };
}

test "feature state ABI structs import" {
    const selector = featureStateSelector("point", "feature-1");
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_feature_state_selector)), selector.size);
    try testing.expectEqual(@as(u32, c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID), selector.fields);
}

test "feature state selector validation" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_owned_texture_descriptor_default();
    descriptor.extent.width = 64;
    descriptor.extent.height = 64;

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_owned_texture_attach(map, &descriptor, &session));
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(session.?)) catch @panic("session destroy failed");

    var selector = featureStateSelector("point", "feature-1");
    selector.fields = c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
    selector.state_key = stringView("hover");
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_remove_feature_state(session.?, &selector));
}
