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
  /// One present-wait semaphore per swapchain image, indexed by the acquired
  /// image. The frame fence proves a submit finished, but only the next
  /// acquire of the same image proves its present consumed the semaphore, so
  /// a single reused semaphore is a race the presentation engine may lose.
  VkSemaphore* render_finished;
  uint32_t render_finished_count;
  VkFence in_flight;
} vulkan_commands;

[[nodiscard]] app_error vulkan_commands_init(
  vulkan_commands* commands, VkDevice device, uint32_t queue_family_index
);
void vulkan_commands_deinit(vulkan_commands* commands, VkDevice device);

/// Sizes the present-wait semaphores to the swapchain, on creation and on
/// every replacement. The caller waits the device idle first.
[[nodiscard]] app_error vulkan_commands_create_present_semaphores(
  vulkan_commands* commands, VkDevice device, uint32_t image_count
);
void vulkan_commands_destroy_present_semaphores(
  vulkan_commands* commands, VkDevice device
);

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
/// Submits the recorded commands, signaling the acquired image's
/// present-wait semaphore.
[[nodiscard]] app_error vulkan_commands_submit(
  vulkan_commands* commands, VkQueue queue, uint32_t image_index
);

#endif  // C_MAP_RENDER_VULKAN_COMMANDS_H
