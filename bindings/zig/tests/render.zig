const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const metal_support = @import("metal_support.zig");
const support = @import("support.zig");

extern "c" fn MTLCreateSystemDefaultDevice() ?*anyopaque;

const vk = if (build_options.supports_vulkan) @cImport({
    @cInclude("vulkan/vulkan.h");
}) else struct {};

const supports_wgl = build_options.supports_opengl and builtin.os.tag == .windows;
const supports_egl = build_options.supports_opengl and (builtin.os.tag == .linux or builtin.os.tag == .macos);

const egl = if (supports_egl) @cImport({
    @cDefine("EGL_EGLEXT_PROTOTYPES", "1");
    @cInclude("EGL/egl.h");
    @cInclude("EGL/eglext.h");
}) else struct {};

const gl = if (supports_wgl or supports_egl) @import("gl") else struct {};
const wgl_test = if (supports_wgl) @import("wgl_test_context") else struct {};

const cluster_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-cluster-query-test",
    \\  "sources": {
    \\    "cluster-source": {
    \\      "type": "geojson",
    \\      "cluster": true,
    \\      "data": {
    \\        "type": "FeatureCollection",
    \\        "features": [
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one"}},
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two"}},
    \\          {"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three"}}
    \\        ]
    \\      }
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#ffffff"}},
    \\    {"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-color":"#2563eb","circle-radius":20}}
    \\  ]
    \\}
;

test "supported render backend is exposed semantically" {
    const support_mask = maplibre.supportedRenderBackends();
    try testing.expect(support_mask.metal or support_mask.opengl or support_mask.vulkan);
    if (build_options.supports_metal) try testing.expect(support_mask.metal);
    if (build_options.supports_opengl) try testing.expect(support_mask.opengl);
    if (build_options.supports_vulkan) try testing.expect(support_mask.vulkan);
}

test "supported OpenGL context providers are exposed semantically" {
    const providers = maplibre.supportedOpenGLContextProviders();
    if (!build_options.supports_opengl) {
        try testing.expect(!providers.wgl);
        try testing.expect(!providers.egl);
    } else {
        try testing.expectEqual(supports_wgl, providers.wgl);
        try testing.expectEqual(supports_egl, providers.egl);
    }
}

fn expectFeaturePropertyString(feature: *const maplibre.QueriedFeature, key: []const u8, expected: []const u8) !void {
    for (feature.feature.properties) |property| {
        if (std.mem.eql(u8, property.key, key)) {
            const actual = switch (property.value) {
                .string => |value| value,
                else => return error.ExpectedString,
            };
            try testing.expectEqualStrings(expected, actual);
            return;
        }
    }
    return error.MissingFeatureProperty;
}

fn waitForRenderedFeatureQuery(
    runtime: *maplibre.RuntimeHandle,
    session: *maplibre.RenderSessionHandle,
    geometry: maplibre.RenderedQueryGeometry,
    options: maplibre.RenderedFeatureQueryOptions,
) !maplibre.FeatureQueryResult {
    for (0..1000) |_| {
        var result = try session.queryRenderedFeatures(testing.allocator, geometry, options);
        if (result.features.len > 0) return result;
        result.deinit();
        try runtime.runOnce();
        try session.renderUpdate();
        try std.Thread.yield();
    }
    return error.RenderedFeatureNotQueryable;
}

fn waitForSourceFeatureQuery(
    runtime: *maplibre.RuntimeHandle,
    session: *maplibre.RenderSessionHandle,
) !maplibre.FeatureQueryResult {
    for (0..1000) |_| {
        var result = try session.querySourceFeatures(testing.allocator, "point", .{
            .filter = .{ .array = &.{
                .{ .string = "==" },
                .{ .array = &.{ .{ .string = "get" }, .{ .string = "kind" } } },
                .{ .string = "capital" },
            } },
        });
        if (result.features.len > 0) return result;
        result.deinit();
        try runtime.runOnce();
        try session.renderUpdate();
        try std.Thread.yield();
    }
    return error.SourceFeatureNotQueryable;
}

fn ownedGeometryAsBorrowed(geometry: maplibre.OwnedGeometry) maplibre.Geometry {
    return switch (geometry) {
        .empty => .empty,
        .point => |point| .{ .point = point },
        .line_string => |coordinates| .{ .line_string = coordinates },
        .polygon => |rings| .{ .polygon = rings },
        .multi_point => |coordinates| .{ .multi_point = coordinates },
        .multi_line_string => |lines| .{ .multi_line_string = lines },
        .multi_polygon => |polygons| .{ .multi_polygon = polygons },
        .collection => .empty,
    };
}

