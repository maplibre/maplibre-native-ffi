const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

extern "c" fn MTLCreateSystemDefaultDevice() ?*anyopaque;

const vk = if (build_options.supports_vulkan) @import("vulkan") else struct {};

const supports_wgl = build_options.supports_opengl and builtin.os.tag == .windows;
const supports_egl = build_options.supports_opengl and (builtin.os.tag == .linux or builtin.os.tag == .macos);
const supports_webgl = build_options.supports_opengl and builtin.os.tag == .emscripten;

const egl = if (supports_egl) @import("egl") else struct {};

const gl = if (supports_wgl or supports_egl) @import("gl") else struct {};
const wgl_test = if (supports_wgl) @import("wgl_test_context") else struct {};

test "supported render backend is exposed semantically" {
    const support_mask = try maplibre.supportedRenderBackendMask();
    try testing.expect(support_mask.metal or support_mask.opengl or support_mask.vulkan or support_mask.webgpu);
    if (build_options.supports_metal) try testing.expect(support_mask.metal);
    if (build_options.supports_opengl) try testing.expect(support_mask.opengl);
    if (build_options.supports_vulkan) try testing.expect(support_mask.vulkan);
}

test "supported OpenGL context providers are exposed semantically" {
    const providers = try maplibre.openglSupportedContextProviderMask();
    if (!build_options.supports_opengl) {
        try testing.expect(!providers.wgl);
        try testing.expect(!providers.egl);
        try testing.expect(!providers.webgl);
    } else {
        try testing.expectEqual(supports_wgl, providers.wgl);
        try testing.expectEqual(supports_egl, providers.egl);
        try testing.expectEqual(supports_webgl, providers.webgl);
    }
}

fn hasNonZeroByte(bytes: []const u8) bool {
    for (bytes) |byte| {
        if (byte != 0) return true;
    }
    return false;
}

fn fakeNativePointer() ?*anyopaque {
    return @ptrFromInt(1);
}

fn fakeOpenGLContext() maplibre.OpenglContextDescriptor {
    const fake_pointer = fakeNativePointer();
    if (supports_wgl) {
        return .{ .data = .{
            .wgl = .{
                .device_context = fake_pointer,
                .share_context = fake_pointer,
            },
        } };
    }
    if (supports_egl) {
        return .{ .data = .{
            .egl = .{
                .display = fake_pointer,
                .config = fake_pointer,
                .share_context = fake_pointer,
            },
        } };
    }
    return .{ .data = .{ .wgl = .{ .device_context = fake_pointer, .share_context = fake_pointer } } };
}

fn fakeVulkanContext() maplibre.VulkanContextDescriptor {
    const fake_pointer = fakeNativePointer();
    return .{
        .instance = fake_pointer,
        .physical_device = fake_pointer,
        .device = fake_pointer,
        .graphics_queue = fake_pointer,
        .graphics_queue_family_index = 0,
    };
}

fn vulkanHandleToBinding(handle: anytype) u64 {
    const Handle = @TypeOf(handle);
    const bits: u64 = switch (@typeInfo(Handle)) {
        .optional => if (handle) |value| @intFromPtr(value) else 0,
        .pointer => @intFromPtr(handle),
        .int => @intCast(handle),
        .@"enum" => @intCast(@intFromEnum(handle)),
        else => @compileError("unsupported Vulkan handle representation"),
    };
    return bits;
}

fn nullVulkanHandle(comptime Handle: type) Handle {
    return std.mem.zeroes(Handle);
}

fn isNullVulkanHandle(handle: anytype) bool {
    return std.meta.eql(handle, nullVulkanHandle(@TypeOf(handle)));
}

const supports_test_owned_texture = build_options.supports_metal or build_options.supports_vulkan or build_options.supports_opengl;

const TestOwnedTextureContext = if (build_options.supports_vulkan) VulkanAttachContext else if (supports_wgl) WglAttachContext else if (supports_egl) EglAttachContext else if (build_options.supports_metal) struct {
    device: *anyopaque,

    pub fn init() !@This() {
        return .{ .device = MTLCreateSystemDefaultDevice() orelse return error.MetalDeviceUnavailable };
    }

    pub fn deinit(_: *@This()) void {}

    pub fn descriptor(self: *const @This()) maplibre.MetalContextDescriptor {
        return .{ .device = self.device };
    }
} else struct {};

const WglAttachContext = if (supports_wgl) struct {
    context: wgl_test.Context,

    pub fn init() !WglAttachContext {
        return initWithSize(32, 32);
    }

    pub fn initWithSize(width: u32, height: u32) !WglAttachContext {
        return .{ .context = try wgl_test.Context.initWithClassName("MaplibreZigBindingWglTest", width, height) };
    }

    pub fn deinit(self: *WglAttachContext) void {
        self.context.deinit();
    }

    pub fn descriptor(self: *const WglAttachContext) maplibre.OpenglContextDescriptor {
        return .{ .data = .{ .wgl = .{
            .device_context = self.context.deviceContextPointer(),
            .share_context = self.context.shareContextPointer(),
            .get_proc_address = wgl_test.Context.getProcAddressPointer(),
        } } };
    }

    pub fn surface(self: *const WglAttachContext) ?*anyopaque {
        return self.context.deviceContextPointer();
    }

    pub fn readSurfaceRGBA8(self: *const WglAttachContext, width: u32, height: u32, pixels: []u8) !void {
        try self.context.readSurfaceRgba(width, height, pixels);
    }

    pub fn destroyTexture(self: *const WglAttachContext, texture: gl.uint) void {
        self.context.destroyTexture(texture);
    }

    pub fn readRgbaTexture(self: *const WglAttachContext, texture: gl.uint, width: u32, height: u32, pixels: []u8) !void {
        _ = width;
        _ = height;
        try self.context.readRgbaTexture(texture, pixels);
    }
} else struct {};

const WglBorrowedTexture = if (supports_wgl) struct {
    context: WglAttachContext,
    texture: gl.uint,
    width: u32,
    height: u32,

    pub fn create(width: u32, height: u32) !WglBorrowedTexture {
        var context = try WglAttachContext.initWithSize(width, height);
        errdefer context.deinit();
        const texture = try context.context.createRgbaTexture(width, height);
        return .{ .context = context, .texture = texture, .width = width, .height = height };
    }

    pub fn deinit(self: *WglBorrowedTexture) void {
        if (self.texture != 0) {
            self.context.destroyTexture(self.texture);
            self.texture = 0;
        }
        self.context.deinit();
    }

    /// Allocates a replacement in this helper's own context. The outgoing
    /// texture stays live until `adopt`.
    pub fn allocateReplacement(self: *const WglBorrowedTexture, width: u32, height: u32) !gl.uint {
        return self.context.context.createRgbaTexture(width, height);
    }

    /// Tracks a replacement the session has taken and releases the outgoing one.
    pub fn adopt(self: *WglBorrowedTexture, texture: gl.uint, width: u32, height: u32) void {
        if (self.texture != 0) self.context.destroyTexture(self.texture);
        self.texture = texture;
        self.width = width;
        self.height = height;
    }

    pub fn descriptor(self: *const WglBorrowedTexture) maplibre.OpenglBorrowedTextureDescriptor {
        return self.descriptorFor(self.texture, self.width, self.height);
    }

    pub fn descriptorFor(self: *const WglBorrowedTexture, texture: gl.uint, width: u32, height: u32) maplibre.OpenglBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = width, .height = height, .scale_factor = 1.0 },
            .physical_width = width,
            .physical_height = height,
            .context = self.context.descriptor(),
            .texture = texture,
            .target = gl.TEXTURE_2D,
        };
    }

    pub fn readRGBA8(self: *const WglBorrowedTexture, pixels: []u8) !void {
        try self.context.context.readRgbaTexture(self.texture, pixels);
    }
} else struct {};

fn GlProc(comptime name: []const u8) type {
    if (!supports_egl) return void;
    return @TypeOf(@field(@as(gl.ProcTable, undefined), name));
}

fn glProcName(comptime command: []const u8) [:0]const u8 {
    return "gl" ++ command;
}

const EglProcs = if (supports_egl) struct {
    BindTexture: GlProc("BindTexture"),
    BindFramebuffer: GlProc("BindFramebuffer"),
    CheckFramebufferStatus: GlProc("CheckFramebufferStatus"),
    DeleteTextures: GlProc("DeleteTextures"),
    DeleteFramebuffers: GlProc("DeleteFramebuffers"),
    FramebufferTexture2D: GlProc("FramebufferTexture2D"),
    GenTextures: GlProc("GenTextures"),
    GenFramebuffers: GlProc("GenFramebuffers"),
    GetError: GlProc("GetError"),
    ReadPixels: GlProc("ReadPixels"),
    TexImage2D: GlProc("TexImage2D"),
    TexParameteri: GlProc("TexParameteri"),

    fn init() !EglProcs {
        var procs: EglProcs = undefined;
        inline for (.{
            "BindTexture",
            "BindFramebuffer",
            "CheckFramebufferStatus",
            "DeleteTextures",
            "DeleteFramebuffers",
            "FramebufferTexture2D",
            "GenTextures",
            "GenFramebuffers",
            "GetError",
            "ReadPixels",
            "TexImage2D",
            "TexParameteri",
        }) |command| {
            @field(procs, command) = @ptrCast(egl.eglGetProcAddress(glProcName(command)) orelse return error.EglUnavailable);
        }
        return procs;
    }
} else struct {};

