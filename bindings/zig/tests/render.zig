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
    const support_mask = maplibre.supportedRenderBackends();
    try testing.expect(support_mask.metal or support_mask.opengl or support_mask.vulkan or support_mask.webgpu);
    if (build_options.supports_metal) try testing.expect(support_mask.metal);
    if (build_options.supports_opengl) try testing.expect(support_mask.opengl);
    if (build_options.supports_vulkan) try testing.expect(support_mask.vulkan);
}

test "supported OpenGL context providers are exposed semantically" {
    const providers = maplibre.supportedOpenGLContextProviders();
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

test "queried features compare copied buffers by content" {
    const feature = "{\"type\":\"Feature\"}";
    const state = "{\"hover\":true}";
    var left = maplibre.QueriedFeature{
        .allocator = testing.allocator,
        .feature = try testing.allocator.dupe(u8, feature),
        .source_id = try testing.allocator.dupe(u8, "points"),
        .source_layer_id = try testing.allocator.dupe(u8, "layer"),
        .state = try testing.allocator.dupe(u8, state),
    };
    defer left.deinit();
    var right = maplibre.QueriedFeature{
        .allocator = testing.allocator,
        .feature = try testing.allocator.dupe(u8, feature),
        .source_id = try testing.allocator.dupe(u8, "points"),
        .source_layer_id = try testing.allocator.dupe(u8, "layer"),
        .state = try testing.allocator.dupe(u8, state),
    };
    defer right.deinit();
    try testing.expect(left.eql(right));

    var other_feature = right;
    other_feature.feature = try testing.allocator.dupe(u8, "{\"type\":\"Feature\",\"id\":1}");
    defer testing.allocator.free(other_feature.feature);
    try testing.expect(!left.eql(other_feature));

    var absent_state = right;
    absent_state.state = null;
    var empty_state = right;
    empty_state.state = &.{};
    try testing.expect(!absent_state.eql(empty_state));

    var absent_source = right;
    absent_source.source_id = null;
    var empty_source = right;
    empty_source.source_id = "";
    try testing.expect(!absent_source.eql(empty_source));
}

fn hasNonZeroByte(bytes: []const u8) bool {
    for (bytes) |byte| {
        if (byte != 0) return true;
    }
    return false;
}

const TestOwnedTextureDescriptor = struct {
    extent: maplibre.RenderTargetExtent = .{},
};

const gl_texture_2d = if (supports_wgl) gl.TEXTURE_2D else 0x0DE1;

fn fakeNativePointer() maplibre.NativePointer {
    return maplibre.NativePointer.fromPtr(@ptrFromInt(1));
}

fn fakeOpenGLContext() maplibre.OpenGLContextDescriptor {
    const fake_pointer = fakeNativePointer();
    if (supports_wgl) {
        return .{
            .wgl = .{
                .device_context = fake_pointer,
                .share_context = fake_pointer,
            },
        };
    }
    if (supports_egl) {
        return .{
            .egl = .{
                .display = fake_pointer,
                .config = fake_pointer,
                .share_context = fake_pointer,
            },
        };
    }
    return .{ .wgl = .{ .device_context = fake_pointer, .share_context = fake_pointer } };
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

const supports_test_owned_texture = build_options.supports_metal or build_options.supports_vulkan or build_options.supports_opengl;

const TestOwnedTextureContext = if (build_options.supports_vulkan) VulkanAttachContext else if (supports_wgl) WglAttachContext else if (supports_egl) EglAttachContext else if (build_options.supports_metal) struct {
    device: *anyopaque,

    pub fn init() !@This() {
        return .{ .device = MTLCreateSystemDefaultDevice() orelse return error.MetalDeviceUnavailable };
    }

    pub fn deinit(_: *@This()) void {}

    pub fn descriptor(self: *const @This()) maplibre.MetalContextDescriptor {
        return .{ .device = maplibre.NativePointer.fromPtr(self.device) };
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

    pub fn descriptor(self: *const WglAttachContext) maplibre.OpenGLContextDescriptor {
        return .{ .wgl = .{
            .device_context = maplibre.NativePointer.fromPtr(self.context.deviceContextPointer()),
            .share_context = maplibre.NativePointer.fromPtr(self.context.shareContextPointer()),
            .get_proc_address = maplibre.NativePointer.fromPtr(wgl_test.Context.getProcAddressPointer()),
        } };
    }

    pub fn surface(self: *const WglAttachContext) maplibre.NativePointer {
        return maplibre.NativePointer.fromPtr(self.context.deviceContextPointer());
    }

    pub fn readSurfaceRGBA8(self: *const WglAttachContext, width: u32, height: u32, pixels: []u8) !void {
        try self.context.readSurfaceRgba(width, height, pixels);
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
            self.context.context.destroyTexture(self.texture);
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
        if (self.texture != 0) self.context.context.destroyTexture(self.texture);
        self.texture = texture;
        self.width = width;
        self.height = height;
    }

    pub fn descriptor(self: *const WglBorrowedTexture) maplibre.OpenGLBorrowedTextureDescriptor {
        return self.descriptorFor(self.texture, self.width, self.height);
    }

    pub fn descriptorFor(self: *const WglBorrowedTexture, texture: gl.uint, width: u32, height: u32) maplibre.OpenGLBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = width, .height = height },
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

    pub fn descriptor(self: *const EglAttachContext) maplibre.OpenGLContextDescriptor {
        return .{ .egl = .{
            .display = maplibre.NativePointer.fromPtr(@ptrCast(self.display.?)),
            .config = maplibre.NativePointer.fromPtr(@ptrCast(self.config.?)),
            .share_context = maplibre.NativePointer.fromPtr(@ptrCast(self.share_context.?)),
            .get_proc_address = null,
        } };
    }

    pub fn surface(self: *const EglAttachContext) maplibre.NativePointer {
        return maplibre.NativePointer.fromPtr(@ptrCast(self.egl_surface.?));
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

/// Pbuffer fixture for a dedicated session. It creates no EGL context and makes
/// nothing current, because naming dedicated ownership is what asks the session
/// to create its own context and keep it current.
const DedicatedEglSurface = if (supports_egl) struct {
    display: egl.EGLDisplay,
    config: egl.EGLConfig,
    egl_surface: egl.EGLSurface,

    pub fn initWithSize(width: u32, height: u32) !DedicatedEglSurface {
        const display = try EglAttachContext.initDisplay();
        errdefer _ = egl.eglTerminate(display);

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

        const surface_attributes = [_]egl.EGLint{
            egl.EGL_WIDTH,  @intCast(width),
            egl.EGL_HEIGHT, @intCast(height),
            egl.EGL_NONE,
        };
        const pbuffer = egl.eglCreatePbufferSurface(display, config, &surface_attributes);
        if (pbuffer == egl.EGL_NO_SURFACE) return error.EglUnavailable;
        return .{ .display = display, .config = config, .egl_surface = pbuffer };
    }

    pub fn deinit(self: *DedicatedEglSurface) void {
        _ = egl.eglDestroySurface(self.display, self.egl_surface);
        _ = egl.eglTerminate(self.display);
    }

    pub fn descriptor(self: *const DedicatedEglSurface) maplibre.OpenGLContextDescriptor {
        return .{ .egl = .{
            .display = maplibre.NativePointer.fromPtr(@ptrCast(self.display.?)),
            .config = maplibre.NativePointer.fromPtr(@ptrCast(self.config.?)),
            .share_context = null,
            .client_api = .gles,
            .ownership = .dedicated,
        } };
    }

    pub fn surface(self: *const DedicatedEglSurface) maplibre.NativePointer {
        return maplibre.NativePointer.fromPtr(@ptrCast(self.egl_surface.?));
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

    pub fn descriptor(self: *const @This()) maplibre.OpenGLBorrowedTextureDescriptor {
        return self.descriptorFor(self.texture, self.width, self.height);
    }

    pub fn descriptorFor(self: *const @This(), texture: gl.uint, width: u32, height: u32) maplibre.OpenGLBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = width, .height = height },
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
    session: maplibre.RenderSessionHandle,
    context_active: bool = true,

    pub fn close(self: *@This()) !void {
        if (!self.context_active) return;
        defer {
            self.context.deinit();
            self.context_active = false;
        }
        try finishOperation(self.session, try self.session.detach());
        try self.session.destroy();
    }
};

fn resolveFuture(comptime T: type, session: maplibre.RenderSessionHandle, future_value: maplibre.Future(T)) !T {
    var future = future_value;
    defer future.deinit();
    for (0..wait_turns) |turn| {
        if (try future.poll()) return future.wait(null);
        _ = session.serviceDriverWork(64) catch 0;
        try waitOneTurn(turn);
    }
    return error.OperationTimedOut;
}

fn finishOperation(session: maplibre.RenderSessionHandle, future: maplibre.Future(void)) !void {
    _ = try resolveFuture(void, session, future);
}

fn takeQueryResult(session: maplibre.RenderSessionHandle, future: maplibre.Future(maplibre.OwnedString)) !maplibre.OwnedString {
    return resolveFuture(maplibre.OwnedString, session, future);
}

fn takeQueryFeaturesResult(session: maplibre.RenderSessionHandle, future: maplibre.Future(maplibre.QueriedFeatureList)) !maplibre.QueriedFeatureList {
    return resolveFuture(maplibre.QueriedFeatureList, session, future);
}

fn finishAttachment(attachment: maplibre.RenderSessionAttachment) !maplibre.RenderSessionHandle {
    errdefer {
        var session = attachment.session;
        session.destroy() catch {};
    }
    try finishOperation(attachment.session, attachment.completion);
    return attachment.session;
}

fn attachTestOwnedTexture(map: *maplibre.MapHandle, descriptor: TestOwnedTextureDescriptor) !TestOwnedTextureSession {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var context = try TestOwnedTextureContext.init();
    errdefer context.deinit();

    var session = if (build_options.supports_vulkan)
        try finishAttachment(try maplibre.attachVulkanOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        }, .{ .driver = .core_worker, .requested_texture_ring_depth = 2 }))
    else if (build_options.supports_opengl)
        try finishAttachment(try maplibre.attachOpenGLOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        }, .{ .driver = .caller_graphics_thread, .requested_texture_ring_depth = 2 }))
    else if (build_options.supports_metal)
        try finishAttachment(try maplibre.attachMetalOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        }, .{ .driver = .core_worker, .requested_texture_ring_depth = 2 }))
    else
        unreachable;
    errdefer {
        if (session.detach()) |operation| {
            finishOperation(session, operation) catch {};
        } else |_| {}
        session.destroy() catch {};
    }

    return .{ .context = context, .session = session };
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
            .instance = maplibre.NativePointer.fromPtr(@ptrCast(self.instance.?)),
            .physical_device = maplibre.NativePointer.fromPtr(@ptrCast(self.physical_device.?)),
            .device = maplibre.NativePointer.fromPtr(@ptrCast(self.device.?)),
            .graphics_queue = maplibre.NativePointer.fromPtr(@ptrCast(self.queue.?)),
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

