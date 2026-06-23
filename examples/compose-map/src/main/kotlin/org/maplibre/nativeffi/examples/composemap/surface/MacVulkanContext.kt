package org.maplibre.nativeffi.examples.composemap.surface

import java.util.LinkedHashSet
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.vulkan.EXTDebugUtils.VK_EXT_DEBUG_UTILS_EXTENSION_NAME
import org.lwjgl.vulkan.EXTMetalObjects.VK_EXPORT_METAL_OBJECT_TYPE_METAL_DEVICE_BIT_EXT
import org.lwjgl.vulkan.EXTMetalObjects.VK_EXT_METAL_OBJECTS_EXTENSION_NAME
import org.lwjgl.vulkan.EXTMetalObjects.VK_STRUCTURE_TYPE_EXPORT_METAL_DEVICE_INFO_EXT
import org.lwjgl.vulkan.EXTMetalObjects.VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT
import org.lwjgl.vulkan.EXTMetalObjects.VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECT_CREATE_INFO_EXT
import org.lwjgl.vulkan.EXTMetalObjects.VK_STRUCTURE_TYPE_IMPORT_METAL_TEXTURE_INFO_EXT
import org.lwjgl.vulkan.EXTMetalObjects.vkExportMetalObjectsEXT
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
import org.lwjgl.vulkan.KHRPortabilitySubset.VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
import org.lwjgl.vulkan.VK10.VK_API_VERSION_1_0
import org.lwjgl.vulkan.VK10.VK_FORMAT_B8G8R8A8_UNORM
import org.lwjgl.vulkan.VK10.VK_IMAGE_ASPECT_COLOR_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
import org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_UNDEFINED
import org.lwjgl.vulkan.VK10.VK_IMAGE_TILING_OPTIMAL
import org.lwjgl.vulkan.VK10.VK_IMAGE_TYPE_2D
import org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_SAMPLED_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_VIEW_TYPE_2D
import org.lwjgl.vulkan.VK10.VK_SAMPLE_COUNT_1_BIT
import org.lwjgl.vulkan.VK10.VK_SHARING_MODE_EXCLUSIVE
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_APPLICATION_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_SUCCESS
import org.lwjgl.vulkan.VK10.vkCreateDevice
import org.lwjgl.vulkan.VK10.vkCreateImage
import org.lwjgl.vulkan.VK10.vkCreateImageView
import org.lwjgl.vulkan.VK10.vkCreateInstance
import org.lwjgl.vulkan.VK10.vkDestroyDevice
import org.lwjgl.vulkan.VK10.vkDestroyImage
import org.lwjgl.vulkan.VK10.vkDestroyImageView
import org.lwjgl.vulkan.VK10.vkDestroyInstance
import org.lwjgl.vulkan.VK10.vkDeviceWaitIdle
import org.lwjgl.vulkan.VK10.vkEnumeratePhysicalDevices
import org.lwjgl.vulkan.VK10.vkGetDeviceQueue
import org.lwjgl.vulkan.VkApplicationInfo
import org.lwjgl.vulkan.VkDevice
import org.lwjgl.vulkan.VkDeviceCreateInfo
import org.lwjgl.vulkan.VkDeviceQueueCreateInfo
import org.lwjgl.vulkan.VkExportMetalDeviceInfoEXT
import org.lwjgl.vulkan.VkExportMetalObjectCreateInfoEXT
import org.lwjgl.vulkan.VkExportMetalObjectsInfoEXT
import org.lwjgl.vulkan.VkExtent3D
import org.lwjgl.vulkan.VkImageCreateInfo
import org.lwjgl.vulkan.VkImageSubresourceRange
import org.lwjgl.vulkan.VkImageViewCreateInfo
import org.lwjgl.vulkan.VkImportMetalTextureInfoEXT
import org.lwjgl.vulkan.VkInstance
import org.lwjgl.vulkan.VkInstanceCreateInfo
import org.lwjgl.vulkan.VkPhysicalDevice
import org.lwjgl.vulkan.VkQueue

