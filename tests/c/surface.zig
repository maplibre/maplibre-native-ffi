const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

test "surface descriptors expose defaults" {
    const metal = c.mln_metal_surface_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_metal_surface_descriptor)), metal.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), metal.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_metal_context_descriptor)), metal.context.size);
    try testing.expect(metal.extent.width > 0);
    try testing.expect(metal.extent.height > 0);
    try testing.expect(metal.extent.scale_factor > 0);
    try testing.expect(metal.layer == null);
    try testing.expect(metal.context.device == null);

    const vulkan = c.mln_vulkan_surface_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_vulkan_surface_descriptor)), vulkan.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), vulkan.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_vulkan_context_descriptor)), vulkan.context.size);
    try testing.expect(vulkan.extent.width > 0);
    try testing.expect(vulkan.extent.height > 0);
    try testing.expect(vulkan.extent.scale_factor > 0);
    try testing.expect(vulkan.context.instance == null);
    try testing.expect(vulkan.context.physical_device == null);
    try testing.expect(vulkan.context.device == null);
    try testing.expect(vulkan.context.graphics_queue == null);
    try testing.expect(vulkan.surface == null);
}