fn nativeFunctionPointer(function: anytype) maplibre.NativePointer {
    return maplibre.NativePointer.fromPtr(@ptrFromInt(@intFromPtr(function.?)));
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
        var image: vk.VkImage = null;
        var memory: vk.VkDeviceMemory = null;
        var image_view: vk.VkImageView = null;
        errdefer {
            if (image_view != null) context.dispatch.destroy_image_view.?(context.device, image_view, null);
            if (image != null) context.dispatch.destroy_image.?(context.device, image, null);
            if (memory != null) context.dispatch.free_memory.?(context.device, memory, null);
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
            .extent = .{ .width = width, .height = height },
            .physical_width = width,
            .physical_height = height,
            .context = self.context.descriptor(),
            .image = maplibre.NativePointer.fromPtr(@ptrCast(allocation.image.?)),
            .image_view = maplibre.NativePointer.fromPtr(@ptrCast(allocation.image_view.?)),
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

var next_frame_token: u64 = 1;

// Turns a driver poll waits before giving up. The first `spin_turns` yield, so
// work that lands immediately costs nothing; the rest sleep a millisecond
// each, which puts a wall-clock bound on the wait. Yields alone cannot: they
// measure scheduler turns, and on an idle host a hundred thousand of them
// elapse in tens of milliseconds -- less than a real frame takes, so the wait
// expired while the frame was still on its way.
const spin_turns = 1_000;
const wait_turns = spin_turns + 30_000;

fn waitOneTurn(turn: usize) !void {
    if (turn < spin_turns) return std.Thread.yield();
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

fn awaitRuntimeBarrier(runtime: *maplibre.RuntimeHandle) !void {
    var future = try runtime.barrier();
    defer future.deinit();
    _ = try future.wait(null);
}

fn renderFrame(session: maplibre.RenderSessionHandle, if_needed: bool) !maplibre.FrameResult {
    const token = next_frame_token;
    next_frame_token += 1;
    const capabilities = try session.capabilities();
    try session.requestFrame(.{
        .if_needed = if_needed,
        .present = capabilities.presentation,
        .token = token,
    });

    for (0..wait_turns) |turn| {
        _ = session.serviceDriverWork(64) catch 0;
        var batch = session.drainFrameResults() catch |err| switch (err) {
            error.NotReady => {
                try waitOneTurn(turn);
                continue;
            },
            else => return err,
        };
        defer batch.release();
        for (0..try batch.count()) |index| {
            const result = try batch.get(index);
            if (result.token == token) return result;
        }
        try waitOneTurn(turn);
    }
    return error.FrameTimedOut;
}

fn releaseFrame(frame: *maplibre.AcquiredFrame) !void {
    try frame.release(.cpu_complete);
}

fn expectOwnedFrameExtent(
    session: maplibre.RenderSessionHandle,
    extent: maplibre.RenderTargetExtent,
) !void {
    var frame = try session.acquireFrame();
    if (build_options.supports_vulkan) {
        const info = try frame.vulkanTexture();
        try testing.expectEqual(extent.width, info.width);
        try testing.expectEqual(extent.height, info.height);
        try testing.expectEqual(extent.scale_factor, info.scale_factor);
        try testing.expect(@intFromEnum(info.image) != 0);
    } else if (build_options.supports_opengl) {
        const info = try frame.openGLTexture();
        try testing.expectEqual(extent.width, info.width);
        try testing.expectEqual(extent.height, info.height);
        try testing.expectEqual(extent.scale_factor, info.scale_factor);
        try testing.expect(info.texture != 0);
    } else if (build_options.supports_metal) {
        const info = try frame.metalTexture();
        try testing.expectEqual(extent.width, info.width);
        try testing.expectEqual(extent.height, info.height);
        try testing.expectEqual(extent.scale_factor, info.scale_factor);
        try testing.expect(@intFromEnum(info.texture) != 0);
    } else {
        unreachable;
    }
    try releaseFrame(&frame);
}

fn closeSession(session: *maplibre.RenderSessionHandle) !void {
    const operation = try session.detach();
    try finishOperation(session.*, operation);
    try session.destroy();
}

test "owned texture session renders acquires resizes and reads back" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 32, .height = 16 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");

    const initial_extent = maplibre.RenderTargetExtent{ .width = 32, .height = 16 };
    var owned = try attachTestOwnedTexture(&map, .{ .extent = initial_extent });
    defer owned.close() catch @panic("render session close failed");
    const capabilities = try owned.session.capabilities();
    try testing.expect(capabilities.frame_acquisition);
    try testing.expectEqual(.attached, std.meta.activeTag((try owned.session.snapshot()).state));

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try awaitRuntimeBarrier(&runtime);
    try testing.expectEqual(.rendered, std.meta.activeTag((try renderFrame(owned.session, false)).disposition));
    try expectOwnedFrameExtent(owned.session, initial_extent);

    if (capabilities.readback) {
        var image = try resolveFuture(maplibre.OwnedReadback, owned.session, try owned.session.readback(testing.allocator));
        defer image.deinit();
        try testing.expectEqual(@as(u32, 32), image.info.width);
        try testing.expectEqual(@as(u32, 16), image.info.height);
        try testing.expectEqual(image.info.byte_length, image.data.len);
        try testing.expect(hasNonZeroByte(image.data));
    }

    const resized_extent = maplibre.RenderTargetExtent{ .width = 48, .height = 24 };
    try finishOperation(owned.session, try owned.session.resize(resized_extent));
    _ = try map.resize(48, 24, 1.0);
    try awaitRuntimeBarrier(&runtime);
    for (0..1000) |_| {
        const result = try renderFrame(owned.session, false);
        if (std.meta.activeTag(result.disposition) == .rendered) break;
        try testing.expectEqual(.size_pending, std.meta.activeTag(result.disposition));
        // The repaint signal is meaningful only on a rendered frame.
        try testing.expect(!result.needs_repaint);
    } else return error.ResizeDidNotConverge;
    try expectOwnedFrameExtent(owned.session, resized_extent);
    const snapshot = try owned.session.snapshot();
    try testing.expectEqual(resized_extent.width, snapshot.extent.width);
    try testing.expectEqual(resized_extent.height, snapshot.extent.height);
}

test "texture readback before a frame completes with invalid state" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 16, .height = 16 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 16, .height = 16 } });
    defer owned.close() catch @panic("render session close failed");

    if ((try owned.session.capabilities()).readback) {
        try testing.expectError(error.InvalidState, resolveFuture(maplibre.OwnedReadback, owned.session, try owned.session.readback(testing.allocator)));
    }
}

