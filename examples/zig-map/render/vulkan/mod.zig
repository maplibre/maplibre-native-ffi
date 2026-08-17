const std = @import("std");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const maplibre = @import("maplibre_native_ffi");
const render_target = @import("../../render_target.zig");
const types = @import("../../types.zig");
const Commands = @import("commands.zig").Commands;
const Context = @import("context.zig").Context;
const Pipeline = @import("pipeline.zig").Pipeline;
const Swapchain = @import("swapchain.zig").Swapchain;
const util = @import("util.zig");

pub const VulkanRenderTarget = union(enum) {
    pub const window_flags = c.SDL_WINDOW_VULKAN;

    owned_texture: VulkanOwnedTextureBackend,
    borrowed_texture: VulkanBorrowedTextureBackend,
    native_surface: VulkanSurfaceBackend,

    pub fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !VulkanRenderTarget {
        return switch (mode) {
            .owned_texture => .{ .owned_texture = try VulkanOwnedTextureBackend.init(allocator, window, viewport) },
            .borrowed_texture => .{ .borrowed_texture = try VulkanBorrowedTextureBackend.init(allocator, window, viewport) },
            .native_surface => .{ .native_surface = try VulkanSurfaceBackend.init(allocator, window, viewport) },
        };
    }

    /// Attaches the render session on the graphics thread.
    pub fn attach(self: *VulkanRenderTarget, map: *maplibre.MapHandle, viewport: types.Viewport) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.attach(map, viewport),
            .borrowed_texture => |*backend| try backend.attach(map, viewport),
            .native_surface => |*backend| try backend.attach(map, viewport),
        }
    }

    pub fn deinit(self: *VulkanRenderTarget) void {
        switch (self.*) {
            .owned_texture => |*backend| backend.deinit(),
            .borrowed_texture => |*backend| backend.deinit(),
            .native_surface => |*backend| backend.deinit(),
        }
    }

    pub fn resize(self: *VulkanRenderTarget, viewport: types.Viewport) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.resize(viewport),
            .borrowed_texture => |*backend| try backend.resize(viewport),
            .native_surface => |*backend| try backend.resize(viewport),
        }
    }

    pub fn finishFrame(self: *VulkanRenderTarget) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.finishFrame(),
            .borrowed_texture => |*backend| try backend.finishFrame(),
            .native_surface => |*backend| try backend.finishFrame(),
        }
    }

    pub fn renderUpdate(
        self: *VulkanRenderTarget,
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

const VulkanTextureCompositor = struct {
    context: Context,
    swapchain: Swapchain,
    pipeline: Pipeline,
    commands: Commands,
    current_viewport: types.Viewport,
    swapchain_stale: bool = false,

    fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
    ) !VulkanTextureCompositor {
        var context = try Context.init(allocator, window);
        errdefer context.deinit();

        var swapchain = try Swapchain.init(allocator, &context, viewport, null);
        errdefer swapchain.deinit(context.device);

        var pipeline = try Pipeline.init(allocator, context.device, swapchain.format);
        errdefer pipeline.deinit(context.device);
        try swapchain.createFramebuffers(context.device, pipeline.render_pass);

        var commands = try Commands.init(allocator, context.device, context.queue_family_index);
        errdefer commands.deinit(context.device);
        try commands.createPresentSemaphores(context.device, @intCast(swapchain.images.len));

        return .{
            .context = context,
            .swapchain = swapchain,
            .pipeline = pipeline,
            .commands = commands,
            .current_viewport = viewport,
        };
    }

    fn deinit(self: *VulkanTextureCompositor) void {
        self.context.waitIdle();
        self.commands.deinit(self.context.device);
        self.swapchain.deinit(self.context.device);
        self.pipeline.deinit(self.context.device);
        self.context.deinit();
    }

    fn waitIdle(self: *VulkanTextureCompositor) void {
        self.context.waitIdle();
    }

    /// Notes a resized window without touching the swapchain. The compositor
    /// only presents when a map frame is ready, so destroying the swapchain
    /// here would blank the window until the map renders at the new extent.
    fn resize(self: *VulkanTextureCompositor, viewport: types.Viewport) void {
        self.current_viewport = viewport;
        self.swapchain_stale = true;
    }

    fn recreateSwapchain(self: *VulkanTextureCompositor) !void {
        self.context.waitIdle();
        // Create the replacement naming the retired swapchain as oldSwapchain
        // before destroying it: on MoltenVK, destroying first leaves presents
        // that succeed but reach no drawable the window shows.
        var previous = self.swapchain;
        const previous_format = previous.format;
        const replacement = Swapchain.init(
            previous.allocator,
            &self.context,
            self.current_viewport,
            previous.handle,
        ) catch |err| {
            // Storing the emptied struct back keeps teardown from destroying
            // the retired swapchain's handles a second time.
            previous.deinit(self.context.device);
            self.swapchain = previous;
            return err;
        };
        previous.deinit(self.context.device);
        self.swapchain = replacement;

        if (self.swapchain.format != previous_format) {
            self.pipeline.deinit(self.context.device);
            self.pipeline = try Pipeline.init(
                self.pipeline.allocator,
                self.context.device,
                self.swapchain.format,
            );
        }
        try self.swapchain.createFramebuffers(
            self.context.device,
            self.pipeline.render_pass,
        );
        self.commands.destroyPresentSemaphores(self.context.device);
        try self.commands.createPresentSemaphores(
            self.context.device,
            @intCast(self.swapchain.images.len),
        );
    }

    fn waitForFrame(self: *VulkanTextureCompositor) !void {
        try self.commands.waitForFrameFence(self.context.device);
    }

    fn presentImageView(self: *VulkanTextureCompositor, image_view: c.VkImageView) !bool {
        try self.waitForFrame();

        if (self.swapchain_stale) {
            try self.recreateSwapchain();
            self.swapchain_stale = false;
        }

        // Must follow the fence wait, so no in-flight command reads the
        // descriptor set, and the swapchain replacement, which can rebuild the
        // pipeline.
        if (image_view != self.pipeline.descriptor_image_view) {
            self.pipeline.updateDescriptor(self.context.device, image_view);
        }

        var image_index: u32 = 0;
        const acquire = c.vkAcquireNextImageKHR(
            self.context.device,
            self.swapchain.handle,
            std.math.maxInt(u64),
            self.commands.image_available,
            null,
            &image_index,
        );
        if (acquire == c.VK_ERROR_OUT_OF_DATE_KHR) {
            self.swapchain_stale = true;
            return false;
        }
        if (acquire == c.VK_SUBOPTIMAL_KHR) {
            // Still presentable, but the surface has moved on.
            self.swapchain_stale = true;
        }
        try util.expectVkOrSuboptimal(acquire);
        try self.commands.resetFence(self.context.device);

        try self.commands.record(
            self.context.device,
            &self.swapchain,
            &self.pipeline,
            image_index,
        );
        try self.commands.submit(self.context.queue, image_index);

        const present_info = c.VkPresentInfoKHR{
            .sType = c.VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = null,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &self.commands.render_finished[image_index],
            .swapchainCount = 1,
            .pSwapchains = &self.swapchain.handle,
            .pImageIndices = &image_index,
            .pResults = null,
        };
        const present = c.vkQueuePresentKHR(self.context.queue, &present_info);
        if (present == c.VK_ERROR_OUT_OF_DATE_KHR) {
            // Nothing reached the screen, but the sampling pass was submitted;
            // wait it out before the caller releases its frame.
            self.swapchain_stale = true;
            self.waitForFrame() catch {};
            return false;
        }
        if (present != c.VK_SUCCESS and present != c.VK_SUBOPTIMAL_KHR) {
            self.waitForFrame() catch {};
            try util.expectVk(present);
        }
        if (present == c.VK_SUBOPTIMAL_KHR) {
            self.swapchain_stale = true;
        }
        return true;
    }
};

