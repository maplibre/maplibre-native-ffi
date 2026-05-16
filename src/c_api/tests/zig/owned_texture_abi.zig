// Raw C ABI coverage: owned texture null/out-pointer and undersized descriptor validation are hidden by public Zig descriptors.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

test "owned texture attach rejects invalid arguments" {
    var texture: ?*c.mln_render_session = null;
    var descriptor = c.mln_owned_texture_descriptor_default();

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_owned_texture_attach(null, &descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_owned_texture_attach(map, null, &texture));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_owned_texture_attach(map, &descriptor, null));

    texture = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_owned_texture_attach(map, &descriptor, &texture));

    texture = null;
    var small_descriptor = descriptor;
    small_descriptor.size = @sizeOf(c.mln_owned_texture_descriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_owned_texture_attach(map, &small_descriptor, &texture));

    var invalid_descriptor = descriptor;
    invalid_descriptor.extent.size = @sizeOf(c.mln_render_target_extent) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_owned_texture_attach(map, &invalid_descriptor, &texture));
}

test "render session maintenance rejects null raw handles" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_reduce_memory_use(null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_clear_data(null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_dump_debug_logs(null));
}

test "render session rejects stale raw handles" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_owned_texture_descriptor_default();
    var texture: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_owned_texture_attach(map, &descriptor, &texture));
    const session = texture orelse return error.SessionAttachFailed;

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_destroy(session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_render_update(session));
}