fn ownedJsonAsBorrowed(value: maplibre.OwnedJsonValue) maplibre.JsonValue {
    return switch (value) {
        .null => .null,
        .bool => |item| .{ .bool = item },
        .uint => |item| .{ .uint = item },
        .int => |item| .{ .int = item },
        .double => |item| .{ .double = item },
        .string => |item| .{ .string = item },
        .array, .object => .null,
    };
}

fn queriedFeatureAsBorrowed(allocator: std.mem.Allocator, queried: *const maplibre.QueriedFeature) !struct { feature: maplibre.Feature, properties: []maplibre.JsonMember } {
    const properties = try allocator.alloc(maplibre.JsonMember, queried.feature.properties.len);
    for (queried.feature.properties, properties) |property, *out| {
        out.* = .{ .key = property.key, .value = ownedJsonAsBorrowed(property.value) };
    }
    return .{
        .feature = .{
            .geometry = ownedGeometryAsBorrowed(queried.feature.geometry),
            .properties = properties,
            .identifier = queried.feature.identifier,
        },
        .properties = properties,
    };
}

fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent()) |event| {
            if (std.meta.eql(event.event_type, event_type)) return true;
        }
        try std.Thread.yield();
    }
    return false;
}

fn expectPixelApprox(actual: [4]u8, expected: [4]u8, tolerance: u8) !void {
    for (actual, expected) |actual_channel, expected_channel| {
        const delta = if (actual_channel > expected_channel)
            actual_channel - expected_channel
        else
            expected_channel - actual_channel;
        try testing.expect(delta <= tolerance);
    }
}

fn hasNonZeroByte(bytes: []const u8) bool {
    for (bytes) |byte| {
        if (byte != 0) return true;
    }
    return false;
}

const RenderSessionThreadCall = enum {
    render_update,
    resize,
    detach,
    reduce_memory_use,
    clear_data,
    dump_debug_logs,
    close,
    acquire_metal_frame,
    acquire_opengl_frame,
    acquire_vulkan_frame,
};

fn callRenderSessionOnThread(session: *maplibre.RenderSessionHandle, call: RenderSessionThreadCall, out_error: *?anyerror) void {
    const result = switch (call) {
        .render_update => session.renderUpdate(),
        .resize => session.resize(.{ .width = 16, .height = 16, .scale_factor = 1.0 }),
        .detach => session.detach(),
        .reduce_memory_use => session.reduceMemoryUse(),
        .clear_data => session.clearData(),
        .dump_debug_logs => session.dumpDebugLogs(),
        .close => session.close(),
        .acquire_metal_frame => blk: {
            var frame = session.acquireMetalOwnedTextureFrame() catch |err| break :blk err;
            frame.release() catch {};
            break :blk {};
        },
        .acquire_opengl_frame => blk: {
            var frame = session.acquireOpenGLOwnedTextureFrame() catch |err| break :blk err;
            frame.release() catch {};
            break :blk {};
        },
        .acquire_vulkan_frame => blk: {
            var frame = session.acquireVulkanOwnedTextureFrame() catch |err| break :blk err;
            frame.release() catch {};
            break :blk {};
        },
    };
    if (result) |_| {
        out_error.* = null;
    } else |err| {
        out_error.* = err;
    }
}

fn expectRenderSessionCallWrongThread(session: *maplibre.RenderSessionHandle, call: RenderSessionThreadCall) !void {
    var observed: ?anyerror = null;
    const thread = try std.Thread.spawn(.{}, callRenderSessionOnThread, .{ session, call, &observed });
    thread.join();
    try testing.expect(observed != null);
    try testing.expect(observed.? == error.WrongThread);
}

const TestOwnedTextureDescriptor = struct {
    extent: maplibre.RenderTargetExtent = .{},
};

const gl_texture_2d = if (supports_wgl) gl.TEXTURE_2D else 0x0DE1;

