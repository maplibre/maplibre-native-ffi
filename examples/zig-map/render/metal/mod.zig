const std = @import("std");
const objc = @import("objc");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const maplibre = @import("maplibre_native");
const render_target = @import("../../render_target.zig");
const types = @import("../../types.zig");

extern "c" fn MTLCreateSystemDefaultDevice() objc.c.id;

const MTLPixelFormatBGRA8Unorm: u64 = 80;
const MTLPixelFormatRGBA8Unorm: u64 = 70;
const MTLLoadActionClear: u64 = 2;
const MTLStoreActionStore: u64 = 1;
const MTLPrimitiveTypeTriangle: u64 = 3;
const MTLTextureUsageShaderRead: u64 = 1;
const MTLTextureUsageRenderTarget: u64 = 4;

const CGSize = extern struct { width: f64, height: f64 };
const MTLClearColor = extern struct {
    red: f64,
    green: f64,
    blue: f64,
    alpha: f64,
};

pub const MetalRenderTarget = union(enum) {
    pub const window_flags = c.SDL_WINDOW_METAL;

    owned_texture: MetalOwnedTextureBackend,
    borrowed_texture: MetalBorrowedTextureBackend,
    native_surface: MetalSurfaceBackend,

    pub fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
        map: *maplibre.MapHandle,
    ) !MetalRenderTarget {
        _ = allocator;
        return switch (mode) {
            .owned_texture => .{ .owned_texture = try MetalOwnedTextureBackend.init(window, viewport, map) },
            .borrowed_texture => .{ .borrowed_texture = try MetalBorrowedTextureBackend.init(window, viewport, map) },
            .native_surface => .{ .native_surface = try MetalSurfaceBackend.init(window, viewport, map) },
        };
    }

    pub fn deinit(self: *MetalRenderTarget) void {
        switch (self.*) {
            .owned_texture => |*backend| backend.deinit(),
            .borrowed_texture => |*backend| backend.deinit(),
            .native_surface => |*backend| backend.deinit(),
        }
    }

    pub fn resize(self: *MetalRenderTarget, viewport: types.Viewport) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.resize(viewport),
            .borrowed_texture => |*backend| try backend.resize(viewport),
            .native_surface => |*backend| try backend.resize(viewport),
        }
    }

    pub fn needsReattachOnResize(self: *const MetalRenderTarget) bool {
        return switch (self.*) {
            .owned_texture, .native_surface => false,
            .borrowed_texture => true,
        };
    }

    pub fn finishFrame(_: *MetalRenderTarget) !void {}

    pub fn renderUpdate(
        self: *MetalRenderTarget,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        return switch (self.*) {
            .owned_texture => |*backend| backend.renderUpdate(diagnostic_store, viewport),
            .borrowed_texture => |*backend| backend.renderUpdate(diagnostic_store, viewport),
            .native_surface => |*backend| backend.renderUpdate(diagnostic_store),
        };
    }
};

const MetalView = struct {
    view: c.SDL_MetalView,
    device: objc.Object,
    layer: objc.Object,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !MetalView {
        const view = c.SDL_Metal_CreateView(window);
        if (view == null) return types.AppError.BackendSetupFailed;
        errdefer c.SDL_Metal_DestroyView(view);

        const device_id = MTLCreateSystemDefaultDevice();
        if (device_id == null) return types.AppError.BackendSetupFailed;
        const device = objc.Object.fromId(device_id);
        errdefer device.release();

        const layer_ptr = c.SDL_Metal_GetLayer(view) orelse
            return types.AppError.BackendSetupFailed;
        const layer = objc.Object.fromId(layer_ptr);
        layer.setProperty("device", device);
        layer.setProperty("pixelFormat", @as(u64, MTLPixelFormatBGRA8Unorm));
        layer.setProperty("drawableSize", drawableSize(viewport));

        return .{ .view = view, .device = device, .layer = layer };
    }

    fn deinit(self: *MetalView) void {
        self.device.release();
        c.SDL_Metal_DestroyView(self.view);
    }

    fn resize(self: *MetalView, viewport: types.Viewport) void {
        self.layer.setProperty("drawableSize", drawableSize(viewport));
    }
};

