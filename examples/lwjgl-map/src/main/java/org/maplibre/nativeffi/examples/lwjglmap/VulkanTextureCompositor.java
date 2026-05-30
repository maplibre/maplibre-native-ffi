package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.system.MemoryUtil.NULL;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_compilation_status_success;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_compile_into_spv;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_compile_options_initialize;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_compile_options_release;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_compiler_initialize;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_compiler_release;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_fragment_shader;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_result_get_bytes;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_result_get_compilation_status;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_result_get_error_message;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_result_release;
import static org.lwjgl.util.shaderc.Shaderc.shaderc_vertex_shader;
import static org.lwjgl.vulkan.KHRSurface.VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
import static org.lwjgl.vulkan.KHRSurface.VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
import static org.lwjgl.vulkan.KHRSurface.VK_PRESENT_MODE_FIFO_KHR;
import static org.lwjgl.vulkan.KHRSurface.vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
import static org.lwjgl.vulkan.KHRSurface.vkGetPhysicalDeviceSurfaceFormatsKHR;
import static org.lwjgl.vulkan.KHRSwapchain.VK_ERROR_OUT_OF_DATE_KHR;
import static org.lwjgl.vulkan.KHRSwapchain.VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
import static org.lwjgl.vulkan.KHRSwapchain.VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
import static org.lwjgl.vulkan.KHRSwapchain.VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
import static org.lwjgl.vulkan.KHRSwapchain.VK_SUBOPTIMAL_KHR;
import static org.lwjgl.vulkan.KHRSwapchain.vkAcquireNextImageKHR;
import static org.lwjgl.vulkan.KHRSwapchain.vkCreateSwapchainKHR;
import static org.lwjgl.vulkan.KHRSwapchain.vkDestroySwapchainKHR;
import static org.lwjgl.vulkan.KHRSwapchain.vkGetSwapchainImagesKHR;
import static org.lwjgl.vulkan.KHRSwapchain.vkQueuePresentKHR;
import static org.lwjgl.vulkan.VK10.VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
import static org.lwjgl.vulkan.VK10.VK_ATTACHMENT_LOAD_OP_CLEAR;
import static org.lwjgl.vulkan.VK10.VK_ATTACHMENT_LOAD_OP_DONT_CARE;
import static org.lwjgl.vulkan.VK10.VK_ATTACHMENT_STORE_OP_DONT_CARE;
import static org.lwjgl.vulkan.VK10.VK_ATTACHMENT_STORE_OP_STORE;
import static org.lwjgl.vulkan.VK10.VK_BORDER_COLOR_INT_OPAQUE_BLACK;
import static org.lwjgl.vulkan.VK10.VK_COLOR_COMPONENT_A_BIT;
import static org.lwjgl.vulkan.VK10.VK_COLOR_COMPONENT_B_BIT;
import static org.lwjgl.vulkan.VK10.VK_COLOR_COMPONENT_G_BIT;
import static org.lwjgl.vulkan.VK10.VK_COLOR_COMPONENT_R_BIT;
import static org.lwjgl.vulkan.VK10.VK_COMMAND_BUFFER_LEVEL_PRIMARY;
import static org.lwjgl.vulkan.VK10.VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
import static org.lwjgl.vulkan.VK10.VK_COMPARE_OP_ALWAYS;
import static org.lwjgl.vulkan.VK10.VK_CULL_MODE_NONE;
import static org.lwjgl.vulkan.VK10.VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
import static org.lwjgl.vulkan.VK10.VK_DYNAMIC_STATE_SCISSOR;
import static org.lwjgl.vulkan.VK10.VK_DYNAMIC_STATE_VIEWPORT;
import static org.lwjgl.vulkan.VK10.VK_FILTER_LINEAR;
import static org.lwjgl.vulkan.VK10.VK_FORMAT_B8G8R8A8_UNORM;
import static org.lwjgl.vulkan.VK10.VK_FORMAT_R8G8B8A8_UNORM;
import static org.lwjgl.vulkan.VK10.VK_FRONT_FACE_COUNTER_CLOCKWISE;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_ASPECT_COLOR_BIT;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_UNDEFINED;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
import static org.lwjgl.vulkan.VK10.VK_LOGIC_OP_COPY;
import static org.lwjgl.vulkan.VK10.VK_PIPELINE_BIND_POINT_GRAPHICS;
import static org.lwjgl.vulkan.VK10.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
import static org.lwjgl.vulkan.VK10.VK_POLYGON_MODE_FILL;
import static org.lwjgl.vulkan.VK10.VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
import static org.lwjgl.vulkan.VK10.VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
import static org.lwjgl.vulkan.VK10.VK_SAMPLER_MIPMAP_MODE_LINEAR;
import static org.lwjgl.vulkan.VK10.VK_SAMPLE_COUNT_1_BIT;
import static org.lwjgl.vulkan.VK10.VK_SHADER_STAGE_FRAGMENT_BIT;
import static org.lwjgl.vulkan.VK10.VK_SHADER_STAGE_VERTEX_BIT;
import static org.lwjgl.vulkan.VK10.VK_SHARING_MODE_EXCLUSIVE;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_SUBMIT_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
import static org.lwjgl.vulkan.VK10.VK_SUBPASS_CONTENTS_INLINE;
import static org.lwjgl.vulkan.VK10.VK_SUBPASS_EXTERNAL;
import static org.lwjgl.vulkan.VK10.VK_SUCCESS;
import static org.lwjgl.vulkan.VK10.vkAllocateCommandBuffers;
import static org.lwjgl.vulkan.VK10.vkAllocateDescriptorSets;
import static org.lwjgl.vulkan.VK10.vkBeginCommandBuffer;
import static org.lwjgl.vulkan.VK10.vkCmdBeginRenderPass;
import static org.lwjgl.vulkan.VK10.vkCmdBindDescriptorSets;
import static org.lwjgl.vulkan.VK10.vkCmdBindPipeline;
import static org.lwjgl.vulkan.VK10.vkCmdDraw;
import static org.lwjgl.vulkan.VK10.vkCmdEndRenderPass;
import static org.lwjgl.vulkan.VK10.vkCmdSetScissor;
import static org.lwjgl.vulkan.VK10.vkCmdSetViewport;
import static org.lwjgl.vulkan.VK10.vkCreateCommandPool;
import static org.lwjgl.vulkan.VK10.vkCreateDescriptorPool;
import static org.lwjgl.vulkan.VK10.vkCreateDescriptorSetLayout;
import static org.lwjgl.vulkan.VK10.vkCreateFence;
import static org.lwjgl.vulkan.VK10.vkCreateFramebuffer;
import static org.lwjgl.vulkan.VK10.vkCreateGraphicsPipelines;
import static org.lwjgl.vulkan.VK10.vkCreateImageView;
import static org.lwjgl.vulkan.VK10.vkCreatePipelineLayout;
import static org.lwjgl.vulkan.VK10.vkCreateRenderPass;
import static org.lwjgl.vulkan.VK10.vkCreateSampler;
import static org.lwjgl.vulkan.VK10.vkCreateSemaphore;
import static org.lwjgl.vulkan.VK10.vkCreateShaderModule;
import static org.lwjgl.vulkan.VK10.vkDestroyCommandPool;
import static org.lwjgl.vulkan.VK10.vkDestroyDescriptorPool;
import static org.lwjgl.vulkan.VK10.vkDestroyDescriptorSetLayout;
import static org.lwjgl.vulkan.VK10.vkDestroyFence;
import static org.lwjgl.vulkan.VK10.vkDestroyFramebuffer;
import static org.lwjgl.vulkan.VK10.vkDestroyImageView;
import static org.lwjgl.vulkan.VK10.vkDestroyPipeline;
import static org.lwjgl.vulkan.VK10.vkDestroyPipelineLayout;
import static org.lwjgl.vulkan.VK10.vkDestroyRenderPass;
import static org.lwjgl.vulkan.VK10.vkDestroySampler;
import static org.lwjgl.vulkan.VK10.vkDestroySemaphore;
import static org.lwjgl.vulkan.VK10.vkDestroyShaderModule;
import static org.lwjgl.vulkan.VK10.vkEndCommandBuffer;
import static org.lwjgl.vulkan.VK10.vkQueueSubmit;
import static org.lwjgl.vulkan.VK10.vkQueueWaitIdle;
import static org.lwjgl.vulkan.VK10.vkResetCommandBuffer;
import static org.lwjgl.vulkan.VK10.vkResetFences;
import static org.lwjgl.vulkan.VK10.vkUpdateDescriptorSets;
import static org.lwjgl.vulkan.VK10.vkWaitForFences;

