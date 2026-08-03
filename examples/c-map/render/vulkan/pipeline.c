#include "pipeline.h"

#include "fullscreen_vert.h"
#include "sample_frag.h"
#include "util.h"

static app_error create_shader(
  VkDevice device, const uint32_t* code, size_t code_size,
  VkShaderModule* out_shader
) {
  const VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code_size,
    .pCode = code,
  };
  return expect_vk(
    vkCreateShaderModule(device, &create_info, nullptr, out_shader)
  );
}

static VkPipelineShaderStageCreateInfo shader_stage(
  VkShaderStageFlagBits stage, VkShaderModule module
) {
  return (VkPipelineShaderStageCreateInfo){
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = stage,
    .module = module,
    .pName = "main",
  };
}

static app_error create_render_pass(
  vulkan_pipeline* pipeline, VkDevice device, VkFormat swapchain_format
) {
  const VkAttachmentDescription attachment = {
    .format = swapchain_format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };
  const VkAttachmentReference color_ref = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  const VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_ref,
  };
  const VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };
  const VkRenderPassCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &attachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency,
  };
  return expect_vk(
    vkCreateRenderPass(device, &create_info, nullptr, &pipeline->render_pass)
  );
}

static app_error create_descriptor_state(
  vulkan_pipeline* pipeline, VkDevice device
) {
  const VkDescriptorSetLayoutBinding binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  const VkDescriptorSetLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &binding,
  };
  MAP_TRY(expect_vk(vkCreateDescriptorSetLayout(
    device, &layout_info, nullptr, &pipeline->descriptor_set_layout
  )));

  const VkSamplerCreateInfo sampler_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .maxAnisotropy = 1,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
  };
  MAP_TRY(expect_vk(
    vkCreateSampler(device, &sampler_info, nullptr, &pipeline->sampler)
  ));

  const VkDescriptorPoolSize pool_size = {
    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
  };
  const VkDescriptorPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = 1,
    .poolSizeCount = 1,
    .pPoolSizes = &pool_size,
  };
  MAP_TRY(expect_vk(vkCreateDescriptorPool(
    device, &pool_info, nullptr, &pipeline->descriptor_pool
  )));
  const VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = pipeline->descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &pipeline->descriptor_set_layout,
  };
  return expect_vk(
    vkAllocateDescriptorSets(device, &alloc_info, &pipeline->descriptor_set)
  );
}

static app_error create_pipeline(vulkan_pipeline* pipeline, VkDevice device) {
  VkShaderModule vert = VK_NULL_HANDLE;
  MAP_TRY(create_shader(
    device, fullscreen_vert_spv, sizeof(fullscreen_vert_spv), &vert
  ));
  VkShaderModule frag = VK_NULL_HANDLE;
  app_error error =
    create_shader(device, sample_frag_spv, sizeof(sample_frag_spv), &frag);
  if (error != APP_OK) {
    vkDestroyShaderModule(device, vert, nullptr);
    return error;
  }

  const VkPipelineShaderStageCreateInfo stages[] = {
    shader_stage(VK_SHADER_STAGE_VERTEX_BIT, vert),
    shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, frag),
  };
  const VkPipelineVertexInputStateCreateInfo vertex_input = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };
  const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };
  const VkPipelineViewportStateCreateInfo viewport_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1,
  };
  const VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .lineWidth = 1,
  };
  const VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1,
  };
  const VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  const VkPipelineColorBlendStateCreateInfo color_blending = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment,
  };
  const VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  const VkPipelineDynamicStateCreateInfo dynamic_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 2,
    .pDynamicStates = dynamic_states,
  };
  const VkPipelineLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &pipeline->descriptor_set_layout,
  };
  error = expect_vk(vkCreatePipelineLayout(
    device, &layout_info, nullptr, &pipeline->pipeline_layout
  ));
  if (error == APP_OK) {
    const VkGraphicsPipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = stages,
      .pVertexInputState = &vertex_input,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_state,
      .layout = pipeline->pipeline_layout,
      .renderPass = pipeline->render_pass,
      .subpass = 0,
      .basePipelineIndex = -1,
    };
    error = expect_vk(vkCreateGraphicsPipelines(
      device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline->handle
    ));
  }
  vkDestroyShaderModule(device, vert, nullptr);
  vkDestroyShaderModule(device, frag, nullptr);
  return error;
}

app_error vulkan_pipeline_init(
  vulkan_pipeline* pipeline, VkDevice device, VkFormat swapchain_format
) {
  *pipeline = (vulkan_pipeline){};
  app_error error = create_render_pass(pipeline, device, swapchain_format);
  if (error == APP_OK) {
    error = create_descriptor_state(pipeline, device);
  }
  if (error == APP_OK) {
    error = create_pipeline(pipeline, device);
  }
  if (error != APP_OK) {
    vulkan_pipeline_deinit(pipeline, device);
  }
  return error;
}

void vulkan_pipeline_deinit(vulkan_pipeline* pipeline, VkDevice device) {
  if (pipeline->handle != VK_NULL_HANDLE) {
    vkDestroyPipeline(device, pipeline->handle, nullptr);
  }
  if (pipeline->pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, pipeline->pipeline_layout, nullptr);
  }
  if (pipeline->descriptor_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, pipeline->descriptor_pool, nullptr);
  }
  if (pipeline->sampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, pipeline->sampler, nullptr);
  }
  if (pipeline->descriptor_set_layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(
      device, pipeline->descriptor_set_layout, nullptr
    );
  }
  if (pipeline->render_pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device, pipeline->render_pass, nullptr);
  }
  *pipeline = (vulkan_pipeline){};
}

void vulkan_pipeline_update_descriptor(
  vulkan_pipeline* pipeline, VkDevice device, VkImageView image_view
) {
  const VkDescriptorImageInfo image_info = {
    .sampler = pipeline->sampler,
    .imageView = image_view,
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  const VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = pipeline->descriptor_set,
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &image_info,
  };
  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  pipeline->descriptor_image_view = image_view;
}