const EglAttachContext = if (supports_egl) struct {
    display: egl.EGLDisplay,
    config: egl.EGLConfig,
    egl_surface: egl.EGLSurface,
    share_context: egl.EGLContext,
    procs: EglProcs,

    pub fn init() !EglAttachContext {
        return initWithSize(8, 8);
    }

    pub fn initWithSize(width: u32, height: u32) !EglAttachContext {
        const display = try initDisplay();
        errdefer _ = egl.eglTerminate(display);

        if (egl.eglBindAPI(egl.EGL_OPENGL_ES_API) == egl.EGL_FALSE) return error.EglUnavailable;

        const config_attributes = [_]egl.EGLint{
            egl.EGL_SURFACE_TYPE,    egl.EGL_PBUFFER_BIT,
            egl.EGL_RENDERABLE_TYPE, egl.EGL_OPENGL_ES3_BIT,
            egl.EGL_RED_SIZE,        8,
            egl.EGL_GREEN_SIZE,      8,
            egl.EGL_BLUE_SIZE,       8,
            egl.EGL_ALPHA_SIZE,      8,
            egl.EGL_DEPTH_SIZE,      24,
            egl.EGL_STENCIL_SIZE,    8,
            egl.EGL_NONE,
        };
        var config: egl.EGLConfig = null;
        var config_count: egl.EGLint = 0;
        if (egl.eglChooseConfig(display, &config_attributes, &config, 1, &config_count) == egl.EGL_FALSE or
            config_count == 0 or config == null)
        {
            return error.EglUnavailable;
        }

        const context_attributes = [_]egl.EGLint{
            egl.EGL_CONTEXT_CLIENT_VERSION, 3,
            egl.EGL_NONE,
        };
        const share_context = egl.eglCreateContext(display, config, egl.EGL_NO_CONTEXT, &context_attributes);
        if (share_context == egl.EGL_NO_CONTEXT) return error.EglUnavailable;
        errdefer _ = egl.eglDestroyContext(display, share_context);

        const surface_attributes = [_]egl.EGLint{
            egl.EGL_WIDTH,  @intCast(width),
            egl.EGL_HEIGHT, @intCast(height),
            egl.EGL_NONE,
        };
        const pbuffer = egl.eglCreatePbufferSurface(display, config, &surface_attributes);
        if (pbuffer == egl.EGL_NO_SURFACE) return error.EglUnavailable;
        errdefer _ = egl.eglDestroySurface(display, pbuffer);

        if (egl.eglMakeCurrent(display, pbuffer, pbuffer, share_context) == egl.EGL_FALSE) return error.EglUnavailable;
        return .{
            .display = display,
            .config = config,
            .egl_surface = pbuffer,
            .share_context = share_context,
            .procs = try EglProcs.init(),
        };
    }

    pub fn deinit(self: *EglAttachContext) void {
        _ = egl.eglMakeCurrent(self.display, egl.EGL_NO_SURFACE, egl.EGL_NO_SURFACE, egl.EGL_NO_CONTEXT);
        _ = egl.eglDestroySurface(self.display, self.egl_surface);
        _ = egl.eglDestroyContext(self.display, self.share_context);
        _ = egl.eglTerminate(self.display);
    }

    const EGL_PLATFORM_SURFACELESS_MESA: egl.EGLenum = 0x31DD;

    fn initDisplay() !egl.EGLDisplay {
        if (builtin.os.tag == .macos) {
            const display_attributes = [_]egl.EGLint{
                egl.EGL_PLATFORM_ANGLE_TYPE_ANGLE,        egl.EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
                egl.EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, egl.EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
                egl.EGL_NONE,
            };
            return initializeDisplay(egl.eglGetPlatformDisplayEXT(egl.EGL_PLATFORM_ANGLE_ANGLE, null, &display_attributes));
        }
        // Android and OpenHarmony EGL serve their own window systems, so they
        // keep the default display.
        if (builtin.abi == .android or builtin.abi.isOpenHarmony()) {
            return initializeDisplay(egl.eglGetDisplay(egl.EGL_DEFAULT_DISPLAY));
        }
        // These fixtures render into pbuffers and never present, so they name
        // the surfaceless platform: EGL_DEFAULT_DISPLAY resolves to whatever
        // platform libEGL was built for, commonly x11, which fails to
        // initialize with no display server.
        return initializeDisplay(egl.eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, null, null));
    }

    fn initializeDisplay(display: egl.EGLDisplay) !egl.EGLDisplay {
        if (display == egl.EGL_NO_DISPLAY) return error.EglUnavailable;

        var major: egl.EGLint = 0;
        var minor: egl.EGLint = 0;
        if (egl.eglInitialize(display, &major, &minor) == egl.EGL_FALSE) return error.EglUnavailable;
        return display;
    }

    pub fn makeCurrent(self: *const EglAttachContext) !void {
        if (egl.eglMakeCurrent(self.display, self.egl_surface, self.egl_surface, self.share_context) == egl.EGL_FALSE) return error.EglUnavailable;
    }

    pub fn descriptor(self: *const EglAttachContext) maplibre.OpenglContextDescriptor {
        return .{ .data = .{ .egl = .{
            .display = @ptrCast(self.display.?),
            .config = @ptrCast(self.config.?),
            .share_context = @ptrCast(self.share_context.?),
            .get_proc_address = null,
        } } };
    }

    pub fn surface(self: *const EglAttachContext) ?*anyopaque {
        return @ptrCast(self.egl_surface.?);
    }

    pub fn createRgbaTexture(self: *const EglAttachContext, width: u32, height: u32) !gl.uint {
        try self.makeCurrent();

        var texture: gl.uint = 0;
        self.procs.GenTextures(1, @ptrCast(&texture));
        if (texture == 0) return error.EglUnavailable;
        errdefer self.procs.DeleteTextures(1, @ptrCast(&texture));

        self.procs.BindTexture(gl.TEXTURE_2D, texture);
        self.procs.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        self.procs.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        // Zeroed so a readback test starts from a known value.
        const blank = testing.allocator.alloc(u8, width * height * 4) catch @panic("oom");
        defer testing.allocator.free(blank);
        @memset(blank, 0);
        self.procs.TexImage2D(
            gl.TEXTURE_2D,
            0,
            gl.RGBA8,
            @intCast(width),
            @intCast(height),
            0,
            gl.RGBA,
            gl.UNSIGNED_BYTE,
            blank.ptr,
        );
        self.procs.BindTexture(gl.TEXTURE_2D, 0);
        try testing.expectEqual(@as(gl.@"enum", gl.NO_ERROR), self.procs.GetError());
        return texture;
    }

    pub fn destroyTexture(self: *const EglAttachContext, texture: gl.uint) void {
        self.procs.DeleteTextures(1, @ptrCast(&texture));
    }

    pub fn readRgbaTexture(self: *const EglAttachContext, texture: gl.uint, width: u32, height: u32, pixels: []u8) !void {
        try self.makeCurrent();
        var framebuffer: gl.uint = 0;
        self.procs.GenFramebuffers(1, @ptrCast(&framebuffer));
        if (framebuffer == 0) return error.EglUnavailable;
        defer self.procs.DeleteFramebuffers(1, @ptrCast(&framebuffer));
        self.procs.BindFramebuffer(gl.FRAMEBUFFER, framebuffer);
        defer self.procs.BindFramebuffer(gl.FRAMEBUFFER, 0);
        self.procs.FramebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
        try testing.expectEqual(@as(gl.@"enum", gl.FRAMEBUFFER_COMPLETE), self.procs.CheckFramebufferStatus(gl.FRAMEBUFFER));
        self.procs.ReadPixels(0, 0, @intCast(width), @intCast(height), gl.RGBA, gl.UNSIGNED_BYTE, pixels.ptr);
        try testing.expectEqual(@as(gl.@"enum", gl.NO_ERROR), self.procs.GetError());
    }

    pub fn readSurfaceRGBA8(self: *const EglAttachContext, width: u32, height: u32, pixels: []u8) !void {
        try self.makeCurrent();
        self.procs.ReadPixels(0, 0, @intCast(width), @intCast(height), gl.RGBA, gl.UNSIGNED_BYTE, pixels.ptr);
        try testing.expectEqual(@as(gl.@"enum", gl.NO_ERROR), self.procs.GetError());
    }
} else struct {};

const OpenGLBorrowedTexture = if (supports_wgl) WglBorrowedTexture else if (supports_egl) struct {
    context: EglAttachContext,
    texture: gl.uint,
    width: u32,
    height: u32,

    pub fn create(width: u32, height: u32) !@This() {
        var context = try EglAttachContext.initWithSize(width, height);
        errdefer context.deinit();
        const texture = try context.createRgbaTexture(width, height);
        return .{ .context = context, .texture = texture, .width = width, .height = height };
    }

    pub fn deinit(self: *@This()) void {
        if (self.texture != 0) {
            self.context.destroyTexture(self.texture);
            self.texture = 0;
        }
        self.context.deinit();
    }

    /// Allocates a replacement in this helper's own context. The outgoing
    /// texture stays live until `adopt`.
    pub fn allocateReplacement(self: *const @This(), width: u32, height: u32) !gl.uint {
        return self.context.createRgbaTexture(width, height);
    }

    /// Tracks a replacement the session has taken and releases the outgoing one.
    pub fn adopt(self: *@This(), texture: gl.uint, width: u32, height: u32) void {
        if (self.texture != 0) self.context.destroyTexture(self.texture);
        self.texture = texture;
        self.width = width;
        self.height = height;
    }

    pub fn descriptor(self: *const @This()) maplibre.OpenglBorrowedTextureDescriptor {
        return self.descriptorFor(self.texture, self.width, self.height);
    }

    pub fn descriptorFor(self: *const @This(), texture: gl.uint, width: u32, height: u32) maplibre.OpenglBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = width, .height = height, .scale_factor = 1.0 },
            .physical_width = width,
            .physical_height = height,
            .context = self.context.descriptor(),
            .texture = texture,
            .target = gl.TEXTURE_2D,
        };
    }

    pub fn readRGBA8(self: *const @This(), pixels: []u8) !void {
        try self.context.readRgbaTexture(self.texture, self.width, self.height, pixels);
    }
} else struct {};

