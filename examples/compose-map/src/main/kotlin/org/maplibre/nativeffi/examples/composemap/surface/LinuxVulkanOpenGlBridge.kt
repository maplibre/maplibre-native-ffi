package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import java.util.LinkedHashSet
import org.lwjgl.opengl.EXTMemoryObject.GL_DEDICATED_MEMORY_OBJECT_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_DEVICE_UUID_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_NUM_DEVICE_UUIDS_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_OPTIMAL_TILING_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_TEXTURE_TILING_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_UUID_SIZE_EXT
import org.lwjgl.opengl.EXTMemoryObject.glCreateMemoryObjectsEXT
import org.lwjgl.opengl.EXTMemoryObject.glDeleteMemoryObjectsEXT
import org.lwjgl.opengl.EXTMemoryObject.glGetUnsignedBytei_vEXT
import org.lwjgl.opengl.EXTMemoryObject.glMemoryObjectParameteriEXT
import org.lwjgl.opengl.EXTMemoryObject.glTexStorageMem2DEXT
import org.lwjgl.opengl.EXTMemoryObjectFD.GL_HANDLE_TYPE_OPAQUE_FD_EXT
import org.lwjgl.opengl.EXTMemoryObjectFD.glImportMemoryFdEXT
import org.lwjgl.opengl.GL
import org.lwjgl.opengl.GL11.GL_LINEAR
import org.lwjgl.opengl.GL11.GL_NO_ERROR
import org.lwjgl.opengl.GL11.GL_RGBA8
import org.lwjgl.opengl.GL11.GL_TEXTURE_2D
import org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER
import org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER
import org.lwjgl.opengl.GL11.GL_TEXTURE_WRAP_S
import org.lwjgl.opengl.GL11.GL_TEXTURE_WRAP_T
import org.lwjgl.opengl.GL11.GL_TRUE
import org.lwjgl.opengl.GL11.glBindTexture
import org.lwjgl.opengl.GL11.glDeleteTextures
import org.lwjgl.opengl.GL11.glFinish
import org.lwjgl.opengl.GL11.glGenTextures
import org.lwjgl.opengl.GL11.glGetError
import org.lwjgl.opengl.GL11.glGetInteger
import org.lwjgl.opengl.GL11.glTexParameteri
import org.lwjgl.opengl.GL12.GL_CLAMP_TO_EDGE
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.system.linux.UNISTD
import org.lwjgl.vulkan.EXTDebugUtils.VK_EXT_DEBUG_UTILS_EXTENSION_NAME
import org.lwjgl.vulkan.KHRExternalMemoryFd.VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME
import org.lwjgl.vulkan.KHRExternalMemoryFd.VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR
import org.lwjgl.vulkan.KHRExternalMemoryFd.vkGetMemoryFdKHR
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
import org.lwjgl.vulkan.KHRPortabilitySubset.VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
import org.lwjgl.vulkan.VK10.VK_FORMAT_R8G8B8A8_UNORM
import org.lwjgl.vulkan.VK10.VK_IMAGE_ASPECT_COLOR_BIT
import org.lwjgl.vulkan.VK10.VK_IMAGE_LAYOUT_GENERAL
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
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
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
import org.lwjgl.vulkan.VK10.vkEnumeratePhysicalDevices
import org.lwjgl.vulkan.VK10.vkFreeMemory
import org.lwjgl.vulkan.VK10.vkGetDeviceQueue
import org.lwjgl.vulkan.VK10.vkGetImageMemoryRequirements
import org.lwjgl.vulkan.VK11.VK_API_VERSION_1_1
import org.lwjgl.vulkan.VK11.VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
import org.lwjgl.vulkan.VK11.VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO
import org.lwjgl.vulkan.VK11.VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO
import org.lwjgl.vulkan.VK11.VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO
import org.lwjgl.vulkan.VK11.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES
import org.lwjgl.vulkan.VK11.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
import org.lwjgl.vulkan.VK11.vkGetPhysicalDeviceProperties2
import org.lwjgl.vulkan.VkApplicationInfo
import org.lwjgl.vulkan.VkDevice
import org.lwjgl.vulkan.VkDeviceCreateInfo
import org.lwjgl.vulkan.VkDeviceQueueCreateInfo
import org.lwjgl.vulkan.VkExportMemoryAllocateInfo
import org.lwjgl.vulkan.VkExtent3D
import org.lwjgl.vulkan.VkExternalMemoryImageCreateInfo
import org.lwjgl.vulkan.VkImageCreateInfo
import org.lwjgl.vulkan.VkImageSubresourceRange
import org.lwjgl.vulkan.VkImageViewCreateInfo
import org.lwjgl.vulkan.VkInstance
import org.lwjgl.vulkan.VkInstanceCreateInfo
import org.lwjgl.vulkan.VkMemoryAllocateInfo
import org.lwjgl.vulkan.VkMemoryDedicatedAllocateInfo
import org.lwjgl.vulkan.VkMemoryGetFdInfoKHR
import org.lwjgl.vulkan.VkMemoryRequirements
import org.lwjgl.vulkan.VkPhysicalDevice
import org.lwjgl.vulkan.VkPhysicalDeviceIDProperties
import org.lwjgl.vulkan.VkPhysicalDeviceProperties2
import org.lwjgl.vulkan.VkQueue

