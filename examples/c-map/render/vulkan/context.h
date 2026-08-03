// The Vulkan graphics context: instance, window surface, device, and the one
// graphics queue both the compositor and the render session share.

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
  VkQueue queue;
  uint32_t queue_family_index;
} vulkan_context;

[[nodiscard]] app_error vulkan_context_init(
  vulkan_context* context, SDL_Window* window
);
void vulkan_context_deinit(vulkan_context* context);
void vulkan_context_wait_idle(vulkan_context* context);

#endif  // C_MAP_RENDER_VULKAN_CONTEXT_H