fn fakeNativePointer() maplibre.NativePointer {
    return .{ .ptr = @ptrFromInt(1) };
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
        return .{ .device = .{ .ptr = self.device } };
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
            .device_context = .{ .ptr = self.context.deviceContextPointer() },
            .share_context = .{ .ptr = self.context.shareContextPointer() },
            .get_proc_address = .{ .ptr = wgl_test.Context.getProcAddressPointer() },
        } };
    }

    pub fn surface(self: *const WglAttachContext) maplibre.NativePointer {
        return .{ .ptr = self.context.deviceContextPointer() };
    }

    pub fn readSurfaceRGBA8(self: *const WglAttachContext, width: u32, height: u32, pixels: []u8) !void {
        try self.context.readSurfaceRgba(width, height, pixels);
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

    pub fn descriptor(self: *const WglBorrowedTexture) maplibre.OpenGLBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = self.width, .height = self.height },
            .context = self.context.descriptor(),
            .texture = self.texture,
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

    fn initDisplay() !egl.EGLDisplay {
        if (builtin.os.tag == .macos) {
            const display_attributes = [_]egl.EGLint{
                egl.EGL_PLATFORM_ANGLE_TYPE_ANGLE,        egl.EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
                egl.EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, egl.EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
                egl.EGL_NONE,
            };
            return initializeDisplay(egl.eglGetPlatformDisplayEXT(egl.EGL_PLATFORM_ANGLE_ANGLE, null, &display_attributes));
        }
        return initializeDisplay(egl.eglGetDisplay(egl.EGL_DEFAULT_DISPLAY));
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
            .display = .{ .ptr = @ptrCast(self.display.?) },
            .config = .{ .ptr = @ptrCast(self.config.?) },
            .share_context = .{ .ptr = @ptrCast(self.share_context.?) },
            .get_proc_address = null,
        } };
    }

    pub fn surface(self: *const EglAttachContext) maplibre.NativePointer {
        return .{ .ptr = @ptrCast(self.egl_surface.?) };
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
        self.procs.TexImage2D(
            gl.TEXTURE_2D,
            0,
            gl.RGBA8,
            @intCast(width),
            @intCast(height),
            0,
            gl.RGBA,
            gl.UNSIGNED_BYTE,
            null,
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

    pub fn descriptor(self: *const @This()) maplibre.OpenGLBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = self.width, .height = self.height },
            .context = self.context.descriptor(),
            .texture = self.texture,
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
        try self.session.close();
    }
};

fn attachTestOwnedTexture(map: *maplibre.MapHandle, descriptor: TestOwnedTextureDescriptor) !TestOwnedTextureSession {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var context = try TestOwnedTextureContext.init();
    errdefer context.deinit();

    var session = if (build_options.supports_vulkan)
        try maplibre.attachVulkanOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        })
    else if (build_options.supports_opengl)
        try maplibre.attachOpenGLOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        })
    else if (build_options.supports_metal)
        try maplibre.attachMetalOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        })
    else
        unreachable;
    errdefer session.close() catch {};

    return .{ .context = context, .session = session };
}

fn expectInvalidOwnedTextureExtent(map: *maplibre.MapHandle, extent: maplibre.RenderTargetExtent) !void {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    if (build_options.supports_vulkan) {
        try testing.expectError(error.InvalidArgument, maplibre.attachVulkanOwnedTexture(map, .{
            .extent = extent,
            .context = fakeVulkanContext(),
        }));
    } else if (build_options.supports_opengl) {
        try testing.expectError(error.InvalidArgument, maplibre.attachOpenGLOwnedTexture(map, .{
            .extent = extent,
            .context = fakeOpenGLContext(),
        }));
    } else if (build_options.supports_metal) {
        try testing.expectError(error.InvalidArgument, maplibre.attachMetalOwnedTexture(map, .{
            .extent = extent,
            .context = .{ .device = fakeNativePointer() },
        }));
    } else {
        unreachable;
    }
}

