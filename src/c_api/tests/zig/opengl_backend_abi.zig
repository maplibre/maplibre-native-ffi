// Raw C ABI/backend coverage: OpenGL descriptors expose provider unions,
// texture object names, size fields, and output-handle cases that bindings
// should wrap more safely.

const build_options = @import("build_options");
const builtin = @import("builtin");
const gl = if (builtin.os.tag == .windows) @import("gl") else struct {};
const std = @import("std");
const testing = @import("std").testing;
const support = @import("support.zig");
const common = @import("render_session_abi.zig");
const c = support.c;
const wgl_test = if (builtin.os.tag == .windows) @import("wgl_test_context") else struct {};

const gl_texture_2d = if (builtin.os.tag == .windows) gl.TEXTURE_2D else 0x0de1;
const gl_rgba8 = if (builtin.os.tag == .windows) gl.RGBA8 else 0x8058;
const gl_rgba = if (builtin.os.tag == .windows) gl.RGBA else 0x1908;
const gl_unsigned_byte = if (builtin.os.tag == .windows) gl.UNSIGNED_BYTE else 0x1401;

const fake_handle: *anyopaque = @ptrFromInt(1);

const raw_render_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-raw-opengl-render-test",
    \\  "sources": {},
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#2c7fb8"}}
    \\  ]
    \\}
;

fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

fn emptyEvent() c.mln_runtime_event {
    return .{
        .size = @sizeOf(c.mln_runtime_event),
        .type = 0,
        .source_type = c.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
        .source = null,
        .code = 0,
        .payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_NONE,
        .payload = null,
        .payload_size = 0,
        .message = null,
        .message_size = 0,
    };
}

fn waitForRenderedFrame(runtime: *c.mln_runtime, map: *c.mln_map, session: *c.mln_render_session) !void {
    for (0..5000) |_| {
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_run_once(runtime));
        while (true) {
            var event = emptyEvent();
            var has_event = false;
            try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_poll_event(runtime, &event, &has_event));
            if (!has_event) break;
            if (event.source_type != c.MLN_RUNTIME_EVENT_SOURCE_MAP or event.source != @as(?*anyopaque, @ptrCast(map))) continue;

            switch (event.type) {
                c.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE => {
                    const render_status = c.mln_render_session_render_update(session);
                    if (render_status == c.MLN_STATUS_INVALID_STATE) continue;
                    try testing.expectEqual(c.MLN_STATUS_OK, render_status);
                    return;
                },
                c.MLN_RUNTIME_EVENT_MAP_LOADING_FAILED => return error.MapLoadingFailed,
                c.MLN_RUNTIME_EVENT_MAP_RENDER_ERROR => return error.MapRenderFailed,
                c.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED => return error.StillImageFailed,
                else => {},
            }
        }
        try sleepOneMillisecond();
    }
    return error.RenderTimedOut;
}

fn hasNonZeroByte(bytes: []const u8) bool {
    for (bytes) |byte| {
        if (byte != 0) return true;
    }
    return false;
}

fn emptyOpenGLOwnedTextureFrame() c.mln_opengl_owned_texture_frame {
    return .{
        .size = @sizeOf(c.mln_opengl_owned_texture_frame),
        .generation = 0,
        .width = 0,
        .height = 0,
        .scale_factor = 0.0,
        .frame_id = 0,
        .texture = 0,
        .target = 0,
        .internal_format = 0,
        .format = 0,
        .type = 0,
    };
}

fn configureContext(context: *c.mln_opengl_context_descriptor) void {
    if (builtin.os.tag == .windows) {
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
    if (builtin.os.tag == .windows) {
        context.data.wgl.size = @sizeOf(c.mln_wgl_context_descriptor) - 1;
    } else {
        context.data.egl.size = @sizeOf(c.mln_egl_context_descriptor) - 1;
    }
}

fn clearRequiredContextHandle(context: *c.mln_opengl_context_descriptor) void {
    if (builtin.os.tag == .windows) {
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

const WglContext = if (builtin.os.tag == .windows) wgl_test.Context else opaque {};

fn wglContextDescriptor(context: *const WglContext) c.mln_opengl_context_descriptor {
    if (builtin.os.tag != .windows) unreachable;
    return .{
        .size = @sizeOf(c.mln_opengl_context_descriptor),
        .platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WGL,
        .data = .{
            .wgl = .{
                .size = @sizeOf(c.mln_wgl_context_descriptor),
                .device_context = context.deviceContextPointer(),
                .share_context = context.shareContextPointer(),
                .get_proc_address = wgl_test.Context.getProcAddressPointer(),
            },
        },
    };
}

test "OpenGL default descriptors initialize ABI sizes" {
    const owned = c.mln_opengl_owned_texture_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_owned_texture_descriptor)), owned.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), owned.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_context_descriptor)), owned.context.size);

    const borrowed = c.mln_opengl_borrowed_texture_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_borrowed_texture_descriptor)), borrowed.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), borrowed.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_context_descriptor)), borrowed.context.size);

    const surface = c.mln_opengl_surface_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_surface_descriptor)), surface.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), surface.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_context_descriptor)), surface.context.size);
}

