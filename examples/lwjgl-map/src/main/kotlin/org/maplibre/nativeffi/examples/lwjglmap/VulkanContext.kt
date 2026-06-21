package org.maplibre.nativeffi.examples.lwjglmap

import java.util.LinkedHashSet
import java.util.Locale
import org.lwjgl.PointerBuffer
import org.lwjgl.glfw.GLFW.GLFW_CLIENT_API
import org.lwjgl.glfw.GLFW.GLFW_NO_API
import org.lwjgl.glfw.GLFW.GLFW_PLATFORM
import org.lwjgl.glfw.GLFW.GLFW_PLATFORM_WAYLAND
import org.lwjgl.glfw.GLFW.GLFW_RESIZABLE
import org.lwjgl.glfw.GLFW.GLFW_TRUE
import org.lwjgl.glfw.GLFW.glfwCreateWindow
import org.lwjgl.glfw.GLFW.glfwDefaultWindowHints
import org.lwjgl.glfw.GLFW.glfwDestroyWindow
import org.lwjgl.glfw.GLFW.glfwGetPlatform
import org.lwjgl.glfw.GLFW.glfwGetVersionString
import org.lwjgl.glfw.GLFW.glfwInit
import org.lwjgl.glfw.GLFW.glfwInitHint
import org.lwjgl.glfw.GLFW.glfwTerminate
import org.lwjgl.glfw.GLFW.glfwWindowHint
import org.lwjgl.glfw.GLFWVulkan.glfwCreateWindowSurface
import org.lwjgl.glfw.GLFWVulkan.glfwGetRequiredInstanceExtensions
import org.lwjgl.glfw.GLFWVulkan.glfwVulkanSupported
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.vulkan.EXTDebugUtils.VK_EXT_DEBUG_UTILS_EXTENSION_NAME
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
import org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
import org.lwjgl.vulkan.KHRPortabilitySubset.VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
import org.lwjgl.vulkan.KHRSurface.vkDestroySurfaceKHR
import org.lwjgl.vulkan.KHRSurface.vkGetPhysicalDeviceSurfaceSupportKHR
import org.lwjgl.vulkan.KHRSwapchain.VK_KHR_SWAPCHAIN_EXTENSION_NAME
import org.lwjgl.vulkan.VK
import org.lwjgl.vulkan.VK10.VK_API_VERSION_1_0
import org.lwjgl.vulkan.VK10.VK_QUEUE_GRAPHICS_BIT
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_APPLICATION_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
import org.lwjgl.vulkan.VK10.VK_SUCCESS
import org.lwjgl.vulkan.VK10.vkCreateDevice
import org.lwjgl.vulkan.VK10.vkCreateInstance
import org.lwjgl.vulkan.VK10.vkDestroyDevice
import org.lwjgl.vulkan.VK10.vkDestroyInstance
import org.lwjgl.vulkan.VK10.vkDeviceWaitIdle
import org.lwjgl.vulkan.VK10.vkEnumerateDeviceExtensionProperties
import org.lwjgl.vulkan.VK10.vkEnumerateInstanceExtensionProperties
import org.lwjgl.vulkan.VK10.vkEnumeratePhysicalDevices
import org.lwjgl.vulkan.VK10.vkGetDeviceQueue
import org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceQueueFamilyProperties
import org.lwjgl.vulkan.VkApplicationInfo
import org.lwjgl.vulkan.VkDevice
import org.lwjgl.vulkan.VkDeviceCreateInfo
import org.lwjgl.vulkan.VkDeviceQueueCreateInfo
import org.lwjgl.vulkan.VkExtensionProperties
import org.lwjgl.vulkan.VkInstance
import org.lwjgl.vulkan.VkInstanceCreateInfo
import org.lwjgl.vulkan.VkPhysicalDevice
import org.lwjgl.vulkan.VkQueue
import org.lwjgl.vulkan.VkQueueFamilyProperties
import org.maplibre.nativeffi.render.RenderBackend