import java.nio.ByteBuffer;
import org.lwjgl.system.MemoryStack;
import org.lwjgl.vulkan.VkAttachmentDescription;
import org.lwjgl.vulkan.VkAttachmentReference;
import org.lwjgl.vulkan.VkClearValue;
import org.lwjgl.vulkan.VkCommandBuffer;
import org.lwjgl.vulkan.VkCommandBufferAllocateInfo;
import org.lwjgl.vulkan.VkCommandBufferBeginInfo;
import org.lwjgl.vulkan.VkCommandPoolCreateInfo;
import org.lwjgl.vulkan.VkDescriptorImageInfo;
import org.lwjgl.vulkan.VkDescriptorPoolCreateInfo;
import org.lwjgl.vulkan.VkDescriptorPoolSize;
import org.lwjgl.vulkan.VkDescriptorSetAllocateInfo;
import org.lwjgl.vulkan.VkDescriptorSetLayoutBinding;
import org.lwjgl.vulkan.VkDescriptorSetLayoutCreateInfo;
import org.lwjgl.vulkan.VkExtent2D;
import org.lwjgl.vulkan.VkFenceCreateInfo;
import org.lwjgl.vulkan.VkFramebufferCreateInfo;
import org.lwjgl.vulkan.VkGraphicsPipelineCreateInfo;
import org.lwjgl.vulkan.VkImageSubresourceRange;
import org.lwjgl.vulkan.VkImageViewCreateInfo;
import org.lwjgl.vulkan.VkOffset2D;
import org.lwjgl.vulkan.VkPipelineColorBlendAttachmentState;
import org.lwjgl.vulkan.VkPipelineColorBlendStateCreateInfo;
import org.lwjgl.vulkan.VkPipelineDynamicStateCreateInfo;
import org.lwjgl.vulkan.VkPipelineInputAssemblyStateCreateInfo;
import org.lwjgl.vulkan.VkPipelineLayoutCreateInfo;
import org.lwjgl.vulkan.VkPipelineMultisampleStateCreateInfo;
import org.lwjgl.vulkan.VkPipelineRasterizationStateCreateInfo;
import org.lwjgl.vulkan.VkPipelineShaderStageCreateInfo;
import org.lwjgl.vulkan.VkPipelineVertexInputStateCreateInfo;
import org.lwjgl.vulkan.VkPipelineViewportStateCreateInfo;
import org.lwjgl.vulkan.VkPresentInfoKHR;
import org.lwjgl.vulkan.VkRect2D;
import org.lwjgl.vulkan.VkRenderPassBeginInfo;
import org.lwjgl.vulkan.VkRenderPassCreateInfo;
import org.lwjgl.vulkan.VkSamplerCreateInfo;
import org.lwjgl.vulkan.VkSemaphoreCreateInfo;
import org.lwjgl.vulkan.VkShaderModuleCreateInfo;
import org.lwjgl.vulkan.VkSubmitInfo;
import org.lwjgl.vulkan.VkSubpassDependency;
import org.lwjgl.vulkan.VkSubpassDescription;
import org.lwjgl.vulkan.VkSurfaceCapabilitiesKHR;
import org.lwjgl.vulkan.VkSurfaceFormatKHR;
import org.lwjgl.vulkan.VkSwapchainCreateInfoKHR;
import org.lwjgl.vulkan.VkViewport;
import org.lwjgl.vulkan.VkWriteDescriptorSet;
import org.maplibre.nativeffi.render.VulkanOwnedTextureFrameHandle;