const TestOwnedTextureSession = struct {
    context: TestOwnedTextureContext,
    session: maplibre.RenderSession,
    context_active: bool = true,

    pub fn close(self: *@This()) !void {
        if (!self.context_active) return;
        defer {
            self.context.deinit();
            self.context_active = false;
        }
        try support.closeSession(&self.session, true);
    }
};

fn resolveFuture(comptime T: type, session: maplibre.RenderSession, future_value: maplibre.Future(T)) !T {
    return support.resolveSessionFuture(T, session, future_value, true);
}

fn finishOperation(session: maplibre.RenderSession, future: anytype) !void {
    try support.finishOperation(session, future, true);
}

fn finishAttachment(attachment: anytype) !maplibre.RenderSession {
    return support.finishAttachment(attachment, true);
}

fn attachTestOwnedTexture(map: *maplibre.Map, extent: maplibre.RenderTargetExtent) !TestOwnedTextureSession {
    var context = try TestOwnedTextureContext.init();
    errdefer context.deinit();
    const session = try attachOwnedTexture(map, &context, extent);
    return .{ .context = context, .session = session };
}

/// Attaches an owned-texture session on the backend this build supports,
/// leaving the graphics context to the caller.
fn attachOwnedTexture(
    map: *maplibre.Map,
    context: *TestOwnedTextureContext,
    extent: maplibre.RenderTargetExtent,
) !maplibre.RenderSession {
    const session = if (build_options.supports_vulkan)
        try finishAttachment(try maplibre.vulkanOwnedTextureAttach(testing.allocator, support.handle(map), .{
            .extent = extent,
            .context = context.descriptor(),
        }, .{ .driver = .core_worker, .requested_texture_ring_depth = 2 }))
    else if (build_options.supports_opengl)
        try finishAttachment(try maplibre.openglOwnedTextureAttach(testing.allocator, support.handle(map), .{
            .extent = extent,
            .context = context.descriptor(),
        }, .{ .driver = .caller_graphics_thread, .requested_texture_ring_depth = 2 }))
    else if (build_options.supports_metal)
        try finishAttachment(try maplibre.metalOwnedTextureAttach(testing.allocator, support.handle(map), .{
            .extent = extent,
            .context = context.descriptor(),
        }, .{ .driver = .core_worker, .requested_texture_ring_depth = 2 }))
    else
        unreachable;
    errdefer {
        if (maplibre.renderSessionDetach(support.handle(session))) |operation| {
            finishOperation(session, operation) catch {};
        } else |_| {}
        maplibre.renderSessionDestroy(support.handle(session)) catch {};
    }

    return session;
}

