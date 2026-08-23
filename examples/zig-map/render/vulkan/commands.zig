const std = @import("std");

const c = @import("../../c.zig").c;
const Pipeline = @import("pipeline.zig").Pipeline;
const Swapchain = @import("swapchain.zig").Swapchain;
const util = @import("util.zig");

pub const Commands = struct {
    allocator: std.mem.Allocator,
    command_pool: c.VkCommandPool,
    command_buffer: c.VkCommandBuffer,
    image_available: c.VkSemaphore,
    /// One present-wait semaphore per swapchain image, indexed by the acquired
    /// image. Only the next acquire of the same image proves its present
    /// consumed the semaphore, so a single reused semaphore races.
    render_finished: []c.VkSemaphore,
    in_flight: c.VkFence,

    pub fn init(
        allocator: std.mem.Allocator,
        device: c.VkDevice,
        queue_family_index: u32,
    ) !Commands {
        var self = Commands{
            .allocator = allocator,
            .command_pool = util.nullHandle(c.VkCommandPool),
            .command_buffer = null,
            .image_available = util.nullHandle(c.VkSemaphore),
            .render_finished = &.{},
            .in_flight = util.nullHandle(c.VkFence),
        };
        errdefer self.deinit(device);

        const pool_info = c.VkCommandPoolCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = null,
            .flags = c.VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queue_family_index,
        };
        try util.expectVk(c.vkCreateCommandPool(
            device,
            &pool_info,
            null,
            &self.command_pool,
        ));
        const alloc_info = c.VkCommandBufferAllocateInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = null,
            .commandPool = self.command_pool,
            .level = c.VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        try util.expectVk(c.vkAllocateCommandBuffers(
            device,
            &alloc_info,
            &self.command_buffer,
        ));
        const semaphore_info = c.VkSemaphoreCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
        };
        try util.expectVk(c.vkCreateSemaphore(
            device,
            &semaphore_info,
            null,
            &self.image_available,
        ));
        const fence_info = c.VkFenceCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = null,
            .flags = c.VK_FENCE_CREATE_SIGNALED_BIT,
        };
        try util.expectVk(c.vkCreateFence(device, &fence_info, null, &self.in_flight));
        return self;
    }

    pub fn deinit(self: *Commands, device: c.VkDevice) void {
        self.destroyPresentSemaphores(device);
        if (!util.isNullHandle(self.in_flight)) c.vkDestroyFence(device, self.in_flight, null);
        if (!util.isNullHandle(self.image_available)) {
            c.vkDestroySemaphore(device, self.image_available, null);
        }
        if (!util.isNullHandle(self.command_pool)) {
            c.vkDestroyCommandPool(device, self.command_pool, null);
        }
    }

    /// Sizes the present-wait semaphores to the swapchain, on creation and on
    /// every replacement. The caller waits the device idle first.
    pub fn createPresentSemaphores(
        self: *Commands,
        device: c.VkDevice,
        image_count: u32,
    ) !void {
        self.render_finished = try self.allocator.alloc(c.VkSemaphore, image_count);
        @memset(self.render_finished, util.nullHandle(c.VkSemaphore));
        const semaphore_info = c.VkSemaphoreCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
        };
        for (self.render_finished) |*semaphore| {
            try util.expectVk(c.vkCreateSemaphore(
                device,
                &semaphore_info,
                null,
                semaphore,
            ));
        }
    }

    pub fn destroyPresentSemaphores(self: *Commands, device: c.VkDevice) void {
        for (self.render_finished) |semaphore| {
            if (!util.isNullHandle(semaphore)) c.vkDestroySemaphore(device, semaphore, null);
        }
        self.allocator.free(self.render_finished);
        self.render_finished = &.{};
    }

    pub fn waitForFrameFence(self: *Commands, device: c.VkDevice) !void {
        try util.expectVk(c.vkWaitForFences(
            device,
            1,
            &self.in_flight,
            c.VK_TRUE,
            std.math.maxInt(u64),
        ));
    }

    pub fn resetFence(self: *Commands, device: c.VkDevice) !void {
        try util.expectVk(c.vkResetFences(device, 1, &self.in_flight));
    }

    /// Submits the recorded commands, signaling the acquired image's
    /// present-wait semaphore.
    pub fn submit(self: *Commands, queue: c.VkQueue, image_index: u32) !void {
        const wait_stages = [_]c.VkPipelineStageFlags{
            c.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
        const submit_info = c.VkSubmitInfo{
            .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = null,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &self.image_available,
            .pWaitDstStageMask = &wait_stages,
            .commandBufferCount = 1,
            .pCommandBuffers = &self.command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &self.render_finished[image_index],
        };
        try util.expectVk(c.vkQueueSubmit(queue, 1, &submit_info, self.in_flight));
    }

    pub fn record(
        self: *Commands,
        device: c.VkDevice,
        swapchain: *const Swapchain,
        pipeline: *const Pipeline,
        image_index: u32,
    ) !void {
        try util.expectVk(c.vkResetCommandBuffer(self.command_buffer, 0));
        const begin_info = c.VkCommandBufferBeginInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = null,
            .flags = 0,
            .pInheritanceInfo = null,
        };
        try util.expectVk(c.vkBeginCommandBuffer(self.command_buffer, &begin_info));
        const clear = c.VkClearValue{
            .color = .{ .float32 = .{ 0.08, 0.09, 0.11, 1.0 } },
        };
        const render_pass_info = c.VkRenderPassBeginInfo{
            .sType = c.VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = null,
            .renderPass = pipeline.render_pass,
            .framebuffer = swapchain.framebuffers[image_index],
            .renderArea = .{
                .offset = .{ .x = 0, .y = 0 },
                .extent = swapchain.extent,
            },
            .clearValueCount = 1,
            .pClearValues = &clear,
        };
        c.vkCmdBeginRenderPass(
            self.command_buffer,
            &render_pass_info,
            c.VK_SUBPASS_CONTENTS_INLINE,
        );
        const viewport = c.VkViewport{
            .x = 0,
            .y = 0,
            .width = @floatFromInt(swapchain.extent.width),
            .height = @floatFromInt(swapchain.extent.height),
            .minDepth = 0,
            .maxDepth = 1,
        };
        const scissor = c.VkRect2D{
            .offset = .{ .x = 0, .y = 0 },
            .extent = swapchain.extent,
        };
        c.vkCmdBindPipeline(
            self.command_buffer,
            c.VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.handle,
        );
        c.vkCmdSetViewport(self.command_buffer, 0, 1, &viewport);
        c.vkCmdSetScissor(self.command_buffer, 0, 1, &scissor);
        c.vkCmdBindDescriptorSets(
            self.command_buffer,
            c.VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.pipeline_layout,
            0,
            1,
            &pipeline.descriptor_set,
            0,
            null,
        );
        c.vkCmdDraw(self.command_buffer, 3, 1, 0, 0);
        c.vkCmdEndRenderPass(self.command_buffer);
        try util.expectVk(c.vkEndCommandBuffer(self.command_buffer));
        _ = device;
    }
};