final class VulkanTextureCompositor implements AutoCloseable {
  private static final String VERTEX_SHADER =
      """
      #version 450
      layout(location = 0) out vec2 out_uv;
      vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
      vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
      void main() {
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
        out_uv = uvs[gl_VertexIndex];
      }
      """;
  private static final String FRAGMENT_SHADER =
      """
      #version 450
      layout(set = 0, binding = 0) uniform sampler2D map_texture;
      layout(location = 0) in vec2 in_uv;
      layout(location = 0) out vec4 out_color;
      void main() {
        out_color = texture(map_texture, in_uv);
      }
      """;

  private final VulkanContext context;
  private long swapchain;
  private int swapchainFormat;
  private int extentWidth;
  private int extentHeight;
  private long[] imageViews = new long[0];
  private long[] framebuffers = new long[0];
  private long renderPass;
  private long descriptorSetLayout;
  private long pipelineLayout;
  private long pipeline;
  private long sampler;
  private long descriptorPool;
  private long descriptorSet;
  private long commandPool;
  private VkCommandBuffer commandBuffer;
  private long imageAvailable;
  private long renderFinished;
  private long inFlight;

  VulkanTextureCompositor(VulkanContext context, Viewport viewport) {
    this.context = context;
    try {
      createSwapchain(viewport);
      createRenderPass();
      createDescriptorState();
      createPipeline();
      createFramebuffers();
      createCommands();
    } catch (RuntimeException error) {
      try {
        close();
      } catch (RuntimeException cleanupError) {
        error.addSuppressed(cleanupError);
      }
      throw error;
    }
  }