fn createMovedMetalSessionWithFrame(device: *anyopaque) !struct {
    runtime: maplibre.RuntimeHandle,
    map: maplibre.MapHandle,
    session: maplibre.RenderSessionHandle,
    frame: maplibre.MetalOwnedTextureFrameHandle,
} {
    var runtime = try maplibre.RuntimeHandle.init(null);
    errdefer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    errdefer map.close() catch @panic("map close failed");

    var session = try maplibre.attachMetalOwnedTexture(&map, .{
        .extent = .{ .width = 32, .height = 32, .scale_factor = 1.0 },
        .context = .{ .device = .{ .ptr = device } },
    });
    errdefer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    const frame = try session.acquireMetalOwnedTextureFrame();
    return .{ .runtime = runtime, .map = map, .session = session, .frame = frame };
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
            .instance = .{ .ptr = @ptrCast(self.instance.?) },
            .physical_device = .{ .ptr = @ptrCast(self.physical_device.?) },
            .device = .{ .ptr = @ptrCast(self.device.?) },
            .graphics_queue = .{ .ptr = @ptrCast(self.queue.?) },
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
    return .{ .ptr = @ptrFromInt(@intFromPtr(function.?)) };
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

    pub fn create(width: u32, height: u32) !VulkanBorrowedImage {
        var context = try VulkanAttachContext.init();
        errdefer context.deinit();

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

        return .{ .context = context, .image = image, .image_view = image_view, .memory = memory, .width = width, .height = height };
    }

    pub fn deinit(self: *VulkanBorrowedImage) void {
        _ = self.context.dispatch.device_wait_idle.?(self.context.device);
        self.context.dispatch.destroy_image_view.?(self.context.device, self.image_view, null);
        self.context.dispatch.destroy_image.?(self.context.device, self.image, null);
        self.context.dispatch.free_memory.?(self.context.device, self.memory, null);
        self.context.deinit();
    }

    pub fn descriptor(self: *const VulkanBorrowedImage) maplibre.VulkanBorrowedTextureDescriptor {
        return .{
            .extent = .{ .width = self.width, .height = self.height },
            .context = self.context.descriptor(),
            .image = .{ .ptr = @ptrCast(self.image.?) },
            .image_view = .{ .ptr = @ptrCast(self.image_view.?) },
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

test "owned texture render session lifecycle and readback" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var owned = try attachTestOwnedTexture(&map, .{
        .extent = .{ .width = 32, .height = 16, .scale_factor = 1.0 },
    });
    defer owned.close() catch {};
    const session = &owned.session;

    try testing.expectError(error.InvalidState, session.readPremultipliedRgba8(testing.allocator));

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();
    try session.reduceMemoryUse();
    try session.dumpDebugLogs();
    try session.clearData();

    var small: [4]u8 = .{ 0, 0, 0, 0 };
    try testing.expectError(error.InvalidArgument, session.readPremultipliedRgba8Into(small[0..]));

    const probed_info = try session.textureImageInfo();
    try testing.expectEqual(@as(u32, 32), probed_info.width);
    try testing.expectEqual(@as(u32, 16), probed_info.height);
    try testing.expectEqual(@as(usize, 32 * 16 * 4), probed_info.byte_length);

    var image = try session.readPremultipliedRgba8(testing.allocator);
    defer image.deinit();
    try testing.expectEqual(@as(u32, 32), image.info.width);
    try testing.expectEqual(@as(u32, 16), image.info.height);
    try testing.expectEqual(@as(u32, 32 * 4), image.info.stride);
    try testing.expectEqual(@as(usize, 32 * 16 * 4), image.info.byte_length);
    try testing.expectEqual(image.info.byte_length, image.data.len);

    try session.resize(.{ .width = 64, .height = 64, .scale_factor = 1.0 });
    try session.detach();
    try testing.expectError(error.InvalidState, session.renderUpdate());
    try owned.close();
}

test "still-image map modes drive owned texture rendering" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    inline for (.{ maplibre.MapMode.static, maplibre.MapMode.tile }) |mode| {
        var runtime = try maplibre.RuntimeHandle.init(null);
        defer runtime.close() catch @panic("runtime close failed");

        var map = try maplibre.MapHandle.create(&runtime, .{ .mode = mode });
        defer map.close() catch @panic("map close failed");

        var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 32, .height = 32 } });
        defer owned.close() catch {};

        try map.setStyleJson(testing.allocator, support.style_json);
        try map.requestStillImage();
        try testing.expectError(error.InvalidState, map.requestStillImage());
    }
}

test "owned texture attachment validates public descriptors" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try expectInvalidOwnedTextureExtent(&map, .{ .width = 0 });
    try expectInvalidOwnedTextureExtent(&map, .{ .height = 0 });
    try expectInvalidOwnedTextureExtent(&map, .{ .scale_factor = 0 });
}

test "owned texture attachment rejects another active session" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var first = try attachTestOwnedTexture(&map, .{});
    defer first.close() catch {};
    try testing.expectError(error.InvalidState, attachTestOwnedTexture(&map, .{}));
}