internal class LinuxVulkanOpenGlBridge : NativeSurfaceBridge {
  private val rendererDispatcher =
    NativeSurfaceRendererDispatcher("compose-map-linux-vulkan-renderer")
  private var vulkan: LinuxVulkanContext? = null
  private var exportedTexture: LinuxExportedVulkanTexture? = null
  private var importedTexture: LinuxOpenGlImportedTexture? = null
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val backend: ProducerBackend = ProducerBackend.VULKAN

  override val consumerBackend: ConsumerBackend = ConsumerBackend.OPENGL

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
    )

  override fun resize(extent: SurfaceExtent) {
    // Actual GL import must run while Skiko's OpenGL context is current, so resize is applied
    // lazily by acquireFrame inside the Compose draw callback.
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (importedTexture == null || exportedTexture == null || extent != currentExtent) {
      recreateTexture(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    rendererDispatcher.run { vulkan?.waitIdle() }
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run(action)

  override fun <T> withRendererAccess(action: () -> T): T = rendererDispatcher.run(action)

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is VulkanImageTarget) {
      return false
    }
    val texture = importedTexture ?: return false
    return SkikoHost.drawOpenGlTexture(scope, texture.target(target.generation))
  }

  override fun close() {
    try {
      disposeTexture(consumerContextCurrent = false)
    } finally {
      val closingVulkan = vulkan
      vulkan = null
      try {
        closingVulkan?.close()
      } finally {
        rendererDispatcher.close()
      }
    }
  }

  private fun target(generation: Long): NativeSurfaceTarget =
    checkNotNull(exportedTexture) { "Linux Vulkan texture is not initialized" }.target(generation)

  private fun recreateTexture(extent: SurfaceExtent) {
    if (extent.isEmpty) {
      disposeTexture(consumerContextCurrent = true)
      currentExtent = SurfaceExtent.Empty
      generation += 1
      return
    }

    disposeTexture(consumerContextCurrent = true)
    val context =
      vulkan ?: LinuxVulkanContext.create(currentOpenGlDeviceUuids()).also { vulkan = it }
    val exported = context.createExportedTexture(extent)
    try {
      val imported =
        LinuxOpenGlImportedTexture.create(exported.exportFd(), exported.memorySize(), extent)
      exportedTexture = exported
      importedTexture = imported
      currentExtent = extent
      generation += 1
    } catch (error: RuntimeException) {
      exported.close()
      throw error
    }
  }

  private fun disposeTexture(consumerContextCurrent: Boolean = true) {
    importedTexture?.let { texture ->
      if (consumerContextCurrent) {
        texture.close()
      } else {
        SkikoHost.withLinuxOpenGlContext { texture.close() }
      }
    }
    importedTexture = null
    exportedTexture?.close()
    exportedTexture = null
  }
}

internal fun currentOpenGlDeviceUuids(): Set<String> {
  val capabilities = ensureLwjglOpenGlCapabilities()
  if (!capabilities.GL_EXT_memory_object) {
    return emptySet()
  }
  val count = glGetInteger(GL_NUM_DEVICE_UUIDS_EXT)
  if (count <= 0) {
    return emptySet()
  }
  MemoryStack.stackPush().use { stack ->
    return (0..<count).mapTo(linkedSetOf()) { index ->
      val uuid = stack.malloc(GL_UUID_SIZE_EXT)
      glGetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT, index, uuid)
      uuid.toUuidHex(GL_UUID_SIZE_EXT)
    }
  }
}