  void resize(Viewport viewport) {
    context.waitIdle();
    destroySwapchainDependents();
    createSwapchain(viewport);
    createFramebuffers();
  }

  void draw(VulkanOwnedTextureFrameHandle frameHandle) {
    var frame = frameHandle.frame();
    if (frame.width() <= 0 || frame.height() <= 0) {
      throw new IllegalStateException("MapLibre returned an empty Vulkan owned texture frame");
    }
    if (frame.layout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      throw new IllegalStateException(
          "MapLibre owned texture frame is not shader-readable: layout=" + frame.layout());
    }
    drawImageView(frame.imageView().address());
  }

  void drawImageView(long imageView) {
    try (var stack = MemoryStack.stackPush()) {
      check(vkWaitForFences(context.device(), inFlight, true, Long.MAX_VALUE), "vkWaitForFences");
      var imageIndex = stack.mallocInt(1);
      var acquire =
          vkAcquireNextImageKHR(
              context.device(), swapchain, Long.MAX_VALUE, imageAvailable, NULL, imageIndex);
      if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
      }
      if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        throw new IllegalStateException(
            "vkAcquireNextImageKHR failed with Vulkan status " + acquire);
      }
      check(vkResetFences(context.device(), inFlight), "vkResetFences");

      updateDescriptor(imageView);
      record(imageIndex.get(0));
      submit(stack);
      check(vkWaitForFences(context.device(), inFlight, true, Long.MAX_VALUE), "vkWaitForFences");
      present(stack, imageIndex.get(0));
    }
  }

  private void createSwapchain(Viewport viewport) {
    try (var stack = MemoryStack.stackPush()) {
      var capabilities = VkSurfaceCapabilitiesKHR.calloc(stack);
      check(
          vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
              context.physicalDevice(), context.surface(), capabilities),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
      var formatCount = stack.mallocInt(1);
      check(
          vkGetPhysicalDeviceSurfaceFormatsKHR(
              context.physicalDevice(), context.surface(), formatCount, null),
          "vkGetPhysicalDeviceSurfaceFormatsKHR(count)");
      var formats = VkSurfaceFormatKHR.calloc(formatCount.get(0), stack);
      check(
          vkGetPhysicalDeviceSurfaceFormatsKHR(
              context.physicalDevice(), context.surface(), formatCount, formats),
          "vkGetPhysicalDeviceSurfaceFormatsKHR");
      var chosen = formats.get(0);
      for (int i = 0; i < formats.capacity(); i++) {
        var candidate = formats.get(i);
        if ((candidate.format() == VK_FORMAT_B8G8R8A8_UNORM
                || candidate.format() == VK_FORMAT_R8G8B8A8_UNORM)
            && candidate.colorSpace() == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          chosen = candidate;
          break;
        }
      }
      swapchainFormat = chosen.format();
      extentWidth = chooseExtentDimension(capabilities, viewport.framebufferWidth(), true);
      extentHeight = chooseExtentDimension(capabilities, viewport.framebufferHeight(), false);
      var imageCount = capabilities.minImageCount() + 1;
      if (capabilities.maxImageCount() > 0 && imageCount > capabilities.maxImageCount()) {
        imageCount = capabilities.maxImageCount();
      }
      var createInfo =
          VkSwapchainCreateInfoKHR.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR)
              .surface(context.surface())
              .minImageCount(imageCount)
              .imageFormat(swapchainFormat)
              .imageColorSpace(chosen.colorSpace())
              .imageExtent(e -> e.width(extentWidth).height(extentHeight))
              .imageArrayLayers(1)
              .imageUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
              .imageSharingMode(VK_SHARING_MODE_EXCLUSIVE)
              .preTransform(capabilities.currentTransform())
              .compositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
              .presentMode(VK_PRESENT_MODE_FIFO_KHR)
              .clipped(true)
              .oldSwapchain(NULL);
      var out = stack.mallocLong(1);
      check(vkCreateSwapchainKHR(context.device(), createInfo, null, out), "vkCreateSwapchainKHR");
      swapchain = out.get(0);

      var actualCount = stack.mallocInt(1);
      check(
          vkGetSwapchainImagesKHR(context.device(), swapchain, actualCount, null),
          "vkGetSwapchainImagesKHR(count)");
      var images = stack.mallocLong(actualCount.get(0));
      check(
          vkGetSwapchainImagesKHR(context.device(), swapchain, actualCount, images),
          "vkGetSwapchainImagesKHR");
      imageViews = new long[actualCount.get(0)];
      for (int i = 0; i < imageViews.length; i++) {
        imageViews[i] = createImageView(images.get(i), swapchainFormat);
      }
    }
  }

  private int chooseExtentDimension(
      VkSurfaceCapabilitiesKHR capabilities, int viewportValue, boolean width) {
    var current =
        width ? capabilities.currentExtent().width() : capabilities.currentExtent().height();
    if (current != 0xFFFFFFFF) {
      return current;
    }
    var min =
        width ? capabilities.minImageExtent().width() : capabilities.minImageExtent().height();
    var max =
        width ? capabilities.maxImageExtent().width() : capabilities.maxImageExtent().height();
    return Math.max(min, Math.min(max, viewportValue));
  }

  private long createImageView(long image, int format) {
    try (var stack = MemoryStack.stackPush()) {
      var createInfo =
          VkImageViewCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
              .image(image)
              .viewType(org.lwjgl.vulkan.VK10.VK_IMAGE_VIEW_TYPE_2D)
              .format(format)
              .subresourceRange(
                  VkImageSubresourceRange.calloc(stack)
                      .aspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
                      .baseMipLevel(0)
                      .levelCount(1)
                      .baseArrayLayer(0)
                      .layerCount(1));
      var out = stack.mallocLong(1);
      check(vkCreateImageView(context.device(), createInfo, null, out), "vkCreateImageView");
      return out.get(0);
    }
  }

  private void createRenderPass() {
    try (var stack = MemoryStack.stackPush()) {
      var attachment =
          VkAttachmentDescription.calloc(1, stack)
              .format(swapchainFormat)
              .samples(VK_SAMPLE_COUNT_1_BIT)
              .loadOp(VK_ATTACHMENT_LOAD_OP_CLEAR)
              .storeOp(VK_ATTACHMENT_STORE_OP_STORE)
              .stencilLoadOp(VK_ATTACHMENT_LOAD_OP_DONT_CARE)
              .stencilStoreOp(VK_ATTACHMENT_STORE_OP_DONT_CARE)
              .initialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
              .finalLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
      var colorRef =
          VkAttachmentReference.calloc(1, stack)
              .attachment(0)
              .layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      var subpass =
          VkSubpassDescription.calloc(1, stack)
              .pipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
              .colorAttachmentCount(1)
              .pColorAttachments(colorRef);
      var dependency =
          VkSubpassDependency.calloc(1, stack)
              .srcSubpass(VK_SUBPASS_EXTERNAL)
              .dstSubpass(0)
              .srcStageMask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
              .dstStageMask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
              .srcAccessMask(0)
              .dstAccessMask(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
      var createInfo =
          VkRenderPassCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
              .pAttachments(attachment)
              .pSubpasses(subpass)
              .pDependencies(dependency);
      var out = stack.mallocLong(1);
      check(vkCreateRenderPass(context.device(), createInfo, null, out), "vkCreateRenderPass");
      renderPass = out.get(0);
    }
  }

  private void createDescriptorState() {
    try (var stack = MemoryStack.stackPush()) {
      var binding =
          VkDescriptorSetLayoutBinding.calloc(1, stack)
              .binding(0)
              .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
              .descriptorCount(1)
              .stageFlags(VK_SHADER_STAGE_FRAGMENT_BIT);
      var layoutInfo =
          VkDescriptorSetLayoutCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
              .pBindings(binding);
      var out = stack.mallocLong(1);
      check(
          vkCreateDescriptorSetLayout(context.device(), layoutInfo, null, out),
          "vkCreateDescriptorSetLayout");
      descriptorSetLayout = out.get(0);

      var samplerInfo =
          VkSamplerCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
              .magFilter(VK_FILTER_LINEAR)
              .minFilter(VK_FILTER_LINEAR)
              .mipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR)
              .addressModeU(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
              .addressModeV(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
              .addressModeW(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
              .anisotropyEnable(false)
              .maxAnisotropy(1.0f)
              .compareEnable(false)
              .compareOp(VK_COMPARE_OP_ALWAYS)
              .minLod(0.0f)
              .maxLod(0.0f)
              .borderColor(VK_BORDER_COLOR_INT_OPAQUE_BLACK);
      check(vkCreateSampler(context.device(), samplerInfo, null, out), "vkCreateSampler");
      sampler = out.get(0);

      var poolSize =
          VkDescriptorPoolSize.calloc(1, stack)
              .type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
              .descriptorCount(1);
      var poolInfo =
          VkDescriptorPoolCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
              .maxSets(1)
              .pPoolSizes(poolSize);
      check(
          vkCreateDescriptorPool(context.device(), poolInfo, null, out), "vkCreateDescriptorPool");
      descriptorPool = out.get(0);

      var layouts = stack.longs(descriptorSetLayout);
      var allocInfo =
          VkDescriptorSetAllocateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO)
              .descriptorPool(descriptorPool)
              .pSetLayouts(layouts);
      check(vkAllocateDescriptorSets(context.device(), allocInfo, out), "vkAllocateDescriptorSets");
      descriptorSet = out.get(0);
    }
  }

  private void createPipeline() {
    var vert =
        createShaderModule(compileShader(VERTEX_SHADER, shaderc_vertex_shader, "fullscreen.vert"));
    var frag =
        createShaderModule(compileShader(FRAGMENT_SHADER, shaderc_fragment_shader, "sample.frag"));
    try (var stack = MemoryStack.stackPush()) {
      var stages = VkPipelineShaderStageCreateInfo.calloc(2, stack);
      stages
          .get(0)
          .sType(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
          .stage(VK_SHADER_STAGE_VERTEX_BIT)
          .module(vert)
          .pName(stack.UTF8("main"));
      stages
          .get(1)
          .sType(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
          .stage(VK_SHADER_STAGE_FRAGMENT_BIT)
          .module(frag)
          .pName(stack.UTF8("main"));
      var vertexInput =
          VkPipelineVertexInputStateCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
      var inputAssembly =
          VkPipelineInputAssemblyStateCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
              .topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
      var viewportState =
          VkPipelineViewportStateCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO)
              .viewportCount(1)
              .scissorCount(1);
      var rasterizer =
          VkPipelineRasterizationStateCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO)
              .polygonMode(VK_POLYGON_MODE_FILL)
              .cullMode(VK_CULL_MODE_NONE)
              .frontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
              .lineWidth(1.0f);
      var multisample =
          VkPipelineMultisampleStateCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
              .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT);
      var blendAttachment =
          VkPipelineColorBlendAttachmentState.calloc(1, stack)
              .colorWriteMask(
                  VK_COLOR_COMPONENT_R_BIT
                      | VK_COLOR_COMPONENT_G_BIT
                      | VK_COLOR_COMPONENT_B_BIT
                      | VK_COLOR_COMPONENT_A_BIT);
      var colorBlend =
          VkPipelineColorBlendStateCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
              .logicOpEnable(false)
              .logicOp(VK_LOGIC_OP_COPY)
              .pAttachments(blendAttachment);
      var dynamicStates = stack.ints(VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR);
      var dynamicState =
          VkPipelineDynamicStateCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO)
              .pDynamicStates(dynamicStates);
      var layoutInfo =
          VkPipelineLayoutCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO)
              .pSetLayouts(stack.longs(descriptorSetLayout));
      var out = stack.mallocLong(1);
      check(
          vkCreatePipelineLayout(context.device(), layoutInfo, null, out),
          "vkCreatePipelineLayout");
      pipelineLayout = out.get(0);
      var pipelineInfo =
          VkGraphicsPipelineCreateInfo.calloc(1, stack)
              .sType(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
              .pStages(stages)
              .pVertexInputState(vertexInput)
              .pInputAssemblyState(inputAssembly)
              .pViewportState(viewportState)
              .pRasterizationState(rasterizer)
              .pMultisampleState(multisample)
              .pColorBlendState(colorBlend)
              .pDynamicState(dynamicState)
              .layout(pipelineLayout)
              .renderPass(renderPass)
              .subpass(0);
      check(
          vkCreateGraphicsPipelines(context.device(), NULL, pipelineInfo, null, out),
          "vkCreateGraphicsPipelines");
      pipeline = out.get(0);
    } finally {
      vkDestroyShaderModule(context.device(), frag, null);
      vkDestroyShaderModule(context.device(), vert, null);
    }
  }

  private ByteBuffer compileShader(String source, int kind, String name) {
    var compiler = shaderc_compiler_initialize();
    var options = shaderc_compile_options_initialize();
    try {
      var result = shaderc_compile_into_spv(compiler, source, kind, name, "main", options);
      if (result == NULL) {
        throw new IllegalStateException("shaderc returned null compiling " + name);
      }
      try {
        if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
          throw new IllegalStateException(
              "shaderc failed for " + name + ": " + shaderc_result_get_error_message(result));
        }
        var compiled = shaderc_result_get_bytes(result);
        var copy = ByteBuffer.allocateDirect(compiled.remaining());
        copy.put(compiled);
        return copy.flip();
      } finally {
        shaderc_result_release(result);
      }
    } finally {
      shaderc_compile_options_release(options);
      shaderc_compiler_release(compiler);
    }
  }

  private long createShaderModule(ByteBuffer code) {
    try (var stack = MemoryStack.stackPush()) {
      var createInfo =
          VkShaderModuleCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)
              .pCode(code);
      var out = stack.mallocLong(1);
      check(vkCreateShaderModule(context.device(), createInfo, null, out), "vkCreateShaderModule");
      return out.get(0);
    }
  }

  private void createFramebuffers() {
    framebuffers = new long[imageViews.length];
    try (var stack = MemoryStack.stackPush()) {
      var attachments = stack.mallocLong(1);
      for (int i = 0; i < imageViews.length; i++) {
        attachments.put(0, imageViews[i]);
        var createInfo =
            VkFramebufferCreateInfo.calloc(stack)
                .sType(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
                .renderPass(renderPass)
                .pAttachments(attachments)
                .width(extentWidth)
                .height(extentHeight)
                .layers(1);
        var out = stack.mallocLong(1);
        check(vkCreateFramebuffer(context.device(), createInfo, null, out), "vkCreateFramebuffer");
        framebuffers[i] = out.get(0);
      }
    }
  }

  private void createCommands() {
    try (var stack = MemoryStack.stackPush()) {
      var poolInfo =
          VkCommandPoolCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
              .flags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
              .queueFamilyIndex(context.graphicsQueueFamilyIndex());
      var out = stack.mallocLong(1);
      check(vkCreateCommandPool(context.device(), poolInfo, null, out), "vkCreateCommandPool");
      commandPool = out.get(0);
      var allocInfo =
          VkCommandBufferAllocateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO)
              .commandPool(commandPool)
              .level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
              .commandBufferCount(1);
      var commandOut = stack.mallocPointer(1);
      check(
          vkAllocateCommandBuffers(context.device(), allocInfo, commandOut),
          "vkAllocateCommandBuffers");
      commandBuffer = new VkCommandBuffer(commandOut.get(0), context.device());
      var semaphoreInfo =
          VkSemaphoreCreateInfo.calloc(stack).sType(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
      check(
          vkCreateSemaphore(context.device(), semaphoreInfo, null, out),
          "vkCreateSemaphore(imageAvailable)");
      imageAvailable = out.get(0);
      check(
          vkCreateSemaphore(context.device(), semaphoreInfo, null, out),
          "vkCreateSemaphore(renderFinished)");
      renderFinished = out.get(0);
      var fenceInfo =
          VkFenceCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO)
              .flags(org.lwjgl.vulkan.VK10.VK_FENCE_CREATE_SIGNALED_BIT);
      check(vkCreateFence(context.device(), fenceInfo, null, out), "vkCreateFence");
      inFlight = out.get(0);
    }
  }

  private void updateDescriptor(long imageView) {
    try (var stack = MemoryStack.stackPush()) {
      var imageInfo =
          VkDescriptorImageInfo.calloc(1, stack)
              .sampler(sampler)
              .imageView(imageView)
              .imageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      var write =
          VkWriteDescriptorSet.calloc(1, stack)
              .sType(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
              .dstSet(descriptorSet)
              .dstBinding(0)
              .descriptorCount(1)
              .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
              .pImageInfo(imageInfo);
      vkUpdateDescriptorSets(context.device(), write, null);
    }
  }

  private void record(int imageIndex) {
    try (var stack = MemoryStack.stackPush()) {
      check(vkResetCommandBuffer(commandBuffer, 0), "vkResetCommandBuffer");
      var beginInfo =
          VkCommandBufferBeginInfo.calloc(stack).sType(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
      check(vkBeginCommandBuffer(commandBuffer, beginInfo), "vkBeginCommandBuffer");
      var clear = VkClearValue.calloc(1, stack);
      clear.color().float32(0, 0.08f).float32(1, 0.09f).float32(2, 0.11f).float32(3, 1.0f);
      var renderArea =
          VkRect2D.calloc(stack)
              .offset(VkOffset2D.calloc(stack).set(0, 0))
              .extent(VkExtent2D.calloc(stack).set(extentWidth, extentHeight));
      var passInfo =
          VkRenderPassBeginInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO)
              .renderPass(renderPass)
              .framebuffer(framebuffers[imageIndex])
              .renderArea(renderArea)
              .pClearValues(clear);
      vkCmdBeginRenderPass(commandBuffer, passInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
      var viewport =
          VkViewport.calloc(1, stack)
              .x(0)
              .y(0)
              .width(extentWidth)
              .height(extentHeight)
              .minDepth(0)
              .maxDepth(1);
      var scissor =
          VkRect2D.calloc(1, stack)
              .offset(VkOffset2D.calloc(stack).set(0, 0))
              .extent(VkExtent2D.calloc(stack).set(extentWidth, extentHeight));
      vkCmdSetViewport(commandBuffer, 0, viewport);
      vkCmdSetScissor(commandBuffer, 0, scissor);
      vkCmdBindDescriptorSets(
          commandBuffer,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayout,
          0,
          stack.longs(descriptorSet),
          null);
      vkCmdDraw(commandBuffer, 3, 1, 0, 0);
      vkCmdEndRenderPass(commandBuffer);
      check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");
    }
  }

  private void submit(MemoryStack stack) {
    var waitStages = stack.ints(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    var submitInfo =
        VkSubmitInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_SUBMIT_INFO)
            .pWaitSemaphores(stack.longs(imageAvailable))
            .pWaitDstStageMask(waitStages)
            .pCommandBuffers(stack.pointers(commandBuffer))
            .pSignalSemaphores(stack.longs(renderFinished));
    check(vkQueueSubmit(context.graphicsQueue(), submitInfo, inFlight), "vkQueueSubmit");
  }

  private void present(MemoryStack stack, int imageIndex) {
    var indices = stack.ints(imageIndex);
    var presentInfo =
        VkPresentInfoKHR.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR)
            .pWaitSemaphores(stack.longs(renderFinished))
            .swapchainCount(1)
            .pSwapchains(stack.longs(swapchain))
            .pImageIndices(indices);
    var status = vkQueuePresentKHR(context.graphicsQueue(), presentInfo);
    if (status != VK_SUCCESS && status != VK_SUBOPTIMAL_KHR && status != VK_ERROR_OUT_OF_DATE_KHR) {
      throw new IllegalStateException("vkQueuePresentKHR failed with Vulkan status " + status);
    }
    check(vkQueueWaitIdle(context.graphicsQueue()), "vkQueueWaitIdle");
  }

  private void destroySwapchainDependents() {
    for (var framebuffer : framebuffers) {
      if (framebuffer != NULL) {
        vkDestroyFramebuffer(context.device(), framebuffer, null);
      }
    }
    framebuffers = new long[0];
    for (var imageView : imageViews) {
      if (imageView != NULL) {
        vkDestroyImageView(context.device(), imageView, null);
      }
    }
    imageViews = new long[0];
    if (swapchain != NULL) {
      vkDestroySwapchainKHR(context.device(), swapchain, null);
      swapchain = NULL;
    }
  }

  @Override
  public void close() {
    context.waitIdle();
    if (inFlight != NULL) {
      vkDestroyFence(context.device(), inFlight, null);
      inFlight = NULL;
    }
    if (renderFinished != NULL) {
      vkDestroySemaphore(context.device(), renderFinished, null);
      renderFinished = NULL;
    }
    if (imageAvailable != NULL) {
      vkDestroySemaphore(context.device(), imageAvailable, null);
      imageAvailable = NULL;
    }
    if (commandPool != NULL) {
      vkDestroyCommandPool(context.device(), commandPool, null);
      commandPool = NULL;
    }
    destroySwapchainDependents();
    if (pipeline != NULL) {
      vkDestroyPipeline(context.device(), pipeline, null);
      pipeline = NULL;
    }
    if (pipelineLayout != NULL) {
      vkDestroyPipelineLayout(context.device(), pipelineLayout, null);
      pipelineLayout = NULL;
    }
    if (descriptorPool != NULL) {
      vkDestroyDescriptorPool(context.device(), descriptorPool, null);
      descriptorPool = NULL;
    }
    if (sampler != NULL) {
      vkDestroySampler(context.device(), sampler, null);
      sampler = NULL;
    }
    if (descriptorSetLayout != NULL) {
      vkDestroyDescriptorSetLayout(context.device(), descriptorSetLayout, null);
      descriptorSetLayout = NULL;
    }
    if (renderPass != NULL) {
      vkDestroyRenderPass(context.device(), renderPass, null);
      renderPass = NULL;
    }
  }

  private static void check(int status, String operation) {
    if (status != VK_SUCCESS) {
      throw new IllegalStateException(operation + " failed with Vulkan status " + status);
    }
  }
}
