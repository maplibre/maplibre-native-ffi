package org.maplibre.nativeffi.render

import org.lwjgl.system.MemoryStack
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
import org.lwjgl.vulkan.KHRPortabilitySubset.VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
import org.lwjgl.vulkan.VK
import org.lwjgl.vulkan.VK10.*
import org.lwjgl.vulkan.VK11.VK_API_VERSION_1_1
import org.lwjgl.vulkan.VkApplicationInfo
import org.lwjgl.vulkan.VkDevice
import org.lwjgl.vulkan.VkDeviceCreateInfo
import org.lwjgl.vulkan.VkDeviceQueueCreateInfo
import org.lwjgl.vulkan.VkExtensionProperties
import org.lwjgl.vulkan.VkInstance
import org.lwjgl.vulkan.VkInstanceCreateInfo
import org.lwjgl.vulkan.VkPhysicalDevice
import org.lwjgl.vulkan.VkPhysicalDeviceFeatures
import org.lwjgl.vulkan.VkQueueFamilyProperties
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor

internal fun attachJvmVulkan(
  map: MapHandle,
  width: Int,
  height: Int,
  depth: UInt,
): OwnedTextureTestSession {
  val context = VulkanTestContext()
  val descriptor = context.descriptor
  return attachOwnedTextureFixture(
    map,
    width,
    height,
    depth,
    attach = { target, w, h, options ->
      target.vulkanOwnedTextureAttach(
        VulkanOwnedTextureDescriptor(RenderTargetExtent(w.toUInt(), h.toUInt(), 1.0), descriptor),
        options,
      )
    },
    frameSize = { frame ->
      frame.withGetVulkanTexture { OwnedTextureFrameSize(it.width.toInt(), it.height.toInt()) }
    },
    releaseGraphics = context::close,
  )
}

private class VulkanTestContext : AutoCloseable {
  private var instance: VkInstance? = null
  private var device: VkDevice? = null
  val descriptor: VulkanContextDescriptor

  init {
    try {
      descriptor =
        MemoryStack.stackPush().use { stack ->
          val count = stack.mallocInt(1)
          success(vkEnumerateInstanceExtensionProperties(null as String?, count, null))
          val extensions = VkExtensionProperties.calloc(count[0], stack)
          success(vkEnumerateInstanceExtensionProperties(null as String?, count, extensions))
          val portability = extensions.any {
            it.extensionNameString() == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
          }
          val app =
            VkApplicationInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_APPLICATION_INFO)
              .pApplicationName(stack.UTF8("Kotlin binding tests"))
              .apiVersion(VK_API_VERSION_1_1)
          val info =
            VkInstanceCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
              .pApplicationInfo(app)
          if (portability)
            info
              .flags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR)
              .ppEnabledExtensionNames(
                stack.pointers(stack.UTF8(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
              )
          val out = stack.mallocPointer(1)
          success(vkCreateInstance(info, null, out))
          val createdInstance = VkInstance(out[0], info)
          instance = createdInstance
          success(vkEnumeratePhysicalDevices(createdInstance, count, null))
          check(count[0] > 0) { "No Vulkan physical device" }
          val physicalDevices = stack.mallocPointer(count[0])
          success(vkEnumeratePhysicalDevices(createdInstance, count, physicalDevices))
          var selected: Pair<VkPhysicalDevice, Int>? = null
          for (index in 0 until count[0]) {
            val candidate = VkPhysicalDevice(physicalDevices[index], createdInstance)
            val queueCount = stack.mallocInt(1)
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, queueCount, null)
            val families = VkQueueFamilyProperties.calloc(queueCount[0], stack)
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, queueCount, families)
            val family =
              (0 until queueCount[0]).firstOrNull {
                families[it].queueCount() > 0 &&
                  families[it].queueFlags() and VK_QUEUE_GRAPHICS_BIT != 0
              }
            if (family != null) {
              selected = candidate to family
              break
            }
          }
          val (physical, family) = checkNotNull(selected) { "No Vulkan graphics queue" }
          success(vkEnumerateDeviceExtensionProperties(physical, null as String?, count, null))
          val deviceExtensions = VkExtensionProperties.calloc(count[0], stack)
          success(
            vkEnumerateDeviceExtensionProperties(physical, null as String?, count, deviceExtensions)
          )
          val devicePortability = deviceExtensions.any {
            it.extensionNameString() == VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
          }
          val supported = VkPhysicalDeviceFeatures.calloc(stack)
          vkGetPhysicalDeviceFeatures(physical, supported)
          val enabled =
            VkPhysicalDeviceFeatures.calloc(stack)
              .samplerAnisotropy(supported.samplerAnisotropy())
              .wideLines(supported.wideLines())
          val queueInfo =
            VkDeviceQueueCreateInfo.calloc(1, stack)
              .sType(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO)
              .queueFamilyIndex(family)
              .pQueuePriorities(stack.floats(1.0f))
          val deviceInfo =
            VkDeviceCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
              .pQueueCreateInfos(queueInfo)
              .pEnabledFeatures(enabled)
          if (devicePortability)
            deviceInfo.ppEnabledExtensionNames(
              stack.pointers(stack.UTF8(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            )
          success(vkCreateDevice(physical, deviceInfo, null, out))
          val createdDevice = VkDevice(out[0], physical, deviceInfo)
          device = createdDevice
          vkGetDeviceQueue(createdDevice, family, 0, out)
          VulkanContextDescriptor(
            NativePointer.ofAddress(createdInstance.address()),
            NativePointer.ofAddress(physical.address()),
            NativePointer.ofAddress(createdDevice.address()),
            NativePointer.ofAddress(out[0]),
            family.toUInt(),
            NativePointer.ofAddress(
              VK.getFunctionProvider().getFunctionAddress("vkGetInstanceProcAddr")
            ),
            NativePointer.ofAddress(
              VK.getFunctionProvider().getFunctionAddress("vkGetDeviceProcAddr")
            ),
          )
        }
    } catch (error: Throwable) {
      close()
      throw error
    }
  }

  override fun close() {
    device?.let {
      vkDeviceWaitIdle(it)
      vkDestroyDevice(it, null)
    }
    device = null
    instance?.let { vkDestroyInstance(it, null) }
    instance = null
  }
}

private fun success(status: Int) {
  check(status == VK_SUCCESS) { "Vulkan driver returned $status" }
}
