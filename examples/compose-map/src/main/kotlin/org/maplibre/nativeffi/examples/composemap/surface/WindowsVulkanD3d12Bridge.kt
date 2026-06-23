package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import java.lang.foreign.Arena
import java.lang.foreign.FunctionDescriptor
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.SymbolLookup
import java.lang.foreign.ValueLayout
import java.util.LinkedHashSet
import org.lwjgl.PointerBuffer
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.vulkan.EXTDebugUtils.VK_EXT_DEBUG_UTILS_EXTENSION_NAME
import org.lwjgl.vulkan.KHRExternalMemoryWin32.VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
import org.lwjgl.vulkan.KHRExternalMemoryWin32.VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR
import org.lwjgl.vulkan.KHRExternalMemoryWin32.VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR
import org.lwjgl.vulkan.KHRExternalMemoryWin32.vkGetMemoryWin32HandlePropertiesKHR
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
import org.lwjgl.vulkan.KHRPortabilitySubset.VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
import org.lwjgl.vulkan.VK
import org.lwjgl.vulkan.VK10.VK_FORMAT_B8G8R8A8_UNORM
import org.lwjgl.vulkan.VK10.VK_IMAGE_ASPECT_COLOR_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_GENERAL
import org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_UNDEFINED
import org.lwjgl.vulkan.VK10.VK_IMAGE_TILING_OPTIMAL
import org.lwjgl.vulkan.VK10.VK_IMAGE_TYPE_2D
import org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_USAGE_SAMPLED_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_VIEW_TYPE_2D
import org.lwjgl.vulkan.VK10.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
import org.lwjgl.vulkan.VK10.VK_QUEUE_GRAPHICS_BIT
import org.lwjgl.vulkan.VK10.VK_SAMPLE_COUNT_1_BIT
import org.lwjgl.vulkan.VK10.VK_SHARING_MODE_EXCLUSIVE
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_APPLICATION_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
import org.lwjgl.vulkan.VK10.VK_SUCCESS
import org.lwjgl.vulkan.VK10.vkAllocateMemory
import org.lwjgl.vulkan.VK10.vkBindImageMemory
import org.lwjgl.vulkan.VK10.vkCreateDevice
import org.lwjgl.vulkan.VK10.vkCreateImage
import org.lwjgl.vulkan.VK10.vkCreateImageView
import org.lwjgl.vulkan.VK10.vkCreateInstance
import org.lwjgl.vulkan.VK10.vkDestroyDevice
import org.lwjgl.vulkan.VK10.vkDestroyImage
import org.lwjgl.vulkan.VK10.vkDestroyImageView
import org.lwjgl.vulkan.VK10.vkDestroyInstance
import org.lwjgl.vulkan.VK10.vkDeviceWaitIdle
import org.lwjgl.vulkan.VK10.vkEnumerateDeviceExtensionProperties
import org.lwjgl.vulkan.VK10.vkEnumerateInstanceExtensionProperties
import org.lwjgl.vulkan.VK10.vkEnumeratePhysicalDevices
import org.lwjgl.vulkan.VK10.vkFreeMemory
import org.lwjgl.vulkan.VK10.vkGetDeviceQueue
import org.lwjgl.vulkan.VK10.vkGetImageMemoryRequirements
import org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceMemoryProperties
import org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceQueueFamilyProperties
import org.lwjgl.vulkan.VK11.VK_API_VERSION_1_1
import org.lwjgl.vulkan.VK11.VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT
import org.lwjgl.vulkan.VK11.VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO
import org.lwjgl.vulkan.VK11.VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO
import org.lwjgl.vulkan.VkApplicationInfo
import org.lwjgl.vulkan.VkDevice
import org.lwjgl.vulkan.VkDeviceCreateInfo
import org.lwjgl.vulkan.VkDeviceQueueCreateInfo
import org.lwjgl.vulkan.VkExtensionProperties
import org.lwjgl.vulkan.VkExtent3D
import org.lwjgl.vulkan.VkExternalMemoryImageCreateInfo
import org.lwjgl.vulkan.VkImageCreateInfo
import org.lwjgl.vulkan.VkImageSubresourceRange
import org.lwjgl.vulkan.VkImageViewCreateInfo
import org.lwjgl.vulkan.VkImportMemoryWin32HandleInfoKHR
import org.lwjgl.vulkan.VkInstance
import org.lwjgl.vulkan.VkInstanceCreateInfo
import org.lwjgl.vulkan.VkMemoryAllocateInfo
import org.lwjgl.vulkan.VkMemoryDedicatedAllocateInfo
import org.lwjgl.vulkan.VkMemoryRequirements
import org.lwjgl.vulkan.VkMemoryWin32HandlePropertiesKHR
import org.lwjgl.vulkan.VkPhysicalDevice
import org.lwjgl.vulkan.VkPhysicalDeviceMemoryProperties
import org.lwjgl.vulkan.VkQueue
import org.lwjgl.vulkan.VkQueueFamilyProperties

