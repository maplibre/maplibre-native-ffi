// Raw C ABI/backend coverage: OpenGL descriptors expose provider unions,
// texture object names, size fields, and output-handle cases that bindings
// should wrap more safely.

const build_options = @import("build_options");
const builtin = @import("builtin");
const supports_wgl = build_options.supports_opengl and builtin.os.tag == .windows;
const testing = @import("std").testing;
const support = @import("support.zig");
const common = @import("render_session_abi.zig");
const c = support.c;

const gl_texture_2d = 0x0de1;

const fake_handle: *anyopaque = @ptrFromInt(1);

fn configureContext(context: *c.mln_opengl_context_descriptor) void {
    if (supports_wgl) {
        context.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WGL;
        context.data.wgl.size = @sizeOf(c.mln_wgl_context_descriptor);
        context.data.wgl.device_context = fake_handle;
        context.data.wgl.share_context = fake_handle;
    } else {
        context.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_EGL;
        context.data.egl.size = @sizeOf(c.mln_egl_context_descriptor);
        context.data.egl.display = fake_handle;
        context.data.egl.config = fake_handle;
        context.data.egl.share_context = fake_handle;
    }
}

fn shrinkContext(context: *c.mln_opengl_context_descriptor) void {
    if (supports_wgl) {
        context.data.wgl.size = @sizeOf(c.mln_wgl_context_descriptor) - 1;
    } else {
        context.data.egl.size = @sizeOf(c.mln_egl_context_descriptor) - 1;
    }
}

fn clearRequiredContextHandle(context: *c.mln_opengl_context_descriptor) void {
    if (supports_wgl) {
        context.data.wgl.share_context = null;
    } else {
        context.data.egl.share_context = null;
    }
}

const OpenGLOwnedTexture = struct {
    pub const descriptor_size = @sizeOf(c.mln_opengl_owned_texture_descriptor);

    pub fn descriptor() c.mln_opengl_owned_texture_descriptor {
        var value = c.mln_opengl_owned_texture_descriptor_default();
        configureContext(&value.context);
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_opengl_owned_texture_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_opengl_owned_texture_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_opengl_owned_texture_descriptor) void {
        clearRequiredContextHandle(&descriptor_ptr.context);
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_opengl_owned_texture_descriptor) void {
        opengl_backend_abi.shrinkContext(&descriptor_ptr.context);
    }
};

const OpenGLSurface = struct {
    pub const descriptor_size = @sizeOf(c.mln_opengl_surface_descriptor);

    pub fn descriptor() c.mln_opengl_surface_descriptor {
        var value = c.mln_opengl_surface_descriptor_default();
        configureContext(&value.context);
        value.surface = fake_handle;
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_opengl_surface_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_opengl_surface_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_opengl_surface_descriptor) void {
        descriptor_ptr.surface = null;
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_opengl_surface_descriptor) void {
        opengl_backend_abi.shrinkContext(&descriptor_ptr.context);
    }
};

const opengl_backend_abi = @This();

// PRUNING REVIEW: KEEP.
// This verifies nulls, a non-null output handle, undersized descriptors, and missing required OpenGL texture handles.
test "OpenGL owned texture attach rejects unsafe raw inputs" {
    try common.expectAttachRejectsUnsafeInputs(OpenGLOwnedTexture);
}

// PRUNING REVIEW: KEEP.
// This verifies nulls, a non-null output handle, undersized descriptors, and missing required OpenGL surface handles.
test "OpenGL surface attach rejects unsafe raw inputs" {
    try common.expectAttachRejectsUnsafeInputs(OpenGLSurface);
}

// PRUNING REVIEW: KEEP.
// This verifies nested sizes and required raw texture values that typed OpenGL descriptors prevent.
test "OpenGL borrowed texture rejects unsafe raw descriptors" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_opengl_borrowed_texture_descriptor_default();
    configureContext(&descriptor.context);
    descriptor.texture = 1;
    descriptor.target = gl_texture_2d;

    var texture: ?*c.mln_render_session = null;
    var invalid_extent_descriptor = descriptor;
    invalid_extent_descriptor.extent.size = @sizeOf(c.mln_render_target_extent) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_opengl_borrowed_texture_attach(map, &invalid_extent_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    var invalid_context_descriptor = descriptor;
    invalid_context_descriptor.context.size = @sizeOf(c.mln_opengl_context_descriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_opengl_borrowed_texture_attach(map, &invalid_context_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    var missing_texture_descriptor = descriptor;
    missing_texture_descriptor.texture = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_opengl_borrowed_texture_attach(map, &missing_texture_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);
}