test "live render session blocks map close until detached" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, &diagnostics);
    errdefer runtime.close() catch {};
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 32, .height = 32 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    errdefer map.close() catch {};
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 32, .height = 32 } });
    errdefer owned.close() catch {};

    try testing.expectError(error.InvalidState, map.close());
    try testing.expectEqualStrings("map has an attached render session", diagnostics.get().?.message);

    try owned.close();
    try map.close();
    try runtime.close();
}

test "still-image map modes complete owned texture renders" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    inline for (.{ maplibre.MapMode.static, maplibre.MapMode.tile }) |mode| {
        var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
        defer runtime.close() catch @panic("runtime close failed");
        var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 32, .height = 32, .mode = mode });
        defer map_future.deinit();
        var map = try map_future.wait(null);
        defer map.close() catch @panic("map close failed");
        var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 32, .height = 32 } });
        defer owned.close() catch @panic("render session close failed");

        _ = try map.setStyleJson(testing.allocator, support.style_json);
        try awaitRuntimeBarrier(&runtime);
        var future = try map.requestStillImage();
        defer future.deinit();
        for (0..1000) |_| {
            _ = try renderFrame(owned.session, false);
            if (try future.poll()) break;
            try std.Thread.yield();
        } else return error.StillImageDidNotComplete;
        _ = try future.wait(null);
        try expectOwnedFrameExtent(owned.session, .{ .width = 32, .height = 32 });
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
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 64, .height = 64 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 64, .height = 64 } });
    defer owned.close() catch @panic("render session close failed");

    _ = try map.setStyleJson(testing.allocator, feature_state_style_json);
    try awaitRuntimeBarrier(&runtime);
    _ = try renderFrame(owned.session, false);

    const selector = maplibre.FeatureStateSelector{ .source_id = "point", .feature_id = "feature-1" };
    try testing.expect(std.meta.eql(
        try support.waitForCommandDisposition(&runtime, try map.setFeatureState(testing.allocator, selector, "{\"hover\":true,\"count\":3}")),
        maplibre.CommandDisposition.committed,
    ));
    var state_future = try map.getFeatureState(testing.allocator, selector);
    defer state_future.deinit();
    var state = try state_future.wait(null);
    defer state.deinit();
    try testing.expect(std.mem.indexOf(u8, state.value, "\"hover\":true") != null);

    for (0..1000) |_| {
        var result = try takeQueryFeaturesResult(
            owned.session,
            try owned.session.queryRenderedFeatures(
                testing.allocator,
                .{ .box = .{
                    .min = .{ .x = 0, .y = 0 },
                    .max = .{ .x = 64, .y = 64 },
                } },
                null,
            ),
        );
        defer result.deinit();
        if (result.items.len != 0 and result.items[0].state != null) {
            try testing.expectEqualStrings("point", result.items[0].source_id.?);
            try testing.expect(std.mem.indexOf(u8, result.items[0].state.?, "\"hover\":true") != null);
            break;
        }
        _ = try map.requestRepaint();
        try awaitRuntimeBarrier(&runtime);
        _ = try renderFrame(owned.session, false);
    } else return error.RenderedFeatureNotQueryable;

    var source = try takeQueryFeaturesResult(
        owned.session,
        try owned.session.querySourceFeatures(testing.allocator, "point", null),
    );
    defer source.deinit();
    try testing.expectEqualStrings("point", source.items[0].source_id.?);
    try testing.expect(std.mem.indexOf(u8, source.items[0].feature, "\"type\":\"Point\"") != null);

    try testing.expect(std.meta.eql(
        try support.waitForCommandDisposition(&runtime, try map.removeFeatureState(testing.allocator, selector)),
        maplibre.CommandDisposition.committed,
    ));
}