const VulkanAttachContext = if (build_options.supports_vulkan) struct {
    dispatch: VulkanDispatch,
    instance: vk.VkInstance,
    physical_device: vk.VkPhysicalDevice,
    device: vk.VkDevice,
    queue: vk.VkQueue,
    queue_family_index: u32,

    pub fn init() !VulkanAttachContext {
        var dispatch = try VulkanDispatch.init();
        errdefer dispatch.deinit();

        var app_info = std.mem.zeroes(vk.VkApplicationInfo);
        app_info.sType = vk.VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "maplibre-native-zig-binding-tests";
        app_info.applicationVersion = 1;
        app_info.pEngineName = "maplibre-native-zig-binding-tests";
        app_info.engineVersion = 1;
        app_info.apiVersion = vk.VK_API_VERSION_1_1;

        var instance_info = std.mem.zeroes(vk.VkInstanceCreateInfo);
        instance_info.sType = vk.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &app_info;
        if (builtin.os.tag == .macos) {
            const instance_extensions = [_][*c]const u8{vk.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
            instance_info.flags = vk.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            instance_info.enabledExtensionCount = instance_extensions.len;
            instance_info.ppEnabledExtensionNames = &instance_extensions;
        }

        var instance: vk.VkInstance = null;
        try expectVk(dispatch.create_instance.?(&instance_info, null, &instance));
        dispatch.loadInstanceFunctions(instance);
        errdefer dispatch.destroy_instance.?(instance, null);

        var physical_device_count: u32 = 0;
        try expectVk(dispatch.enumerate_physical_devices.?(instance, &physical_device_count, null));
        try testing.expect(physical_device_count != 0);

        const physical_devices = try testing.allocator.alloc(vk.VkPhysicalDevice, physical_device_count);
        defer testing.allocator.free(physical_devices);
        try expectVk(dispatch.enumerate_physical_devices.?(instance, &physical_device_count, physical_devices.ptr));

        for (physical_devices) |physical_device| {
            var queue_family_count: u32 = 0;
            dispatch.get_physical_device_queue_family_properties.?(physical_device, &queue_family_count, null);
            if (queue_family_count == 0) continue;

            const queue_families = try testing.allocator.alloc(vk.VkQueueFamilyProperties, queue_family_count);
            defer testing.allocator.free(queue_families);
            dispatch.get_physical_device_queue_family_properties.?(physical_device, &queue_family_count, queue_families.ptr);

            for (queue_families, 0..) |queue_family, index| {
                if ((queue_family.queueFlags & vk.VK_QUEUE_GRAPHICS_BIT) == 0 or queue_family.queueCount == 0) continue;

                var priority: f32 = 1.0;
                var queue_info = std.mem.zeroes(vk.VkDeviceQueueCreateInfo);
                queue_info.sType = vk.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info.queueFamilyIndex = @intCast(index);
                queue_info.queueCount = 1;
                queue_info.pQueuePriorities = &priority;

                var supported_features = std.mem.zeroes(vk.VkPhysicalDeviceFeatures);
                dispatch.get_physical_device_features.?(physical_device, &supported_features);
                var features = std.mem.zeroes(vk.VkPhysicalDeviceFeatures);
                features.samplerAnisotropy = supported_features.samplerAnisotropy;
                features.wideLines = supported_features.wideLines;

                const portability_subset_extensions = [_][*c]const u8{"VK_KHR_portability_subset"};
                const enabled_device_extensions = if (try hasDeviceExtension(&dispatch, physical_device, "VK_KHR_portability_subset"))
                    portability_subset_extensions[0..]
                else
                    portability_subset_extensions[0..0];

                var device_info = std.mem.zeroes(vk.VkDeviceCreateInfo);
                device_info.sType = vk.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                device_info.queueCreateInfoCount = 1;
                device_info.pQueueCreateInfos = &queue_info;
                device_info.enabledExtensionCount = @intCast(enabled_device_extensions.len);
                device_info.ppEnabledExtensionNames = enabled_device_extensions.ptr;
                device_info.pEnabledFeatures = &features;

                var device: vk.VkDevice = null;
                if (dispatch.create_device.?(physical_device, &device_info, null, &device) != vk.VK_SUCCESS) continue;
                dispatch.loadDeviceFunctions(device);

                var queue: vk.VkQueue = null;
                dispatch.get_device_queue.?(device, @intCast(index), 0, &queue);
                return .{
                    .dispatch = dispatch,
                    .instance = instance,
                    .physical_device = physical_device,
                    .device = device,
                    .queue = queue,
                    .queue_family_index = @intCast(index),
                };
            }
        }

        return error.NoUsableVulkanGraphicsQueue;
    }

    pub fn deinit(self: *VulkanAttachContext) void {
        _ = self.dispatch.device_wait_idle.?(self.device);
        self.dispatch.destroy_device.?(self.device, null);
        self.dispatch.destroy_instance.?(self.instance, null);
        self.dispatch.deinit();
    }

    pub fn descriptor(self: *const VulkanAttachContext) maplibre.VulkanContextDescriptor {
        return .{
            .instance = @ptrCast(self.instance.?),
            .physical_device = @ptrCast(self.physical_device.?),
            .device = @ptrCast(self.device.?),
            .graphics_queue = @ptrCast(self.queue.?),
            .graphics_queue_family_index = self.queue_family_index,
            .get_instance_proc_addr = nativeFunctionPointer(self.dispatch.get_instance_proc_addr),
            .get_device_proc_addr = nativeFunctionPointer(self.dispatch.get_device_proc_addr),
        };
    }
} else struct {};

const VulkanDispatch = if (build_options.supports_vulkan) struct {
    get_instance_proc_addr: vk.PFN_vkGetInstanceProcAddr,
    get_device_proc_addr: vk.PFN_vkGetDeviceProcAddr,
    create_instance: vk.PFN_vkCreateInstance,
    destroy_instance: vk.PFN_vkDestroyInstance = null,
    enumerate_physical_devices: vk.PFN_vkEnumeratePhysicalDevices = null,
    get_physical_device_queue_family_properties: vk.PFN_vkGetPhysicalDeviceQueueFamilyProperties = null,
    get_physical_device_features: vk.PFN_vkGetPhysicalDeviceFeatures = null,
    get_physical_device_memory_properties: vk.PFN_vkGetPhysicalDeviceMemoryProperties = null,
    enumerate_device_extension_properties: vk.PFN_vkEnumerateDeviceExtensionProperties = null,
    create_device: vk.PFN_vkCreateDevice = null,
    destroy_device: vk.PFN_vkDestroyDevice = null,
    device_wait_idle: vk.PFN_vkDeviceWaitIdle = null,
    get_device_queue: vk.PFN_vkGetDeviceQueue = null,
    create_image: vk.PFN_vkCreateImage = null,
    destroy_image: vk.PFN_vkDestroyImage = null,
    get_image_memory_requirements: vk.PFN_vkGetImageMemoryRequirements = null,
    allocate_memory: vk.PFN_vkAllocateMemory = null,
    free_memory: vk.PFN_vkFreeMemory = null,
    bind_image_memory: vk.PFN_vkBindImageMemory = null,
    create_image_view: vk.PFN_vkCreateImageView = null,
    destroy_image_view: vk.PFN_vkDestroyImageView = null,

    fn init() !VulkanDispatch {
        return .{
            .get_instance_proc_addr = vk.vkGetInstanceProcAddr,
            .get_device_proc_addr = vk.vkGetDeviceProcAddr,
            .create_instance = vk.vkCreateInstance,
            .destroy_instance = vk.vkDestroyInstance,
            .enumerate_physical_devices = vk.vkEnumeratePhysicalDevices,
            .get_physical_device_queue_family_properties = vk.vkGetPhysicalDeviceQueueFamilyProperties,
            .get_physical_device_features = vk.vkGetPhysicalDeviceFeatures,
            .get_physical_device_memory_properties = vk.vkGetPhysicalDeviceMemoryProperties,
            .enumerate_device_extension_properties = vk.vkEnumerateDeviceExtensionProperties,
            .create_device = vk.vkCreateDevice,
            .destroy_device = vk.vkDestroyDevice,
            .device_wait_idle = vk.vkDeviceWaitIdle,
            .get_device_queue = vk.vkGetDeviceQueue,
            .create_image = vk.vkCreateImage,
            .destroy_image = vk.vkDestroyImage,
            .get_image_memory_requirements = vk.vkGetImageMemoryRequirements,
            .allocate_memory = vk.vkAllocateMemory,
            .free_memory = vk.vkFreeMemory,
            .bind_image_memory = vk.vkBindImageMemory,
            .create_image_view = vk.vkCreateImageView,
            .destroy_image_view = vk.vkDestroyImageView,
        };
    }

    fn deinit(_: *VulkanDispatch) void {}

    fn loadInstanceFunctions(_: *VulkanDispatch, _: vk.VkInstance) void {}

    fn loadDeviceFunctions(_: *VulkanDispatch, _: vk.VkDevice) void {}
} else struct {};

fn nativeFunctionPointer(function: anytype) ?*anyopaque {
    return @ptrFromInt(@intFromPtr(function.?));
}

fn hasDeviceExtension(dispatch: *const VulkanDispatch, physical_device: if (build_options.supports_vulkan) vk.VkPhysicalDevice else ?*anyopaque, name: [*c]const u8) !bool {
    if (!build_options.supports_vulkan) return false;

    var count: u32 = 0;
    try expectVk(dispatch.enumerate_device_extension_properties.?(physical_device, null, &count, null));

    var properties_buffer: [256]vk.VkExtensionProperties = undefined;
    if (count > properties_buffer.len) count = properties_buffer.len;
    try expectVk(dispatch.enumerate_device_extension_properties.?(physical_device, null, &count, &properties_buffer));

    const expected = std.mem.span(name);
    for (properties_buffer[0..count]) |property| {
        if (std.mem.eql(u8, std.mem.span(@as([*:0]const u8, @ptrCast(&property.extensionName))), expected)) return true;
    }
    return false;
}

const VulkanBorrowedImage = if (build_options.supports_vulkan) struct {
    context: VulkanAttachContext,
    image: vk.VkImage,
    image_view: vk.VkImageView,
    memory: vk.VkDeviceMemory,
    width: u32,
    height: u32,

    /// A caller-owned image with the memory and view that go with it.
    pub const Allocation = struct {
        image: vk.VkImage,
        image_view: vk.VkImageView,
        memory: vk.VkDeviceMemory,
    };

    pub fn create(width: u32, height: u32) !VulkanBorrowedImage {
        var context = try VulkanAttachContext.init();
        errdefer context.deinit();

        const allocation = try allocate(&context, width, height);
        return .{
            .context = context,
            .image = allocation.image,
            .image_view = allocation.image_view,
            .memory = allocation.memory,
            .width = width,
            .height = height,
        };
    }

    fn allocate(context: *const VulkanAttachContext, width: u32, height: u32) !Allocation {
        var image = nullVulkanHandle(vk.VkImage);
        var memory = nullVulkanHandle(vk.VkDeviceMemory);
        var image_view = nullVulkanHandle(vk.VkImageView);
        errdefer {
            if (!isNullVulkanHandle(image_view)) context.dispatch.destroy_image_view.?(context.device, image_view, null);
            if (!isNullVulkanHandle(image)) context.dispatch.destroy_image.?(context.device, image, null);
            if (!isNullVulkanHandle(memory)) context.dispatch.free_memory.?(context.device, memory, null);
        }

        var image_info = std.mem.zeroes(vk.VkImageCreateInfo);
        image_info.sType = vk.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = vk.VK_IMAGE_TYPE_2D;
        image_info.format = vk.VK_FORMAT_R8G8B8A8_UNORM;
        image_info.extent = .{ .width = width, .height = height, .depth = 1 };
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = vk.VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = vk.VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = vk.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | vk.VK_IMAGE_USAGE_SAMPLED_BIT | vk.VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        image_info.sharingMode = vk.VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = vk.VK_IMAGE_LAYOUT_UNDEFINED;
        try expectVk(context.dispatch.create_image.?(context.device, &image_info, null, &image));

        var requirements: vk.VkMemoryRequirements = undefined;
        context.dispatch.get_image_memory_requirements.?(context.device, image, &requirements);

        var allocate_info = std.mem.zeroes(vk.VkMemoryAllocateInfo);
        allocate_info.sType = vk.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = requirements.size;
        allocate_info.memoryTypeIndex = try findVulkanMemoryType(&context.dispatch, context.physical_device, requirements.memoryTypeBits, vk.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        try expectVk(context.dispatch.allocate_memory.?(context.device, &allocate_info, null, &memory));
        try expectVk(context.dispatch.bind_image_memory.?(context.device, image, memory, 0));

        var view_info = std.mem.zeroes(vk.VkImageViewCreateInfo);
        view_info.sType = vk.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = vk.VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = vk.VK_FORMAT_R8G8B8A8_UNORM;
        view_info.subresourceRange = .{
            .aspectMask = vk.VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        try expectVk(context.dispatch.create_image_view.?(context.device, &view_info, null, &image_view));

        return .{ .image = image, .image_view = image_view, .memory = memory };
    }

    pub fn deinit(self: *VulkanBorrowedImage) void {
        _ = self.context.dispatch.device_wait_idle.?(self.context.device);
        self.release(.{ .image = self.image, .image_view = self.image_view, .memory = self.memory });
        self.context.deinit();
    }

    /// Allocates a replacement on this helper's own device. The outgoing image
    /// stays live until `adopt`.
    pub fn allocateReplacement(self: *const VulkanBorrowedImage, width: u32, height: u32) !Allocation {
        return allocate(&self.context, width, height);
    }

    /// Tracks a replacement the session has taken and releases the outgoing one.
    pub fn adopt(self: *VulkanBorrowedImage, allocation: Allocation, width: u32, height: u32) void {
        _ = self.context.dispatch.device_wait_idle.?(self.context.device);
        self.release(.{ .image = self.image, .image_view = self.image_view, .memory = self.memory });
        self.image = allocation.image;
        self.image_view = allocation.image_view;
        self.memory = allocation.memory;
        self.width = width;
        self.height = height;
    }

    fn release(self: *const VulkanBorrowedImage, allocation: Allocation) void {
        self.context.dispatch.destroy_image_view.?(self.context.device, allocation.image_view, null);
        self.context.dispatch.destroy_image.?(self.context.device, allocation.image, null);
        self.context.dispatch.free_memory.?(self.context.device, allocation.memory, null);
    }

    pub fn descriptor(self: *const VulkanBorrowedImage) maplibre.VulkanBorrowedTextureDescriptor {
        return self.descriptorFor(.{ .image = self.image, .image_view = self.image_view, .memory = self.memory }, self.width, self.height);
    }

    pub fn descriptorFor(self: *const VulkanBorrowedImage, allocation: Allocation, width: u32, height: u32) maplibre.VulkanBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = width, .height = height, .scale_factor = 1.0 },
            .physical_width = width,
            .physical_height = height,
            .context = self.context.descriptor(),
            .image = vulkanHandleToBinding(allocation.image),
            .image_view = vulkanHandleToBinding(allocation.image_view),
            .format = @as(u32, vk.VK_FORMAT_R8G8B8A8_UNORM),
            .initial_layout = @as(u32, vk.VK_IMAGE_LAYOUT_UNDEFINED),
            .final_layout = @as(u32, vk.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        };
    }
} else struct {};

fn expectVk(result: if (build_options.supports_vulkan) vk.VkResult else i32) !void {
    if (build_options.supports_vulkan) try testing.expectEqual(vk.VK_SUCCESS, result);
}

fn findVulkanMemoryType(dispatch: *const VulkanDispatch, physical_device: if (build_options.supports_vulkan) vk.VkPhysicalDevice else ?*anyopaque, type_filter: u32, properties: if (build_options.supports_vulkan) vk.VkMemoryPropertyFlags else u32) !u32 {
    var memory_properties: vk.VkPhysicalDeviceMemoryProperties = undefined;
    dispatch.get_physical_device_memory_properties.?(physical_device, &memory_properties);

    for (0..memory_properties.memoryTypeCount) |index| {
        const type_bit = @as(u32, 1) << @as(u5, @intCast(index));
        const memory_type = memory_properties.memoryTypes[index];
        if ((type_filter & type_bit) != 0 and (memory_type.propertyFlags & properties) == properties) {
            return @intCast(index);
        }
    }
    return error.NoSuitableVulkanMemoryType;
}

fn expectOwnedFrameExtent(
    session: maplibre.RenderSession,
    extent: maplibre.RenderTargetExtent,
) !void {
    const frame = try maplibre.renderSessionAcquireFrame(session);
    const View = if (build_options.supports_vulkan) maplibre.VulkanOwnedTextureFrame else if (build_options.supports_opengl) maplibre.OpenglOwnedTextureFrame else maplibre.MetalOwnedTextureFrame;
    const inspect = struct {
        fn use(expected: maplibre.RenderTargetExtent, info: View) anyerror!void {
            try testing.expectEqual(expected.width, info.width);
            try testing.expectEqual(expected.height, info.height);
            try testing.expectEqual(expected.scale_factor, info.scale_factor);
            if (build_options.supports_vulkan) {
                try testing.expect(info.image != 0);
                try testing.expect(info.image != @intFromPtr(info.device.?));
            } else if (build_options.supports_opengl) {
                try testing.expect(info.texture != 0);
            } else {
                try testing.expect(info.texture != null);
            }
        }
    }.use;
    if (build_options.supports_vulkan) try maplibre.acquiredFrameGetVulkanTexture(void, frame, extent, inspect) else if (build_options.supports_opengl) try maplibre.acquiredFrameGetOpenglTexture(void, frame, extent, inspect) else try maplibre.acquiredFrameGetMetalTexture(void, frame, extent, inspect);
    try maplibre.acquiredFrameGetProducerSync(void, frame, {}, struct {
        fn use(_: void, _: maplibre.GpuSync) anyerror!void {}
    }.use);
    try maplibre.acquiredFrameRelease(testing.allocator, frame, .{ .kind = .cpu_complete });
}

test "owned texture session renders acquires resizes and reads back" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    const initial_extent = maplibre.RenderTargetExtent{ .width = 32, .height = 16, .scale_factor = 1.0 };
    var owned = try attachTestOwnedTexture(&map, initial_extent);
    defer owned.close() catch @panic("render session close failed");
    const capabilities = try maplibre.renderSessionGetCapabilities(support.handle(owned.session));
    try testing.expect(capabilities.flags.frame_acquisition);
    try testing.expectEqual(.attached, (try maplibre.renderSessionGetSnapshot(support.handle(owned.session))).state);

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(.rendered, (try support.renderFrame(owned.session, false, true)).disposition);
    try expectOwnedFrameExtent(owned.session, initial_extent);

    try testing.expect(capabilities.flags.readback);
    {
        var image = try resolveFuture(maplibre.OwnedValue(maplibre.TextureReadbackResult), owned.session, try maplibre.textureReadPremultipliedRgba8(testing.allocator, owned.session));
        defer image.deinit();
        try testing.expectEqual(@as(u32, 32), image.value.info.width);
        try testing.expectEqual(@as(u32, 16), image.value.info.height);
        try testing.expectEqual(@as(u32, 32 * 4), image.value.info.stride);
        try testing.expectEqual(image.value.info.byte_length, image.value.data.len);
        try testing.expect(hasNonZeroByte(image.value.data));
    }

    const resized_extent = maplibre.RenderTargetExtent{ .width = 48, .height = 24, .scale_factor = 1.0 };
    try finishOperation(owned.session, try maplibre.renderSessionResize(testing.allocator, support.handle(owned.session), resized_extent));
    try support.waitForBarrier(&runtime);
    for (0..1000) |_| {
        const result = try support.renderFrame(owned.session, false, true);
        if (result.disposition == .rendered) break;
        try testing.expectEqual(.size_pending, result.disposition);
        // The repaint signal is meaningful only on a rendered frame.
        try testing.expect(!result.needs_repaint);
    } else return error.ResizeDidNotConverge;
    try expectOwnedFrameExtent(owned.session, resized_extent);
    const snapshot = try maplibre.renderSessionGetSnapshot(support.handle(owned.session));
    try testing.expectEqual(resized_extent.width, snapshot.extent.width);
    try testing.expectEqual(resized_extent.height, snapshot.extent.height);
}