test "render session feature state set get and remove" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var owned = try attachTestOwnedTexture(&map, .{ .extent = .{ .width = 64, .height = 64 } });
    defer owned.close() catch {};
    const session = &owned.session;

    const selector = maplibre.FeatureStateSelector{ .source_id = "point", .feature_id = "feature-1" };
    const state_members = [_]maplibre.JsonMember{
        .{ .key = "hover", .value = .{ .bool = true } },
        .{ .key = "radius", .value = .{ .uint = 20 } },
    };
    try testing.expectError(error.InvalidState, session.setFeatureState(testing.allocator, selector, .{ .object = state_members[0..] }));

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    try session.setFeatureState(testing.allocator, selector, .{ .object = state_members[0..] });
    var snapshot = try session.getFeatureState(testing.allocator, selector);
    defer snapshot.deinit(testing.allocator);
    const members = switch (snapshot) {
        .object => |items| items,
        else => return error.ExpectedObject,
    };
    try testing.expectEqual(@as(usize, 2), members.len);
    var saw_hover = false;
    var saw_radius = false;
    for (members) |member| {
        if (std.mem.eql(u8, member.key, "hover")) {
            try testing.expectEqual(true, member.value.bool);
            saw_hover = true;
        } else if (std.mem.eql(u8, member.key, "radius")) {
            try testing.expectEqual(@as(u64, 20), member.value.uint);
            saw_radius = true;
        }
    }
    try testing.expect(saw_hover);
    try testing.expect(saw_radius);

    try session.removeFeatureState(testing.allocator, .{ .source_id = "point", .feature_id = "feature-1", .state_key = "hover" });
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    var after_remove = try session.getFeatureState(testing.allocator, selector);
    defer after_remove.deinit(testing.allocator);
    const after_members = switch (after_remove) {
        .object => |items| items,
        else => return error.ExpectedObject,
    };
    try testing.expectEqual(@as(usize, 1), after_members.len);
    try testing.expectEqualStrings("radius", after_members[0].key);
    try testing.expectEqual(@as(u64, 20), after_members[0].value.uint);

    try testing.expectError(error.InvalidArgument, session.removeFeatureState(testing.allocator, .{ .source_id = "point", .state_key = "hover" }));
}

test "render session queries rendered and source features" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var owned = try attachTestOwnedTexture(&map, .{});
    defer owned.close() catch {};
    const session = &owned.session;

    try testing.expectError(error.InvalidState, session.queryRenderedFeatures(testing.allocator, .{ .point = .{ .x = 256, .y = 256 } }, null));

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    const query_point = try map.pixelForLatLng(.{ .latitude = 37.7749, .longitude = -122.4194 });
    var rendered = try waitForRenderedFeatureQuery(&runtime, session, .{ .box = .{
        .min = .{ .x = query_point.x - 20, .y = query_point.y - 20 },
        .max = .{ .x = query_point.x + 20, .y = query_point.y + 20 },
    } }, .{
        .layer_ids = &.{"point-circle"},
        .filter = .{ .array = &.{
            .{ .string = "==" },
            .{ .array = &.{ .{ .string = "get" }, .{ .string = "kind" } } },
            .{ .string = "capital" },
        } },
    });
    defer rendered.deinit();
    try testing.expect(rendered.features[0].source_id != null);
    try testing.expectEqualStrings("point", rendered.features[0].source_id.?);
    try expectFeaturePropertyString(&rendered.features[0], "kind", "capital");

    var source = try waitForSourceFeatureQuery(&runtime, session);
    defer source.deinit();
    try testing.expect(source.features[0].source_id != null);
    try testing.expectEqualStrings("point", source.features[0].source_id.?);
    try expectFeaturePropertyString(&source.features[0], "kind", "capital");

    try testing.expectError(error.InvalidArgument, session.queryRenderedFeatures(testing.allocator, .{ .point = .{ .x = std.math.inf(f64), .y = 0.0 } }, null));
    try testing.expectError(error.InvalidArgument, session.querySourceFeatures(testing.allocator, "", null));
}

test "render session queries cluster feature extensions" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var owned = try attachTestOwnedTexture(&map, .{});
    defer owned.close() catch {};
    const session = &owned.session;

    try map.jumpTo(.{ .center = .{ .latitude = 0, .longitude = 0 }, .zoom = 0 });
    try map.setStyleJson(testing.allocator, cluster_style_json);
    for (0..5) |_| {
        if (!try waitForEvent(&runtime, .map_render_update_available)) break;
        try session.renderUpdate();
    }

    const query_point = try map.pixelForLatLng(.{ .latitude = 0, .longitude = 0 });
    var clusters = try waitForRenderedFeatureQuery(&runtime, session, .{ .box = .{
        .min = .{ .x = query_point.x - 30, .y = query_point.y - 30 },
        .max = .{ .x = query_point.x + 30, .y = query_point.y + 30 },
    } }, .{ .layer_ids = &.{"cluster-circle"} });
    defer clusters.deinit();

    const borrowed = try queriedFeatureAsBorrowed(testing.allocator, &clusters.features[0]);
    defer testing.allocator.free(borrowed.properties);

    var children = try session.queryFeatureExtension(testing.allocator, "cluster-source", borrowed.feature, "supercluster", "children", null);
    defer children.deinit(testing.allocator);
    const child_collection = switch (children) {
        .feature_collection => |collection| collection,
        else => return error.ExpectedFeatureCollection,
    };
    try testing.expect(child_collection.features.len > 0);

    var expansion_zoom = try session.queryFeatureExtension(testing.allocator, "cluster-source", borrowed.feature, "supercluster", "expansion-zoom", null);
    defer expansion_zoom.deinit(testing.allocator);
    const zoom_value = switch (expansion_zoom) {
        .value => |value| value,
        else => return error.ExpectedValue,
    };
    try testing.expect(zoom_value == .uint);

    const args_members = [_]maplibre.JsonMember{
        .{ .key = "limit", .value = .{ .uint = 1 } },
        .{ .key = "offset", .value = .{ .uint = 0 } },
    };
    var leaves = try session.queryFeatureExtension(testing.allocator, "cluster-source", borrowed.feature, "supercluster", "leaves", .{ .object = args_members[0..] });
    defer leaves.deinit(testing.allocator);
    const leaf_collection = switch (leaves) {
        .feature_collection => |collection| collection,
        else => return error.ExpectedFeatureCollection,
    };
    try testing.expectEqual(@as(usize, 1), leaf_collection.features.len);
}