const MetalTextureCompositor = struct {
    view: MetalView,
    queue: objc.Object,
    pipeline: objc.Object,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !MetalTextureCompositor {
        var view = try MetalView.init(window, viewport);
        errdefer view.deinit();

        const queue = view.device.msgSend(objc.Object, "newCommandQueue", .{});
        if (queue.value == null) return types.AppError.BackendSetupFailed;
        errdefer queue.release();

        const pipeline = try createPipeline(view.device);
        errdefer pipeline.release();

        return .{ .view = view, .queue = queue, .pipeline = pipeline };
    }

    fn deinit(self: *MetalTextureCompositor) void {
        self.pipeline.release();
        self.queue.release();
        self.view.deinit();
    }

    fn resize(self: *MetalTextureCompositor, viewport: types.Viewport) void {
        self.view.resize(viewport);
    }

    fn drawMetalTexture(self: *MetalTextureCompositor, metal_texture: *anyopaque) !bool {
        const drawable = self.view.layer.msgSend(objc.Object, "nextDrawable", .{});
        if (drawable.value == null) return types.AppError.BackendDrawFailed;

        const drawable_texture = drawable.getProperty(objc.Object, "texture");
        const pass_descriptor = objc.getClass("MTLRenderPassDescriptor").?
            .msgSend(objc.Object, "renderPassDescriptor", .{});
        const color_attachments = pass_descriptor.getProperty(objc.Object, "colorAttachments");
        const attachment = color_attachments.msgSend(
            objc.Object,
            "objectAtIndexedSubscript:",
            .{@as(c_ulong, 0)},
        );
        attachment.setProperty("texture", drawable_texture);
        attachment.setProperty("loadAction", @as(u64, MTLLoadActionClear));
        attachment.setProperty("storeAction", @as(u64, MTLStoreActionStore));
        attachment.setProperty("clearColor", clearColor());

        const command_buffer = self.queue.msgSend(objc.Object, "commandBuffer", .{});
        if (command_buffer.value == null) return types.AppError.BackendDrawFailed;
        const encoder = command_buffer.msgSend(
            objc.Object,
            "renderCommandEncoderWithDescriptor:",
            .{pass_descriptor},
        );
        if (encoder.value == null) return types.AppError.BackendDrawFailed;

        encoder.msgSend(void, "setRenderPipelineState:", .{self.pipeline});
        encoder.msgSend(void, "setFragmentTexture:atIndex:", .{
            objc.Object.fromId(metal_texture),
            @as(c_ulong, 0),
        });
        encoder.msgSend(void, "drawPrimitives:vertexStart:vertexCount:", .{
            @as(u64, MTLPrimitiveTypeTriangle),
            @as(c_ulong, 0),
            @as(c_ulong, 3),
        });
        encoder.msgSend(void, "endEncoding", .{});
        command_buffer.msgSend(void, "presentDrawable:", .{drawable});
        command_buffer.msgSend(void, "commit", .{});
        command_buffer.msgSend(void, "waitUntilCompleted", .{});
        return true;
    }
};