internal class MacVulkanContext private constructor(private val requiredMetalDevice: Long) :
  AutoCloseable {
  private var instance: VkInstance? = null
  private var physicalDevice: VkPhysicalDevice? = null
  private var device: VkDevice? = null
  private var graphicsQueue: VkQueue? = null
  private var graphicsQueueFamilyIndex = 0

  val handles: VulkanContextHandles
    get() =
      VulkanContextHandles(
        instance = NativeHandle(instance().address()),
        physicalDevice = NativeHandle(physicalDevice().address()),
        device = NativeHandle(device().address()),
        graphicsQueue = NativeHandle(graphicsQueue().address()),
        graphicsQueueFamilyIndex = graphicsQueueFamilyIndex,
        getInstanceProcAddr = NativeHandle(getInstanceProcAddrAddress()),
        getDeviceProcAddr = NativeHandle(getDeviceProcAddrAddress()),
      )

  fun createImportedTexture(
    metalTexture: NativeHandle,
    extent: SurfaceExtent,
  ): MacVulkanImportedTexture = MacVulkanImportedTexture.create(this, metalTexture, extent)

  fun waitIdle() {
    device?.let { checkVulkan(vkDeviceWaitIdle(it), "vkDeviceWaitIdle") }
  }

  internal fun device(): VkDevice = checkNotNull(device) { "Vulkan device is not initialized" }

  private fun instance(): VkInstance =
    checkNotNull(instance) { "Vulkan instance is not initialized" }

  private fun physicalDevice(): VkPhysicalDevice =
    checkNotNull(physicalDevice) { "Vulkan physical device is not initialized" }

  private fun graphicsQueue(): VkQueue =
    checkNotNull(graphicsQueue) { "Vulkan graphics queue is not initialized" }

  private fun getInstanceProcAddrAddress(): Long {
    return vulkanFunctionAddress("vkGetInstanceProcAddr")
  }

  private fun getDeviceProcAddrAddress(): Long {
    return vulkanFunctionAddress("vkGetDeviceProcAddr")
  }

  private fun createInstance() {
    ensureVulkanFunctionProvider()
    MemoryStack.stackPush().use { stack ->
      val available = stack.vulkanInstanceExtensions()
      val extensions = LinkedHashSet<String>()
      val enablePortability = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME in available
      if (enablePortability) {
        extensions.add(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
      }
      if (VK_EXT_DEBUG_UTILS_EXTENSION_NAME in available) {
        extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
      }
      val exportCreate =
        if (requiredMetalDevice == 0L) {
          null
        } else {
          VkExportMetalObjectCreateInfoEXT.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECT_CREATE_INFO_EXT)
            .exportObjectType(VK_EXPORT_METAL_OBJECT_TYPE_METAL_DEVICE_BIT_EXT)
        }
      val app =
        VkApplicationInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_APPLICATION_INFO)
          .pApplicationName(stack.UTF8("compose-map"))
          .pEngineName(stack.UTF8("maplibre-native-ffi"))
          .apiVersion(VK_API_VERSION_1_0)
      val createInfo =
        VkInstanceCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
          .pNext(exportCreate?.address() ?: NULL)
          .pApplicationInfo(app)
          .ppEnabledExtensionNames(stack.vulkanStringBuffer(extensions))
      if (enablePortability) {
        createInfo.flags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR)
      }
      val out = stack.mallocPointer(1)
      checkVulkan(vkCreateInstance(createInfo, null, out), "vkCreateInstance")
      instance = VkInstance(out[0], createInfo)
    }
  }

  private fun pickPhysicalDeviceAndQueue() {
    MemoryStack.stackPush().use { stack ->
      val count = stack.mallocInt(1)
      checkVulkan(
        vkEnumeratePhysicalDevices(instance(), count, null),
        "vkEnumeratePhysicalDevices(count)",
      )
      check(count[0] != 0) { "No Vulkan physical devices found" }
      val devices = stack.mallocPointer(count[0])
      checkVulkan(
        vkEnumeratePhysicalDevices(instance(), count, devices),
        "vkEnumeratePhysicalDevices",
      )
      for (index in 0..<devices.capacity()) {
        val candidate = VkPhysicalDevice(devices[index], instance())
        if (VK_EXT_METAL_OBJECTS_EXTENSION_NAME !in stack.vulkanDeviceExtensions(candidate)) {
          continue
        }
        val queueFamily = stack.findVulkanGraphicsQueueFamily(candidate)
        if (queueFamily >= 0 && exportsRequiredMetalDevice(candidate, queueFamily)) {
          physicalDevice = candidate
          graphicsQueueFamilyIndex = queueFamily
          return
        }
      }
      val metalRequirement =
        if (requiredMetalDevice == 0L) "" else " and matches the Skiko Metal device"
      error(
        "No Vulkan device supports graphics, $VK_EXT_METAL_OBJECTS_EXTENSION_NAME$metalRequirement"
      )
    }
  }

  private fun exportsRequiredMetalDevice(candidate: VkPhysicalDevice, queueFamily: Int): Boolean {
    if (requiredMetalDevice == 0L) {
      return true
    }
    MemoryStack.stackPush().use { stack ->
      val deviceExtensions = stack.vulkanDeviceExtensions(candidate)
      val extensions = LinkedHashSet<String>()
      extensions.add(VK_EXT_METAL_OBJECTS_EXTENSION_NAME)
      if (VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME in deviceExtensions) {
        extensions.add(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
      }
      val priorities = stack.floats(1.0f)
      val queueInfo =
        VkDeviceQueueCreateInfo.calloc(1, stack)
          .sType(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO)
          .queueFamilyIndex(queueFamily)
          .pQueuePriorities(priorities)
      val createInfo =
        VkDeviceCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
          .pQueueCreateInfos(queueInfo)
          .ppEnabledExtensionNames(stack.vulkanStringBuffer(extensions))
      val out = stack.mallocPointer(1)
      if (vkCreateDevice(candidate, createInfo, null, out) != VK_SUCCESS) {
        return false
      }
      val probeDevice = VkDevice(out[0], candidate, createInfo)
      return try {
        val deviceInfo =
          VkExportMetalDeviceInfoEXT.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_EXPORT_METAL_DEVICE_INFO_EXT)
        val objectsInfo =
          VkExportMetalObjectsInfoEXT.calloc(stack)
            .sType(VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT)
            .pNext(deviceInfo.address())
        vkExportMetalObjectsEXT(probeDevice, objectsInfo)
        deviceInfo.mtlDevice() == requiredMetalDevice
      } finally {
        vkDestroyDevice(probeDevice, null)
      }
    }
  }

  private fun createDevice() {
    MemoryStack.stackPush().use { stack ->
      val deviceExtensions = stack.vulkanDeviceExtensions(physicalDevice())
      val extensions = LinkedHashSet<String>()
      check(VK_EXT_METAL_OBJECTS_EXTENSION_NAME in deviceExtensions) {
        "Selected Vulkan device does not support $VK_EXT_METAL_OBJECTS_EXTENSION_NAME"
      }
      extensions.add(VK_EXT_METAL_OBJECTS_EXTENSION_NAME)
      if (VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME in deviceExtensions) {
        extensions.add(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
      }
      val priorities = stack.floats(1.0f)
      val queueInfo =
        VkDeviceQueueCreateInfo.calloc(1, stack)
          .sType(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO)
          .queueFamilyIndex(graphicsQueueFamilyIndex)
          .pQueuePriorities(priorities)
      val createInfo =
        VkDeviceCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
          .pQueueCreateInfos(queueInfo)
          .ppEnabledExtensionNames(stack.vulkanStringBuffer(extensions))
      val out = stack.mallocPointer(1)
      checkVulkan(vkCreateDevice(physicalDevice(), createInfo, null, out), "vkCreateDevice")
      device = VkDevice(out[0], physicalDevice(), createInfo)
      val queueOut = stack.mallocPointer(1)
      vkGetDeviceQueue(device(), graphicsQueueFamilyIndex, 0, queueOut)
      graphicsQueue = VkQueue(queueOut[0], device())
    }
  }

  override fun close() {
    device?.let {
      vkDeviceWaitIdle(it)
      vkDestroyDevice(it, null)
      device = null
    }
    instance?.let {
      vkDestroyInstance(it, null)
      instance = null
    }
  }

  companion object {
    fun create(requiredMetalDevice: Long = 0L): MacVulkanContext {
      val context = MacVulkanContext(requiredMetalDevice)
      try {
        context.createInstance()
        context.pickPhysicalDeviceAndQueue()
        context.createDevice()
        return context
      } catch (error: RuntimeException) {
        context.close()
        throw error
      }
    }
  }
}

