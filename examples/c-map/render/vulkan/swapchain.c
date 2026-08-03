#include <stdlib.h>

#include "swapchain.h"

#include "util.h"

static VkSurfaceFormatKHR choose_surface_format(
  const VkSurfaceFormatKHR* formats, uint32_t count
) {
  for (uint32_t i = 0; i < count; i += 1) {
    const bool supported_format =
      formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
      formats[i].format == VK_FORMAT_R8G8B8A8_UNORM;
    const bool supported_color_space =
      formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    if (supported_format && supported_color_space) {
      return formats[i];
    }
  }
  return formats[0];
}

static uint32_t clamp_u32(uint32_t value, uint32_t min, uint32_t max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

static VkExtent2D choose_extent(
  const VkSurfaceCapabilitiesKHR* capabilities, viewport current_viewport
) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  }
  return (VkExtent2D){
    .width = clamp_u32(
      current_viewport.physical_width, capabilities->minImageExtent.width,
      capabilities->maxImageExtent.width
    ),
    .height = clamp_u32(
      current_viewport.physical_height, capabilities->minImageExtent.height,
      capabilities->maxImageExtent.height
    ),
  };
}

static app_error swapchain_create(
  vulkan_swapchain* swapchain, const vulkan_context* context,
  viewport current_viewport, VkSwapchainKHR old_swapchain
) {
  VkSurfaceCapabilitiesKHR capabilities;
  MAP_TRY(expect_vk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    context->physical_device, context->surface, &capabilities
  )));
  uint32_t format_count = 0;
  MAP_TRY(expect_vk(vkGetPhysicalDeviceSurfaceFormatsKHR(
    context->physical_device, context->surface, &format_count, nullptr
  )));
  VkSurfaceFormatKHR* formats =
    calloc(format_count, sizeof(VkSurfaceFormatKHR));
  if (formats == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  app_error error = expect_vk(vkGetPhysicalDeviceSurfaceFormatsKHR(
    context->physical_device, context->surface, &format_count, formats
  ));
  if (error != APP_OK) {
    free(formats);
    return error;
  }
  const VkSurfaceFormatKHR format =
    choose_surface_format(formats, format_count);
  free(formats);

  swapchain->format = format.format;
  swapchain->extent = choose_extent(&capabilities, current_viewport);

  uint32_t image_count = capabilities.minImageCount + 1;
  if (
    capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount
  ) {
    image_count = capabilities.maxImageCount;
  }

  const VkSwapchainCreateInfoKHR create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = context->surface,
    .minImageCount = image_count,
    .imageFormat = swapchain->format,
    .imageColorSpace = format.colorSpace,
    .imageExtent = swapchain->extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform = capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .clipped = VK_TRUE,
    .oldSwapchain = old_swapchain,
  };
  MAP_TRY(expect_vk(vkCreateSwapchainKHR(
    context->device, &create_info, nullptr, &swapchain->handle
  )));

  uint32_t actual_count = 0;
  MAP_TRY(expect_vk(vkGetSwapchainImagesKHR(
    context->device, swapchain->handle, &actual_count, nullptr
  )));
  swapchain->images = calloc(actual_count, sizeof(VkImage));
  swapchain->views = calloc(actual_count, sizeof(VkImageView));
  if (swapchain->images == nullptr || swapchain->views == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  MAP_TRY(expect_vk(vkGetSwapchainImagesKHR(
    context->device, swapchain->handle, &actual_count, swapchain->images
  )));
  swapchain->image_count = actual_count;
  for (uint32_t i = 0; i < actual_count; i += 1) {
    MAP_TRY(vulkan_create_image_view(
      context->device, swapchain->images[i], swapchain->format,
      &swapchain->views[i]
    ));
  }
  return APP_OK;
}

app_error vulkan_swapchain_init(
  vulkan_swapchain* swapchain, const vulkan_context* context,
  viewport current_viewport, VkSwapchainKHR old_swapchain
) {
  *swapchain = (vulkan_swapchain){.format = VK_FORMAT_UNDEFINED};
  const app_error error =
    swapchain_create(swapchain, context, current_viewport, old_swapchain);
  if (error != APP_OK) {
    vulkan_swapchain_deinit(swapchain, context->device);
  }
  return error;
}

void vulkan_swapchain_deinit(vulkan_swapchain* swapchain, VkDevice device) {
  if (swapchain->framebuffers != nullptr) {
    for (uint32_t i = 0; i < swapchain->image_count; i += 1) {
      if (swapchain->framebuffers[i] != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, swapchain->framebuffers[i], nullptr);
      }
    }
    free(swapchain->framebuffers);
  }
  if (swapchain->views != nullptr) {
    for (uint32_t i = 0; i < swapchain->image_count; i += 1) {
      if (swapchain->views[i] != VK_NULL_HANDLE) {
        vkDestroyImageView(device, swapchain->views[i], nullptr);
      }
    }
    free(swapchain->views);
  }
  free(swapchain->images);
  if (swapchain->handle != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, swapchain->handle, nullptr);
  }
  *swapchain = (vulkan_swapchain){.format = VK_FORMAT_UNDEFINED};
}

app_error vulkan_swapchain_create_framebuffers(
  vulkan_swapchain* swapchain, VkDevice device, VkRenderPass render_pass
) {
  swapchain->framebuffers =
    calloc(swapchain->image_count, sizeof(VkFramebuffer));
  if (swapchain->framebuffers == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  for (uint32_t i = 0; i < swapchain->image_count; i += 1) {
    const VkFramebufferCreateInfo framebuffer_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = render_pass,
      .attachmentCount = 1,
      .pAttachments = &swapchain->views[i],
      .width = swapchain->extent.width,
      .height = swapchain->extent.height,
      .layers = 1,
    };
    MAP_TRY(expect_vk(vkCreateFramebuffer(
      device, &framebuffer_info, nullptr, &swapchain->framebuffers[i]
    )));
  }
  return APP_OK;
}