const MetalOwnedTextureBackend = struct {
    compositor: MetalTextureCompositor,
    session: render_target.Session,

    fn init(
        window: *c.SDL_Window,
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !MetalOwnedTextureBackend {
        var self = MetalOwnedTextureBackend{
            .compositor = try MetalTextureCompositor.init(window, viewport),
            .session = .none,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
    }

    fn deinit(self: *MetalOwnedTextureBackend) void {
        self.session.deinit();
        self.compositor.deinit();
    }

    fn resize(self: *MetalOwnedTextureBackend, viewport: types.Viewport) !void {
        self.compositor.resize(viewport);
        try self.session.resize(viewport, null);
    }

    fn attachRenderTarget(
        self: *MetalOwnedTextureBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const texture = maplibre.attachMetalOwnedTexture(map, .{
            .extent = render_target.extent(viewport),
            .context = .{ .device = maplibre.NativePointer.fromPtr(self.compositor.view.device.value.?) },
        }) catch |err| {
            diagnostics.logError("Metal texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
    }

    fn renderUpdate(
        self: *MetalOwnedTextureBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        _ = viewport;
        if (!try self.session.renderUpdate(diagnostic_store)) return false;
        const texture = switch (self.session) {
            .texture => |*texture| texture,
            else => return false,
        };
        var frame = texture.acquireMetalOwnedTextureFrame() catch |err| switch (err) {
            error.InvalidState => return false,
            else => {
                diagnostics.logError("Metal texture acquire failed", err, null);
                return types.AppError.BackendDrawFailed;
            },
        };
        defer frame.release() catch |err| diagnostics.logError("Metal texture release failed", err, null);

        const info = try frame.info();
        return try self.compositor.drawMetalTexture(info.texture.toPtr());
    }
};

const MetalBorrowedTextureBackend = struct {
    compositor: MetalTextureCompositor,
    session: render_target.Session,
    borrowed_texture: objc.Object,

    fn init(
        window: *c.SDL_Window,
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !MetalBorrowedTextureBackend {
        var compositor = try MetalTextureCompositor.init(window, viewport);
        errdefer compositor.deinit();
        var self = MetalBorrowedTextureBackend{
            .compositor = compositor,
            .session = .none,
            .borrowed_texture = try createBorrowedTexture(compositor.view.device, viewport),
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
    }

    fn deinit(self: *MetalBorrowedTextureBackend) void {
        self.session.deinit();
        self.borrowed_texture.release();
        self.compositor.deinit();
    }

    fn resize(_: *MetalBorrowedTextureBackend, _: types.Viewport) !void {
        return types.AppError.TextureResizeFailed;
    }

    fn attachRenderTarget(
        self: *MetalBorrowedTextureBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const texture = maplibre.attachMetalBorrowedTexture(map, .{
            .extent = render_target.extent(viewport),
            .texture = maplibre.NativePointer.fromPtr(self.borrowed_texture.value.?),
        }) catch |err| {
            diagnostics.logError("Metal borrowed texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
    }

    fn renderUpdate(
        self: *MetalBorrowedTextureBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        _ = viewport;
        if (!try self.session.renderUpdate(diagnostic_store)) return false;
        return try self.compositor.drawMetalTexture(self.borrowed_texture.value.?);
    }
};

const MetalSurfaceBackend = struct {
    view: MetalView,
    session: render_target.Session,

    fn init(
        window: *c.SDL_Window,
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !MetalSurfaceBackend {
        var self = MetalSurfaceBackend{
            .view = try MetalView.init(window, viewport),
            .session = .none,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
    }

    fn deinit(self: *MetalSurfaceBackend) void {
        self.session.deinit();
        self.view.deinit();
    }

    fn resize(self: *MetalSurfaceBackend, viewport: types.Viewport) !void {
        self.view.resize(viewport);
        try self.session.resize(viewport, null);
    }

    fn renderUpdate(
        self: *MetalSurfaceBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
    ) !bool {
        return try self.session.renderUpdate(diagnostic_store);
    }

    fn attachRenderTarget(
        self: *MetalSurfaceBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const surface = maplibre.attachMetalSurface(map, .{
            .extent = render_target.extent(viewport),
            .context = .{ .device = maplibre.NativePointer.fromPtr(self.view.device.value.?) },
            .layer = maplibre.NativePointer.fromPtr(self.view.layer.value.?),
        }) catch |err| {
            diagnostics.logError("Metal surface attach failed", err, null);
            return types.AppError.SurfaceAttachFailed;
        };
        return .{ .surface = surface };
    }
};

fn createBorrowedTexture(device: objc.Object, viewport: types.Viewport) !objc.Object {
    const descriptor = objc.getClass("MTLTextureDescriptor").?
        .msgSend(objc.Object, "texture2DDescriptorWithPixelFormat:width:height:mipmapped:", .{
        @as(u64, MTLPixelFormatRGBA8Unorm),
        @as(c_ulong, viewport.physical_width),
        @as(c_ulong, viewport.physical_height),
        false,
    });
    if (descriptor.value == null) return types.AppError.BackendSetupFailed;
    descriptor.setProperty(
        "usage",
        @as(u64, MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget),
    );
    const texture = device.msgSend(objc.Object, "newTextureWithDescriptor:", .{descriptor});
    if (texture.value == null) return types.AppError.BackendSetupFailed;
    return texture;
}

fn drawableSize(viewport: types.Viewport) CGSize {
    return .{
        .width = @floatFromInt(viewport.physical_width),
        .height = @floatFromInt(viewport.physical_height),
    };
}

fn clearColor() MTLClearColor {
    return .{ .red = 0.08, .green = 0.09, .blue = 0.11, .alpha = 1.0 };
}

fn createPipeline(device: objc.Object) !objc.Object {
    const NSString = objc.getClass("NSString").?;
    const source = NSString.msgSend(
        objc.Object,
        "stringWithUTF8String:",
        .{metal_shader_source.ptr},
    );
    if (source.value == null) return types.AppError.BackendSetupFailed;

    var error_object: objc.c.id = null;
    const library = device.msgSend(
        objc.Object,
        "newLibraryWithSource:options:error:",
        .{ source, @as(objc.c.id, null), &error_object },
    );
    if (library.value == null) return types.AppError.BackendSetupFailed;
    defer library.release();

    const vertex_name = NSString.msgSend(
        objc.Object,
        "stringWithUTF8String:",
        .{"vertex_main"},
    );
    const fragment_name = NSString.msgSend(
        objc.Object,
        "stringWithUTF8String:",
        .{"fragment_main"},
    );
    const vertex = library.msgSend(objc.Object, "newFunctionWithName:", .{vertex_name});
    if (vertex.value == null) return types.AppError.BackendSetupFailed;
    defer vertex.release();
    const fragment = library.msgSend(objc.Object, "newFunctionWithName:", .{fragment_name});
    if (fragment.value == null) return types.AppError.BackendSetupFailed;
    defer fragment.release();

    const descriptor = objc.getClass("MTLRenderPipelineDescriptor").?
        .msgSend(objc.Object, "alloc", .{})
        .msgSend(objc.Object, "init", .{});
    if (descriptor.value == null) return types.AppError.BackendSetupFailed;
    defer descriptor.release();
    descriptor.setProperty("vertexFunction", vertex);
    descriptor.setProperty("fragmentFunction", fragment);
    const attachments = descriptor.getProperty(objc.Object, "colorAttachments");
    const attachment = attachments.msgSend(
        objc.Object,
        "objectAtIndexedSubscript:",
        .{@as(c_ulong, 0)},
    );
    attachment.setProperty("pixelFormat", @as(u64, MTLPixelFormatBGRA8Unorm));

    var pipeline_error: objc.c.id = null;
    const pipeline = device.msgSend(
        objc.Object,
        "newRenderPipelineStateWithDescriptor:error:",
        .{ descriptor, &pipeline_error },
    );
    if (pipeline.value == null) return types.AppError.BackendSetupFailed;
    return pipeline;
}

const metal_shader_source = @embedFile("shader.metal");
