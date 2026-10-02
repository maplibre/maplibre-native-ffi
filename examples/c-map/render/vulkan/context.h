// The Vulkan graphics context: instance, window surface, device, and the
// graphics queues of the compositor and the render session.

#ifndef C_MAP_RENDER_VULKAN_CONTEXT_H
#define C_MAP_RENDER_VULKAN_CONTEXT_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include "../../types.h"

typedef struct vulkan_context {
  VkInstance instance;
  VkSurfaceKHR surface;
  VkPhysicalDevice physical_device;
  VkDevice device;
  /// The queue the host submits to.
  VkQueue queue;
  /// The queue the render session submits to. A core worker submits from its
  /// own thread, and Vulkan requires a queue's submissions to be externally
  /// synchronized, so a host that also submits gives the session a second
  /// queue from the same family when the family has one. Otherwise this is
  /// queue.
  VkQueue session_queue;
  uint32_t queue_family_index;
} vulkan_context;

/// Creates the context, with a separate session queue when
/// separate_session_queue is set and the graphics family exposes two queues.
[[nodiscard]] app_error vulkan_context_init(
  vulkan_context* context, SDL_Window* window, bool separate_session_queue
);
void vulkan_context_deinit(vulkan_context* context);
/// Waits for every queue. Call only once the session submits nothing more.
void vulkan_context_wait_idle(vulkan_context* context);

#endif  // C_MAP_RENDER_VULKAN_CONTEXT_H