test "a session with no rendered frame has nothing to acquire or read back" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try testing.expect((try maplibre.renderSessionGetCapabilities(support.handle(owned.session))).flags.readback);
    // No frame has rendered, so neither a readback nor an acquisition has a
    // frame to take.
    try testing.expectError(error.NotReady, maplibre.renderSessionAcquireFrame(support.handle(owned.session)));
    try testing.expectError(error.InvalidState, resolveFuture(maplibre.OwnedValue(maplibre.TextureReadbackResult), owned.session, try maplibre.textureReadPremultipliedRgba8(testing.allocator, owned.session)));
}

test "live render session blocks map close until detached" {
    if (!supports_test_owned_texture) return error.SkipZigTest;

    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();
    var runtime = try support.createRuntimeWithDiagnostics(.{}, &diagnostics);
    errdefer support.closeRuntime(&runtime) catch {};
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
    errdefer support.closeMap(&map) catch {};
    var owned = try attachTestOwnedTexture(&map, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
    errdefer owned.close() catch {};

    try testing.expectError(error.InvalidState, maplibre.mapRelease(support.handle(map)));
    try testing.expectEqualStrings("map still has an attached render session", diagnostics.get().?.message);

    try owned.close();
    try support.closeMap(&map);
    try support.closeRuntime(&runtime);
}

test "still-image map modes complete owned texture renders" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    inline for (.{ maplibre.MapMode.static, maplibre.MapMode.tile }) |mode| {
        var runtime = try support.createRuntime(.{});
        defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
        var map = try support.createMap(&runtime, .{ .width = 32, .height = 32, .mode = mode });
        defer support.closeMap(&map) catch @panic("map close failed");
        var owned = try attachTestOwnedTexture(&map, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
        defer owned.close() catch @panic("render session close failed");

        try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
        try support.waitForBarrier(&runtime);
        var future = try maplibre.mapRequestStillImage(support.handle(map));
        defer future.deinit();
        for (0..1000) |_| {
            _ = try support.renderFrame(owned.session, false, true);
            if (try future.poll()) break;
            try std.Thread.yield();
        } else return error.StillImageDidNotComplete;
        _ = try future.wait(null);
        try expectOwnedFrameExtent(owned.session, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
    }
}

const feature_state_style_json =
    \\{"version":8,"sources":{"point":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","properties":{},"geometry":{"type":"Point","coordinates":[0,0]}}]}}},"layers":[{"id":"circle","type":"circle","source":"point","paint":{"circle-radius":8}}]}
;

const cluster_style_json =
    \\{"version":8,"sources":{"cluster-source":{"type":"geojson","cluster":true,"data":{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one"}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two"}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three"}}]}}},"layers":[{"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-radius":20}}]}
;

fn skipJsonWhitespace(json: []const u8, start: usize) usize {
    var cursor = start;
    while (cursor < json.len and std.ascii.isWhitespace(json[cursor])) cursor += 1;
    return cursor;
}

fn jsonStringEnd(json: []const u8, start: usize) ?usize {
    var escaped = false;
    var cursor = start + 1;
    while (cursor < json.len) : (cursor += 1) {
        if (escaped) escaped = false else if (json[cursor] == '\\') escaped = true else if (json[cursor] == '"') return cursor + 1;
    }
    return null;
}

fn jsonValueEnd(json: []const u8, start: usize) ?usize {
    if (start >= json.len) return null;
    if (json[start] == '"') return jsonStringEnd(json, start);
    if (json[start] != '{' and json[start] != '[') {
        var cursor = start;
        while (cursor < json.len and !std.ascii.isWhitespace(json[cursor]) and json[cursor] != ',' and json[cursor] != '}' and json[cursor] != ']') cursor += 1;
        return cursor;
    }
    var depth: usize = 0;
    var cursor = start;
    while (cursor < json.len) {
        if (json[cursor] == '"') {
            cursor = jsonStringEnd(json, cursor) orelse return null;
            continue;
        }
        if (json[cursor] == '{' or json[cursor] == '[') depth += 1;
        if (json[cursor] == '}' or json[cursor] == ']') {
            depth -= 1;
            if (depth == 0) return cursor + 1;
        }
        cursor += 1;
    }
    return null;
}

fn rawJsonMember(json: []const u8, key: []const u8) ?[]const u8 {
    var cursor = skipJsonWhitespace(json, 0);
    if (cursor >= json.len or json[cursor] != '{') return null;
    cursor += 1;
    while (true) {
        cursor = skipJsonWhitespace(json, cursor);
        if (cursor >= json.len or json[cursor] == '}') return null;
        const key_end = jsonStringEnd(json, cursor) orelse return null;
        const member_name = json[cursor + 1 .. key_end - 1];
        cursor = skipJsonWhitespace(json, key_end);
        if (cursor >= json.len or json[cursor] != ':') return null;
        const value_start = skipJsonWhitespace(json, cursor + 1);
        const value_end = jsonValueEnd(json, value_start) orelse return null;
        if (std.mem.eql(u8, member_name, key)) return json[value_start..value_end];
        cursor = skipJsonWhitespace(json, value_end);
        if (cursor >= json.len or json[cursor] != ',') return null;
        cursor += 1;
    }
}

fn firstJsonArrayElement(json: []const u8) ?[]const u8 {
    var cursor = skipJsonWhitespace(json, 0);
    if (cursor >= json.len or json[cursor] != '[') return null;
    cursor = skipJsonWhitespace(json, cursor + 1);
    if (cursor >= json.len or json[cursor] == ']') return null;
    return json[cursor .. jsonValueEnd(json, cursor) orelse return null];
}

fn firstLeafName(collection: []const u8) ?[]const u8 {
    const features = rawJsonMember(collection, "features") orelse return null;
    const feature = firstJsonArrayElement(features) orelse return null;
    const properties = rawJsonMember(feature, "properties") orelse return null;
    return rawJsonMember(properties, "name");
}

test "feature state and rendered queries copy operation results" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), feature_state_style_json));
    try support.waitForBarrier(&runtime);
    _ = try support.renderFrame(owned.session, false, true);

    const selector = maplibre.FeatureStateSelector{ .source_id = "point", .feature_id = "feature-1" };
    try support.expectCommitted(try maplibre.mapSetFeatureState(testing.allocator, support.handle(map), selector, "{\"hover\":true,\"count\":3}"));
    var state_future = try maplibre.mapGetFeatureState(testing.allocator, support.handle(map), selector);
    defer state_future.deinit();
    var state = try state_future.wait(null);
    defer state.deinit();
    try testing.expect(std.mem.indexOf(u8, state.value, "\"hover\":true") != null);

    for (0..1000) |_| {
        var result = try resolveFuture(
            maplibre.OwnedValue([]const maplibre.QueriedFeature),
            owned.session,
            try maplibre.renderSessionQueryRenderedFeatures(
                testing.allocator,
                support.handle(owned.session),
                .{ .data = .{ .box = .{
                    .min = .{ .x = 0, .y = 0 },
                    .max = .{ .x = 64, .y = 64 },
                } } },
                null,
            ),
        );
        defer result.deinit();
        if (result.value.len != 0 and result.value[0].state != null) {
            try testing.expectEqualStrings("point", result.value[0].source_id.?);
            try testing.expect(std.mem.indexOf(u8, result.value[0].state.?, "\"hover\":true") != null);
            break;
        }
        try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
        try support.waitForBarrier(&runtime);
        _ = try support.renderFrame(owned.session, false, true);
    } else return error.RenderedFeatureNotQueryable;

    var source = try resolveFuture(
        maplibre.OwnedValue([]const maplibre.QueriedFeature),
        owned.session,
        try maplibre.renderSessionQuerySourceFeatures(testing.allocator, support.handle(owned.session), "point", null),
    );
    defer source.deinit();
    try testing.expectEqualStrings("point", source.value[0].source_id.?);
    try testing.expect(std.mem.indexOf(u8, source.value[0].feature, "\"type\":\"Point\"") != null);

    try support.expectCommitted(try maplibre.mapRemoveFeatureState(testing.allocator, support.handle(map), selector));
}

fn featureState(map: *maplibre.Map, selector: maplibre.FeatureStateSelector) !maplibre.OwnedValue([]const u8) {
    var future = try maplibre.mapGetFeatureState(testing.allocator, support.handle(map), selector);
    defer future.deinit();
    return future.wait(null);
}

// Feature state belongs to the map, so it needs no loaded style, ordered reads
// observe every earlier command, and it survives style loads and a renderer
// retirement driven by a scale-factor change.
test "map feature state set get and remove" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    const selector = maplibre.FeatureStateSelector{ .source_id = "point", .feature_id = "feature-1" };
    const feature_state = "{\"hover\":true,\"radius\":18446744073709551615}";
    try support.expectCommitted(try maplibre.mapSetFeatureState(testing.allocator, support.handle(map), selector, feature_state));
    try support.expectCommitted(try maplibre.mapRemoveFeatureState(testing.allocator, support.handle(map), .{ .source_id = "point", .feature_id = "feature-1", .state_key = "hover" }));
    var queued = try featureState(&map, selector);
    defer queued.deinit();
    try testing.expect(rawJsonMember(queued.value, "hover") == null);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(queued.value, "radius").?);

    // A style load drops style-owned objects, not map-owned feature state.
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), feature_state_style_json));
    try support.waitForBarrier(&runtime);
    _ = try support.renderFrame(owned.session, false, true);
    var after_style = try featureState(&map, selector);
    defer after_style.deinit();
    try testing.expect(rawJsonMember(after_style.value, "hover") == null);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(after_style.value, "radius").?);

    try support.expectCommitted(try maplibre.mapSetFeatureState(testing.allocator, support.handle(map), selector, feature_state));
    var restored = try featureState(&map, selector);
    defer restored.deinit();
    try testing.expectEqualStrings("true", rawJsonMember(restored.value, "hover").?);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(restored.value, "radius").?);

    try testing.expectError(error.InvalidArgument, maplibre.mapRemoveFeatureState(testing.allocator, support.handle(map), .{ .source_id = "point", .state_key = "hover" }));

    // The scale factor is fixed at attach, so a resize that changes it is
    // rejected and the session keeps the extent it had.
    try testing.expectError(
        error.InvalidArgument,
        maplibre.renderSessionResize(testing.allocator, support.handle(owned.session), .{ .width = 64, .height = 64, .scale_factor = 2.0 }),
    );
    try testing.expectEqual(@as(f64, 1.0), (try maplibre.renderSessionGetSnapshot(support.handle(owned.session))).extent.scale_factor);

    // A size change retires the renderer; map-owned state survives.
    try finishOperation(owned.session, try maplibre.renderSessionResize(testing.allocator, support.handle(owned.session), .{ .width = 96, .height = 48, .scale_factor = 1.0 }));
    _ = try support.expectRenderedFrame(owned.session, true);
    var after_resize = try featureState(&map, selector);
    defer after_resize.deinit();
    try testing.expectEqualStrings("true", rawJsonMember(after_resize.value, "hover").?);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(after_resize.value, "radius").?);
}