fn featureStateForTesting(map: *maplibre.MapHandle, selector: maplibre.FeatureStateSelector) !maplibre.OwnedString {
    var future = try map.getFeatureState(testing.allocator, selector);
    defer future.deinit();
    return future.wait(null);
}

fn commitMapCommand(runtime: *maplibre.RuntimeHandle, future: maplibre.Future(maplibre.CommandCompletion)) !void {
    try testing.expect(std.meta.eql(
        try support.waitForCommandDisposition(runtime, future),
        maplibre.CommandDisposition.committed,
    ));
}

// Feature state belongs to the map, so it needs no loaded style, ordered reads
// observe every earlier command, and it survives style loads and a renderer
// retirement driven by a scale-factor change.
test "map feature state set get and remove" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 64, .height = 64 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 64, .height = 64 } });
    defer owned.close() catch @panic("render session close failed");

    const selector = maplibre.FeatureStateSelector{ .source_id = "point", .feature_id = "feature-1" };
    const feature_state = "{\"hover\":true,\"radius\":18446744073709551615}";
    try commitMapCommand(&runtime, try map.setFeatureState(testing.allocator, selector, feature_state));
    try commitMapCommand(&runtime, try map.removeFeatureState(testing.allocator, .{ .source_id = "point", .feature_id = "feature-1", .state_key = "hover" }));
    var queued = try featureStateForTesting(&map, selector);
    defer queued.deinit();
    try testing.expect(rawJsonMember(queued.value, "hover") == null);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(queued.value, "radius").?);

    // A style load drops style-owned objects, not map-owned feature state.
    _ = try map.setStyleJson(testing.allocator, feature_state_style_json);
    try awaitRuntimeBarrier(&runtime);
    _ = try renderFrame(owned.session, false);
    var after_style = try featureStateForTesting(&map, selector);
    defer after_style.deinit();
    try testing.expect(rawJsonMember(after_style.value, "hover") == null);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(after_style.value, "radius").?);

    try commitMapCommand(&runtime, try map.setFeatureState(testing.allocator, selector, feature_state));
    var restored = try featureStateForTesting(&map, selector);
    defer restored.deinit();
    try testing.expectEqualStrings("true", rawJsonMember(restored.value, "hover").?);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(restored.value, "radius").?);

    try testing.expectError(error.InvalidArgument, map.removeFeatureState(testing.allocator, .{ .source_id = "point", .state_key = "hover" }));

    // A scale-factor change retires the renderer; map-owned state survives.
    try finishOperation(owned.session, try owned.session.resize(.{ .width = 64, .height = 64, .scale_factor = 2.0 }));
    for (0..100) |_| {
        if (std.meta.activeTag((try renderFrame(owned.session, false)).disposition) == .rendered) break;
    } else return error.RenderDidNotComplete;
    var after_scale = try featureStateForTesting(&map, selector);
    defer after_scale.deinit();
    try testing.expectEqualStrings("true", rawJsonMember(after_scale.value, "hover").?);
    try testing.expectEqualStrings("18446744073709551615", rawJsonMember(after_scale.value, "radius").?);
}

