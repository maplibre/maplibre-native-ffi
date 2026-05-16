// Raw C ABI/backend coverage: Vulkan descriptors expose unsafe pointer, size, and output-handle cases that the Zig binding cannot construct.

const build_options = @import("build_options");
const testing = @import("std").testing;
const support = @import("support.zig");
const common = @import("render_session_abi.zig");
const c = support.c;

const fake_handle: *anyopaque = @ptrFromInt(1);

const VulkanSurface = struct {
    pub const descriptor_size = @sizeOf(c.mln_vulkan_surface_descriptor);

    pub fn descriptor() c.mln_vulkan_surface_descriptor {
        var value = c.mln_vulkan_surface_descriptor_default();
        value.context.instance = fake_handle;
        value.context.physical_device = fake_handle;
        value.context.device = fake_handle;
        value.context.graphics_queue = fake_handle;
        value.surface = fake_handle;
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_vulkan_surface_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_vulkan_surface_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_vulkan_surface_descriptor) void {
        descriptor_ptr.surface = null;
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_vulkan_surface_descriptor) void {
        descriptor_ptr.context.size = @sizeOf(c.mln_vulkan_context_descriptor) - 1;
    }
};

const VulkanOwnedTexture = struct {
    pub const descriptor_size = @sizeOf(c.mln_vulkan_owned_texture_descriptor);

    pub fn descriptor() c.mln_vulkan_owned_texture_descriptor {
        var value = c.mln_vulkan_owned_texture_descriptor_default();
        value.context.instance = fake_handle;
        value.context.physical_device = fake_handle;
        value.context.device = fake_handle;
        value.context.graphics_queue = fake_handle;
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_vulkan_owned_texture_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_vulkan_owned_texture_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_vulkan_owned_texture_descriptor) void {
        descriptor_ptr.context.device = null;
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_vulkan_owned_texture_descriptor) void {
        descriptor_ptr.context.size = @sizeOf(c.mln_vulkan_context_descriptor) - 1;
    }
};

test "Vulkan surface attach rejects unsafe raw inputs" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;
    try common.expectAttachRejectsUnsafeInputs(VulkanSurface);
}

test "Vulkan owned texture attach rejects unsafe raw inputs" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;
    try common.expectAttachRejectsUnsafeInputs(VulkanOwnedTexture);
}

test "Vulkan borrowed texture rejects unsafe raw descriptors" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_vulkan_borrowed_texture_descriptor_default();
    descriptor.context.instance = fake_handle;
    descriptor.context.physical_device = fake_handle;
    descriptor.context.device = fake_handle;
    descriptor.context.graphics_queue = fake_handle;
    descriptor.image = fake_handle;
    descriptor.image_view = fake_handle;
    descriptor.format = 37;
    descriptor.initial_layout = 0;
    descriptor.final_layout = 5;

    var texture: ?*c.mln_render_session = null;
    var invalid_extent_descriptor = descriptor;
    invalid_extent_descriptor.extent.size = @sizeOf(c.mln_render_target_extent) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_borrowed_texture_attach(map, &invalid_extent_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    var invalid_context_descriptor = descriptor;
    invalid_context_descriptor.context.size = @sizeOf(c.mln_vulkan_context_descriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_borrowed_texture_attach(map, &invalid_context_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    var missing_image_descriptor = descriptor;
    missing_image_descriptor.image = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_vulkan_borrowed_texture_attach(map, &missing_image_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);
}