test "OpenGL provider mask matches OpenGL build platform" {
    const mask = c.mln_opengl_supported_context_provider_mask();
    if (!build_options.supports_opengl) {
        try testing.expectEqual(@as(u32, 0), mask);
    } else if (builtin.os.tag == .windows) {
        try testing.expect((mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL) != 0);
    } else {
        try testing.expect((mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL) != 0);
    }
}

test "OpenGL attach reports unsupported when backend is unavailable" {
    if (build_options.supports_opengl) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var session: ?*c.mln_render_session = null;

    var owned = c.mln_opengl_owned_texture_descriptor_default();
    configureContext(&owned.context);
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_opengl_owned_texture_attach(map, &owned, &session));
    try testing.expectEqual(@as(?*c.mln_render_session, null), session);

    var borrowed = c.mln_opengl_borrowed_texture_descriptor_default();
    configureContext(&borrowed.context);
    borrowed.texture = 1;
    borrowed.target = gl_texture_2d;
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_opengl_borrowed_texture_attach(map, &borrowed, &session));
    try testing.expectEqual(@as(?*c.mln_render_session, null), session);

    var surface = c.mln_opengl_surface_descriptor_default();
    configureContext(&surface.context);
    surface.surface = fake_handle;
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_opengl_surface_attach(map, &surface, &session));
    try testing.expectEqual(@as(?*c.mln_render_session, null), session);
}

test "OpenGL owned texture attach rejects unsafe raw inputs" {
    try common.expectAttachRejectsUnsafeInputs(OpenGLOwnedTexture);
}

test "OpenGL surface attach rejects unsafe raw inputs" {
    try common.expectAttachRejectsUnsafeInputs(OpenGLSurface);
}

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

test "OpenGL EGL owned texture renders through raw C ABI" {
    if (!build_options.supports_opengl or builtin.os.tag != .linux) return error.SkipZigTest;

    var egl_context = try support.OwnedTextureAttachContext.init();
    defer egl_context.deinit();

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_opengl_owned_texture_descriptor_default();
    descriptor.extent.width = 256;
    descriptor.extent.height = 256;
    descriptor.extent.scale_factor = 1.0;
    descriptor.context = egl_context.descriptor();

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_owned_texture_attach(map, &descriptor, &session));
    const handle = session orelse return error.SessionAttachFailed;
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(handle)) catch @panic("render session destroy failed");

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, raw_render_style_json));
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_request_repaint(map));
    try waitForRenderedFrame(runtime, map, handle);

    var image_info = c.mln_texture_image_info_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_texture_read_premultiplied_rgba8(handle, null, 0, &image_info));
    try testing.expectEqual(@as(u32, 256), image_info.width);
    try testing.expectEqual(@as(u32, 256), image_info.height);
    try testing.expectEqual(@as(u32, 256 * 4), image_info.stride);

    const pixels = try testing.allocator.alloc(u8, image_info.byte_length);
    defer testing.allocator.free(pixels);
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_texture_read_premultiplied_rgba8(handle, pixels.ptr, pixels.len, &image_info));
    try testing.expect(hasNonZeroByte(pixels));

    var frame = emptyOpenGLOwnedTextureFrame();
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_owned_texture_acquire_frame(handle, &frame));
    try testing.expectEqual(@as(u32, 256), frame.width);
    try testing.expectEqual(@as(u32, 256), frame.height);
    try testing.expectEqual(@as(u32, gl_texture_2d), frame.target);
    try testing.expectEqual(@as(u32, gl_rgba8), frame.internal_format);
    try testing.expectEqual(@as(u32, gl_rgba), frame.format);
    try testing.expectEqual(@as(u32, gl_unsigned_byte), frame.type);
    try testing.expect(frame.texture != 0);
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_owned_texture_release_frame(handle, &frame));
}

