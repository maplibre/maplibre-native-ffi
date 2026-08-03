// The compositor's graphics pipeline: the render pass, the sampled-image
// descriptor, and the fullscreen-triangle pipeline itself.

#ifndef C_MAP_RENDER_VULKAN_PIPELINE_H
#define C_MAP_RENDER_VULKAN_PIPELINE_H

#include <vulkan/vulkan.h>

#include "../../types.h"

typedef struct vulkan_pipeline {
  VkRenderPass render_pass;
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline handle;
  VkSampler sampler;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;
  VkImageView descriptor_image_view;
} vulkan_pipeline;

[[nodiscard]] app_error vulkan_pipeline_init(
  vulkan_pipeline* pipeline, VkDevice device, VkFormat swapchain_format
);
void vulkan_pipeline_deinit(vulkan_pipeline* pipeline, VkDevice device);

void vulkan_pipeline_update_descriptor(
  vulkan_pipeline* pipeline, VkDevice device, VkImageView image_view
);

#endif  // C_MAP_RENDER_VULKAN_PIPELINE_H