test "cluster feature extensions copy values and feature collections" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 64, .height = 64 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 64, .height = 64 } });
    defer owned.close() catch @panic("render session close failed");

    _ = try map.setStyleJson(testing.allocator, cluster_style_json);
    try awaitRuntimeBarrier(&runtime);
    _ = try renderFrame(owned.session, false);

    var cluster_result: ?maplibre.QueriedFeatureList = null;
    for (0..1000) |_| {
        var result = try takeQueryFeaturesResult(
            owned.session,
            try owned.session.queryRenderedFeatures(
                testing.allocator,
                .{ .box = .{
                    .min = .{ .x = 0, .y = 0 },
                    .max = .{ .x = 64, .y = 64 },
                } },
                null,
            ),
        );
        if (result.items.len != 0) {
            cluster_result = result;
            break;
        }
        result.deinit();
        _ = try map.requestRepaint();
        try awaitRuntimeBarrier(&runtime);
        _ = try renderFrame(owned.session, false);
    }
    var clusters = cluster_result orelse return error.ClusterFeatureNotQueryable;
    defer clusters.deinit();
    const feature = clusters.items[0].feature;
    const properties = rawJsonMember(feature, "properties").?;
    _ = try std.fmt.parseInt(u64, rawJsonMember(properties, "cluster_id").?, 10);
    try testing.expectEqualStrings("3", rawJsonMember(properties, "point_count").?);

    var children = try takeQueryResult(
        owned.session,
        try owned.session.queryFeatureExtension(
            testing.allocator,
            "cluster-source",
            feature,
            "supercluster",
            "children",
            null,
        ),
    );
    defer children.deinit();
    try testing.expect(firstJsonArrayElement(rawJsonMember(children.value, "features").?) != null);

    var expansion_zoom = try takeQueryResult(
        owned.session,
        try owned.session.queryFeatureExtension(
            testing.allocator,
            "cluster-source",
            feature,
            "supercluster",
            "expansion-zoom",
            null,
        ),
    );
    defer expansion_zoom.deinit();
    _ = try std.fmt.parseInt(u64, expansion_zoom.value, 10);

    var first_leaf = try takeQueryResult(
        owned.session,
        try owned.session.queryFeatureExtension(
            testing.allocator,
            "cluster-source",
            feature,
            "supercluster",
            "leaves",
            "{\"limit\":1,\"offset\":0}",
        ),
    );
    defer first_leaf.deinit();
    var second_leaf = try takeQueryResult(
        owned.session,
        try owned.session.queryFeatureExtension(
            testing.allocator,
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
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 32, .height = 32 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 32, .height = 32 } });
    defer owned.close() catch @panic("render session close failed");
    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try awaitRuntimeBarrier(&runtime);

    for (0..64) |_| {
        _ = try map.requestRepaint();
        try awaitRuntimeBarrier(&runtime);
        try testing.expectEqual(.rendered, std.meta.activeTag((try renderFrame(owned.session, false)).disposition));
    }
    try testing.expect((try owned.session.snapshot()).frame_generation >= 64);
}

