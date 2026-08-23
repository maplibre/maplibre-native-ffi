// Small expectation helpers shared by the Vulkan backend files.

#ifndef C_MAP_RENDER_VULKAN_UTIL_H
#define C_MAP_RENDER_VULKAN_UTIL_H

#include <vulkan/vulkan.h>

#include "../../types.h"
#include "../../util.h"

[[nodiscard]] app_error expect_vk_named(VkResult result, const char* what);
[[nodiscard]] app_error expect_vk_or_suboptimal_named(
  VkResult result, const char* what
);
#define expect_vk(expr) expect_vk_named((expr), #expr)
#define expect_vk_or_suboptimal(expr) \
  expect_vk_or_suboptimal_named((expr), #expr)
[[nodiscard]] app_error expect_sdl(bool ok);

static inline uint64_t vulkan_surface_to_abi(VkSurfaceKHR handle) {
#if VK_USE_64_BIT_PTR_DEFINES
  return (uint64_t)(uintptr_t)handle;
#else
  return (uint64_t)handle;
#endif
}

static inline uint64_t vulkan_image_to_abi(VkImage handle) {
#if VK_USE_64_BIT_PTR_DEFINES
  return (uint64_t)(uintptr_t)handle;
#else
  return (uint64_t)handle;
#endif
}

static inline uint64_t vulkan_image_view_to_abi(VkImageView handle) {
#if VK_USE_64_BIT_PTR_DEFINES
  return (uint64_t)(uintptr_t)handle;
#else
  return (uint64_t)handle;
#endif
}

static inline VkImageView vulkan_image_view_from_abi(uint64_t handle) {
#if VK_USE_64_BIT_PTR_DEFINES
  return (VkImageView)(uintptr_t)handle;
#else
  return (VkImageView)handle;
#endif
}

/// Creates a 2D color image view with identity swizzle over one mip and one
/// layer, the shape every image in this example uses.
[[nodiscard]] app_error vulkan_create_image_view(
  VkDevice device, VkImage image, VkFormat format, VkImageView* out_view
);

#endif  // C_MAP_RENDER_VULKAN_UTIL_H