internal class VulkanContext private constructor(private val window: Long) : GraphicsContext {
  private var instance: VkInstance? = null
  private var surface = NULL
  private var physicalDevice: VkPhysicalDevice? = null
  private var device: VkDevice? = null
  private var graphicsQueue: VkQueue? = null
  private var graphicsQueueFamilyIndex = 0

  override fun window(): Long = window

  override fun backend(): RenderBackend = RenderBackend.VULKAN

  fun instanceAddress(): Long = instance().address()

  fun physicalDeviceAddress(): Long = physicalDevice().address()

  fun deviceAddress(): Long = device().address()

  fun graphicsQueueAddress(): Long = graphicsQueue().address()

  fun getInstanceProcAddrAddress(): Long {
    ensureVulkanFunctionProvider()
    return VK.getFunctionProvider().getFunctionAddress("vkGetInstanceProcAddr")
  }

  fun getDeviceProcAddrAddress(): Long {
    ensureVulkanFunctionProvider()
    return VK.getFunctionProvider().getFunctionAddress("vkGetDeviceProcAddr")
  }

  fun graphicsQueueFamilyIndex(): Int = graphicsQueueFamilyIndex

  fun surfaceAddress(): Long = surface

  fun instance(): VkInstance = checkNotNull(instance) { "Vulkan instance is not initialized" }

  fun surface(): Long = surface

  fun physicalDevice(): VkPhysicalDevice =
    checkNotNull(physicalDevice) { "Vulkan physical device is not initialized" }

  fun device(): VkDevice = checkNotNull(device) { "Vulkan device is not initialized" }

  fun graphicsQueue(): VkQueue =
    checkNotNull(graphicsQueue) { "Vulkan graphics queue is not initialized" }

  fun waitIdle() {
    device?.let { check(vkDeviceWaitIdle(it), "vkDeviceWaitIdle") }
  }

  private fun createInstance() {
    MemoryStack.stackPush().use { stack ->
      val required =
        glfwGetRequiredInstanceExtensions()
          ?: error("GLFW did not return Vulkan instance extensions")
      val available = instanceExtensions(stack)
      val extensions = LinkedHashSet<String>()
      for (index in 0..<required.remaining()) {
        extensions.add(required.getStringUTF8(index))
      }
      val enablePortability = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME in available
      if (enablePortability) {
        extensions.add(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
      }
      if (VK_EXT_DEBUG_UTILS_EXTENSION_NAME in available) {
        // Enable the extension when present so validation layers can attach during local runs.
        extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
      }
      val extensionBuffer = stringBuffer(stack, extensions)
      val app =
        VkApplicationInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_APPLICATION_INFO)
          .pApplicationName(stack.UTF8("lwjgl-map"))
          .pEngineName(stack.UTF8("maplibre-native-ffi"))
          .apiVersion(VK_API_VERSION_1_0)
      val createInfo =
        VkInstanceCreateInfo.calloc(stack)
          .sType(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
          .pApplicationInfo(app)
          .ppEnabledExtensionNames(extensionBuffer)
      if (enablePortability) {
        createInfo.flags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR)
      }
      val out = stack.mallocPointer(1)
      check(vkCreateInstance(createInfo, null, out), "vkCreateInstance")
      instance = VkInstance(out[0], createInfo)
      println("Enabled Vulkan instance extensions: $extensions")
    }
  }

