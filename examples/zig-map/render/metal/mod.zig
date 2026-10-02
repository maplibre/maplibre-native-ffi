const std = @import("std");
const objc = @import("objc");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const maplibre = @import("maplibre_native_ffi");
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
    ) !MetalRenderTarget {
        _ = allocator;
        return switch (mode) {
            .owned_texture => .{ .owned_texture = try MetalOwnedTextureBackend.init(window, viewport) },
            .borrowed_texture => .{ .borrowed_texture = try MetalBorrowedTextureBackend.init(window, viewport) },
            .native_surface => .{ .native_surface = .{ .view = try MetalView.init(window) } },
        };
    }

    /// Attaches the render session. Every Metal target accepts a core
    /// worker, which renders on its own thread.
    pub fn attach(self: *MetalRenderTarget, map: *maplibre.Map, viewport: types.Viewport) !void {
        switch (self.*) {
            inline else => |*backend| try backend.attach(map, viewport, render_target.attachOptions(.core_worker)),
        }
    }

    /// Releases any held frame, then detaches the session.
    pub fn detach(self: *MetalRenderTarget) void {
        if (self.* == .owned_texture) render_target.releaseFrame(&self.owned_texture.held);
        self.session().deinit();
    }

    pub fn deinit(self: *MetalRenderTarget) void {
        switch (self.*) {
            inline else => |*backend| backend.deinit(),
        }
    }

    pub fn session(self: *MetalRenderTarget) *render_target.Session {
        return switch (self.*) {
            inline else => |*backend| &backend.session,
        };
    }

    /// Starts the session resize or target replacement a new viewport needs.
    pub fn resize(self: *MetalRenderTarget, viewport: types.Viewport) !void {
        switch (self.*) {
            inline else => |*backend| try backend.resize(viewport),
        }
    }

    /// Follows a completed borrowed-texture replacement.
    pub fn showReplacements(self: *MetalRenderTarget) !void {
        if (self.* == .borrowed_texture) try self.borrowed_texture.showReplacements();
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window.
    pub fn present(self: *MetalRenderTarget, viewport: types.Viewport) !bool {
        _ = viewport;
        return switch (self.*) {
            .owned_texture => |*backend| backend.present(),
            .borrowed_texture => |*backend| blk: {
                try backend.showReplacements();
                break :blk backend.compositor.drawMetalTexture(backend.texture.value.?, .{ .kind = .cpu_complete });
            },
            // The driver already presented the frame.
            .native_surface => true,
        };
    }
};