test "cluster feature extensions copy values and feature collections" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), cluster_style_json));
    try support.waitForBarrier(&runtime);
    _ = try support.renderFrame(owned.session, false, true);

    var cluster_result: ?maplibre.OwnedValue([]const maplibre.QueriedFeature) = null;
    for (0..1000) |_| {
        var result = try resolveFuture(
            maplibre.OwnedValue([]const maplibre.QueriedFeature),
            owned.session,
            try maplibre.renderSessionQueryRenderedFeatures(
                testing.allocator,
                support.handle(owned.session),
                .{ .data = .{ .box = .{
                    .min = .{ .x = 0, .y = 0 },
                    .max = .{ .x = 64, .y = 64 },
                } } },
                null,
            ),
        );
        if (result.value.len != 0) {
            cluster_result = result;
            break;
        }
        result.deinit();
        try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
        try support.waitForBarrier(&runtime);
        _ = try support.renderFrame(owned.session, false, true);
    }
    var clusters = cluster_result orelse return error.ClusterFeatureNotQueryable;
    defer clusters.deinit();
    const feature = clusters.value[0].feature;
    const properties = rawJsonMember(feature, "properties").?;
    _ = try std.fmt.parseInt(u64, rawJsonMember(properties, "cluster_id").?, 10);
    try testing.expectEqualStrings("3", rawJsonMember(properties, "point_count").?);

    var children = try resolveFuture(
        maplibre.OwnedValue([]const u8),
        owned.session,
        try maplibre.renderSessionQueryFeatureExtensions(
            testing.allocator,
            owned.session,
            "cluster-source",
            feature,
            "supercluster",
            "children",
            null,
        ),
    );
    defer children.deinit();
    try testing.expect(firstJsonArrayElement(rawJsonMember(children.value, "features").?) != null);

    var expansion_zoom = try resolveFuture(
        maplibre.OwnedValue([]const u8),
        owned.session,
        try maplibre.renderSessionQueryFeatureExtensions(
            testing.allocator,
            owned.session,
            "cluster-source",
            feature,
            "supercluster",
            "expansion-zoom",
            null,
        ),
    );
    defer expansion_zoom.deinit();
    _ = try std.fmt.parseInt(u64, expansion_zoom.value, 10);

    var first_leaf = try resolveFuture(
        maplibre.OwnedValue([]const u8),
        owned.session,
        try maplibre.renderSessionQueryFeatureExtensions(
            testing.allocator,
            owned.session,
            "cluster-source",
            feature,
            "supercluster",
            "leaves",
            "{\"limit\":1,\"offset\":0}",
        ),
    );
    defer first_leaf.deinit();
    var second_leaf = try resolveFuture(
        maplibre.OwnedValue([]const u8),
        owned.session,
        try maplibre.renderSessionQueryFeatureExtensions(
            testing.allocator,
            owned.session,
            "cluster-source",
            feature,
            "supercluster",
            "leaves",
            "{\"limit\":1,\"offset\":1}",
        ),
    );
    defer second_leaf.deinit();
    try testing.expect(!std.mem.eql(u8, firstLeafName(first_leaf.value).?, firstLeafName(second_leaf.value).?));
}

test "sustained frame demands outlast the texture ring depth" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);

    for (0..64) |_| {
        try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
        try support.waitForBarrier(&runtime);
        try testing.expectEqual(.rendered, (try support.renderFrame(owned.session, false, true)).disposition);
    }
    try testing.expect((try maplibre.renderSessionGetSnapshot(support.handle(owned.session))).frame_generation >= 64);
}

test "a rendered frame during an ease reports needs repaint" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);
    // Settle the style's own frames so the transition drives what follows.
    try testing.expectEqual(.rendered, (try support.renderFrame(owned.session, false, true)).disposition);

    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{
        .mode = .ease,
        .camera = .{ .center = .{ .latitude = 37.7749, .longitude = -122.4194 }, .zoom = 4.0 },
        .animation = .{ .duration_ms = 60_000 },
    }));
    try support.waitForBarrier(&runtime);

    // Mid-transition the map asks for the next frame with the one it renders,
    // the same signal a map-render-frame-finished event carries.
    var observed_repaint = false;
    for (0..100) |_| {
        const result = try support.renderFrame(owned.session, false, true);
        if (result.disposition == .rendered and result.needs_repaint) {
            observed_repaint = true;
            break;
        }
    }
    try testing.expect(observed_repaint);
}

