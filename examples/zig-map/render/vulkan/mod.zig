const std = @import("std");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const maplibre = @import("maplibre_native");
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
        map: *maplibre.MapHandle,
    ) !VulkanRenderTarget {
        return switch (mode) {
            .owned_texture => .{ .owned_texture = try VulkanOwnedTextureBackend.init(allocator, window, viewport, map) },
            .borrowed_texture => .{ .borrowed_texture = try VulkanBorrowedTextureBackend.init(allocator, window, viewport, map) },
            .native_surface => .{ .native_surface = try VulkanSurfaceBackend.init(allocator, window, viewport, map) },
        };
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

    pub fn needsReattachOnResize(self: *const VulkanRenderTarget) bool {
        return switch (self.*) {
            .owned_texture, .native_surface => false,
            .borrowed_texture => true,
        };
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

    fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        exportable_textures: bool,
    ) !VulkanTextureCompositor {
        var context = try Context.init(allocator, window, exportable_textures);
        errdefer context.deinit();

        var swapchain = try Swapchain.init(allocator, &context, viewport);
        errdefer swapchain.deinit(context.device);

        var pipeline = try Pipeline.init(allocator, context.device, swapchain.format);
        errdefer pipeline.deinit(context.device);
        try swapchain.createFramebuffers(context.device, pipeline.render_pass);

        var commands = try Commands.init(context.device, context.queue_family_index);
        errdefer commands.deinit(context.device);

        return .{
            .context = context,
            .swapchain = swapchain,
            .pipeline = pipeline,
            .commands = commands,
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

    fn resize(self: *VulkanTextureCompositor, viewport: types.Viewport) !void {
        const previous_format = self.swapchain.format;
        self.swapchain.deinit(self.context.device);
        self.swapchain = try Swapchain.init(
            self.swapchain.allocator,
            &self.context,
            viewport,
        );

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
    }

    fn waitForFrame(self: *VulkanTextureCompositor) !void {
        try self.commands.waitForFrameFence(self.context.device);
    }

    fn presentImageView(self: *VulkanTextureCompositor, image_view: c.VkImageView) !bool {
        if (image_view != self.pipeline.descriptor_image_view) {
            self.pipeline.updateDescriptor(self.context.device, image_view);
        }

        try self.waitForFrame();

        var image_index: u32 = 0;
        const acquire = c.vkAcquireNextImageKHR(
            self.context.device,
            self.swapchain.handle,
            std.math.maxInt(u64),
            self.commands.image_available,
            null,
            &image_index,
        );
        if (acquire == c.VK_ERROR_OUT_OF_DATE_KHR) return false;
        try util.expectVkOrSuboptimal(acquire);
        try self.commands.resetFence(self.context.device);

        try self.commands.record(
            self.context.device,
            &self.swapchain,
            &self.pipeline,
            image_index,
        );
        try self.commands.submit(self.context.queue);

        const present_info = c.VkPresentInfoKHR{
            .sType = c.VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = null,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &self.commands.render_finished,
            .swapchainCount = 1,
            .pSwapchains = &self.swapchain.handle,
            .pImageIndices = &image_index,
            .pResults = null,
        };
        const present = c.vkQueuePresentKHR(self.context.queue, &present_info);
        if (present != c.VK_SUCCESS and
            present != c.VK_SUBOPTIMAL_KHR and
            present != c.VK_ERROR_OUT_OF_DATE_KHR)
        {
            self.waitForFrame() catch {};
            try util.expectVk(present);
        }
        return true;
    }
};

const VulkanOwnedTextureBackend = struct {
    compositor: VulkanTextureCompositor,
    session: render_target.Session,
    pending_frame: ?maplibre.VulkanOwnedTextureFrameHandle,

    fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !VulkanOwnedTextureBackend {
        var self = VulkanOwnedTextureBackend{
            .compositor = try VulkanTextureCompositor.init(allocator, window, viewport, false),
            .session = .none,
            .pending_frame = null,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
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
        try self.compositor.resize(viewport);
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
        }) catch |err| {
            diagnostics.logError("Vulkan texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
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
        var frame = texture.acquireVulkanOwnedTextureFrame() catch |err| switch (err) {
            error.InvalidState => return false,
            else => {
                diagnostics.logError("Vulkan texture acquire failed", err, null);
                return types.AppError.BackendDrawFailed;
            },
        };
        errdefer frame.release() catch |err| diagnostics.logError("Vulkan texture release failed", err, null);

        const info = try frame.info();
        const image_view: c.VkImageView = @ptrCast(info.image_view.ptr);
        if (!try self.compositor.presentImageView(image_view)) {
            frame.release() catch |err| diagnostics.logError("Vulkan texture release failed", err, null);
            return false;
        }

        self.pending_frame = frame;
        return true;
    }

    fn releasePendingFrame(self: *VulkanOwnedTextureBackend) void {
        if (self.pending_frame) |*frame| frame.release() catch |err| diagnostics.logError("Vulkan texture release failed", err, null);
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
        map: *maplibre.MapHandle,
    ) !VulkanBorrowedTextureBackend {
        var compositor = try VulkanTextureCompositor.init(allocator, window, viewport, false);
        errdefer compositor.deinit();
        var self = VulkanBorrowedTextureBackend{
            .borrowed_image = try BorrowedImage.init(&compositor.context, viewport),
            .session = .none,
            .compositor = compositor,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
    }

    fn deinit(self: *VulkanBorrowedTextureBackend) void {
        self.compositor.waitIdle();
        self.session.deinit();
        self.borrowed_image.deinit(self.compositor.context.device);
        self.compositor.deinit();
    }

    fn resize(_: *VulkanBorrowedTextureBackend, _: types.Viewport) !void {
        return types.AppError.TextureResizeFailed;
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
            .context = vulkanContextDescriptor(&self.compositor.context),
            .image = .{ .ptr = @ptrCast(self.borrowed_image.image.?) },
            .image_view = .{ .ptr = @ptrCast(self.borrowed_image.view.?) },
            .format = c.VK_FORMAT_R8G8B8A8_UNORM,
            .initial_layout = c.VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = c.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        }) catch |err| {
            diagnostics.logError("Vulkan borrowed texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
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
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !VulkanSurfaceBackend {
        var self = VulkanSurfaceBackend{
            .context = try Context.init(allocator, window, false),
            .session = .none,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
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
            .surface = .{ .ptr = @ptrCast(self.context.surface.?) },
        }) catch |err| {
            diagnostics.logError("Vulkan surface attach failed", err, null);
            return types.AppError.SurfaceAttachFailed;
        };
        return .{ .surface = surface };
    }
};

fn vulkanContextDescriptor(context: *const Context) maplibre.VulkanContextDescriptor {
    return .{
        .instance = .{ .ptr = @ptrCast(context.instance.?) },
        .physical_device = .{ .ptr = @ptrCast(context.physical_device.?) },
        .device = .{ .ptr = @ptrCast(context.device.?) },
        .graphics_queue = .{ .ptr = @ptrCast(context.queue.?) },
        .graphics_queue_family_index = context.queue_family_index,
        .get_instance_proc_addr = nativeFunctionPointer(c.vkGetInstanceProcAddr),
        .get_device_proc_addr = nativeFunctionPointer(c.vkGetDeviceProcAddr),
    };
}

fn nativeFunctionPointer(comptime function: anytype) maplibre.NativePointer {
    return .{ .ptr = @ptrFromInt(@intFromPtr(&function)) };
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
