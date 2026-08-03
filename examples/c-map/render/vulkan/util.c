#include <SDL3/SDL.h>
#include <stdio.h>

#include "util.h"

app_error expect_vk_named(VkResult result, const char* what) {
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Vulkan call failed: %d in %s\n", (int)result, what);
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  return APP_OK;
}

app_error expect_vk_or_suboptimal_named(VkResult result, const char* what) {
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    fprintf(stderr, "Vulkan call failed: %d in %s\n", (int)result, what);
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  return APP_OK;
}

app_error expect_sdl(bool ok) {
  if (!ok) {
    fprintf(stderr, "SDL Vulkan call failed: %s\n", SDL_GetError());
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  return APP_OK;
}

app_error vulkan_create_image_view(
  VkDevice device, VkImage image, VkFormat format, VkImageView* out_view
) {
  const VkImageViewCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .components =
      {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
      },
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };
  return expect_vk(vkCreateImageView(device, &create_info, nullptr, out_view));
}
