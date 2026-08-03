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

/// Creates a 2D color image view with identity swizzle over one mip and one
/// layer, the shape every image in this example uses.
[[nodiscard]] app_error vulkan_create_image_view(
  VkDevice device, VkImage image, VkFormat format, VkImageView* out_view
);

#endif  // C_MAP_RENDER_VULKAN_UTIL_H
