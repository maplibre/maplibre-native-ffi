// The Vulkan graphics context: instance, window surface, device, and the
// graphics queue that the compositor and the render session share.

#ifndef C_MAP_RENDER_VULKAN_CONTEXT_H
#define C_MAP_RENDER_VULKAN_CONTEXT_H

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>
#include <vulkan/vulkan.h>

#include "../../types.h"

typedef struct vulkan_context {
  VkInstance instance;
  VkSurfaceKHR surface;
  VkPhysicalDevice physical_device;
  VkDevice device;
  /// The one queue that the host and the render session both submit to.
  VkQueue queue;
  /// Held around every call on queue. A core worker submits from its own
  /// thread, and Vulkan requires the calls on one queue to be externally
  /// synchronized, so the session takes it too, through its queue lock.
  SDL_Mutex* queue_mutex;
  uint32_t queue_family_index;
} vulkan_context;

[[nodiscard]] app_error vulkan_context_init(
  vulkan_context* context, SDL_Window* window
);
/// Call only once the session that took the queue lock has been destroyed.
void vulkan_context_deinit(vulkan_context* context);
/// Waits for the device. Call only once the session submits nothing more.
void vulkan_context_wait_idle(vulkan_context* context);
/// The session's lock on queue, which takes queue_mutex.
mln_queue_lock vulkan_context_queue_lock(const vulkan_context* context);

#endif  // C_MAP_RENDER_VULKAN_CONTEXT_H