test "unsupported backend owned texture attachment reports unsupported" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const fake_pointer = fakeNativePointer();
    if (build_options.supports_metal) {
        try testing.expectError(error.Unsupported, maplibre.attachVulkanOwnedTexture(&map, .{
            .context = .{
                .instance = fake_pointer,
                .physical_device = fake_pointer,
                .device = fake_pointer,
                .graphics_queue = fake_pointer,
                .graphics_queue_family_index = 0,
                .get_instance_proc_addr = null,
                .get_device_proc_addr = null,
            },
        }));
    }
    if (build_options.supports_vulkan) {
        try testing.expectError(error.Unsupported, maplibre.attachMetalOwnedTexture(&map, .{
            .context = .{ .device = fake_pointer },
        }));
    }
    if (!build_options.supports_opengl) {
        const context = fakeOpenGLContext();
        try testing.expectError(error.Unsupported, maplibre.attachOpenGLOwnedTexture(&map, .{
            .context = context,
        }));
        try testing.expectError(error.Unsupported, maplibre.attachOpenGLBorrowedTexture(&map, .{
            .context = context,
            .texture = 1,
            .target = gl_texture_2d,
        }));
        try testing.expectError(error.Unsupported, maplibre.attachOpenGLSurface(&map, .{
            .context = context,
            .surface = fake_pointer,
        }));
    }
}

test "OpenGL texture and surface descriptors validate through public bindings" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    const context = fakeOpenGLContext();
    try testing.expectError(error.InvalidArgument, maplibre.attachOpenGLOwnedTexture(&map, .{
        .extent = .{ .width = 0 },
        .context = context,
    }));
    try testing.expectError(error.InvalidArgument, maplibre.attachOpenGLBorrowedTexture(&map, .{
        .context = context,
        .texture = 0,
        .target = gl_texture_2d,
    }));
    try testing.expectError(error.InvalidArgument, maplibre.attachOpenGLBorrowedTexture(&map, .{
        .context = context,
        .texture = 1,
        .target = 0,
    }));
    try testing.expectError(error.InvalidArgument, maplibre.attachOpenGLSurface(&map, .{
        .extent = .{ .width = 0 },
        .context = context,
        .surface = fakeNativePointer(),
    }));
}

test "OpenGL owned texture frame scopes public binding access" {
    if (!build_options.supports_opengl) return error.SkipZigTest;

    var context = try TestOwnedTextureContext.init();
    defer context.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOpenGLOwnedTexture(&map, .{
        .extent = .{ .width = 32, .height = 32, .scale_factor = 1.0 },
        .context = context.descriptor(),
    });
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    var image = try session.readPremultipliedRgba8(testing.allocator);
    defer image.deinit();
    try testing.expectEqual(@as(u32, 32), image.info.width);
    try testing.expectEqual(@as(u32, 32), image.info.height);
    try testing.expect(hasNonZeroByte(image.data));

    var frame = try session.acquireOpenGLOwnedTextureFrame();
    const info = try frame.info();
    try testing.expectEqual(@as(u32, 32), info.width);
    try testing.expectEqual(@as(u32, 32), info.height);
    try testing.expectEqual(@as(u32, gl.TEXTURE_2D), info.target);
    try testing.expectEqual(@as(u32, gl.RGBA8), info.internal_format);
    try testing.expectEqual(@as(u32, gl.RGBA), info.format);
    try testing.expectEqual(@as(u32, gl.UNSIGNED_BYTE), info.type);
    try testing.expect(info.texture != 0);

    try testing.expectError(error.ActiveBorrow, session.renderUpdate());
    try testing.expectError(error.ActiveBorrow, session.detach());
    try testing.expectError(error.ActiveBorrow, session.acquireOpenGLOwnedTextureFrame());
    try testing.expectError(error.ActiveBorrow, session.close());

    try frame.release();
    try frame.release();

    try expectRenderSessionCallWrongThread(&session, .acquire_opengl_frame);
    try session.close();
}

