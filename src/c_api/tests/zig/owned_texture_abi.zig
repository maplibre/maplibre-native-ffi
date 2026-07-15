// Raw C ABI coverage: render-session null and stale handles are hidden by public Zig handle state.

const testing = @import("std").testing;
const support = @import("support.zig");
const c = support.c;

// PRUNING REVIEW: KEEP.
// This verifies maintenance entry points reject null session handles that bindings prevent.
test "render session maintenance rejects null raw handles" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_reduce_memory_use(null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_clear_data(null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_render_session_dump_debug_logs(null));
}

// PRUNING REVIEW: KEEP.
// This verifies use-after-destroy protection in the native session handle registry.
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