internal class WindowsVulkanD3d12Bridge : NativeSurfaceBridge {
  private val vulkan = WindowsVulkanContext.create()
  private var direct3DTexture = NativeHandle(0)
  private var importedTexture: WindowsVulkanImportedD3D12Texture? = null
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val backend: ProducerBackend = ProducerBackend.VULKAN

  override val consumerBackend: ConsumerBackend = ConsumerBackend.DIRECT3D12

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
      isPlaceholder = false,
    )

  override fun resize(extent: SurfaceExtent) {
    if (extent == currentExtent && importedTexture != null) {
      return
    }
    recreateTexture(extent)
    currentExtent = extent
    generation += 1
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (importedTexture == null || extent != currentExtent) {
      resize(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    vulkan.waitIdle()
  }

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is VulkanImageTarget || direct3DTexture.address == 0L) {
      return false
    }
    return SkikoHost.drawDirect3DTexture(
      scope,
      Direct3DTextureTarget(
        texture = direct3DTexture,
        extent = importedTexture?.storageExtent ?: target.extent,
        generation = target.generation,
      ),
    )
  }

  override fun close() {
    disposeTexture()
    vulkan.close()
  }

  private fun target(generation: Long): NativeSurfaceTarget =
    checkNotNull(importedTexture) { "Windows Vulkan texture is not initialized" }.target(generation)

  private fun recreateTexture(extent: SurfaceExtent) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }

    val device = SkikoHost.requireDirect3DDevice()
    val storageExtent = extent
    disposeTexture()
    direct3DTexture = WindowsD3D12Interop.createSharedTexture(device, storageExtent)
    var sharedHandle = NULL
    try {
      sharedHandle = WindowsD3D12Interop.createSharedHandle(direct3DTexture)
      importedTexture = vulkan.importD3D12Texture(sharedHandle, storageExtent, extent)
    } catch (error: RuntimeException) {
      disposeTexture()
      throw error
    } finally {
      WindowsD3D12Interop.closeSharedHandle(sharedHandle)
    }
  }

  private fun disposeTexture() {
    importedTexture?.close()
    importedTexture = null
    if (direct3DTexture.address != 0L) {
      SkikoHost.forgetDirect3DTexture(direct3DTexture)
      WindowsD3D12Interop.release(direct3DTexture)
      direct3DTexture = NativeHandle(0)
    }
  }
}

private class WindowsVulkanContext private constructor() : AutoCloseable {
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

  fun importD3D12Texture(
    sharedHandle: Long,
    storageExtent: SurfaceExtent,
    renderExtent: SurfaceExtent,
  ): WindowsVulkanImportedD3D12Texture =
    WindowsVulkanImportedD3D12Texture.create(this, sharedHandle, storageExtent, renderExtent)

  fun waitIdle() {
    device?.let { check(vkDeviceWaitIdle(it), "vkDeviceWaitIdle") }
  }

  internal fun physicalDevice(): VkPhysicalDevice =
    checkNotNull(physicalDevice) { "Vulkan physical device is not initialized" }