test "a rendered frame during an ease reports needs repaint" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 32, .height = 16 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 32, .height = 16 } });
    defer owned.close() catch @panic("render session close failed");

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try awaitRuntimeBarrier(&runtime);
    // Settle the style's own frames so the transition drives what follows.
    try testing.expectEqual(.rendered, std.meta.activeTag((try renderFrame(owned.session, false)).disposition));

    _ = try map.updateCamera(.{
        .mode = .ease,
        .camera = .{ .center = .{ .latitude = 37.7749, .longitude = -122.4194 }, .zoom = 4.0 },
        .animation = .{ .duration_ms = 60_000 },
    });
    try awaitRuntimeBarrier(&runtime);

    // Mid-transition the map asks for the next frame with the one it renders,
    // the same signal a map-render-frame-finished event carries.
    var observed_repaint = false;
    for (0..100) |_| {
        const result = try renderFrame(owned.session, false);
        if (std.meta.activeTag(result.disposition) == .rendered and result.needs_repaint) {
            observed_repaint = true;
            break;
        }
    }
    try testing.expect(observed_repaint);
}

const SnapshotThread = struct {
    fn run(session: maplibre.RenderSessionHandle, failure: *?anyerror) void {
        _ = session.snapshot() catch |err| {
            failure.* = err;
            return;
        };
    }
};

