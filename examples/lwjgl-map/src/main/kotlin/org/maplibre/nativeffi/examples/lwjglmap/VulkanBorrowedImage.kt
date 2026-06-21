package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.vulkan.VK10.VK_FORMAT_R8G8B8A8_UNORM
import org.lwjgl.vulkan.VK10.VK_IMAGE_ASPECT_COLOR_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_UNDEFINED
import org.lwjgl.vulkan.VK10.VK_IMAGE_TILING_OPTIMAL
import org.lwjgl.vulkan.VK10.VK_IMAGE_TYPE_2D
import org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_SAMPLED_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_VIEW_TYPE_2D
import org.lwjgl.vulkan.VK10.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
import org.lwjgl.vulkan.VK10.VK_SAMPLE_COUNT_1_BIT
import org.lwjgl.vulkan.VK10.VK_SHARING_MODE_EXCLUSIVE
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
import org.lwjgl.vulkan.VK10.VK_SUCCESS
import org.lwjgl.vulkan.VK10.vkAllocateMemory
import org.lwjgl.vulkan.VK10.vkBindImageMemory
import org.lwjgl.vulkan.VK10.vkCreateImage
import org.lwjgl.vulkan.VK10.vkCreateImageView
import org.lwjgl.vulkan.VK10.vkDestroyImage
import org.lwjgl.vulkan.VK10.vkDestroyImageView
import org.lwjgl.vulkan.VK10.vkFreeMemory
import org.lwjgl.vulkan.VK10.vkGetImageMemoryRequirements
import org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceMemoryProperties
import org.lwjgl.vulkan.VkExtent3D
import org.lwjgl.vulkan.VkImageCreateInfo
import org.lwjgl.vulkan.VkImageSubresourceRange
import org.lwjgl.vulkan.VkImageViewCreateInfo
import org.lwjgl.vulkan.VkMemoryAllocateInfo
import org.lwjgl.vulkan.VkMemoryRequirements
import org.lwjgl.vulkan.VkPhysicalDeviceMemoryProperties

internal class VulkanBorrowedImage private constructor(graphicsContext: GraphicsContext) :
  AutoCloseable {
  private val context = graphicsContext as VulkanContext
  private var image = NULL
  private var memory = NULL
  private var view = NULL

  fun view(): Long = view

  fun imageAddress(): Long = image

  fun viewAddress(): Long = view

  private fun create(viewport: Viewport) {
    MemoryStack.stackPush().use { stack ->
      val imageInfo =
        VkImageCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
          .imageType(VK_IMAGE_TYPE_2D)
          .format(VK_FORMAT_R8G8B8A8_UNORM)
          .extent(
            VkExtent3D.calloc(stack)
              .width(viewport.framebufferWidth())
              .height(viewport.framebufferHeight())
              .depth(1)
          )
          .mipLevels(1)
          .arrayLayers(1)
          .samples(VK_SAMPLE_COUNT_1_BIT)
          .tiling(VK_IMAGE_TILING_OPTIMAL)
          .usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT or VK_IMAGE_USAGE_SAMPLED_BIT)
          .sharingMode(VK_SHARING_MODE_EXCLUSIVE)
          .initialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
      val imageOut = stack.mallocLong(1)
      check(vkCreateImage(context.device(), imageInfo, null, imageOut), "vkCreateImage")
      image = imageOut[0]

      val requirements = VkMemoryRequirements.calloc(stack)
      vkGetImageMemoryRequirements(context.device(), image, requirements)
      val memoryTypeIndex =
        findMemoryType(requirements.memoryTypeBits(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
      val allocateInfo =
        VkMemoryAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
          .allocationSize(requirements.size())
          .memoryTypeIndex(memoryTypeIndex)
      val memoryOut = stack.mallocLong(1)
      check(vkAllocateMemory(context.device(), allocateInfo, null, memoryOut), "vkAllocateMemory")
      memory = memoryOut[0]
      check(vkBindImageMemory(context.device(), image, memory, 0), "vkBindImageMemory")

      val viewInfo =
        VkImageViewCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
          .image(image)
          .viewType(VK_IMAGE_VIEW_TYPE_2D)
          .format(VK_FORMAT_R8G8B8A8_UNORM)
          .subresourceRange(
            VkImageSubresourceRange.calloc(stack)
              .aspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
              .baseMipLevel(0)
              .levelCount(1)
              .baseArrayLayer(0)
              .layerCount(1)
          )
      val viewOut = stack.mallocLong(1)
      check(vkCreateImageView(context.device(), viewInfo, null, viewOut), "vkCreateImageView")
      view = viewOut[0]
    }
  }

  private fun findMemoryType(typeBits: Int, requiredProperties: Int): Int {
    MemoryStack.stackPush().use { stack ->
      val properties = VkPhysicalDeviceMemoryProperties.calloc(stack)
      vkGetPhysicalDeviceMemoryProperties(context.physicalDevice(), properties)
      for (index in 0..<properties.memoryTypeCount()) {
        val supported = (typeBits and (1 shl index)) != 0
        val hasProperties =
          (properties.memoryTypes(index).propertyFlags() and requiredProperties) ==
            requiredProperties
        if (supported && hasProperties) {
          return index
        }
      }
    }
    error("No compatible Vulkan memory type found")
  }

  override fun close() {
    if (view != NULL) {
      vkDestroyImageView(context.device(), view, null)
      view = NULL
    }
    if (image != NULL) {
      vkDestroyImage(context.device(), image, null)
      image = NULL
    }
    if (memory != NULL) {
      vkFreeMemory(context.device(), memory, null)
      memory = NULL
    }
  }

  private fun check(status: Int, operation: String) {
    check(status == VK_SUCCESS) { "$operation failed with Vulkan status $status" }
  }

  internal companion object {
    fun create(graphicsContext: GraphicsContext, viewport: Viewport): VulkanBorrowedImage {
      val result = VulkanBorrowedImage(graphicsContext)
      try {
        result.create(viewport)
        return result
      } catch (error: RuntimeException) {
        result.close()
        throw error
      }
    }
  }
}