internal class MacVulkanImportedTexture
private constructor(
  private val context: MacVulkanContext,
  private val metalTexture: NativeHandle,
  val extent: SurfaceExtent,
) : AutoCloseable {
  private var image = NULL
  private var view = NULL

  fun target(generation: Long): VulkanImageTarget =
    VulkanImageTarget(
      context = context.handles,
      image = NativeHandle(image),
      imageView = NativeHandle(view),
      format = VK_FORMAT_B8G8R8A8_UNORM,
      initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      queueFamilyIndex = context.handles.graphicsQueueFamilyIndex,
      extent = extent,
      generation = generation,
    )

  private fun create() {
    MemoryStack.stackPush().use { stack ->
      val importTexture =
        VkImportMetalTextureInfoEXT.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMPORT_METAL_TEXTURE_INFO_EXT)
          .plane(VK_IMAGE_ASPECT_COLOR_BIT)
          .mtlTexture(metalTexture.address)
      val imageInfo =
        VkImageCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
          .pNext(importTexture.address())
          .imageType(VK_IMAGE_TYPE_2D)
          .format(VK_FORMAT_B8G8R8A8_UNORM)
          .extent(
            VkExtent3D.calloc(stack)
              .width(extent.physicalWidth)
              .height(extent.physicalHeight)
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
      checkVulkan(vkCreateImage(context.device(), imageInfo, null, imageOut), "vkCreateImage")
      image = imageOut[0]

      val viewInfo =
        VkImageViewCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
          .image(image)
          .viewType(VK_IMAGE_VIEW_TYPE_2D)
          .format(VK_FORMAT_B8G8R8A8_UNORM)
          .subresourceRange(
            VkImageSubresourceRange.calloc(stack)
              .aspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
              .baseMipLevel(0)
              .levelCount(1)
              .baseArrayLayer(0)
              .layerCount(1)
          )
      val viewOut = stack.mallocLong(1)
      checkVulkan(vkCreateImageView(context.device(), viewInfo, null, viewOut), "vkCreateImageView")
      view = viewOut[0]
    }
  }

  override fun close() {
    context.waitIdle()
    if (view != NULL) {
      vkDestroyImageView(context.device(), view, null)
      view = NULL
    }
    if (image != NULL) {
      vkDestroyImage(context.device(), image, null)
      image = NULL
    }
  }

  companion object {
    fun create(
      context: MacVulkanContext,
      metalTexture: NativeHandle,
      extent: SurfaceExtent,
    ): MacVulkanImportedTexture {
      val texture = MacVulkanImportedTexture(context, metalTexture, extent)
      try {
        texture.create()
        return texture
      } catch (error: RuntimeException) {
        texture.close()
        throw error
      }
    }
  }
}