test "OpenGL borrowed texture renders through public bindings" {
    if (!build_options.supports_opengl) return error.SkipZigTest;

    var borrowed = try OpenGLBorrowedTexture.create(128, 128);
    defer borrowed.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOpenGLBorrowedTexture(&map, borrowed.descriptor());
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    const pixels = try testing.allocator.alloc(u8, 128 * 128 * 4);
    defer testing.allocator.free(pixels);
    @memset(pixels, 0);
    try borrowed.readRGBA8(pixels);
    try testing.expect(hasNonZeroByte(pixels));

    try testing.expectError(error.Unsupported, session.acquireOpenGLOwnedTextureFrame());
    try testing.expectError(error.Unsupported, session.readPremultipliedRgba8(testing.allocator));
}

test "OpenGL surface renders through public bindings" {
    if (!build_options.supports_opengl) return error.SkipZigTest;

    var context = try TestOwnedTextureContext.initWithSize(128, 128);
    defer context.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachOpenGLSurface(&map, .{
        .extent = .{ .width = 128, .height = 128, .scale_factor = 1.0 },
        .context = context.descriptor(),
        .surface = context.surface(),
    });
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    const pixels = try testing.allocator.alloc(u8, 128 * 128 * 4);
    defer testing.allocator.free(pixels);
    @memset(pixels, 0);
    try context.readSurfaceRGBA8(128, 128, pixels);
    try testing.expect(hasNonZeroByte(pixels));

    try testing.expectError(error.Unsupported, session.acquireOpenGLOwnedTextureFrame());
    try testing.expectError(error.Unsupported, session.readPremultipliedRgba8(testing.allocator));
}

test "Metal owned texture frame handle scopes native pointers" {
    if (!build_options.supports_metal) return error.SkipZigTest;
    const device = MTLCreateSystemDefaultDevice() orelse return error.MetalDeviceUnavailable;

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachMetalOwnedTexture(&map, .{
        .extent = .{ .width = 32, .height = 32, .scale_factor = 1.0 },
        .context = .{ .device = .{ .ptr = device } },
    });
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    var image = try session.readPremultipliedRgba8(testing.allocator);
    defer image.deinit();
    try testing.expectEqual(@as(u32, 32), image.info.width);
    try testing.expectEqual(@as(u32, 32), image.info.height);
    try testing.expectEqual(@as(usize, 32 * 32 * 4), image.info.byte_length);

    var frame = try session.acquireMetalOwnedTextureFrame();
    const info = try frame.info();
    try testing.expectEqual(@as(u32, 32), info.width);
    try testing.expectEqual(@as(u32, 32), info.height);
    try testing.expectEqual(@as(u64, 1), info.generation);
    try testing.expect(info.texture.ptr != info.device.ptr);

    try testing.expectError(error.ActiveBorrow, session.resize(.{ .width = 16, .height = 16, .scale_factor = 1.0 }));
    try testing.expectError(error.ActiveBorrow, session.renderUpdate());
    try testing.expectError(error.ActiveBorrow, session.detach());
    try testing.expectError(error.ActiveBorrow, session.acquireMetalOwnedTextureFrame());
    try testing.expectError(error.ActiveBorrow, session.close());

    try frame.release();
    try frame.release();

    try session.resize(.{ .width = 16, .height = 8, .scale_factor = 2.0 });
    try session.renderUpdate();
    var resized_frame = try session.acquireMetalOwnedTextureFrame();
    const resized_info = try resized_frame.info();
    try testing.expectEqual(@as(u32, 32), resized_info.width);
    try testing.expectEqual(@as(u32, 16), resized_info.height);
    try testing.expectEqual(@as(f64, 2.0), resized_info.scale_factor);
    try testing.expectEqual(@as(u64, 2), resized_info.generation);
    try resized_frame.release();

    try expectRenderSessionCallWrongThread(&session, .acquire_metal_frame);
    try session.close();
}

test "Metal owned texture frame release follows moved session wrapper" {
    if (!build_options.supports_metal) return error.SkipZigTest;
    const device = MTLCreateSystemDefaultDevice() orelse return error.MetalDeviceUnavailable;

    const pool = try metal_support.AutoreleasePool.init();
    defer pool.deinit();

    var handles = try createMovedMetalSessionWithFrame(device);
    try testing.expectError(error.ActiveBorrow, handles.session.close());
    try handles.frame.release();
    try handles.session.close();
    try handles.map.close();
    try handles.runtime.close();
}

