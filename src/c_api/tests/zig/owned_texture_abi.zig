// Raw C ABI coverage: owned texture null/out-pointer and undersized descriptor validation are hidden by public Zig descriptors.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

test "owned texture attach rejects invalid arguments" {
    var texture: ?*c.mln_render_session = null;
    var descriptor = support.defaultOwnedTextureDescriptor();

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, support.callOwnedTextureAttach(null, &descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, support.callOwnedTextureAttach(map, null, &texture));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, support.callOwnedTextureAttach(map, &descriptor, null));

    texture = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, support.callOwnedTextureAttach(map, &descriptor, &texture));

    texture = null;
    var small_descriptor = descriptor;
    small_descriptor.size = @sizeOf(support.OwnedTextureDescriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, support.callOwnedTextureAttach(map, &small_descriptor, &texture));

    var invalid_descriptor = descriptor;
    invalid_descriptor.extent.size = @sizeOf(c.mln_render_target_extent) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, support.callOwnedTextureAttach(map, &invalid_descriptor, &texture));
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

    var context = try support.OwnedTextureAttachContext.init();
    defer context.deinit();
    var descriptor = support.ownedTextureDescriptor(&context);
    const session = try support.attachOwnedTextureSession(map, &descriptor);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_destroy(session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_render_update(session));
}