fn readSnapshotOnThread(session: maplibre.RenderSession, failure: *?anyerror) void {
    _ = maplibre.renderSessionGetSnapshot(session) catch |err| {
        failure.* = err;
        return;
    };
}

test "render session controls are usable from another thread" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    var failure: ?anyerror = null;
    const thread = try std.Thread.spawn(.{}, readSnapshotOnThread, .{ owned.session, &failure });
    thread.join();
    try testing.expectEqual(@as(?anyerror, null), failure);
}

test "Vulkan borrowed texture replaces its target" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var borrowed = try VulkanBorrowedImage.create(32, 16);
    defer borrowed.deinit();
    var session = try finishAttachment(try maplibre.vulkanBorrowedTextureAttach(
        testing.allocator,
        support.handle(map),
        borrowed.descriptor(),
        .{ .driver = .core_worker, .requested_texture_ring_depth = 1 },
    ));
    defer support.closeSession(&session, true) catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);
    for (0..1000) |_| {
        if ((try support.renderFrame(session, false, true)).disposition == .rendered) break;
        try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
        try support.waitForBarrier(&runtime);
    } else return error.FrameDidNotRender;

    {
        const replacement = try borrowed.allocateReplacement(48, 24);
        errdefer borrowed.release(replacement);
        try finishOperation(
            session,
            try maplibre.vulkanBorrowedTextureSetTarget(testing.allocator, session, borrowed.descriptorFor(replacement, 48, 24)),
        );
        borrowed.adopt(replacement, 48, 24);
    }
    // A borrowed texture belongs to the host, so the session cannot resize it;
    // the replacement target carries the new size and the map takes it here.
    try testing.expectError(error.Unsupported, maplibre.renderSessionResize(testing.allocator, support.handle(session), .{ .width = 48, .height = 24, .scale_factor = 1.0 }));
    try support.expectCommitted(try maplibre.mapResize(support.handle(map), .{ .width = 48, .height = 24, .scale_factor = 1.0 }));
    try support.waitForBarrier(&runtime);
    for (0..1000) |_| {
        if ((try support.renderFrame(session, false, true)).disposition == .rendered) break;
        try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
        try support.waitForBarrier(&runtime);
    } else return error.FrameDidNotRender;
    const snapshot = try maplibre.renderSessionGetSnapshot(session);
    try testing.expectEqual(@as(u32, 48), snapshot.extent.width);
    try testing.expectEqual(@as(u32, 24), snapshot.extent.height);
}

test "OpenGL borrowed texture replaces its target" {
    if (!build_options.supports_opengl) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var borrowed = try OpenGLBorrowedTexture.create(32, 16);
    defer borrowed.deinit();
    var session = try finishAttachment(try maplibre.openglBorrowedTextureAttach(
        testing.allocator,
        support.handle(map),
        borrowed.descriptor(),
        .{ .driver = .caller_graphics_thread, .requested_texture_ring_depth = 1 },
    ));
    defer support.closeSession(&session, true) catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(.rendered, (try support.renderFrame(session, false, true)).disposition);
    var initial_pixels: [32 * 16 * 4]u8 = undefined;
    try borrowed.readRGBA8(&initial_pixels);
    try testing.expect(hasNonZeroByte(&initial_pixels));

    {
        const replacement = try borrowed.allocateReplacement(48, 24);
        errdefer borrowed.context.destroyTexture(replacement);
        try finishOperation(
            session,
            try maplibre.openglBorrowedTextureSetTarget(testing.allocator, session, borrowed.descriptorFor(replacement, 48, 24)),
        );
        borrowed.adopt(replacement, 48, 24);
    }
    // A borrowed texture belongs to the host, so the session cannot resize it;
    // the replacement target carries the new size and the map takes it here.
    try testing.expectError(error.Unsupported, maplibre.renderSessionResize(testing.allocator, support.handle(session), .{ .width = 48, .height = 24, .scale_factor = 1.0 }));
    try support.expectCommitted(try maplibre.mapResize(support.handle(map), .{ .width = 48, .height = 24, .scale_factor = 1.0 }));
    try support.waitForBarrier(&runtime);
    for (0..1000) |_| {
        if ((try support.renderFrame(session, false, true)).disposition == .rendered) break;
        try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
        try support.waitForBarrier(&runtime);
    } else return error.FrameDidNotRender;
}

test "a map takes one render session at a time" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try testing.expectError(error.InvalidState, attachTestOwnedTexture(&map, .{ .width = 16, .height = 16, .scale_factor = 1.0 }));

    // Detaching frees the map for the next session.
    try owned.close();
    var replacement = try attachTestOwnedTexture(&map, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer replacement.close() catch @panic("render session close failed");
    try testing.expectEqual(.attached, (try maplibre.renderSessionGetSnapshot(support.handle(replacement.session))).state);
}

test "a detached session rejects the calls that need a target" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var context = try TestOwnedTextureContext.init();
    defer context.deinit();
    const session = try attachOwnedTexture(&map, &context, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer maplibre.renderSessionDestroy(support.handle(session)) catch @panic("render session destroy failed");

    // Establish a renderable update before racing a demand with detach.
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);
    try testing.expectEqual(.rendered, (try support.renderFrame(session, false, true)).disposition);

    // A demand still outstanding at detach reports a target that went away.
    const token = support.nextFrameToken();
    try maplibre.renderSessionRequestFrame(testing.allocator, support.handle(session), .{ .flags = .{ .if_needed = false }, .token = token });
    try finishOperation(session, try maplibre.renderSessionDetach(support.handle(session)));

    try testing.expectEqual(.detached, (try maplibre.renderSessionGetSnapshot(session)).state);
    try testing.expectError(error.InvalidState, maplibre.renderSessionAcquireFrame(support.handle(session)));
    try testing.expectError(error.InvalidState, maplibre.renderSessionResize(testing.allocator, support.handle(session), .{ .width = 32, .height = 32, .scale_factor = 1.0 }));

    // Detach resolves the demand either way: the frame it rendered before the
    // detach, or the target that went away.
    var batch = try maplibre.renderSessionDrainFrameResults(support.handle(session));
    defer batch.deinit();
    var saw_token = false;
    for (0..try maplibre.renderFrameBatchCount(batch)) |index| {
        const result = try maplibre.renderFrameBatchGet(batch, index);
        if (result.token != token) continue;
        saw_token = true;
        switch (result.disposition) {
            .rendered, .target_not_ready => {},
            else => return error.UnexpectedFrameDisposition,
        }
    }
    try testing.expect(saw_token);
}

test "a set-target call for another target kind reports unsupported" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    // The session validates its retarget kind before it reads host handles, so
    // a placeholder surface descriptor is enough.
    const extent = maplibre.RenderTargetExtent{ .width = 16, .height = 16, .scale_factor = 1.0 };
    if (build_options.supports_vulkan) {
        try testing.expectError(error.Unsupported, maplibre.vulkanSurfaceSetTarget(testing.allocator, owned.session, .{
            .extent = extent,
            .surface = 1,
            .context = fakeVulkanContext(),
        }));
    } else if (build_options.supports_opengl) {
        try testing.expectError(error.Unsupported, maplibre.openglSurfaceSetTarget(testing.allocator, owned.session, .{
            .extent = extent,
            .surface = fakeNativePointer(),
            .context = fakeOpenGLContext(),
        }));
    } else if (build_options.supports_metal) {
        try testing.expectError(error.Unsupported, maplibre.metalSurfaceSetTarget(testing.allocator, owned.session, .{
            .extent = extent,
            .layer = fakeNativePointer(),
        }));
    }
}

test "an empty frame-result drain reports an empty batch" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try testing.expectError(error.NotReady, maplibre.renderSessionDrainFrameResults(owned.session));
}

test "memory and data maintenance commands leave the session rendering" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 32, .height = 32, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(owned.session, true);

    try finishOperation(owned.session, try maplibre.renderSessionReduceMemoryUse(support.handle(owned.session)));
    try finishOperation(owned.session, try maplibre.renderSessionClearData(support.handle(owned.session)));
    try finishOperation(owned.session, try maplibre.renderSessionDumpDebugLogs(support.handle(owned.session)));
    try finishOperation(owned.session, try maplibre.renderSessionBarrier(support.handle(owned.session)));

    try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(owned.session, true);
}

test "disposing a parent graph retires its acquired frame and session" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer runtime.deinit();
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    var context = try TestOwnedTextureContext.init();
    defer context.deinit();
    var session = try attachOwnedTexture(&map, &context, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), "{\"version\":8,\"sources\":{},\"layers\":[]}"));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(session, true);
    var frame = try maplibre.renderSessionAcquireFrame(support.handle(session));
    map.deinit();
    frame.deinit();
    session.deinit();
    try testing.expectError(error.InvalidState, maplibre.acquiredFrameGetResult(frame));
    try testing.expectError(error.InvalidState, maplibre.renderSessionGetSnapshot(session));
}

test "abandoning a session releases the map without a graphics call" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var context = try TestOwnedTextureContext.init();
    defer context.deinit();
    const session = try attachOwnedTexture(&map, &context, .{ .width = 16, .height = 16, .scale_factor = 1.0 });

    const abandoned = try maplibre.renderSessionAbandon(support.handle(session));
    switch (abandoned.disposition) {
        .clean => try testing.expectEqual(@as(u32, 0), abandoned.quarantined_resource_count),
        .quarantined => try testing.expect(abandoned.quarantined_resource_count != 0),
        else => return error.UnexpectedAbandonDisposition,
    }
    try testing.expectEqual(.abandoned, (try maplibre.renderSessionGetSnapshot(session)).state);
    // An abandoned session is no longer attached, so a frame acquisition has
    // no target to take from.
    try testing.expectError(error.InvalidState, maplibre.renderSessionAcquireFrame(support.handle(session)));
    try maplibre.renderSessionDestroy(support.handle(session));

    // The abandoned session no longer holds the map, so the map closes.
    try support.closeMap(&map);
}