const VulkanOwnedTextureBackend = struct {
    compositor: VulkanTextureCompositor,
    session: render_target.Session,
    pending_frame: ?maplibre.AcquiredFrame,

    fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
    ) !VulkanOwnedTextureBackend {
        var self = VulkanOwnedTextureBackend{
            .compositor = try VulkanTextureCompositor.init(allocator, window, viewport),
            .session = .none,
            .pending_frame = null,
        };
        errdefer self.deinit();
        return self;
    }

    /// Attaches the render session on this thread, which becomes its owner
    /// thread for the session's whole life.
    fn attach(self: *VulkanOwnedTextureBackend, map: *maplibre.MapHandle, viewport: types.Viewport) !void {
        self.session = try self.attachRenderTarget(map, viewport);
    }

    fn deinit(self: *VulkanOwnedTextureBackend) void {
        self.compositor.waitIdle();
        self.releasePendingFrame();
        self.session.deinit();
        self.compositor.deinit();
    }

    fn resize(self: *VulkanOwnedTextureBackend, viewport: types.Viewport) !void {
        self.compositor.waitIdle();
        self.releasePendingFrame();
        self.compositor.resize(viewport);
        try self.session.resize(viewport, null);
    }

    fn finishFrame(self: *VulkanOwnedTextureBackend) !void {
        if (self.pending_frame == null) return;
        try self.compositor.waitForFrame();
        self.releasePendingFrame();
    }

    fn attachRenderTarget(
        self: *VulkanOwnedTextureBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const texture = maplibre.attachVulkanOwnedTexture(map, .{
            .extent = render_target.extent(viewport),
            .context = vulkanContextDescriptor(&self.compositor.context),
        }, .{ .driver = .caller_graphics_thread, .requested_texture_ring_depth = 2 }) catch |err| {
            diagnostics.logError("Vulkan texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return render_target.textureSession(texture);
    }

    fn renderUpdate(
        self: *VulkanOwnedTextureBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        _ = viewport;
        if (!try self.session.renderUpdate(diagnostic_store)) return false;
        const texture = switch (self.session) {
            .texture => |*texture| texture,
            else => return false,
        };
        var frame = texture.acquireFrame() catch |err| switch (err) {
            error.InvalidState => return false,
            else => {
                diagnostics.logError("Vulkan texture acquire failed", err, null);
                return types.AppError.BackendDrawFailed;
            },
        };
        var frame_owned = true;
        errdefer if (frame_owned) frame.release(.cpu_complete) catch {};
        const info = try frame.vulkanTexture();
        const image_view: c.VkImageView = @ptrCast(info.image_view.toPtr());
        if (!try self.compositor.presentImageView(image_view)) {
            try frame.release(.cpu_complete);
            frame_owned = false;
            return false;
        }

        frame_owned = false;
        self.pending_frame = frame;
        return true;
    }

    fn releasePendingFrame(self: *VulkanOwnedTextureBackend) void {
        if (self.pending_frame) |*frame| {
            frame.release(.cpu_complete) catch |err| diagnostics.logError("Vulkan texture release failed", err, null);
        }
        self.pending_frame = null;
    }
};

const BorrowedImage = struct {
    image: c.VkImage,
    memory: c.VkDeviceMemory,
    view: c.VkImageView,

    fn init(context: *const Context, viewport: types.Viewport) !BorrowedImage {
        var self = BorrowedImage{ .image = null, .memory = null, .view = null };
        errdefer self.deinit(context.device);

        const image_info = c.VkImageCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .imageType = c.VK_IMAGE_TYPE_2D,
            .format = c.VK_FORMAT_R8G8B8A8_UNORM,
            .extent = .{
                .width = viewport.physical_width,
                .height = viewport.physical_height,
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = c.VK_SAMPLE_COUNT_1_BIT,
            .tiling = c.VK_IMAGE_TILING_OPTIMAL,
            .usage = c.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | c.VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = null,
            .initialLayout = c.VK_IMAGE_LAYOUT_UNDEFINED,
        };
        try util.expectVk(c.vkCreateImage(context.device, &image_info, null, &self.image));

        var requirements: c.VkMemoryRequirements = undefined;
        c.vkGetImageMemoryRequirements(context.device, self.image, &requirements);
        const memory_type_index = try findMemoryType(
            context.physical_device,
            requirements.memoryTypeBits,
            c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        );
        const allocate_info = c.VkMemoryAllocateInfo{
            .sType = c.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = null,
            .allocationSize = requirements.size,
            .memoryTypeIndex = memory_type_index,
        };
        try util.expectVk(c.vkAllocateMemory(context.device, &allocate_info, null, &self.memory));
        try util.expectVk(c.vkBindImageMemory(context.device, self.image, self.memory, 0));

        const view_info = c.VkImageViewCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .image = self.image,
            .viewType = c.VK_IMAGE_VIEW_TYPE_2D,
            .format = c.VK_FORMAT_R8G8B8A8_UNORM,
            .components = .{
                .r = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = c.VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = .{
                .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        try util.expectVk(c.vkCreateImageView(context.device, &view_info, null, &self.view));
        return self;
    }

    fn deinit(self: *BorrowedImage, device: c.VkDevice) void {
        if (self.view != null) c.vkDestroyImageView(device, self.view, null);
        if (self.image != null) c.vkDestroyImage(device, self.image, null);
        if (self.memory != null) c.vkFreeMemory(device, self.memory, null);
        self.* = .{ .image = null, .memory = null, .view = null };
    }
};

const VulkanBorrowedTextureBackend = struct {
    compositor: VulkanTextureCompositor,
    session: render_target.Session,
    borrowed_image: BorrowedImage,

    fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
    ) !VulkanBorrowedTextureBackend {
        var compositor = try VulkanTextureCompositor.init(allocator, window, viewport);
        errdefer compositor.deinit();
        var self = VulkanBorrowedTextureBackend{
            .borrowed_image = try BorrowedImage.init(&compositor.context, viewport),
            .session = .none,
            .compositor = compositor,
        };
        errdefer self.deinit();
        return self;
    }

    /// Attaches the render session on this thread, which becomes its owner
    /// thread for the session's whole life.
    fn attach(self: *VulkanBorrowedTextureBackend, map: *maplibre.MapHandle, viewport: types.Viewport) !void {
        self.session = try self.attachRenderTarget(map, viewport);
    }

    fn deinit(self: *VulkanBorrowedTextureBackend) void {
        self.compositor.waitIdle();
        self.session.deinit();
        self.borrowed_image.deinit(self.compositor.context.device);
        self.compositor.deinit();
    }

    /// Follows a resized window: allocates an image at the new size and hands
    /// it to the live session, which stays attached.
    fn resize(self: *VulkanBorrowedTextureBackend, viewport: types.Viewport) !void {
        const session = try self.session.textureHandle();
        self.compositor.waitIdle();
        self.compositor.resize(viewport);

        var replacement = try BorrowedImage.init(&self.compositor.context, viewport);
        errdefer replacement.deinit(self.compositor.context.device);
        var operation = session.setVulkanBorrowedTextureTargetStart(.{
            .extent = render_target.extent(viewport),
            .physical_width = viewport.physical_width,
            .physical_height = viewport.physical_height,
            .context = vulkanContextDescriptor(&self.compositor.context),
            .image = maplibre.NativePointer.fromPtr(@ptrCast(replacement.image.?)),
            .image_view = maplibre.NativePointer.fromPtr(@ptrCast(replacement.view.?)),
            .format = c.VK_FORMAT_R8G8B8A8_UNORM,
            .initial_layout = c.VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = c.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        }) catch |err| {
            diagnostics.logError("Vulkan borrowed texture set target failed", err, null);
            return types.AppError.TextureResizeFailed;
        };
        defer operation.release();
        render_target.serviceUntilComplete(session, operation) catch |err| {
            diagnostics.logError("Vulkan borrowed texture replacement failed", err, null);
            return types.AppError.TextureResizeFailed;
        };
        // Released only once the session has taken the replacement.
        self.borrowed_image.deinit(self.compositor.context.device);
        self.borrowed_image = replacement;
    }

    fn finishFrame(self: *VulkanBorrowedTextureBackend) !void {
        try self.compositor.waitForFrame();
    }

    fn attachRenderTarget(
        self: *VulkanBorrowedTextureBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const texture = maplibre.attachVulkanBorrowedTexture(map, .{
            .extent = render_target.extent(viewport),
            .physical_width = viewport.physical_width,
            .physical_height = viewport.physical_height,
            .context = vulkanContextDescriptor(&self.compositor.context),
            .image = maplibre.NativePointer.fromPtr(@ptrCast(self.borrowed_image.image.?)),
            .image_view = maplibre.NativePointer.fromPtr(@ptrCast(self.borrowed_image.view.?)),
            .format = c.VK_FORMAT_R8G8B8A8_UNORM,
            .initial_layout = c.VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = c.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        }, .{ .driver = .caller_graphics_thread }) catch |err| {
            diagnostics.logError("Vulkan borrowed texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return render_target.textureSession(texture);
    }

    fn renderUpdate(
        self: *VulkanBorrowedTextureBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        _ = viewport;
        if (!try self.session.renderUpdate(diagnostic_store)) return false;
        return try self.compositor.presentImageView(self.borrowed_image.view);
    }
};

const VulkanSurfaceBackend = struct {
    context: Context,
    session: render_target.Session,

    fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        _: types.Viewport,
    ) !VulkanSurfaceBackend {
        var self = VulkanSurfaceBackend{
            .context = try Context.init(allocator, window),
            .session = .none,
        };
        errdefer self.deinit();
        return self;
    }

    /// Attaches the render session on this thread, which becomes its owner
    /// thread for the session's whole life.
    fn attach(self: *VulkanSurfaceBackend, map: *maplibre.MapHandle, viewport: types.Viewport) !void {
        self.session = try self.attachRenderTarget(map, viewport);
    }

    fn deinit(self: *VulkanSurfaceBackend) void {
        self.context.waitIdle();
        self.session.deinit();
        self.context.deinit();
    }

    fn resize(self: *VulkanSurfaceBackend, viewport: types.Viewport) !void {
        try self.session.resize(viewport, null);
    }

    fn finishFrame(_: *VulkanSurfaceBackend) !void {}

    fn renderUpdate(
        self: *VulkanSurfaceBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
    ) !bool {
        return try self.session.renderUpdate(diagnostic_store);
    }

    fn attachRenderTarget(
        self: *VulkanSurfaceBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const surface = maplibre.attachVulkanSurface(map, .{
            .extent = render_target.extent(viewport),
            .context = vulkanContextDescriptor(&self.context),
            .surface = maplibre.NativePointer.fromPtr(@ptrCast(self.context.surface.?)),
        }, .{ .driver = .caller_graphics_thread }) catch |err| {
            diagnostics.logError("Vulkan surface attach failed", err, null);
            return types.AppError.SurfaceAttachFailed;
        };
        return render_target.surfaceSession(surface);
    }
};

fn vulkanContextDescriptor(context: *const Context) maplibre.VulkanContextDescriptor {
    return .{
        .instance = maplibre.NativePointer.fromPtr(@ptrCast(context.instance.?)),
        .physical_device = maplibre.NativePointer.fromPtr(@ptrCast(context.physical_device.?)),
        .device = maplibre.NativePointer.fromPtr(@ptrCast(context.device.?)),
        .graphics_queue = maplibre.NativePointer.fromPtr(@ptrCast(context.queue.?)),
        .graphics_queue_family_index = context.queue_family_index,
        .get_instance_proc_addr = nativeFunctionPointer(c.vkGetInstanceProcAddr),
        .get_device_proc_addr = nativeFunctionPointer(c.vkGetDeviceProcAddr),
    };
}

fn nativeFunctionPointer(comptime function: anytype) maplibre.NativePointer {
    return maplibre.NativePointer.fromPtr(@ptrFromInt(@intFromPtr(&function)));
}

fn findMemoryType(
    physical_device: c.VkPhysicalDevice,
    type_bits: u32,
    properties: c.VkMemoryPropertyFlags,
) !u32 {
    var memory_properties: c.VkPhysicalDeviceMemoryProperties = undefined;
    c.vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    for (0..memory_properties.memoryTypeCount) |index| {
        const bit = @as(u32, 1) << @intCast(index);
        if ((type_bits & bit) == 0) continue;
        const memory_type = memory_properties.memoryTypes[index];
        if ((memory_type.propertyFlags & properties) == properties) {
            return @intCast(index);
        }
    }
    return types.AppError.BackendSetupFailed;
}
