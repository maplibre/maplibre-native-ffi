const std = @import("std");
const build_options = @import("build_options");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

test "Vulkan surface unsupported backend validates arguments" {
    if (build_options.supports_vulkan) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_vulkan_surface_descriptor_default();
    descriptor.instance = @ptrFromInt(1);
    descriptor.physical_device = @ptrFromInt(1);
    descriptor.device = @ptrFromInt(1);
    descriptor.graphics_queue = @ptrFromInt(1);
    descriptor.surface = @ptrFromInt(1);

    var surface: ?*c.mln_render_session = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &descriptor, &surface));
    try testing.expect(surface != null);

    surface = null;
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_vulkan_surface_attach(map, &descriptor, &surface));
    try testing.expectEqual(@as(?*c.mln_render_session, null), surface);
}

test "Vulkan surface attach rejects invalid arguments" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_vulkan_surface_descriptor_default();
    var surface: ?*c.mln_render_session = null;

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(null, &descriptor, &surface));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, null, &surface));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &descriptor, null));

    descriptor.instance = @ptrFromInt(1);
    descriptor.physical_device = @ptrFromInt(1);
    descriptor.device = @ptrFromInt(1);
    descriptor.graphics_queue = @ptrFromInt(1);
    descriptor.surface = @ptrFromInt(1);

    surface = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &descriptor, &surface));
    try testing.expect(surface != null);

    surface = null;
    var small_descriptor = descriptor;
    small_descriptor.size = @sizeOf(c.mln_vulkan_surface_descriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &small_descriptor, &surface));

    var invalid_descriptor = descriptor;
    invalid_descriptor.width = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &invalid_descriptor, &surface));

    invalid_descriptor = descriptor;
    invalid_descriptor.height = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &invalid_descriptor, &surface));

    invalid_descriptor = descriptor;
    invalid_descriptor.scale_factor = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &invalid_descriptor, &surface));

    descriptor = c.mln_vulkan_surface_descriptor_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_surface_attach(map, &descriptor, &surface));
    try testing.expectEqual(@as(?*c.mln_render_session, null), surface);
}