const MetalView = struct {
    view: c.SDL_MetalView,
    device: objc.Object,
    layer: objc.Object,

    fn init(window: *c.SDL_Window) !MetalView {
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
        var view = try MetalView.init(window);
        errdefer view.deinit();
        // A texture mode's compositor sizes the layer's drawable. A surface
        // session sizes it itself.
        view.resize(viewport);

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

    fn drawMetalTexture(self: *MetalTextureCompositor, metal_texture: *anyopaque, producer_sync: maplibre.GpuSync) !bool {
        const drawable = self.view.layer.msgSend(objc.Object, "nextDrawable", .{});
        // A minimized or occluded window has no drawable, so skip the frame
        // rather than fail.
        if (drawable.value == null) return false;

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
        switch (producer_sync.kind) {
            .cpu_complete => {},
            .metal_shared_event => command_buffer.msgSend(void, "encodeWaitForEvent:value:", .{
                objc.Object.fromId(@as(*anyopaque, @ptrFromInt(producer_sync.object))),
                producer_sync.value,
            }),
            else => return types.AppError.BackendDrawFailed,
        }
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
    session: render_target.Session = .{},
    /// The newest frame, held until a newer one replaces it.
    held: ?maplibre.AcquiredFrame = null,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !MetalOwnedTextureBackend {
        return .{ .compositor = try MetalTextureCompositor.init(window, viewport) };
    }

    fn deinit(self: *MetalOwnedTextureBackend) void {
        render_target.releaseFrame(&self.held);
        self.session.deinit();
        self.compositor.deinit();
    }

    fn attach(self: *MetalOwnedTextureBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const attachment = maplibre.metalOwnedTextureAttach(std.heap.smp_allocator, map.*, .{
            .extent = render_target.extent(viewport),
            .context = .{ .device = (self.compositor.view.device.value.?) },
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("Metal texture attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(map, attachment, options, .owned_texture);
    }

    fn resize(self: *MetalOwnedTextureBackend, viewport: types.Viewport) !void {
        self.compositor.resize(viewport);
        // A session resizes only while the host holds none of its frames.
        render_target.releaseFrame(&self.held);
        try self.session.resize(viewport);
    }

    fn present(self: *MetalOwnedTextureBackend) !bool {
        // Without a new frame, the window keeps the one it already shows.
        if (!try self.session.acquireNewest(&self.held)) return true;
        const frame = self.held.?;
        const Context = struct {
            backend: *MetalOwnedTextureBackend,
            frame: maplibre.AcquiredFrame,
            sync: maplibre.GpuSync = .{},
            fn producer(context: @This(), sync: maplibre.GpuSync) anyerror!bool {
                var with_sync = context;
                with_sync.sync = sync;
                return maplibre.acquiredFrameGetMetalTexture(bool, context.frame, with_sync, draw, null);
            }
            fn draw(context: @This(), info: maplibre.MetalOwnedTextureFrame) anyerror!bool {
                return context.backend.compositor.drawMetalTexture(info.texture orelse return types.AppError.BackendDrawFailed, context.sync);
            }
        };
        return maplibre.acquiredFrameGetProducerSync(bool, frame, Context{ .backend = self, .frame = frame }, Context.producer, null);
    }
};

const MetalBorrowedTextureBackend = struct {
    compositor: MetalTextureCompositor,
    session: render_target.Session = .{},
    /// The texture the compositor samples.
    texture: objc.Object,
    replacements: render_target.Replacements(objc.Object) = .{},

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !MetalBorrowedTextureBackend {
        var compositor = try MetalTextureCompositor.init(window, viewport);
        errdefer compositor.deinit();
        return .{
            .compositor = compositor,
            .texture = try createBorrowedTexture(compositor.view.device, viewport),
        };
    }

    fn deinit(self: *MetalBorrowedTextureBackend) void {
        self.session.deinit();
        while (self.replacements.takeAny()) |texture| texture.release();
        self.replacements.deinit();
        self.texture.release();
        self.compositor.deinit();
    }

    fn attach(self: *MetalBorrowedTextureBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const attachment = maplibre.metalBorrowedTextureAttach(std.heap.smp_allocator, map.*, .{
            .extent = render_target.extent(viewport),
            .physical_width = viewport.physical_width,
            .physical_height = viewport.physical_height,
            .texture = (self.texture.value.?),
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("Metal borrowed texture attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(map, attachment, options, .borrowed_texture);
    }

    /// Follows a resized window: allocates a texture at the new size and hands
    /// it to the live session, which stays attached.
    fn resize(self: *MetalBorrowedTextureBackend, viewport: types.Viewport) !void {
        self.compositor.resize(viewport);
        const replacement = try createBorrowedTexture(self.compositor.view.device, viewport);
        errdefer replacement.release();
        var diagnostic: maplibre.Diagnostic = .{};
        var completion = maplibre.metalBorrowedTextureSetTarget(std.heap.smp_allocator, self.session.handle.?, .{
            .extent = render_target.extent(viewport),
            .physical_width = viewport.physical_width,
            .physical_height = viewport.physical_height,
            .texture = (replacement.value.?),
        }, &diagnostic) catch |err| {
            diagnostics.logError("Metal borrowed texture set target failed", err, &diagnostic);
            return types.AppError.ResizeFailed;
        };
        self.replacements.push(completion, replacement) catch |err| {
            completion.deinit();
            return err;
        };
        try self.session.resizeMap(viewport);
    }

    /// Switches the compositor to each replacement a rendered frame has
    /// drawn into, releasing the texture it retires.
    fn showReplacements(self: *MetalBorrowedTextureBackend) !void {
        while (try self.replacements.takeShown(&self.session)) |replacement| {
            self.texture.release();
            self.texture = replacement;
        }
    }
};

const MetalSurfaceBackend = struct {
    view: MetalView,
    session: render_target.Session = .{},

    fn deinit(self: *MetalSurfaceBackend) void {
        self.session.deinit();
        self.view.deinit();
    }

    fn attach(self: *MetalSurfaceBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const attachment = maplibre.metalSurfaceAttach(std.heap.smp_allocator, map.*, .{
            .extent = render_target.extent(viewport),
            .context = .{ .device = (self.view.device.value.?) },
            .layer = (self.view.layer.value.?),
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("Metal surface attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(map, attachment, options, .native_surface);
    }

    fn resize(self: *MetalSurfaceBackend, viewport: types.Viewport) !void {
        // The session sets the layer's drawable size.
        try self.session.resize(viewport);
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