  internal fun device(): VkDevice = checkNotNull(device) { "Vulkan device is not initialized" }

  private fun instance(): VkInstance =
    checkNotNull(instance) { "Vulkan instance is not initialized" }

  private fun graphicsQueue(): VkQueue =
    checkNotNull(graphicsQueue) { "Vulkan graphics queue is not initialized" }

  private fun getInstanceProcAddrAddress(): Long {
    ensureVulkanFunctionProvider()
    return VK.getFunctionProvider().getFunctionAddress("vkGetInstanceProcAddr")
  }

  private fun getDeviceProcAddrAddress(): Long {
    ensureVulkanFunctionProvider()
    return VK.getFunctionProvider().getFunctionAddress("vkGetDeviceProcAddr")
  }

  private fun createInstance() {
    ensureVulkanFunctionProvider()
    MemoryStack.stackPush().use { stack ->
      val available = instanceExtensions(stack)
      val extensions = LinkedHashSet<String>()
      val enablePortability = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME in available
      if (enablePortability) {
        extensions.add(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
      }
      if (VK_EXT_DEBUG_UTILS_EXTENSION_NAME in available) {
        extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
      }
      val app =
        VkApplicationInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_APPLICATION_INFO)
          .pApplicationName(stack.UTF8("compose-map"))
          .pEngineName(stack.UTF8("maplibre-native-ffi"))
          .apiVersion(VK_API_VERSION_1_1)
      val createInfo =
        VkInstanceCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
          .pApplicationInfo(app)
          .ppEnabledExtensionNames(stringBuffer(stack, extensions))
      if (enablePortability) {
        createInfo.flags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR)
      }
      val out = stack.mallocPointer(1)
      check(vkCreateInstance(createInfo, null, out), "vkCreateInstance")
      instance = VkInstance(out[0], createInfo)
    }
  }

  private fun pickPhysicalDeviceAndQueue() {
    MemoryStack.stackPush().use { stack ->
      val count = stack.mallocInt(1)
      check(
        vkEnumeratePhysicalDevices(instance(), count, null),
        "vkEnumeratePhysicalDevices(count)",
      )
      check(count[0] != 0) { "No Vulkan physical devices found" }
      val devices = stack.mallocPointer(count[0])
      check(vkEnumeratePhysicalDevices(instance(), count, devices), "vkEnumeratePhysicalDevices")
      for (index in 0..<devices.capacity()) {
        val candidate = VkPhysicalDevice(devices[index], instance())
        if (VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME !in deviceExtensions(stack, candidate)) {
          continue
        }
        val queueFamily = findGraphicsQueueFamily(stack, candidate)
        if (queueFamily >= 0) {
          physicalDevice = candidate
          graphicsQueueFamilyIndex = queueFamily
          return
        }
      }
      error("No Vulkan device supports graphics and $VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME")
    }
  }

  private fun findGraphicsQueueFamily(stack: MemoryStack, candidate: VkPhysicalDevice): Int {
    val count = stack.mallocInt(1)
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, null)
    val families = VkQueueFamilyProperties.calloc(count[0], stack)
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, families)
    for (index in 0..<families.capacity()) {
      if ((families[index].queueFlags() and VK_QUEUE_GRAPHICS_BIT) != 0) {
        return index
      }
    }
    return -1
  }

  private fun createDevice() {
    MemoryStack.stackPush().use { stack ->
      val deviceExtensions = deviceExtensions(stack, physicalDevice())
      check(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME in deviceExtensions) {
        "Selected Vulkan device does not support $VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME"
      }
      val extensions = LinkedHashSet<String>()
      extensions.add(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
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
          .ppEnabledExtensionNames(stringBuffer(stack, extensions))
      val out = stack.mallocPointer(1)
      check(vkCreateDevice(physicalDevice(), createInfo, null, out), "vkCreateDevice")
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
    fun create(): WindowsVulkanContext {
      val context = WindowsVulkanContext()
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

    private fun instanceExtensions(stack: MemoryStack): Set<String> {
      val count = stack.mallocInt(1)
      check(
        vkEnumerateInstanceExtensionProperties(null as String?, count, null),
        "vkEnumerateInstanceExtensionProperties(count)",
      )
      val props = VkExtensionProperties.calloc(count[0], stack)
      check(
        vkEnumerateInstanceExtensionProperties(null as String?, count, props),
        "vkEnumerateInstanceExtensionProperties",
      )
      return buildSet { props.forEach { add(it.extensionNameString()) } }
    }

    private fun deviceExtensions(stack: MemoryStack, device: VkPhysicalDevice): Set<String> {
      val count = stack.mallocInt(1)
      check(
        vkEnumerateDeviceExtensionProperties(device, null as String?, count, null),
        "vkEnumerateDeviceExtensionProperties(count)",
      )
      val props = VkExtensionProperties.calloc(count[0], stack)
      check(
        vkEnumerateDeviceExtensionProperties(device, null as String?, count, props),
        "vkEnumerateDeviceExtensionProperties",
      )
      return buildSet { props.forEach { add(it.extensionNameString()) } }
    }

    private fun stringBuffer(stack: MemoryStack, values: Set<String>): PointerBuffer {
      val buffer = stack.mallocPointer(values.size)
      for (value in values) {
        buffer.put(stack.UTF8(value))
      }
      return buffer.flip()
    }

    @Suppress("SENSELESS_COMPARISON")
    private fun ensureVulkanFunctionProvider() {
      if (VK.getFunctionProvider() == null) {
        VK.create()
      }
    }

    private fun check(status: Int, operation: String) {
      check(status == VK_SUCCESS) { "$operation failed with Vulkan status $status" }
    }
  }
}

private class WindowsVulkanImportedD3D12Texture
private constructor(
  private val context: WindowsVulkanContext,
  private val sharedHandle: Long,
  val storageExtent: SurfaceExtent,
  private val renderExtent: SurfaceExtent,
) : AutoCloseable {
  private var image = NULL
  private var memory = NULL
  private var view = NULL

  fun target(generation: Long): VulkanImageTarget =
    VulkanImageTarget(
      context = context.handles,
      image = NativeHandle(image),
      imageView = NativeHandle(view),
      format = VK_FORMAT_B8G8R8A8_UNORM,
      initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      finalLayout = VK_IMAGE_LAYOUT_GENERAL,
      queueFamilyIndex = context.handles.graphicsQueueFamilyIndex,
      extent = renderExtent,
      generation = generation,
    )

  private fun create() {
    MemoryStack.stackPush().use { stack ->
      val externalImageInfo =
        VkExternalMemoryImageCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO)
          .handleTypes(VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT)
      val imageInfo =
        VkImageCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
          .pNext(externalImageInfo.address())
          .imageType(VK_IMAGE_TYPE_2D)
          .format(VK_FORMAT_B8G8R8A8_UNORM)
          .extent(
            VkExtent3D.calloc(stack)
              .width(storageExtent.physicalWidth)
              .height(storageExtent.physicalHeight)
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
      val handleProperties =
        VkMemoryWin32HandlePropertiesKHR.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR)
      check(
        vkGetMemoryWin32HandlePropertiesKHR(
          context.device(),
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT,
          sharedHandle,
          handleProperties,
        ),
        "vkGetMemoryWin32HandlePropertiesKHR",
      )
      val importInfo =
        VkImportMemoryWin32HandleInfoKHR.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR)
          .handleType(VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT)
          .handle(sharedHandle)
      val dedicated =
        VkMemoryDedicatedAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO)
          .image(image)
      importInfo.pNext(dedicated.address())
      val allocateInfo =
        VkMemoryAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
          .pNext(importInfo.address())
          .allocationSize(requirements.size())
          .memoryTypeIndex(
            findMemoryType(requirements.memoryTypeBits() and handleProperties.memoryTypeBits())
          )
      val memoryOut = stack.mallocLong(1)
      check(vkAllocateMemory(context.device(), allocateInfo, null, memoryOut), "vkAllocateMemory")
      memory = memoryOut[0]
      check(vkBindImageMemory(context.device(), image, memory, 0), "vkBindImageMemory")

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
      check(vkCreateImageView(context.device(), viewInfo, null, viewOut), "vkCreateImageView")
      view = viewOut[0]
    }
  }

  private fun findMemoryType(typeBits: Int): Int {
    MemoryStack.stackPush().use { stack ->
      val properties = VkPhysicalDeviceMemoryProperties.calloc(stack)
      vkGetPhysicalDeviceMemoryProperties(context.physicalDevice(), properties)
      for (index in 0..<properties.memoryTypeCount()) {
        val supported = (typeBits and (1 shl index)) != 0
        val hasProperties =
          (properties.memoryTypes(index).propertyFlags() and VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        if (supported && hasProperties) {
          return index
        }
      }
    }
    error("No compatible Vulkan memory type found for imported D3D12 resource")
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
    if (memory != NULL) {
      vkFreeMemory(context.device(), memory, null)
      memory = NULL
    }
  }

  companion object {
    fun create(
      context: WindowsVulkanContext,
      sharedHandle: Long,
      storageExtent: SurfaceExtent,
      renderExtent: SurfaceExtent,
    ): WindowsVulkanImportedD3D12Texture {
      val texture =
        WindowsVulkanImportedD3D12Texture(context, sharedHandle, storageExtent, renderExtent)
      try {
        texture.create()
        return texture
      } catch (error: RuntimeException) {
        texture.close()
        throw error
      }
    }

    private fun check(status: Int, operation: String) {
      check(status == VK_SUCCESS) { "$operation failed with Vulkan status $status" }
    }
  }
}