test "OpenGL WGL owned texture renders through raw C ABI" {
    if (!build_options.supports_opengl or builtin.os.tag != .windows) return error.SkipZigTest;

    var wgl_context = try wgl_test.Context.init();
    defer wgl_context.deinit();

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_opengl_owned_texture_descriptor_default();
    descriptor.extent.width = 256;
    descriptor.extent.height = 256;
    descriptor.extent.scale_factor = 1.0;
    descriptor.context = wglContextDescriptor(&wgl_context);

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_owned_texture_attach(map, &descriptor, &session));
    const handle = session orelse return error.SessionAttachFailed;
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(handle)) catch @panic("render session destroy failed");

    var image_info = c.mln_texture_image_info_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_texture_read_premultiplied_rgba8(handle, null, 0, &image_info));

    var frame = c.mln_opengl_owned_texture_frame{
        .size = @sizeOf(c.mln_opengl_owned_texture_frame),
        .generation = 0,
        .width = 0,
        .height = 0,
        .scale_factor = 0.0,
        .frame_id = 0,
        .texture = 0,
        .target = 0,
        .internal_format = 0,
        .format = 0,
        .type = 0,
    };
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_opengl_owned_texture_acquire_frame(handle, &frame));
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_opengl_owned_texture_release_frame(handle, &frame));

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, raw_render_style_json));
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_request_repaint(map));
    try waitForRenderedFrame(runtime, map, handle);

    image_info = c.mln_texture_image_info_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_texture_read_premultiplied_rgba8(handle, null, 0, &image_info));
    try testing.expectEqual(@as(u32, 256), image_info.width);
    try testing.expectEqual(@as(u32, 256), image_info.height);
    try testing.expectEqual(@as(u32, 256 * 4), image_info.stride);

    const pixels = try testing.allocator.alloc(u8, image_info.byte_length);
    defer testing.allocator.free(pixels);
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_texture_read_premultiplied_rgba8(handle, pixels.ptr, pixels.len, &image_info));
    try testing.expect(hasNonZeroByte(pixels));

    frame = .{
        .size = @sizeOf(c.mln_opengl_owned_texture_frame),
        .generation = 0,
        .width = 0,
        .height = 0,
        .scale_factor = 0.0,
        .frame_id = 0,
        .texture = 0,
        .target = 0,
        .internal_format = 0,
        .format = 0,
        .type = 0,
    };
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_owned_texture_acquire_frame(handle, &frame));
    try testing.expectEqual(@as(u32, 256), frame.width);
    try testing.expectEqual(@as(u32, 256), frame.height);
    try testing.expectEqual(@as(u32, gl_texture_2d), frame.target);
    try testing.expectEqual(@as(u32, gl_rgba8), frame.internal_format);
    try testing.expectEqual(@as(u32, gl_rgba), frame.format);
    try testing.expectEqual(@as(u32, gl_unsigned_byte), frame.type);
    try testing.expect(frame.texture != 0);
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_texture_read_premultiplied_rgba8(handle, pixels.ptr, pixels.len, &image_info));
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_owned_texture_release_frame(handle, &frame));
}

test "OpenGL WGL borrowed texture renders through raw C ABI" {
    if (!build_options.supports_opengl or builtin.os.tag != .windows) return error.SkipZigTest;

    var wgl_context = try wgl_test.Context.init();
    defer wgl_context.deinit();

    const texture = try wgl_context.createRgbaTexture(256, 256);
    defer wgl_context.destroyTexture(texture);

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_opengl_borrowed_texture_descriptor_default();
    descriptor.extent.width = 256;
    descriptor.extent.height = 256;
    descriptor.extent.scale_factor = 1.0;
    descriptor.context = wglContextDescriptor(&wgl_context);
    descriptor.texture = texture;
    descriptor.target = gl.TEXTURE_2D;

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_borrowed_texture_attach(map, &descriptor, &session));
    const handle = session orelse return error.SessionAttachFailed;
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(handle)) catch @panic("render session destroy failed");

    var image_info = c.mln_texture_image_info_default();
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_texture_read_premultiplied_rgba8(handle, null, 0, &image_info));
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_render_session_resize(handle, 128, 128, 1.0));

    var frame = emptyOpenGLOwnedTextureFrame();
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_opengl_owned_texture_acquire_frame(handle, &frame));

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, raw_render_style_json));
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_request_repaint(map));
    try waitForRenderedFrame(runtime, map, handle);

    const pixels = try testing.allocator.alloc(u8, 256 * 256 * 4);
    defer testing.allocator.free(pixels);
    @memset(pixels, 0);
    try wgl_context.readRgbaTexture(texture, pixels);
    try testing.expect(hasNonZeroByte(pixels));

    image_info = c.mln_texture_image_info_default();
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_texture_read_premultiplied_rgba8(handle, null, 0, &image_info));
    frame = emptyOpenGLOwnedTextureFrame();
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_opengl_owned_texture_acquire_frame(handle, &frame));
}

test "OpenGL WGL surface renders through raw C ABI" {
    if (!build_options.supports_opengl or builtin.os.tag != .windows) return error.SkipZigTest;

    var wgl_context = try wgl_test.Context.initWithSize(256, 256);
    defer wgl_context.deinit();

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_opengl_surface_descriptor_default();
    descriptor.extent.width = 256;
    descriptor.extent.height = 256;
    descriptor.extent.scale_factor = 1.0;
    descriptor.context = wglContextDescriptor(&wgl_context);
    descriptor.surface = wgl_context.deviceContextPointer();

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_surface_attach(map, &descriptor, &session));
    const handle = session orelse return error.SessionAttachFailed;
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(handle)) catch @panic("render session destroy failed");

    var image_info = c.mln_texture_image_info_default();
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_texture_read_premultiplied_rgba8(handle, null, 0, &image_info));
    var frame = emptyOpenGLOwnedTextureFrame();
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_opengl_owned_texture_acquire_frame(handle, &frame));
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_opengl_owned_texture_release_frame(handle, &frame));

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, raw_render_style_json));
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_request_repaint(map));
    try waitForRenderedFrame(runtime, map, handle);

    const pixels = try testing.allocator.alloc(u8, 256 * 256 * 4);
    defer testing.allocator.free(pixels);
    @memset(pixels, 0);
    try wgl_context.readSurfaceRgba(256, 256, pixels);
    try testing.expect(hasNonZeroByte(pixels));
}
