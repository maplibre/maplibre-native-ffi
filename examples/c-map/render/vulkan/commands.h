// The compositor's command state: one command buffer, the frame fence, and
// the acquire/present semaphores.

#ifndef C_MAP_RENDER_VULKAN_COMMANDS_H
#define C_MAP_RENDER_VULKAN_COMMANDS_H

#include <vulkan/vulkan.h>

#include "../../types.h"
#include "pipeline.h"
#include "swapchain.h"

typedef struct vulkan_commands {
  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;
  VkSemaphore image_available;
  VkSemaphore render_finished;
  VkFence in_flight;
} vulkan_commands;

[[nodiscard]] app_error vulkan_commands_init(
  vulkan_commands* commands, VkDevice device, uint32_t queue_family_index
);
void vulkan_commands_deinit(vulkan_commands* commands, VkDevice device);

[[nodiscard]] app_error vulkan_commands_wait_for_frame_fence(
  vulkan_commands* commands, VkDevice device
);
[[nodiscard]] app_error vulkan_commands_reset_fence(
  vulkan_commands* commands, VkDevice device
);
[[nodiscard]] app_error vulkan_commands_record(
  vulkan_commands* commands, const vulkan_swapchain* swapchain,
  const vulkan_pipeline* pipeline, uint32_t image_index
);
[[nodiscard]] app_error vulkan_commands_submit(
  vulkan_commands* commands, VkQueue queue
);

#endif  // C_MAP_RENDER_VULKAN_COMMANDS_H
