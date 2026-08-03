#include "commands.h"

#include "util.h"

app_error vulkan_commands_init(
  vulkan_commands* commands, VkDevice device, uint32_t queue_family_index
) {
  *commands = (vulkan_commands){};

  const VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queue_family_index,
  };
  app_error error = expect_vk(
    vkCreateCommandPool(device, &pool_info, nullptr, &commands->command_pool)
  );
  if (error == APP_OK) {
    const VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commands->command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    error = expect_vk(
      vkAllocateCommandBuffers(device, &alloc_info, &commands->command_buffer)
    );
  }
  const VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  if (error == APP_OK) {
    error = expect_vk(vkCreateSemaphore(
      device, &semaphore_info, nullptr, &commands->image_available
    ));
  }
  if (error == APP_OK) {
    error = expect_vk(vkCreateSemaphore(
      device, &semaphore_info, nullptr, &commands->render_finished
    ));
  }
  if (error == APP_OK) {
    const VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    error = expect_vk(
      vkCreateFence(device, &fence_info, nullptr, &commands->in_flight)
    );
  }
  if (error != APP_OK) {
    vulkan_commands_deinit(commands, device);
  }
  return error;
}

void vulkan_commands_deinit(vulkan_commands* commands, VkDevice device) {
  if (commands->in_flight != VK_NULL_HANDLE) {
    vkDestroyFence(device, commands->in_flight, nullptr);
  }
  if (commands->render_finished != VK_NULL_HANDLE) {
    vkDestroySemaphore(device, commands->render_finished, nullptr);
  }
  if (commands->image_available != VK_NULL_HANDLE) {
    vkDestroySemaphore(device, commands->image_available, nullptr);
  }
  if (commands->command_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, commands->command_pool, nullptr);
  }
  *commands = (vulkan_commands){};
}

app_error vulkan_commands_wait_for_frame_fence(
  vulkan_commands* commands, VkDevice device
) {
  return expect_vk(
    vkWaitForFences(device, 1, &commands->in_flight, VK_TRUE, UINT64_MAX)
  );
}

app_error vulkan_commands_reset_fence(
  vulkan_commands* commands, VkDevice device
) {
  return expect_vk(vkResetFences(device, 1, &commands->in_flight));
}

app_error vulkan_commands_record(
  vulkan_commands* commands, const vulkan_swapchain* swapchain,
  const vulkan_pipeline* pipeline, uint32_t image_index
) {
  MAP_TRY(expect_vk(vkResetCommandBuffer(commands->command_buffer, 0)));
  const VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  MAP_TRY(
    expect_vk(vkBeginCommandBuffer(commands->command_buffer, &begin_info))
  );
  const VkClearValue clear = {
    .color = {.float32 = {0.08f, 0.09f, 0.11f, 1.0f}},
  };
  const VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = pipeline->render_pass,
    .framebuffer = swapchain->framebuffers[image_index],
    .renderArea = {.offset = {.x = 0, .y = 0}, .extent = swapchain->extent},
    .clearValueCount = 1,
    .pClearValues = &clear,
  };
  vkCmdBeginRenderPass(
    commands->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE
  );
  const VkViewport render_viewport = {
    .x = 0,
    .y = 0,
    .width = (float)swapchain->extent.width,
    .height = (float)swapchain->extent.height,
    .minDepth = 0,
    .maxDepth = 1,
  };
  const VkRect2D scissor = {
    .offset = {.x = 0, .y = 0},
    .extent = swapchain->extent,
  };
  vkCmdBindPipeline(
    commands->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle
  );
  vkCmdSetViewport(commands->command_buffer, 0, 1, &render_viewport);
  vkCmdSetScissor(commands->command_buffer, 0, 1, &scissor);
  vkCmdBindDescriptorSets(
    commands->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline->pipeline_layout, 0, 1, &pipeline->descriptor_set, 0, nullptr
  );
  vkCmdDraw(commands->command_buffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commands->command_buffer);
  return expect_vk(vkEndCommandBuffer(commands->command_buffer));
}

app_error vulkan_commands_submit(vulkan_commands* commands, VkQueue queue) {
  const VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };
  const VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &commands->image_available,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = &commands->command_buffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &commands->render_finished,
  };
  return expect_vk(vkQueueSubmit(queue, 1, &submit_info, commands->in_flight));
}