internal object WindowsD3D12Interop {
  private const val IID_ID3D12_DEVICE_DATA1 = 0x189819F1
  private const val IID_ID3D12_DEVICE_DATA2 = 0x1DB6
  private const val IID_ID3D12_DEVICE_DATA3 = 0x4B57
  private const val IID_ID3D12_RESOURCE_DATA1 = 0x696442BE
  private const val IID_ID3D12_RESOURCE_DATA2 = 0xA72E
  private const val IID_ID3D12_RESOURCE_DATA3 = 0x4059
  private const val GENERIC_ALL = 0x10000000
  private const val DIRECTX_DEVICE_RAW_DEVICE_OFFSET = 16L
  private const val D3D12_HEAP_TYPE_DEFAULT = 1
  private const val D3D12_HEAP_FLAG_SHARED = 0x1
  private const val D3D12_RESOURCE_DIMENSION_TEXTURE2D = 3
  private const val D3D12_RESOURCE_STATE_COMMON = 0
  private const val D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET = 0x1
  private const val D3D12_TEXTURE_LAYOUT_UNKNOWN = 0
  const val DXGI_FORMAT_R8G8B8A8_UNORM = 28
  const val DXGI_FORMAT_B8G8R8A8_UNORM = 87
  private const val ID3D12_DEVICE_CHILD_GET_DEVICE_INDEX = 7
  private const val ID3D12_DEVICE_CREATE_COMMITTED_RESOURCE_INDEX = 27
  private const val ID3D12_DEVICE_CREATE_SHARED_HANDLE_INDEX = 31
  private const val IUNKNOWN_RELEASE_INDEX = 2
  private val linker = Linker.nativeLinker()
  private val kernel32 = SymbolLookup.libraryLookup("kernel32", Arena.global())
  private val closeHandle =
    linker.downcallHandle(
      kernel32.findOrThrow("CloseHandle"),
      FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS),
    )

  fun createSharedTexture(
    device: SkikoDirect3DDevice,
    extent: SurfaceExtent,
    dxgiFormat: Int = DXGI_FORMAT_B8G8R8A8_UNORM,
  ): NativeHandle {
    check(!extent.isEmpty) { "Cannot create a D3D12 texture for an empty extent" }
    Arena.ofConfined().use { arena ->
      val rawDevice = rawD3D12Device(device)
      val resourceOut = arena.allocate(ValueLayout.ADDRESS)
      checkHResult(
        invokeHResult(
          comMethod(rawDevice, ID3D12_DEVICE_CREATE_COMMITTED_RESOURCE_INDEX),
          address(rawDevice),
          heapProperties(arena),
          D3D12_HEAP_FLAG_SHARED,
          textureDesc(arena, extent, dxgiFormat),
          D3D12_RESOURCE_STATE_COMMON,
          MemorySegment.NULL,
          iidId3D12Resource(arena),
          resourceOut,
        ),
        "ID3D12Device::CreateCommittedResource",
      )
      val resource = resourceOut.get(ValueLayout.ADDRESS, 0).address()
      check(resource != NULL) { "ID3D12Device::CreateCommittedResource returned null" }
      return NativeHandle(resource)
    }
  }

  fun createSharedHandle(resource: NativeHandle): Long {
    check(resource.address != 0L) { "Cannot share a null D3D12 resource" }
    Arena.ofConfined().use { arena ->
      val deviceOut = arena.allocate(ValueLayout.ADDRESS)
      checkHResult(
        invokeHResult(
          comMethod(resource.address, ID3D12_DEVICE_CHILD_GET_DEVICE_INDEX),
          address(resource.address),
          iidId3D12Device(arena),
          deviceOut,
        ),
        "ID3D12Resource::GetDevice",
      )
      val device = deviceOut.get(ValueLayout.ADDRESS, 0).address()
      try {
        val handleOut = arena.allocate(ValueLayout.ADDRESS)
        checkHResult(
          invokeHResult(
            comMethod(device, ID3D12_DEVICE_CREATE_SHARED_HANDLE_INDEX),
            address(device),
            address(resource.address),
            MemorySegment.NULL,
            GENERIC_ALL,
            MemorySegment.NULL,
            handleOut,
          ),
          "ID3D12Device::CreateSharedHandle",
        )
        val handle = handleOut.get(ValueLayout.ADDRESS, 0).address()
        check(handle != NULL) { "ID3D12Device::CreateSharedHandle returned a null handle" }
        return handle
      } finally {
        release(device)
      }
    }
  }

  fun release(resource: NativeHandle) {
    release(resource.address)
  }

  fun closeSharedHandle(handle: Long) {
    if (handle != NULL) {
      closeHandle.invokeWithArguments(address(handle))
    }
  }

  private fun rawD3D12Device(device: SkikoDirect3DDevice): Long {
    val rawDevice =
      address(device.ptr)
        .reinterpret(DIRECTX_DEVICE_RAW_DEVICE_OFFSET + Long.SIZE_BYTES)
        .get(ValueLayout.ADDRESS, DIRECTX_DEVICE_RAW_DEVICE_OFFSET)
        .address()
    check(rawDevice != NULL) { "Skiko Direct3D device wrapper did not expose ID3D12Device" }
    return rawDevice
  }

  private fun heapProperties(arena: Arena): MemorySegment {
    val props = arena.allocate(20)
    props.set(ValueLayout.JAVA_INT, 0, D3D12_HEAP_TYPE_DEFAULT)
    props.set(ValueLayout.JAVA_INT, 4, 0)
    props.set(ValueLayout.JAVA_INT, 8, 0)
    props.set(ValueLayout.JAVA_INT, 12, 1)
    props.set(ValueLayout.JAVA_INT, 16, 1)
    return props
  }

  private fun textureDesc(arena: Arena, extent: SurfaceExtent, dxgiFormat: Int): MemorySegment {
    val desc = arena.allocate(56)
    desc.set(ValueLayout.JAVA_INT, 0, D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    desc.set(ValueLayout.JAVA_LONG, 8, 0)
    desc.set(ValueLayout.JAVA_LONG, 16, extent.physicalWidth.toLong())
    desc.set(ValueLayout.JAVA_INT, 24, extent.physicalHeight)
    desc.set(ValueLayout.JAVA_SHORT, 28, 1.toShort())
    desc.set(ValueLayout.JAVA_SHORT, 30, 1.toShort())
    desc.set(ValueLayout.JAVA_INT, 32, dxgiFormat)
    desc.set(ValueLayout.JAVA_INT, 36, 1)
    desc.set(ValueLayout.JAVA_INT, 40, 0)
    desc.set(ValueLayout.JAVA_INT, 44, D3D12_TEXTURE_LAYOUT_UNKNOWN)
    desc.set(ValueLayout.JAVA_INT, 48, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    return desc
  }

  private fun iidId3D12Device(arena: Arena): MemorySegment {
    return guid(
      arena,
      IID_ID3D12_DEVICE_DATA1,
      IID_ID3D12_DEVICE_DATA2,
      IID_ID3D12_DEVICE_DATA3,
      0xBE,
      0x54,
      0x18,
      0x21,
      0x33,
      0x9B,
      0x85,
      0xF7,
    )
  }

  private fun iidId3D12Resource(arena: Arena): MemorySegment {
    return guid(
      arena,
      IID_ID3D12_RESOURCE_DATA1,
      IID_ID3D12_RESOURCE_DATA2,
      IID_ID3D12_RESOURCE_DATA3,
      0xBC,
      0x79,
      0x5B,
      0x5C,
      0x98,
      0x04,
      0x0F,
      0xAD,
    )
  }

  private fun guid(
    arena: Arena,
    data1: Int,
    data2: Int,
    data3: Int,
    vararg data4: Int,
  ): MemorySegment {
    val iid = arena.allocate(16)
    iid.set(ValueLayout.JAVA_INT, 0, data1)
    iid.set(ValueLayout.JAVA_SHORT, 4, data2.toShort())
    iid.set(ValueLayout.JAVA_SHORT, 6, data3.toShort())
    data4.forEachIndexed { index, value ->
      iid.set(ValueLayout.JAVA_BYTE, 8L + index, value.toByte())
    }
    return iid
  }

  private fun comMethod(instance: Long, index: Int): MemorySegment {
    val vtable = address(instance).reinterpret(Long.SIZE_BYTES.toLong()).get(ValueLayout.ADDRESS, 0)
    return vtable
      .reinterpret((index + 1L) * Long.SIZE_BYTES)
      .get(ValueLayout.ADDRESS, index * Long.SIZE_BYTES.toLong())
  }

  private fun release(instance: Long) {
    if (instance != NULL) {
      invokeInt(
        comMethod(instance, IUNKNOWN_RELEASE_INDEX),
        FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS),
        address(instance),
      )
    }
  }

  private fun invokeHResult(function: MemorySegment, vararg args: Any): Int =
    invokeInt(function, hresultDescriptor(args.size), *args)

  private fun invokeInt(
    function: MemorySegment,
    descriptor: FunctionDescriptor,
    vararg args: Any,
  ): Int = linker.downcallHandle(function, descriptor).invokeWithArguments(*args) as Int

  private fun hresultDescriptor(argumentCount: Int): FunctionDescriptor =
    when (argumentCount) {
      3 ->
        FunctionDescriptor.of(
          ValueLayout.JAVA_INT,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
        )
      6 ->
        FunctionDescriptor.of(
          ValueLayout.JAVA_INT,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
          ValueLayout.JAVA_INT,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
        )
      8 ->
        FunctionDescriptor.of(
          ValueLayout.JAVA_INT,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
          ValueLayout.JAVA_INT,
          ValueLayout.ADDRESS,
          ValueLayout.JAVA_INT,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
          ValueLayout.ADDRESS,
        )
      else -> error("Unsupported HRESULT function arity: $argumentCount")
    }

  private fun address(value: Long): MemorySegment = MemorySegment.ofAddress(value)

  private fun checkHResult(hr: Int, operation: String) {
    check(hr >= 0) { "$operation failed with HRESULT 0x${hr.toUInt().toString(16)}" }
  }
}