test "rendered and source queries clip and filter their inputs" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer owned.close() catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 4.0,
    } }));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(owned.session, true);

    const layer_options = maplibre.RenderedFeatureQueryOptions{ .layer_ids = &.{"point-circle"} };
    // A box wider than the viewport, an inverted box, and the viewport box all
    // normalize to the same clipped query.
    const boxes = [_]maplibre.ScreenBox{
        .{ .min = .{ .x = 0, .y = 0 }, .max = .{ .x = 64, .y = 64 } },
        .{ .min = .{ .x = -400, .y = -400 }, .max = .{ .x = 400, .y = 400 } },
        .{ .min = .{ .x = 64, .y = 64 }, .max = .{ .x = 0, .y = 0 } },
    };
    var hits: usize = 0;
    for (0..1000) |_| {
        hits = 0;
        for (boxes) |box| {
            var result = try resolveFuture(
                maplibre.OwnedValue([]const maplibre.QueriedFeature),
                owned.session,
                try maplibre.renderSessionQueryRenderedFeatures(testing.allocator, support.handle(owned.session), .{ .data = .{ .box = box } }, layer_options),
            );
            defer result.deinit();
            if (result.value.len != 0) hits += 1;
        }
        if (hits == boxes.len) break;
        try support.expectCommitted(try maplibre.mapRequestRepaint(support.handle(map)));
        try support.waitForBarrier(&runtime);
        _ = try support.renderFrame(owned.session, false, true);
    }
    try testing.expectEqual(boxes.len, hits);

    // A box entirely outside the viewport clips to nothing.
    var offscreen = try resolveFuture(
        maplibre.OwnedValue([]const maplibre.QueriedFeature),
        owned.session,
        try maplibre.renderSessionQueryRenderedFeatures(testing.allocator, support.handle(owned.session), .{ .data = .{ .box = .{
            .min = .{ .x = 400, .y = 400 },
            .max = .{ .x = 500, .y = 500 },
        } } }, layer_options),
    );
    defer offscreen.deinit();
    try testing.expectEqual(@as(usize, 0), offscreen.value.len);

    // A layer ID that names nothing filters every hit out.
    var filtered = try resolveFuture(
        maplibre.OwnedValue([]const maplibre.QueriedFeature),
        owned.session,
        try maplibre.renderSessionQueryRenderedFeatures(testing.allocator, support.handle(owned.session), .{ .data = .{ .box = boxes[0] } }, .{
            .layer_ids = &.{"no-such-layer"},
        }),
    );
    defer filtered.deinit();
    try testing.expectEqual(@as(usize, 0), filtered.value.len);

    // A source query takes its own options; a filter that matches nothing
    // leaves the result empty.
    var source_hits = try resolveFuture(
        maplibre.OwnedValue([]const maplibre.QueriedFeature),
        owned.session,
        try maplibre.renderSessionQuerySourceFeatures(testing.allocator, support.handle(owned.session), "point", .{
            .filter = "[\"==\", [\"get\", \"kind\"], \"capital\"]",
        }),
    );
    defer source_hits.deinit();
    try testing.expect(source_hits.value.len != 0);

    var source_misses = try resolveFuture(
        maplibre.OwnedValue([]const maplibre.QueriedFeature),
        owned.session,
        try maplibre.renderSessionQuerySourceFeatures(testing.allocator, support.handle(owned.session), "point", .{
            .filter = "[\"==\", [\"get\", \"kind\"], \"village\"]",
        }),
    );
    defer source_misses.deinit();
    try testing.expectEqual(@as(usize, 0), source_misses.value.len);
}

const supports_opengl_surface = supports_wgl or supports_egl;

const OpenGLSurfaceContext = if (supports_wgl) WglAttachContext else if (supports_egl) EglAttachContext else struct {};

test "OpenGL surface renders through the caller's driver" {
    if (!supports_opengl_surface) return error.SkipZigTest;
    var context = try OpenGLSurfaceContext.initWithSize(32, 16);
    defer context.deinit();

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    var session = try support.finishAttachment(try maplibre.openglSurfaceAttach(testing.allocator, support.handle(map), .{
        .extent = .{ .width = 32, .height = 16, .scale_factor = 1.0 },
        .context = context.descriptor(),
        .surface = context.surface(),
    }, .{ .driver = .caller_graphics_thread }), true);
    defer support.closeSession(&session, true) catch @panic("render session close failed");

    const capabilities = try maplibre.renderSessionGetCapabilities(support.handle(session));
    try testing.expectEqual(.caller_graphics_thread, capabilities.driver);
    try testing.expect(capabilities.flags.presentation);

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(session, true);

    var pixels: [32 * 16 * 4]u8 = undefined;
    try context.readSurfaceRGBA8(32, 16, &pixels);
    try testing.expect(hasNonZeroByte(&pixels));
}

test "Vulkan surface attach rejects a descriptor with no surface" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    // The descriptor is validated before any Vulkan call, so placeholder
    // handles reach the rejection rather than the driver.
    try testing.expectError(error.InvalidArgument, maplibre.vulkanSurfaceAttach(testing.allocator, support.handle(map), .{
        .extent = .{ .width = 32, .height = 16, .scale_factor = 1.0 },
        .context = fakeVulkanContext(),
        .surface = 0,
    }, .{ .driver = .core_worker }));
    try testing.expectError(error.InvalidArgument, maplibre.vulkanSurfaceAttach(testing.allocator, support.handle(map), .{
        .extent = .{ .width = 0, .height = 16, .scale_factor = 1.0 },
        .context = fakeVulkanContext(),
        .surface = 1,
    }, .{ .driver = .core_worker }));
}

test "projection captures last rendered update and survives session" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 64, .height = 64, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{
        .width = 64,
        .height = 64,
        .scale_factor = 1.0,
    });
    defer owned.close() catch {};
    const session = &owned.session;
    try testing.expectError(error.InvalidState, maplibre.renderSessionProjectionCreate(support.handle(session)));
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .center = .{ .latitude = 0, .longitude = 0 }, .zoom = 5 } }));
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json));
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
    try testing.expectEqual(.rendered, (try support.renderFrame(session.*, false, true)).disposition);
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, support.handle(map), .{ .camera = .{ .center = .{ .latitude = 10, .longitude = 20 }, .zoom = 3 } }));
    try support.waitForBarrier(&runtime);
    const projection = try maplibre.renderSessionProjectionCreate(support.handle(session));
    defer maplibre.mapProjectionClose(support.handle(projection)) catch @panic("projection close failed");
    try testing.expectApproxEqAbs(@as(f64, 5), (try maplibre.mapProjectionGetCamera(support.handle(projection))).zoom.?, 0.000001);
    try testing.expectEqual(.rendered, (try support.renderFrame(session.*, false, true)).disposition);
    const newer = try maplibre.renderSessionProjectionCreate(support.handle(session));
    defer maplibre.mapProjectionClose(newer) catch @panic("projection close failed");
    try testing.expectApproxEqAbs(@as(f64, 3), (try maplibre.mapProjectionGetCamera(support.handle(newer))).zoom.?, 0.000001);
    try finishOperation(session.*, try maplibre.renderSessionResize(testing.allocator, support.handle(session), .{ .width = 80, .height = 40, .scale_factor = 1.0 }));
    try testing.expectError(error.InvalidState, maplibre.renderSessionProjectionCreate(support.handle(session)));
    try owned.close();
    try support.closeMap(&map);
    try testing.expectApproxEqAbs(@as(f64, 5), (try maplibre.mapProjectionGetCamera(support.handle(projection))).zoom.?, 0.000001);
}

test "scoped GPU reads pin the frame across sibling disposal" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 16, .height = 16 });
    defer support.closeMap(&map) catch @panic("map close failed");
    var context = try TestOwnedTextureContext.init();
    defer context.deinit();
    const session = try attachOwnedTexture(&map, &context, .{ .width = 16, .height = 16, .scale_factor = 1 });
    defer maplibre.renderSessionDestroy(session) catch @panic("session destroy failed");
    try support.expectCommitted(try maplibre.mapSetStyleJson(map, support.style_json));
    _ = try support.expectRenderedFrame(session, true);
    var frame = try maplibre.renderSessionAcquireFrame(session);
    defer frame.deinit();
    _ = try support.expectRenderedFrame(session, true);
    var sibling = try maplibre.renderSessionAcquireFrame(session);
    defer sibling.deinit();
    const Probe = struct {
        frame: maplibre.AcquiredFrame,
        sibling: *maplibre.AcquiredFrame,
        session: maplibre.RenderSession,
        fn inspect(self: @This(), _: maplibre.GpuSync) anyerror!void {
            try testing.expectError(error.Busy, maplibre.acquiredFrameRelease(testing.allocator, self.frame, .{ .kind = .cpu_complete }));
            try testing.expectError(error.Busy, maplibre.renderSessionAbandon(self.session));
            self.sibling.deinit();
            try testing.expectError(error.TargetLost, maplibre.acquiredFrameGetProducerSync(void, self.frame, {}, struct {
                fn use(_: void, _: maplibre.GpuSync) anyerror!void {}
            }.use));
            return error.HostConsumerFailed;
        }
    };
    try testing.expectError(error.HostConsumerFailed, maplibre.acquiredFrameGetProducerSync(void, frame, Probe{ .frame = frame, .sibling = &sibling, .session = session }, Probe.inspect));
    try maplibre.acquiredFrameRelease(testing.allocator, frame, .{ .kind = .cpu_complete });
}