test "render session rejects wrong-thread calls through public bindings" {
    if (!supports_test_owned_texture) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var owned = try attachTestOwnedTexture(&map, .{});
    defer owned.close() catch {};
    const session = &owned.session;

    inline for (.{
        RenderSessionThreadCall.render_update,
        .resize,
        .detach,
        .reduce_memory_use,
        .clear_data,
        .dump_debug_logs,
        .close,
    }) |call| {
        try expectRenderSessionCallWrongThread(session, call);
    }
}

test "Metal borrowed texture renders through public bindings" {
    if (!build_options.supports_metal) return error.SkipZigTest;
    const device = MTLCreateSystemDefaultDevice() orelse return error.MetalDeviceUnavailable;

    const pool = try metal_support.AutoreleasePool.init();
    defer pool.deinit();

    const borrowed = try metal_support.createTexture(device, 128, 128);
    defer metal_support.releaseObject(borrowed);
    try metal_support.clearTextureRGBA8(borrowed, .{ 255, 0, 255, 255 });
    try expectPixelApprox(try metal_support.readTexturePixelRGBA8(borrowed, 0, 0), .{ 255, 0, 255, 255 }, 0);

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachMetalBorrowedTexture(&map, .{
        .extent = .{ .width = 128, .height = 128 },
        .texture = .{ .ptr = borrowed },
    });
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();
    try expectPixelApprox(try metal_support.readTexturePixelRGBA8(borrowed, 0, 0), .{ 0xd8, 0xf1, 0xff, 0xff }, 8);

    try testing.expectError(error.Unsupported, session.acquireMetalOwnedTextureFrame());
    try testing.expectError(error.Unsupported, session.resize(.{ .width = 64, .height = 64, .scale_factor = 1.0 }));
    try testing.expectError(error.Unsupported, session.readPremultipliedRgba8(testing.allocator));
}

test "Vulkan owned texture frame handle scopes native pointers" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;

    var context = try VulkanAttachContext.init();
    defer context.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachVulkanOwnedTexture(&map, .{
        .extent = .{ .width = 32, .height = 32, .scale_factor = 1.0 },
        .context = context.descriptor(),
    });
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    var image = try session.readPremultipliedRgba8(testing.allocator);
    defer image.deinit();
    try testing.expectEqual(@as(u32, 32), image.info.width);
    try testing.expectEqual(@as(u32, 32), image.info.height);
    try testing.expectEqual(@as(usize, 32 * 32 * 4), image.info.byte_length);

    var frame = try session.acquireVulkanOwnedTextureFrame();
    const info = try frame.info();
    try testing.expectEqual(@as(u32, 32), info.width);
    try testing.expectEqual(@as(u32, 32), info.height);
    try testing.expectEqual(@as(u64, 1), info.generation);
    try testing.expect(info.image.ptr != info.device.ptr);

    try testing.expectError(error.ActiveBorrow, session.resize(.{ .width = 16, .height = 16, .scale_factor = 1.0 }));
    try testing.expectError(error.ActiveBorrow, session.renderUpdate());
    try testing.expectError(error.ActiveBorrow, session.detach());
    try testing.expectError(error.ActiveBorrow, session.acquireVulkanOwnedTextureFrame());
    try testing.expectError(error.ActiveBorrow, session.close());

    try frame.release();
    try frame.release();

    try session.resize(.{ .width = 16, .height = 8, .scale_factor = 2.0 });
    try session.renderUpdate();
    var resized_frame = try session.acquireVulkanOwnedTextureFrame();
    const resized_info = try resized_frame.info();
    try testing.expectEqual(@as(u32, 32), resized_info.width);
    try testing.expectEqual(@as(u32, 16), resized_info.height);
    try testing.expectEqual(@as(f64, 2.0), resized_info.scale_factor);
    try testing.expectEqual(@as(u64, 2), resized_info.generation);
    try resized_frame.release();

    try expectRenderSessionCallWrongThread(&session, .acquire_vulkan_frame);
    try session.close();
}

test "Vulkan borrowed texture renders through public bindings" {
    if (!build_options.supports_vulkan) return error.SkipZigTest;

    var borrowed = try VulkanBorrowedImage.create(128, 128);
    defer borrowed.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var session = try maplibre.attachVulkanBorrowedTexture(&map, borrowed.descriptor());
    defer session.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try session.renderUpdate();

    try testing.expectError(error.Unsupported, session.acquireVulkanOwnedTextureFrame());
    try testing.expectError(error.Unsupported, session.resize(.{ .width = 64, .height = 64, .scale_factor = 1.0 }));
    try testing.expectError(error.Unsupported, session.readPremultipliedRgba8(testing.allocator));
}