test "render session controls are usable from another thread" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 16, .height = 16 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 16, .height = 16 } });
    defer owned.close() catch @panic("render session close failed");

    var failure: ?anyerror = null;
    const thread = try std.Thread.spawn(.{}, SnapshotThread.run, .{ owned.session, &failure });
    thread.join();
    try testing.expectEqual(@as(?anyerror, null), failure);
}

test "Vulkan borrowed texture replaces its target" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 32, .height = 16 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var borrowed = try VulkanBorrowedImage.create(32, 16);
    defer borrowed.deinit();
    var session = try finishAttachment(try maplibre.attachVulkanBorrowedTexture(
        &map,
        borrowed.descriptor(),
        .{ .driver = .core_worker, .requested_texture_ring_depth = 1 },
    ));
    defer closeSession(&session) catch @panic("render session close failed");

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try awaitRuntimeBarrier(&runtime);
    for (0..1000) |_| {
        if (std.meta.activeTag((try renderFrame(session, false)).disposition) == .rendered) break;
        _ = try map.requestRepaint();
        try awaitRuntimeBarrier(&runtime);
    } else return error.FrameDidNotRender;

    const replacement = try borrowed.allocateReplacement(48, 24);
    errdefer borrowed.release(replacement);
    try finishOperation(
        session,
        try session.setVulkanBorrowedTextureTarget(borrowed.descriptorFor(replacement, 48, 24)),
    );
    borrowed.adopt(replacement, 48, 24);
    _ = try map.resize(48, 24, 1.0);
    try awaitRuntimeBarrier(&runtime);
    for (0..1000) |_| {
        if (std.meta.activeTag((try renderFrame(session, false)).disposition) == .rendered) break;
        _ = try map.requestRepaint();
        try awaitRuntimeBarrier(&runtime);
    } else return error.FrameDidNotRender;
    const snapshot = try session.snapshot();
    try testing.expectEqual(@as(u32, 48), snapshot.extent.width);
    try testing.expectEqual(@as(u32, 24), snapshot.extent.height);
}