  private fun createSurface() {
    MemoryStack.stackPush().use { stack ->
      val out = stack.mallocLong(1)
      check(glfwCreateWindowSurface(instance(), window, null, out), "glfwCreateWindowSurface")
      surface = out[0]
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
        val queueFamily = findGraphicsPresentQueueFamily(stack, candidate)
        if (queueFamily >= 0) {
          physicalDevice = candidate
          graphicsQueueFamilyIndex = queueFamily
          return
        }
      }
      error("No Vulkan device has a graphics queue that can present")
    }
  }

  private fun findGraphicsPresentQueueFamily(stack: MemoryStack, candidate: VkPhysicalDevice): Int {
    val count = stack.mallocInt(1)
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, null)
    val families = VkQueueFamilyProperties.calloc(count[0], stack)
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, families)
    val present = stack.mallocInt(1)
    for (index in 0..<families.capacity()) {
      val family = families[index]
      if ((family.queueFlags() and VK_QUEUE_GRAPHICS_BIT) == 0) {
        continue
      }
      check(
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, index, surface, present),
        "vkGetPhysicalDeviceSurfaceSupportKHR",
      )
      if (present[0] != 0) {
        return index
      }
    }
    return -1
  }

  private fun createDevice() {
    MemoryStack.stackPush().use { stack ->
      val extensions = LinkedHashSet<String>()
      extensions.add(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
      val deviceExtensions = deviceExtensions(stack, physicalDevice())
      check(VK_KHR_SWAPCHAIN_EXTENSION_NAME in deviceExtensions) {
        "Selected Vulkan device does not support VK_KHR_swapchain"
      }
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
      println("Enabled Vulkan device extensions: $extensions")
    }
  }

  override fun close() {
    device?.let {
      vkDeviceWaitIdle(it)
      vkDestroyDevice(it, null)
      device = null
    }
    if (surface != NULL) {
      vkDestroySurfaceKHR(instance(), surface, null)
      surface = NULL
    }
    instance?.let {
      vkDestroyInstance(it, null)
      instance = null
    }
    if (window != NULL) {
      glfwDestroyWindow(window)
    }
    glfwTerminate()
  }

  internal companion object {
    fun create(title: String, width: Int, height: Int): VulkanContext {
      selectWaylandOnLinux()
      check(glfwInit()) { "GLFW initialization failed" }
      val window: Long
      try {
        check(glfwVulkanSupported()) { "GLFW reports Vulkan is not supported" }
        validateWaylandOnLinux()
        glfwDefaultWindowHints()
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE)
        window = glfwCreateWindow(width, height, title, NULL, NULL)
        check(window != NULL) { "GLFW window creation failed" }
      } catch (error: RuntimeException) {
        glfwTerminate()
        throw error
      }
      val context = VulkanContext(window)
      try {
        context.createInstance()
        context.createSurface()
        context.pickPhysicalDeviceAndQueue()
        context.createDevice()
        System.out.printf(
          "GLFW %s, Vulkan queue family %d, platform %d%n",
          glfwGetVersionString(),
          context.graphicsQueueFamilyIndex,
          glfwGetPlatform(),
        )
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
      val names = LinkedHashSet<String>()
      for (prop in props) {
        names.add(prop.extensionNameString())
      }
      return names
    }

    @Suppress("SENSELESS_COMPARISON")
    private fun ensureVulkanFunctionProvider() {
      if (VK.getFunctionProvider() == null) {
        VK.create()
      }
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
      val names = LinkedHashSet<String>()
      for (prop in props) {
        names.add(prop.extensionNameString())
      }
      return names
    }

    private fun stringBuffer(stack: MemoryStack, values: Set<String>): PointerBuffer {
      val buffer = stack.mallocPointer(values.size)
      for (value in values) {
        buffer.put(stack.UTF8(value))
      }
      return buffer.flip()
    }

    private fun selectWaylandOnLinux() {
      if (!isLinux()) {
        return
      }
      if (!System.getenv("WAYLAND_DISPLAY").isNullOrBlank()) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND)
      }
    }

    private fun validateWaylandOnLinux() {
      if (!isLinux()) {
        return
      }
      if (System.getenv("WAYLAND_DISPLAY").isNullOrBlank()) {
        System.err.println(
          "WAYLAND_DISPLAY is not set; Linux runtime support for this example targets Wayland."
        )
        return
      }
      if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
        error("GLFW did not select Wayland; selected platform=${glfwGetPlatform()}")
      }
    }

    private fun isLinux(): Boolean =
      System.getProperty("os.name").lowercase(Locale.ROOT).contains("linux")

    private fun check(status: Int, operation: String) {
      check(status == VK_SUCCESS) { "$operation failed with Vulkan status $status" }
    }
  }
}
