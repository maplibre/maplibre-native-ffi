// Raw C ABI/backend coverage: Metal descriptors expose unsafe pointer, size, and stale-handle cases that the Zig binding cannot construct.

const build_options = @import("build_options");
const testing = @import("std").testing;
const support = @import("support.zig");
const common = @import("render_session_abi.zig");
const c = support.c;

const fake_handle: *anyopaque = @ptrFromInt(1);

const MetalSurface = struct {
    pub const descriptor_size = @sizeOf(c.mln_metal_surface_descriptor);

    pub fn descriptor() c.mln_metal_surface_descriptor {
        var value = c.mln_metal_surface_descriptor_default();
        value.layer = fake_handle;
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_metal_surface_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_metal_surface_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_metal_surface_descriptor) void {
        descriptor_ptr.layer = null;
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_metal_surface_descriptor) void {
        descriptor_ptr.context.size = @sizeOf(c.mln_metal_context_descriptor) - 1;
    }
};

const MetalOwnedTexture = struct {
    pub const descriptor_size = @sizeOf(c.mln_metal_owned_texture_descriptor);

    pub fn descriptor() c.mln_metal_owned_texture_descriptor {
        var value = c.mln_metal_owned_texture_descriptor_default();
        value.context.device = fake_handle;
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_metal_owned_texture_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_metal_owned_texture_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_metal_owned_texture_descriptor) void {
        descriptor_ptr.context.device = null;
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_metal_owned_texture_descriptor) void {
        descriptor_ptr.context.size = @sizeOf(c.mln_metal_context_descriptor) - 1;
    }
};

test "Metal surface attach rejects unsafe raw inputs" {
    if (!build_options.supports_metal) return error.SkipZigTest;
    try common.expectAttachRejectsUnsafeInputs(MetalSurface);
}

test "Metal owned texture attach rejects unsafe raw inputs" {
    if (!build_options.supports_metal) return error.SkipZigTest;
    try common.expectAttachRejectsUnsafeInputs(MetalOwnedTexture);
}

test "Metal borrowed texture rejects unsafe raw descriptors" {
    if (!build_options.supports_metal) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_metal_borrowed_texture_descriptor_default();
    descriptor.extent.width = 128;
    descriptor.extent.height = 128;

    var texture: ?*c.mln_render_session = null;
    var invalid_extent_descriptor = descriptor;
    invalid_extent_descriptor.extent.size = @sizeOf(c.mln_render_target_extent) - 1;
    invalid_extent_descriptor.texture = fake_handle;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_metal_borrowed_texture_attach(map, &invalid_extent_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_metal_borrowed_texture_attach(map, &descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);
}
