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

pub const VulkanRenderTarget = union(types.RenderTargetMode) {
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
            .owned_texture => .{ .owned_texture = .{ .compositor = try VulkanTextureCompositor.init(allocator, window, viewport) } },
            .borrowed_texture => .{ .borrowed_texture = try VulkanBorrowedTextureBackend.init(allocator, window, viewport) },
            .native_surface => .{ .native_surface = .{ .context = try Context.init(allocator, window) } },
        };
    }

    /// Attaches the render session. Its core worker submits to the host's
    /// queue, so it takes the host's queue lock around each call on it.
    pub fn attach(self: *VulkanRenderTarget, map: *maplibre.Map, viewport: types.Viewport) !void {
        var options = render_target.attachOptions(self.*, .core_worker);
        options.queue_lock = switch (self.*) {
            .native_surface => |*backend| backend.context.queueLock(),
            inline else => |*backend| backend.compositor.context.queueLock(),
        };
        switch (self.*) {
            inline else => |*backend| try backend.attach(map, viewport, options),
        }
    }

    /// Releases any held frame, then detaches the session.
    pub fn detach(self: *VulkanRenderTarget) void {
        switch (self.*) {
            .owned_texture => |*backend| render_target.releaseFrame(&backend.held),
            .borrowed_texture => |*backend| render_target.releaseFrame(&backend.held),
            .native_surface => {},
        }
        self.session().deinit();
    }

    pub fn deinit(self: *VulkanRenderTarget) void {
        switch (self.*) {
            inline else => |*backend| backend.deinit(),
        }
    }

    pub fn session(self: *VulkanRenderTarget) *render_target.Session {
        return switch (self.*) {
            inline else => |*backend| &backend.session,
        };
    }

    /// Starts the session resize or target replacement a new viewport needs.
    pub fn resize(self: *VulkanRenderTarget, viewport: types.Viewport) !void {
        switch (self.*) {
            .owned_texture => |*backend| {
                backend.compositor.resize(viewport);
                // A session resizes only while the host holds none of its
                // frames.
                render_target.releaseFrame(&backend.held);
                try backend.session.resize(viewport);
            },
            .borrowed_texture => |*backend| try backend.resize(viewport),
            .native_surface => |*backend| try backend.session.resize(viewport),
        }
    }

    /// Follows completed borrowed-ring replacements.
    pub fn retireReplaced(self: *VulkanRenderTarget) !void {
        if (self.* == .borrowed_texture) try self.borrowed_texture.retireReplaced();
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window.
    pub fn present(self: *VulkanRenderTarget, viewport: types.Viewport) !bool {
        _ = viewport;
        return switch (self.*) {
            .owned_texture => |*backend| presentAcquired(&backend.session, &backend.compositor, &backend.held),
            .borrowed_texture => |*backend| presentAcquired(&backend.session, &backend.compositor, &backend.held),
            // The driver already presented the frame.
            .native_surface => true,
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

    /// Releases the compositor once the session detached, so nothing else
    /// submits.
    fn deinit(self: *VulkanTextureCompositor) void {
        self.context.waitIdle();
        self.commands.deinit(self.context.device);
        self.swapchain.deinit(self.context.device);
        self.pipeline.deinit(self.context.device);
        self.context.deinit();
    }

    /// Notes a resized window without touching the swapchain. The compositor
    /// only presents when a map frame is ready, so destroying the swapchain
    /// here would blank the window until the map renders at the new extent.
    fn resize(self: *VulkanTextureCompositor, viewport: types.Viewport) void {
        self.current_viewport = viewport;
        self.swapchain_stale = true;
    }

    fn recreateSwapchain(self: *VulkanTextureCompositor) !void {
        self.context.lockQueue();
        const idle = c.vkQueueWaitIdle(self.context.queue);
        self.context.unlockQueue();
        try util.expectVk(idle);
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

    /// Samples image_view into the next swapchain image and waits for the
    /// sampling pass, so the caller may hand the image back to the session.
    /// Reports false when the swapchain was out of date and nothing presented.
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
        {
            self.context.lockQueue();
            defer self.context.unlockQueue();
            try self.commands.submit(self.context.queue, image_index);
        }

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
        self.context.lockQueue();
        const present = c.vkQueuePresentKHR(self.context.queue, &present_info);
        self.context.unlockQueue();
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
        try self.waitForFrame();
        return true;
    }
};

const VulkanOwnedTextureBackend = struct {
    compositor: VulkanTextureCompositor,
    session: render_target.Session = .{},
    /// The newest frame, held until a newer one replaces it.
    held: ?maplibre.AcquiredFrame = null,

    /// The compositor waited for its reads of each frame, so the session may
    /// detach, and once it has, nothing else submits.
    fn deinit(self: *VulkanOwnedTextureBackend) void {
        render_target.releaseFrame(&self.held);
        self.session.deinit();
        self.compositor.deinit();
    }

    fn attach(self: *VulkanOwnedTextureBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const attachment = maplibre.mapAttachVulkanOwnedTexture(std.heap.smp_allocator, map.*, .{
            .extent = render_target.extent(viewport),
            .context = vulkanContextDescriptor(&self.compositor.context),
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("Vulkan texture attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(map, attachment, options, .owned_texture);
    }
};

/// Acquires the newest frame into `held` and samples its image into the
/// window. Both texture modes hand their frames over this way.
fn presentAcquired(session: *render_target.Session, compositor: *VulkanTextureCompositor, held: *?maplibre.AcquiredFrame) !bool {
    // Without a new frame, the window keeps the one it already shows.
    if (!try session.acquireNewest(held)) return true;
    const frame = held.*.?;
    const FrameContext = struct {
        compositor: *VulkanTextureCompositor,
        frame: maplibre.AcquiredFrame,
        fn producer(context: @This(), sync: maplibre.GpuSync) anyerror!bool {
            if (sync.kind != .cpu_complete) return types.AppError.BackendDrawFailed;
            return maplibre.acquiredFrameGetVulkanTexture(bool, context.frame, context, draw, null);
        }
        fn draw(context: @This(), info: maplibre.VulkanTextureFrame) anyerror!bool {
            return context.compositor.presentImageView(vulkanHandleFromBits(c.VkImageView, info.image_view));
        }
    };
    return maplibre.acquiredFrameGetProducerSync(bool, frame, FrameContext{ .compositor = compositor, .frame = frame }, FrameContext.producer, null);
}

const BorrowedImage = struct {
    image: c.VkImage,
    memory: c.VkDeviceMemory,
    view: c.VkImageView,

    fn init(context: *const Context, viewport: types.Viewport) !BorrowedImage {
        var self = BorrowedImage{
            .image = util.nullHandle(c.VkImage),
            .memory = util.nullHandle(c.VkDeviceMemory),
            .view = util.nullHandle(c.VkImageView),
        };
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
        if (!util.isNullHandle(self.view)) c.vkDestroyImageView(device, self.view, null);
        if (!util.isNullHandle(self.image)) c.vkDestroyImage(device, self.image, null);
        if (!util.isNullHandle(self.memory)) c.vkFreeMemory(device, self.memory, null);
        self.* = .{
            .image = util.nullHandle(c.VkImage),
            .memory = util.nullHandle(c.VkDeviceMemory),
            .view = util.nullHandle(c.VkImageView),
        };
    }
};

/// The images of a borrowed ring, one per slot.
const VulkanRing = struct {
    images: [render_target.borrowed_ring_depth]BorrowedImage,

    fn init(context: *const Context, viewport: types.Viewport) !VulkanRing {
        var ring: VulkanRing = undefined;
        var created: usize = 0;
        errdefer for (ring.images[0..created]) |*image| image.deinit(context.device);
        while (created < ring.images.len) : (created += 1) {
            ring.images[created] = try BorrowedImage.init(context, viewport);
        }
        return ring;
    }

    fn deinit(self: *VulkanRing, device: c.VkDevice) void {
        for (&self.images) |*image| image.deinit(device);
    }

    fn entries(self: VulkanRing) [render_target.borrowed_ring_depth]maplibre.VulkanBorrowedTexture {
        var result: [render_target.borrowed_ring_depth]maplibre.VulkanBorrowedTexture = undefined;
        for (self.images, &result) |image, *entry| entry.* = .{
            .image = vulkanHandleToBinding(image.image),
            .image_view = vulkanHandleToBinding(image.view),
        };
        return result;
    }
};

const VulkanBorrowedTextureBackend = struct {
    compositor: VulkanTextureCompositor,
    session: render_target.Session = .{},
    /// The newest frame, held until a newer one replaces it.
    held: ?maplibre.AcquiredFrame = null,
    /// The ring the session renders into.
    ring: VulkanRing,
    replacements: render_target.Replacements(VulkanRing) = .{},

    fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
    ) !VulkanBorrowedTextureBackend {
        var compositor = try VulkanTextureCompositor.init(allocator, window, viewport);
        errdefer compositor.deinit();
        return .{
            .ring = try VulkanRing.init(&compositor.context, viewport),
            .compositor = compositor,
        };
    }

    fn deinit(self: *VulkanBorrowedTextureBackend) void {
        render_target.releaseFrame(&self.held);
        self.session.deinit();
        const device = self.compositor.context.device;
        while (self.replacements.takeAny()) |ring| {
            var retired = ring;
            retired.deinit(device);
        }
        self.replacements.deinit();
        self.ring.deinit(device);
        self.compositor.deinit();
    }

    fn attach(self: *VulkanBorrowedTextureBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const entries = self.ring.entries();
        const attachment = maplibre.mapAttachVulkanBorrowedTexture(std.heap.smp_allocator, map.*, self.descriptor(&entries, viewport), options, &diagnostic) catch |err| {
            diagnostics.logError("Vulkan borrowed texture attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(map, attachment, options, .borrowed_texture);
    }

    /// Follows a resized window: allocates a ring at the new size and hands it
    /// to the live session, which stays attached. A replacement is refused
    /// while the host holds a frame, so the held one goes first; the window
    /// keeps showing what it last presented.
    fn resize(self: *VulkanBorrowedTextureBackend, viewport: types.Viewport) !void {
        self.compositor.resize(viewport);
        render_target.releaseFrame(&self.held);
        try self.replaceRing(viewport);
        try self.session.resizeMap(viewport);
    }

    fn replaceRing(self: *VulkanBorrowedTextureBackend, viewport: types.Viewport) !void {
        var replacement = try VulkanRing.init(&self.compositor.context, viewport);
        errdefer replacement.deinit(self.compositor.context.device);
        var diagnostic: maplibre.Diagnostic = .{};
        const entries = replacement.entries();
        var completion = maplibre.renderSessionSetVulkanBorrowedTextureTarget(std.heap.smp_allocator, self.session.handle.?, self.descriptor(&entries, viewport), &diagnostic) catch |err| {
            diagnostics.logError("Vulkan borrowed texture set target failed", err, &diagnostic);
            return types.AppError.ResizeFailed;
        };
        self.replacements.push(completion, self.ring) catch |err| {
            completion.deinit();
            return err;
        };
        self.ring = replacement;
    }

    /// Destroys each ring that a completed replacement retired. The session
    /// stopped rendering into it when the replacement completed, and the
    /// compositor waited for its own reads before the held frame was
    /// released.
    fn retireReplaced(self: *VulkanBorrowedTextureBackend) !void {
        while (try self.replacements.takeCompleted(&self.session)) |ring| {
            var retired = ring;
            retired.deinit(self.compositor.context.device);
        }
    }

    fn descriptor(self: *const VulkanBorrowedTextureBackend, entries: []const maplibre.VulkanBorrowedTexture, viewport: types.Viewport) maplibre.VulkanBorrowedTextureDescriptor {
        return .{
            .extent = render_target.extent(viewport),
            .physical_width = viewport.physical_width,
            .physical_height = viewport.physical_height,
            .context = vulkanContextDescriptor(&self.compositor.context),
            .textures = entries,
            .format = c.VK_FORMAT_R8G8B8A8_UNORM,
            .initial_layout = c.VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = c.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }
};

const VulkanSurfaceBackend = struct {
    context: Context,
    session: render_target.Session = .{},

    fn deinit(self: *VulkanSurfaceBackend) void {
        self.session.deinit();
        self.context.waitIdle();
        self.context.deinit();
    }

    fn attach(self: *VulkanSurfaceBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const attachment = maplibre.mapAttachVulkanSurface(std.heap.smp_allocator, map.*, .{
            .extent = render_target.extent(viewport),
            .context = vulkanContextDescriptor(&self.context),
            .surface = vulkanHandleToBinding(self.context.surface),
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("Vulkan surface attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(map, attachment, options, .native_surface);
    }
};

fn vulkanContextDescriptor(context: *const Context) maplibre.VulkanContextDescriptor {
    return .{
        .instance = (@ptrCast(context.instance.?)),
        .physical_device = (@ptrCast(context.physical_device.?)),
        .device = (@ptrCast(context.device.?)),
        .graphics_queue = (@ptrCast(context.queue.?)),
        .graphics_queue_family_index = context.queue_family_index,
        .get_instance_proc_addr = nativeFunctionPointer(c.vkGetInstanceProcAddr),
        .get_device_proc_addr = nativeFunctionPointer(c.vkGetDeviceProcAddr),
    };
}

fn nativeFunctionPointer(comptime function: anytype) ?*anyopaque {
    return (@ptrFromInt(@intFromPtr(&function)));
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

fn vulkanHandleFromBits(comptime Handle: type, bits: u64) Handle {
    return switch (@typeInfo(Handle)) {
        .optional, .pointer => @ptrFromInt(@as(usize, @intCast(bits))),
        .int => @intCast(bits),
        .@"enum" => @enumFromInt(bits),
        else => @compileError("unsupported Vulkan handle representation"),
    };
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
