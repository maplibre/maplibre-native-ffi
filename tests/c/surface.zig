const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

test "surface descriptors expose defaults" {
    const metal = c.mln_metal_surface_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_metal_surface_descriptor)), metal.size);
    try testing.expect(metal.width > 0);
    try testing.expect(metal.height > 0);
    try testing.expect(metal.scale_factor > 0);
    try testing.expect(metal.layer == null);
    try testing.expect(metal.device == null);

    const vulkan = c.mln_vulkan_surface_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_vulkan_surface_descriptor)), vulkan.size);
    try testing.expect(vulkan.width > 0);
    try testing.expect(vulkan.height > 0);
    try testing.expect(vulkan.scale_factor > 0);
    try testing.expect(vulkan.instance == null);
    try testing.expect(vulkan.physical_device == null);
    try testing.expect(vulkan.device == null);
    try testing.expect(vulkan.graphics_queue == null);
    try testing.expect(vulkan.surface == null);
}
