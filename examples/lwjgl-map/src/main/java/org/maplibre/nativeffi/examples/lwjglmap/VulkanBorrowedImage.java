package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.system.MemoryUtil.NULL;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_ASPECT_COLOR_BIT;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_UNDEFINED;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_TILING_OPTIMAL;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_TYPE_2D;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_SAMPLED_BIT;
import static org.lwjgl.vulkan.VK10.VK_IMAGE_VIEW_TYPE_2D;
import static org.lwjgl.vulkan.VK10.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
import static org.lwjgl.vulkan.VK10.VK_SAMPLE_COUNT_1_BIT;
import static org.lwjgl.vulkan.VK10.VK_SHARING_MODE_EXCLUSIVE;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_SUCCESS;
import static org.lwjgl.vulkan.VK10.vkAllocateMemory;
import static org.lwjgl.vulkan.VK10.vkBindImageMemory;
import static org.lwjgl.vulkan.VK10.vkCreateImage;
import static org.lwjgl.vulkan.VK10.vkCreateImageView;
import static org.lwjgl.vulkan.VK10.vkDestroyImage;
import static org.lwjgl.vulkan.VK10.vkDestroyImageView;
import static org.lwjgl.vulkan.VK10.vkFreeMemory;
import static org.lwjgl.vulkan.VK10.vkGetImageMemoryRequirements;
import static org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceMemoryProperties;

import org.lwjgl.system.MemoryStack;
import org.lwjgl.vulkan.VkExtent3D;
import org.lwjgl.vulkan.VkImageCreateInfo;
import org.lwjgl.vulkan.VkImageSubresourceRange;
import org.lwjgl.vulkan.VkImageViewCreateInfo;
import org.lwjgl.vulkan.VkMemoryAllocateInfo;
import org.lwjgl.vulkan.VkMemoryRequirements;
import org.lwjgl.vulkan.VkPhysicalDeviceMemoryProperties;
import org.maplibre.nativeffi.render.NativePointer;

final class VulkanBorrowedImage implements AutoCloseable {
  private final VulkanContext context;
  private long image;
  private long memory;
  private long view;

  private VulkanBorrowedImage(VulkanContext context) {
    this.context = context;
  }

  static VulkanBorrowedImage create(VulkanContext context, Viewport viewport) {
    var result = new VulkanBorrowedImage(context);
    try {
      result.create(viewport);
      return result;
    } catch (RuntimeException error) {
      result.close();
      throw error;
    }
  }

  long view() {
    return view;
  }

  NativePointer imagePointer() {
    return NativePointer.ofAddress(image);
  }

  NativePointer viewPointer() {
    return NativePointer.ofAddress(view);
  }

  private void create(Viewport viewport) {
    try (var stack = MemoryStack.stackPush()) {
      var imageInfo =
          VkImageCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
              .imageType(VK_IMAGE_TYPE_2D)
              .format(org.lwjgl.vulkan.VK10.VK_FORMAT_R8G8B8A8_UNORM)
              .extent(
                  VkExtent3D.calloc(stack)
                      .width(viewport.framebufferWidth())
                      .height(viewport.framebufferHeight())
                      .depth(1))
              .mipLevels(1)
              .arrayLayers(1)
              .samples(VK_SAMPLE_COUNT_1_BIT)
              .tiling(VK_IMAGE_TILING_OPTIMAL)
              .usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
              .sharingMode(VK_SHARING_MODE_EXCLUSIVE)
              .initialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
      var imageOut = stack.mallocLong(1);
      check(vkCreateImage(context.device(), imageInfo, null, imageOut), "vkCreateImage");
      image = imageOut.get(0);

      var requirements = VkMemoryRequirements.calloc(stack);
      vkGetImageMemoryRequirements(context.device(), image, requirements);
      var memoryTypeIndex =
          findMemoryType(requirements.memoryTypeBits(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      var allocateInfo =
          VkMemoryAllocateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
              .allocationSize(requirements.size())
              .memoryTypeIndex(memoryTypeIndex);
      var memoryOut = stack.mallocLong(1);
      check(vkAllocateMemory(context.device(), allocateInfo, null, memoryOut), "vkAllocateMemory");
      memory = memoryOut.get(0);
      check(vkBindImageMemory(context.device(), image, memory, 0), "vkBindImageMemory");

      var viewInfo =
          VkImageViewCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
              .image(image)
              .viewType(VK_IMAGE_VIEW_TYPE_2D)
              .format(org.lwjgl.vulkan.VK10.VK_FORMAT_R8G8B8A8_UNORM)
              .subresourceRange(
                  VkImageSubresourceRange.calloc(stack)
                      .aspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
                      .baseMipLevel(0)
                      .levelCount(1)
                      .baseArrayLayer(0)
                      .layerCount(1));
      var viewOut = stack.mallocLong(1);
      check(vkCreateImageView(context.device(), viewInfo, null, viewOut), "vkCreateImageView");
      view = viewOut.get(0);
    }
  }

  private int findMemoryType(int typeBits, int requiredProperties) {
    try (var stack = MemoryStack.stackPush()) {
      var properties = VkPhysicalDeviceMemoryProperties.calloc(stack);
      vkGetPhysicalDeviceMemoryProperties(context.physicalDevice(), properties);
      for (int i = 0; i < properties.memoryTypeCount(); i++) {
        var supported = (typeBits & (1 << i)) != 0;
        var hasProperties =
            (properties.memoryTypes(i).propertyFlags() & requiredProperties) == requiredProperties;
        if (supported && hasProperties) {
          return i;
        }
      }
      throw new IllegalStateException("No compatible Vulkan memory type found");
    }
  }

  @Override
  public void close() {
    if (view != NULL) {
      vkDestroyImageView(context.device(), view, null);
      view = NULL;
    }
    if (image != NULL) {
      vkDestroyImage(context.device(), image, null);
      image = NULL;
    }
    if (memory != NULL) {
      vkFreeMemory(context.device(), memory, null);
      memory = NULL;
    }
  }

  private static void check(int status, String operation) {
    if (status != VK_SUCCESS) {
      throw new IllegalStateException(operation + " failed with Vulkan status " + status);
    }
  }
}
