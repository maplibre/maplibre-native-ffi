package org.maplibre.nativeffi.examples.lwjglmap

import java.nio.ByteBuffer
import kotlin.math.max
import kotlin.math.min
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.util.shaderc.Shaderc.*
import org.lwjgl.vulkan.*
import org.lwjgl.vulkan.KHRSurface.*
import org.lwjgl.vulkan.KHRSwapchain.*
import org.lwjgl.vulkan.VK10.*

internal class VulkanTextureCompositor(private val context: VulkanContext, viewport: Viewport) :
  AutoCloseable {
  private var swapchain = NULL
  private var swapchainFormat = 0
  private var extentWidth = 0
  private var extentHeight = 0
  private var imageViews = LongArray(0)
  private var framebuffers = LongArray(0)
  private var renderPass = NULL
  private var descriptorSetLayout = NULL
  private var pipelineLayout = NULL
  private var pipeline = NULL
  private var sampler = NULL
  private var descriptorPool = NULL
  private var descriptorSet = NULL
  private var commandPool = NULL
  private var commandBuffer: VkCommandBuffer? = null
  private var imageAvailable = NULL
  private var renderFinished = NULL
  private var inFlight = NULL

  init {
    try {
      createSwapchain(viewport)
      createRenderPass()
      createDescriptorState()
      createPipeline()
      createFramebuffers()
      createCommands()
    } catch (error: RuntimeException) {
      try {
        close()
      } catch (cleanupError: RuntimeException) {
        error.addSuppressed(cleanupError)
      }
      throw error
    }
  }

  fun resize(viewport: Viewport) {
    context.waitIdle()
    destroySwapchainDependents()
    createSwapchain(viewport)
    createFramebuffers()
  }

  fun drawImageView(imageView: Long) {
    MemoryStack.stackPush().use { stack ->
      check(vkWaitForFences(context.device(), inFlight, true, Long.MAX_VALUE), "vkWaitForFences")
      val imageIndex = stack.mallocInt(1)
      val acquire =
        vkAcquireNextImageKHR(
          context.device(),
          swapchain,
          Long.MAX_VALUE,
          imageAvailable,
          NULL,
          imageIndex,
        )
      if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        return
      }
      if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        error("vkAcquireNextImageKHR failed with Vulkan status $acquire")
      }
      check(vkResetFences(context.device(), inFlight), "vkResetFences")

      updateDescriptor(imageView)
      record(imageIndex[0])
      submit(stack)
      check(vkWaitForFences(context.device(), inFlight, true, Long.MAX_VALUE), "vkWaitForFences")
      present(stack, imageIndex[0])
    }
  }

  private fun createSwapchain(viewport: Viewport) {
    MemoryStack.stackPush().use { stack ->
      val capabilities = VkSurfaceCapabilitiesKHR.calloc(stack)
      check(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          context.physicalDevice(),
          context.surface(),
          capabilities,
        ),
        "vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
      )
      val formatCount = stack.mallocInt(1)
      check(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
          context.physicalDevice(),
          context.surface(),
          formatCount,
          null,
        ),
        "vkGetPhysicalDeviceSurfaceFormatsKHR(count)",
      )
      val formats = VkSurfaceFormatKHR.calloc(formatCount[0], stack)
      check(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
          context.physicalDevice(),
          context.surface(),
          formatCount,
          formats,
        ),
        "vkGetPhysicalDeviceSurfaceFormatsKHR",
      )
      var chosen = formats[0]
      for (index in 0..<formats.capacity()) {
        val candidate = formats[index]
        if (
          (candidate.format() == VK_FORMAT_B8G8R8A8_UNORM ||
            candidate.format() == VK_FORMAT_R8G8B8A8_UNORM) &&
            candidate.colorSpace() == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
          chosen = candidate
          break
        }
      }
      swapchainFormat = chosen.format()
      extentWidth = chooseExtentDimension(capabilities, viewport.framebufferWidth(), width = true)
      extentHeight =
        chooseExtentDimension(capabilities, viewport.framebufferHeight(), width = false)
      var imageCount = capabilities.minImageCount() + 1
      if (capabilities.maxImageCount() > 0 && imageCount > capabilities.maxImageCount()) {
        imageCount = capabilities.maxImageCount()
      }
      val createInfo =
        VkSwapchainCreateInfoKHR.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR)
          .surface(context.surface())
          .minImageCount(imageCount)
          .imageFormat(swapchainFormat)
          .imageColorSpace(chosen.colorSpace())
          .imageExtent { it.width(extentWidth).height(extentHeight) }
          .imageArrayLayers(1)
          .imageUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
          .imageSharingMode(VK_SHARING_MODE_EXCLUSIVE)
          .preTransform(capabilities.currentTransform())
          .compositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
          .presentMode(VK_PRESENT_MODE_FIFO_KHR)
          .clipped(true)
          .oldSwapchain(NULL)
      val out = stack.mallocLong(1)
      check(vkCreateSwapchainKHR(context.device(), createInfo, null, out), "vkCreateSwapchainKHR")
      swapchain = out[0]

      val actualCount = stack.mallocInt(1)
      check(
        vkGetSwapchainImagesKHR(context.device(), swapchain, actualCount, null),
        "vkGetSwapchainImagesKHR(count)",
      )
      val images = stack.mallocLong(actualCount[0])
      check(
        vkGetSwapchainImagesKHR(context.device(), swapchain, actualCount, images),
        "vkGetSwapchainImagesKHR",
      )
      imageViews = LongArray(actualCount[0])
      for (index in imageViews.indices) {
        imageViews[index] = createImageView(images[index], swapchainFormat)
      }
    }
  }

  private fun chooseExtentDimension(
    capabilities: VkSurfaceCapabilitiesKHR,
    viewportValue: Int,
    width: Boolean,
  ): Int {
    val current =
      if (width) capabilities.currentExtent().width() else capabilities.currentExtent().height()
    if (current != 0xFFFFFFFF.toInt()) {
      return current
    }
    val minimum =
      if (width) capabilities.minImageExtent().width() else capabilities.minImageExtent().height()
    val maximum =
      if (width) capabilities.maxImageExtent().width() else capabilities.maxImageExtent().height()
    return max(minimum, min(maximum, viewportValue))
  }

  private fun createImageView(image: Long, format: Int): Long {
    MemoryStack.stackPush().use { stack ->
      val createInfo =
        VkImageViewCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
          .image(image)
          .viewType(VK_IMAGE_VIEW_TYPE_2D)
          .format(format)
          .subresourceRange(
            VkImageSubresourceRange.calloc(stack)
              .aspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
              .baseMipLevel(0)
              .levelCount(1)
              .baseArrayLayer(0)
              .layerCount(1)
          )
      val out = stack.mallocLong(1)
      check(vkCreateImageView(context.device(), createInfo, null, out), "vkCreateImageView")
      return out[0]
    }
  }

  private fun createRenderPass() {
    MemoryStack.stackPush().use { stack ->
      val attachment =
        VkAttachmentDescription.calloc(1, stack)
          .format(swapchainFormat)
          .samples(VK_SAMPLE_COUNT_1_BIT)
          .loadOp(VK_ATTACHMENT_LOAD_OP_CLEAR)
          .storeOp(VK_ATTACHMENT_STORE_OP_STORE)
          .stencilLoadOp(VK_ATTACHMENT_LOAD_OP_DONT_CARE)
          .stencilStoreOp(VK_ATTACHMENT_STORE_OP_DONT_CARE)
          .initialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
          .finalLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
      val colorRef =
        VkAttachmentReference.calloc(1, stack)
          .attachment(0)
          .layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
      val subpass =
        VkSubpassDescription.calloc(1, stack)
          .pipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
          .colorAttachmentCount(1)
          .pColorAttachments(colorRef)
      val dependency =
        VkSubpassDependency.calloc(1, stack)
          .srcSubpass(VK_SUBPASS_EXTERNAL)
          .dstSubpass(0)
          .srcStageMask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
          .dstStageMask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
          .srcAccessMask(0)
          .dstAccessMask(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
      val createInfo =
        VkRenderPassCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
          .pAttachments(attachment)
          .pSubpasses(subpass)
          .pDependencies(dependency)
      val out = stack.mallocLong(1)
      check(vkCreateRenderPass(context.device(), createInfo, null, out), "vkCreateRenderPass")
      renderPass = out[0]
    }
  }

  private fun createDescriptorState() {
    MemoryStack.stackPush().use { stack ->
      val binding =
        VkDescriptorSetLayoutBinding.calloc(1, stack)
          .binding(0)
          .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
          .descriptorCount(1)
          .stageFlags(VK_SHADER_STAGE_FRAGMENT_BIT)
      val layoutInfo =
        VkDescriptorSetLayoutCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
          .pBindings(binding)
      val out = stack.mallocLong(1)
      check(
        vkCreateDescriptorSetLayout(context.device(), layoutInfo, null, out),
        "vkCreateDescriptorSetLayout",
      )
      descriptorSetLayout = out[0]

      val samplerInfo =
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
          .borderColor(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
      check(vkCreateSampler(context.device(), samplerInfo, null, out), "vkCreateSampler")
      sampler = out[0]

      val poolSize =
        VkDescriptorPoolSize.calloc(1, stack)
          .type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
          .descriptorCount(1)
      val poolInfo =
        VkDescriptorPoolCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
          .maxSets(1)
          .pPoolSizes(poolSize)
      check(vkCreateDescriptorPool(context.device(), poolInfo, null, out), "vkCreateDescriptorPool")
      descriptorPool = out[0]

      val layouts = stack.longs(descriptorSetLayout)
      val allocInfo =
        VkDescriptorSetAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO)
          .descriptorPool(descriptorPool)
          .pSetLayouts(layouts)
      check(vkAllocateDescriptorSets(context.device(), allocInfo, out), "vkAllocateDescriptorSets")
      descriptorSet = out[0]
    }
  }

  private fun createPipeline() {
    val vert =
      createShaderModule(compileShader(VERTEX_SHADER, shaderc_vertex_shader, "fullscreen.vert"))
    val frag =
      createShaderModule(compileShader(FRAGMENT_SHADER, shaderc_fragment_shader, "sample.frag"))
    try {
      MemoryStack.stackPush().use { stack ->
        val stages = VkPipelineShaderStageCreateInfo.calloc(2, stack)
        stages[0]
          .sType(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
          .stage(VK_SHADER_STAGE_VERTEX_BIT)
          .module(vert)
          .pName(stack.UTF8("main"))
        stages[1]
          .sType(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
          .stage(VK_SHADER_STAGE_FRAGMENT_BIT)
          .module(frag)
          .pName(stack.UTF8("main"))
        val vertexInput =
          VkPipelineVertexInputStateCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
        val inputAssembly =
          VkPipelineInputAssemblyStateCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
            .topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        val viewportState =
          VkPipelineViewportStateCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO)
            .viewportCount(1)
            .scissorCount(1)
        val rasterizer =
          VkPipelineRasterizationStateCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO)
            .polygonMode(VK_POLYGON_MODE_FILL)
            .cullMode(VK_CULL_MODE_NONE)
            .frontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .lineWidth(1.0f)
        val multisample =
          VkPipelineMultisampleStateCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
            .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
        val blendAttachment =
          VkPipelineColorBlendAttachmentState.calloc(1, stack)
            .colorWriteMask(
              VK_COLOR_COMPONENT_R_BIT or
                VK_COLOR_COMPONENT_G_BIT or
                VK_COLOR_COMPONENT_B_BIT or
                VK_COLOR_COMPONENT_A_BIT
            )
        val colorBlend =
          VkPipelineColorBlendStateCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
            .logicOpEnable(false)
            .logicOp(VK_LOGIC_OP_COPY)
            .pAttachments(blendAttachment)
        val dynamicStates = stack.ints(VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR)
        val dynamicState =
          VkPipelineDynamicStateCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO)
            .pDynamicStates(dynamicStates)
        val layoutInfo =
          VkPipelineLayoutCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO)
            .pSetLayouts(stack.longs(descriptorSetLayout))
        val out = stack.mallocLong(1)
        check(
          vkCreatePipelineLayout(context.device(), layoutInfo, null, out),
          "vkCreatePipelineLayout",
        )
        pipelineLayout = out[0]
        val pipelineInfo =
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
            .subpass(0)
        check(
          vkCreateGraphicsPipelines(context.device(), NULL, pipelineInfo, null, out),
          "vkCreateGraphicsPipelines",
        )
        pipeline = out[0]
      }
    } finally {
      vkDestroyShaderModule(context.device(), frag, null)
      vkDestroyShaderModule(context.device(), vert, null)
    }
  }

  private fun compileShader(source: String, kind: Int, name: String): ByteBuffer {
    val compiler = shaderc_compiler_initialize()
    val options = shaderc_compile_options_initialize()
    try {
      val result = shaderc_compile_into_spv(compiler, source, kind, name, "main", options)
      check(result != NULL) { "shaderc returned null compiling $name" }
      try {
        if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
          error("shaderc failed for $name: ${shaderc_result_get_error_message(result)}")
        }
        val compiled =
          checkNotNull(shaderc_result_get_bytes(result)) {
            "shaderc returned no bytes compiling $name"
          }
        val copy = ByteBuffer.allocateDirect(compiled.remaining())
        copy.put(compiled)
        copy.flip()
        return copy
      } finally {
        shaderc_result_release(result)
      }
    } finally {
      shaderc_compile_options_release(options)
      shaderc_compiler_release(compiler)
    }
  }

  private fun createShaderModule(code: ByteBuffer): Long {
    MemoryStack.stackPush().use { stack ->
      val createInfo =
        VkShaderModuleCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)
          .pCode(code)
      val out = stack.mallocLong(1)
      check(vkCreateShaderModule(context.device(), createInfo, null, out), "vkCreateShaderModule")
      return out[0]
    }
  }

  private fun createFramebuffers() {
    framebuffers = LongArray(imageViews.size)
    MemoryStack.stackPush().use { stack ->
      val attachments = stack.mallocLong(1)
      for (index in imageViews.indices) {
        attachments.put(0, imageViews[index])
        val createInfo =
          VkFramebufferCreateInfo.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
            .renderPass(renderPass)
            .pAttachments(attachments)
            .width(extentWidth)
            .height(extentHeight)
            .layers(1)
        val out = stack.mallocLong(1)
        check(vkCreateFramebuffer(context.device(), createInfo, null, out), "vkCreateFramebuffer")
        framebuffers[index] = out[0]
      }
    }
  }

  private fun createCommands() {
    MemoryStack.stackPush().use { stack ->
      val poolInfo =
        VkCommandPoolCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
          .flags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
          .queueFamilyIndex(context.graphicsQueueFamilyIndex())
      val out = stack.mallocLong(1)
      check(vkCreateCommandPool(context.device(), poolInfo, null, out), "vkCreateCommandPool")
      commandPool = out[0]
      val allocInfo =
        VkCommandBufferAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO)
          .commandPool(commandPool)
          .level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
          .commandBufferCount(1)
      val commandOut = stack.mallocPointer(1)
      check(
        vkAllocateCommandBuffers(context.device(), allocInfo, commandOut),
        "vkAllocateCommandBuffers",
      )
      commandBuffer = VkCommandBuffer(commandOut[0], context.device())
      val semaphoreInfo =
        VkSemaphoreCreateInfo.calloc(stack).sType(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
      check(
        vkCreateSemaphore(context.device(), semaphoreInfo, null, out),
        "vkCreateSemaphore(imageAvailable)",
      )
      imageAvailable = out[0]
      check(
        vkCreateSemaphore(context.device(), semaphoreInfo, null, out),
        "vkCreateSemaphore(renderFinished)",
      )
      renderFinished = out[0]
      val fenceInfo =
        VkFenceCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO)
          .flags(VK_FENCE_CREATE_SIGNALED_BIT)
      check(vkCreateFence(context.device(), fenceInfo, null, out), "vkCreateFence")
      inFlight = out[0]
    }
  }

  private fun updateDescriptor(imageView: Long) {
    MemoryStack.stackPush().use { stack ->
      val imageInfo =
        VkDescriptorImageInfo.calloc(1, stack)
          .sampler(sampler)
          .imageView(imageView)
          .imageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
      val write =
        VkWriteDescriptorSet.calloc(1, stack)
          .sType(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
          .dstSet(descriptorSet)
          .dstBinding(0)
          .descriptorCount(1)
          .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
          .pImageInfo(imageInfo)
      vkUpdateDescriptorSets(context.device(), write, null)
    }
  }

  private fun record(imageIndex: Int) {
    MemoryStack.stackPush().use { stack ->
      val commandBuffer = commandBuffer()
      check(vkResetCommandBuffer(commandBuffer, 0), "vkResetCommandBuffer")
      val beginInfo =
        VkCommandBufferBeginInfo.calloc(stack).sType(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO)
      check(vkBeginCommandBuffer(commandBuffer, beginInfo), "vkBeginCommandBuffer")
      val clear = VkClearValue.calloc(1, stack)
      clear.color().float32(0, 0.08f).float32(1, 0.09f).float32(2, 0.11f).float32(3, 1.0f)
      val renderArea =
        VkRect2D.calloc(stack)
          .offset(VkOffset2D.calloc(stack).set(0, 0))
          .extent(VkExtent2D.calloc(stack).set(extentWidth, extentHeight))
      val passInfo =
        VkRenderPassBeginInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO)
          .renderPass(renderPass)
          .framebuffer(framebuffers[imageIndex])
          .renderArea(renderArea)
          .pClearValues(clear)
      vkCmdBeginRenderPass(commandBuffer, passInfo, VK_SUBPASS_CONTENTS_INLINE)
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline)
      val viewport =
        VkViewport.calloc(1, stack)
          .x(0.0f)
          .y(0.0f)
          .width(extentWidth.toFloat())
          .height(extentHeight.toFloat())
          .minDepth(0.0f)
          .maxDepth(1.0f)
      val scissor =
        VkRect2D.calloc(1, stack)
          .offset(VkOffset2D.calloc(stack).set(0, 0))
          .extent(VkExtent2D.calloc(stack).set(extentWidth, extentHeight))
      vkCmdSetViewport(commandBuffer, 0, viewport)
      vkCmdSetScissor(commandBuffer, 0, scissor)
      vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        stack.longs(descriptorSet),
        null,
      )
      vkCmdDraw(commandBuffer, 3, 1, 0, 0)
      vkCmdEndRenderPass(commandBuffer)
      check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer")
    }
  }

  private fun submit(stack: MemoryStack) {
    val waitStages = stack.ints(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
    val submitInfo =
      VkSubmitInfo.calloc(stack)
        .sType(VK_STRUCTURE_TYPE_SUBMIT_INFO)
        .pWaitSemaphores(stack.longs(imageAvailable))
        .pWaitDstStageMask(waitStages)
        .pCommandBuffers(stack.pointers(commandBuffer()))
        .pSignalSemaphores(stack.longs(renderFinished))
    check(vkQueueSubmit(context.graphicsQueue(), submitInfo, inFlight), "vkQueueSubmit")
  }

  private fun present(stack: MemoryStack, imageIndex: Int) {
    val indices = stack.ints(imageIndex)
    val presentInfo =
      VkPresentInfoKHR.calloc(stack)
        .sType(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR)
        .pWaitSemaphores(stack.longs(renderFinished))
        .swapchainCount(1)
        .pSwapchains(stack.longs(swapchain))
        .pImageIndices(indices)
    val status = vkQueuePresentKHR(context.graphicsQueue(), presentInfo)
    if (status != VK_SUCCESS && status != VK_SUBOPTIMAL_KHR && status != VK_ERROR_OUT_OF_DATE_KHR) {
      error("vkQueuePresentKHR failed with Vulkan status $status")
    }
    check(vkQueueWaitIdle(context.graphicsQueue()), "vkQueueWaitIdle")
  }

  private fun destroySwapchainDependents() {
    for (framebuffer in framebuffers) {
      if (framebuffer != NULL) {
        vkDestroyFramebuffer(context.device(), framebuffer, null)
      }
    }
    framebuffers = LongArray(0)
    for (imageView in imageViews) {
      if (imageView != NULL) {
        vkDestroyImageView(context.device(), imageView, null)
      }
    }
    imageViews = LongArray(0)
    if (swapchain != NULL) {
      vkDestroySwapchainKHR(context.device(), swapchain, null)
      swapchain = NULL
    }
  }

  override fun close() {
    context.waitIdle()
    if (inFlight != NULL) {
      vkDestroyFence(context.device(), inFlight, null)
      inFlight = NULL
    }
    if (renderFinished != NULL) {
      vkDestroySemaphore(context.device(), renderFinished, null)
      renderFinished = NULL
    }
    if (imageAvailable != NULL) {
      vkDestroySemaphore(context.device(), imageAvailable, null)
      imageAvailable = NULL
    }
    if (commandPool != NULL) {
      vkDestroyCommandPool(context.device(), commandPool, null)
      commandPool = NULL
      commandBuffer = null
    }
    destroySwapchainDependents()
    if (pipeline != NULL) {
      vkDestroyPipeline(context.device(), pipeline, null)
      pipeline = NULL
    }
    if (pipelineLayout != NULL) {
      vkDestroyPipelineLayout(context.device(), pipelineLayout, null)
      pipelineLayout = NULL
    }
    if (descriptorPool != NULL) {
      vkDestroyDescriptorPool(context.device(), descriptorPool, null)
      descriptorPool = NULL
    }
    if (sampler != NULL) {
      vkDestroySampler(context.device(), sampler, null)
      sampler = NULL
    }
    if (descriptorSetLayout != NULL) {
      vkDestroyDescriptorSetLayout(context.device(), descriptorSetLayout, null)
      descriptorSetLayout = NULL
    }
    if (renderPass != NULL) {
      vkDestroyRenderPass(context.device(), renderPass, null)
      renderPass = NULL
    }
  }

  private fun commandBuffer(): VkCommandBuffer =
    checkNotNull(commandBuffer) { "Vulkan command buffer is not initialized" }

  private fun check(status: Int, operation: String) {
    check(status == VK_SUCCESS) { "$operation failed with Vulkan status $status" }
  }

  private companion object {
    private val VERTEX_SHADER =
      """
      #version 450
      layout(location = 0) out vec2 out_uv;
      vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
      vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
      void main() {
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
        out_uv = uvs[gl_VertexIndex];
      }
      """
        .trimIndent()

    private val FRAGMENT_SHADER =
      """
      #version 450
      layout(set = 0, binding = 0) uniform sampler2D map_texture;
      layout(location = 0) in vec2 in_uv;
      layout(location = 0) out vec4 out_color;
      void main() {
        out_color = texture(map_texture, in_uv);
      }
      """
        .trimIndent()
  }
}
