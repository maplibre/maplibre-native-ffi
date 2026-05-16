// Raw C ABI/backend coverage: backend attach APIs expose unsafe pointer, size, and output-handle cases that the Zig binding cannot construct.

const testing = @import("std").testing;
const support = @import("support.zig");
const c = support.c;

pub fn expectAttachRejectsUnsafeInputs(comptime Backend: type) !void {
    var texture: ?*c.mln_render_session = null;
    var descriptor = Backend.descriptor();

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(null, &descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(map, null, &texture));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(map, &descriptor, null));

    texture = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(map, &descriptor, &texture));

    texture = null;
    var small_descriptor = Backend.descriptor();
    small_descriptor.size = Backend.descriptor_size - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(map, &small_descriptor, &texture));

    var invalid_descriptor = Backend.descriptor();
    invalid_descriptor.extent.size = @sizeOf(c.mln_render_target_extent) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(map, &invalid_descriptor, &texture));

    invalid_descriptor = Backend.descriptor();
    Backend.shrinkContext(&invalid_descriptor);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(map, &invalid_descriptor, &texture));

    invalid_descriptor = Backend.descriptor();
    Backend.clearRequiredHandle(&invalid_descriptor);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, Backend.attach(map, &invalid_descriptor, &texture));
}