test "OpenGL borrowed texture replaces its target" {
    if (!build_options.supports_opengl) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map_future = try maplibre.MapHandle.create(&runtime, .{ .width = 32, .height = 16 });
    defer map_future.deinit();
    var map = try map_future.wait(null);
    defer map.close() catch @panic("map close failed");
    var borrowed = try OpenGLBorrowedTexture.create(32, 16);
    defer borrowed.deinit();
    var session = try finishAttachment(try maplibre.attachOpenGLBorrowedTexture(
        &map,
        borrowed.descriptor(),
        .{ .driver = .caller_graphics_thread, .requested_texture_ring_depth = 1 },
    ));
    defer closeSession(&session) catch @panic("render session close failed");

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try awaitRuntimeBarrier(&runtime);
    try testing.expectEqual(.rendered, std.meta.activeTag((try renderFrame(session, false)).disposition));
    var initial_pixels: [32 * 16 * 4]u8 = undefined;
    try borrowed.readRGBA8(&initial_pixels);
    try testing.expect(hasNonZeroByte(&initial_pixels));

    const replacement = try borrowed.allocateReplacement(48, 24);
    errdefer borrowed.context.destroyTexture(replacement);
    try finishOperation(
        session,
        try session.setOpenGLBorrowedTextureTarget(borrowed.descriptorFor(replacement, 48, 24)),
    );
    borrowed.adopt(replacement, 48, 24);
    _ = try map.resize(48, 24, 1.0);
    try awaitRuntimeBarrier(&runtime);
    for (0..1000) |_| {
        if (std.meta.activeTag((try renderFrame(session, false)).disposition) == .rendered) break;
        _ = try map.requestRepaint();
        try awaitRuntimeBarrier(&runtime);
    } else return error.FrameDidNotRender;
}