internal class LinuxVulkanContext
private constructor(private val requiredDeviceUuids: Set<String>) : AutoCloseable {
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

  fun createExportedTexture(extent: SurfaceExtent): LinuxExportedVulkanTexture =
    LinuxExportedVulkanTexture.create(this, extent)

  fun waitIdle() {
    device?.let { checkVulkan(vkDeviceWaitIdle(it), "vkDeviceWaitIdle") }
  }

  internal fun physicalDevice(): VkPhysicalDevice =
    checkNotNull(physicalDevice) { "Vulkan physical device is not initialized" }

  internal fun device(): VkDevice = checkNotNull(device) { "Vulkan device is not initialized" }

  private fun instance(): VkInstance =
    checkNotNull(instance) { "Vulkan instance is not initialized" }

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
        if (VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME !in stack.vulkanDeviceExtensions(candidate)) {
          continue
        }
        if (requiredDeviceUuids.isNotEmpty() && deviceUuid(candidate) !in requiredDeviceUuids) {
          continue
        }
        val queueFamily = stack.findVulkanGraphicsQueueFamily(candidate)
        if (queueFamily >= 0) {
          physicalDevice = candidate
          graphicsQueueFamilyIndex = queueFamily
          return
        }
      }
      val deviceRequirement =
        if (requiredDeviceUuids.isEmpty()) "" else " and matches the Skiko OpenGL device UUID"
      error(
        "No Vulkan device supports graphics, $VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME$deviceRequirement"
      )
    }
  }

  private fun deviceUuid(candidate: VkPhysicalDevice): String {
    MemoryStack.stackPush().use { stack ->
      val id =
        VkPhysicalDeviceIDProperties.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES)
      val properties =
        VkPhysicalDeviceProperties2.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2)
          .pNext(id.address())
      vkGetPhysicalDeviceProperties2(candidate, properties)
      return id.deviceUUID().toUuidHex(GL_UUID_SIZE_EXT)
    }
  }

  private fun createDevice() {
    MemoryStack.stackPush().use { stack ->
      val deviceExtensions = stack.vulkanDeviceExtensions(physicalDevice())
      check(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME in deviceExtensions) {
        "Selected Vulkan device does not support $VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME"
      }
      val extensions = LinkedHashSet<String>()
      extensions.add(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME)
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
    fun create(requiredDeviceUuids: Set<String> = emptySet()): LinuxVulkanContext {
      val context = LinuxVulkanContext(requiredDeviceUuids)
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

internal class LinuxExportedVulkanTexture
private constructor(private val context: LinuxVulkanContext, private val extent: SurfaceExtent) :
  AutoCloseable {
  private var image = NULL
  private var memory = NULL
  private var view = NULL
  private var memorySize = 0L

  fun memorySize(): Long = memorySize

  fun exportFd(): Int {
    MemoryStack.stackPush().use { stack ->
      val fdInfo =
        VkMemoryGetFdInfoKHR.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR)
          .memory(memory)
          .handleType(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
      val fdOut = stack.mallocInt(1)
      checkVulkan(vkGetMemoryFdKHR(context.device(), fdInfo, fdOut), "vkGetMemoryFdKHR")
      return fdOut[0]
    }
  }

  fun target(generation: Long): VulkanImageTarget =
    VulkanImageTarget(
      context = context.handles,
      image = NativeHandle(image),
      imageView = NativeHandle(view),
      format = VK_FORMAT_R8G8B8A8_UNORM,
      initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      finalLayout = VK_IMAGE_LAYOUT_GENERAL,
      queueFamilyIndex = context.handles.graphicsQueueFamilyIndex,
      extent = extent,
      generation = generation,
    )

  private fun create() {
    MemoryStack.stackPush().use { stack ->
      val externalImageInfo =
        VkExternalMemoryImageCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO)
          .handleTypes(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
      val imageInfo =
        VkImageCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
          .pNext(externalImageInfo.address())
          .imageType(VK_IMAGE_TYPE_2D)
          .format(VK_FORMAT_R8G8B8A8_UNORM)
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

      val requirements = VkMemoryRequirements.calloc(stack)
      vkGetImageMemoryRequirements(context.device(), image, requirements)
      memorySize = requirements.size()
      val exportMemory =
        VkExportMemoryAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO)
          .handleTypes(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)
      val dedicated =
        VkMemoryDedicatedAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO)
          .image(image)
      exportMemory.pNext(dedicated.address())
      val allocateInfo =
        VkMemoryAllocateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
          .pNext(exportMemory.address())
          .allocationSize(requirements.size())
          .memoryTypeIndex(
            findVulkanDeviceLocalMemoryType(
              context.physicalDevice(),
              requirements.memoryTypeBits(),
              "No compatible Vulkan memory type found",
            )
          )
      val memoryOut = stack.mallocLong(1)
      checkVulkan(
        vkAllocateMemory(context.device(), allocateInfo, null, memoryOut),
        "vkAllocateMemory",
      )
      memory = memoryOut[0]
      checkVulkan(vkBindImageMemory(context.device(), image, memory, 0), "vkBindImageMemory")

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
    if (memory != NULL) {
      vkFreeMemory(context.device(), memory, null)
      memory = NULL
    }
  }

  companion object {
    fun create(context: LinuxVulkanContext, extent: SurfaceExtent): LinuxExportedVulkanTexture {
      val texture = LinuxExportedVulkanTexture(context, extent)
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

internal class LinuxOpenGlImportedTexture
private constructor(
  private val fd: Int,
  private val memorySize: Long,
  private val extent: SurfaceExtent,
  private val origin: TextureOrigin,
) : AutoCloseable {
  private var memoryObject = 0
  private var textureName = 0

  fun target(generation: Long): OpenGlTextureTarget =
    OpenGlTextureTarget(
      context =
        EglContextHandles(NativeHandle(0), NativeHandle(0), NativeHandle(0), NativeHandle(0)),
      textureName = textureName,
      textureTarget = GL_TEXTURE_2D,
      format = GL_RGBA8,
      origin = origin,
      contextProvider = OpenGlContextProvider {},
      extent = extent,
      generation = generation,
    )

  private fun create() {
    val capabilities = ensureLwjglOpenGlCapabilities()
    check(capabilities.GL_EXT_memory_object) {
      "Skiko OpenGL context does not expose GL_EXT_memory_object"
    }
    check(capabilities.GL_EXT_memory_object_fd) {
      "Skiko OpenGL context does not expose GL_EXT_memory_object_fd"
    }

    var imported = false
    try {
      memoryObject = glCreateMemoryObjectsEXT()
      glMemoryObjectParameteriEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT, GL_TRUE)
      glImportMemoryFdEXT(memoryObject, memorySize, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd)
      checkGl("glImportMemoryFdEXT")
      imported = true

      textureName = glGenTextures()
      glBindTexture(GL_TEXTURE_2D, textureName)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_OPTIMAL_TILING_EXT)
      glTexStorageMem2DEXT(
        GL_TEXTURE_2D,
        1,
        GL_RGBA8,
        extent.physicalWidth,
        extent.physicalHeight,
        memoryObject,
        0,
      )
      glBindTexture(GL_TEXTURE_2D, 0)
      checkGl("glTexStorageMem2DEXT")
    } catch (error: RuntimeException) {
      if (!imported) {
        closeFd(fd)
      }
      throw error
    }
  }

  override fun close() {
    runCatching {
      ensureLwjglOpenGlCapabilities()
      glFinish()
      if (textureName != 0) {
        SkikoHost.forgetOpenGlTexture(textureName)
        glDeleteTextures(textureName)
        textureName = 0
      }
      if (memoryObject != 0) {
        glDeleteMemoryObjectsEXT(memoryObject)
        memoryObject = 0
      }
    }
  }

  companion object {
    fun create(
      fd: Int,
      memorySize: Long,
      extent: SurfaceExtent,
      origin: TextureOrigin = TextureOrigin.TOP_LEFT,
    ): LinuxOpenGlImportedTexture {
      val imported = LinuxOpenGlImportedTexture(fd, memorySize, extent, origin)
      try {
        imported.create()
        return imported
      } catch (error: RuntimeException) {
        imported.close()
        throw error
      }
    }
  }
}

internal fun ensureLwjglOpenGlCapabilities() =
  runCatching { GL.getCapabilities() }.getOrNull() ?: GL.createCapabilities()

private fun checkGl(operation: String) {
  val error = glGetError()
  check(error == GL_NO_ERROR) { "$operation failed with GL error 0x${error.toString(16)}" }
}

internal fun closeFd(fd: Int) {
  if (fd >= 0) {
    runCatching { UNISTD.close(null, fd) }
  }
}
