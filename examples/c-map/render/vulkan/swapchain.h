// The compositor's presentation swapchain: images, views, and framebuffers
// for the window surface.

#ifndef C_MAP_RENDER_VULKAN_SWAPCHAIN_H
#define C_MAP_RENDER_VULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>

#include "../../types.h"
#include "context.h"

typedef struct vulkan_swapchain {
  VkSwapchainKHR handle;
  VkFormat format;
  VkExtent2D extent;
  uint32_t image_count;
  VkImage* images;
  VkImageView* views;
  VkFramebuffer* framebuffers;
} vulkan_swapchain;

/// Creates the swapchain. A replacement passes the swapchain it retires as
/// old_swapchain, so the presentation engine hands the surface over without a
/// gap; the caller destroys the retired one after this returns.
[[nodiscard]] app_error vulkan_swapchain_init(
  vulkan_swapchain* swapchain, const vulkan_context* context,
  viewport current_viewport, VkSwapchainKHR old_swapchain
);
void vulkan_swapchain_deinit(vulkan_swapchain* swapchain, VkDevice device);

[[nodiscard]] app_error vulkan_swapchain_create_framebuffers(
  vulkan_swapchain* swapchain, VkDevice device, VkRenderPass render_pass
);

#endif  // C_MAP_RENDER_VULKAN_SWAPCHAIN_H
