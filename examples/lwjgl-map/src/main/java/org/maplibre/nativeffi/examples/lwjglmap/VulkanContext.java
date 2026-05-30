package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.GLFW_CLIENT_API;
import static org.lwjgl.glfw.GLFW.GLFW_NO_API;
import static org.lwjgl.glfw.GLFW.GLFW_PLATFORM;
import static org.lwjgl.glfw.GLFW.GLFW_PLATFORM_WAYLAND;
import static org.lwjgl.glfw.GLFW.GLFW_RESIZABLE;
import static org.lwjgl.glfw.GLFW.GLFW_TRUE;
import static org.lwjgl.glfw.GLFW.glfwCreateWindow;
import static org.lwjgl.glfw.GLFW.glfwDefaultWindowHints;
import static org.lwjgl.glfw.GLFW.glfwDestroyWindow;
import static org.lwjgl.glfw.GLFW.glfwGetPlatform;
import static org.lwjgl.glfw.GLFW.glfwGetVersionString;
import static org.lwjgl.glfw.GLFW.glfwInit;
import static org.lwjgl.glfw.GLFW.glfwInitHint;
import static org.lwjgl.glfw.GLFW.glfwTerminate;
import static org.lwjgl.glfw.GLFW.glfwWindowHint;
import static org.lwjgl.glfw.GLFWVulkan.glfwCreateWindowSurface;
import static org.lwjgl.glfw.GLFWVulkan.glfwGetRequiredInstanceExtensions;
import static org.lwjgl.glfw.GLFWVulkan.glfwVulkanSupported;
import static org.lwjgl.system.MemoryUtil.NULL;
import static org.lwjgl.vulkan.EXTDebugUtils.VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
import static org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
import static org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
import static org.lwjgl.vulkan.KHRPortabilitySubset.VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
import static org.lwjgl.vulkan.KHRSurface.vkDestroySurfaceKHR;
import static org.lwjgl.vulkan.KHRSurface.vkGetPhysicalDeviceSurfaceSupportKHR;
import static org.lwjgl.vulkan.KHRSwapchain.VK_KHR_SWAPCHAIN_EXTENSION_NAME;
import static org.lwjgl.vulkan.VK10.VK_API_VERSION_1_0;
import static org.lwjgl.vulkan.VK10.VK_QUEUE_GRAPHICS_BIT;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_APPLICATION_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
import static org.lwjgl.vulkan.VK10.VK_SUCCESS;
import static org.lwjgl.vulkan.VK10.vkCreateDevice;
import static org.lwjgl.vulkan.VK10.vkCreateInstance;
import static org.lwjgl.vulkan.VK10.vkDestroyDevice;
import static org.lwjgl.vulkan.VK10.vkDestroyInstance;
import static org.lwjgl.vulkan.VK10.vkDeviceWaitIdle;
import static org.lwjgl.vulkan.VK10.vkEnumerateDeviceExtensionProperties;
import static org.lwjgl.vulkan.VK10.vkEnumerateInstanceExtensionProperties;
import static org.lwjgl.vulkan.VK10.vkEnumeratePhysicalDevices;
import static org.lwjgl.vulkan.VK10.vkGetDeviceQueue;
import static org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceQueueFamilyProperties;

import java.util.LinkedHashSet;
import java.util.Locale;
import java.util.Set;
import org.lwjgl.PointerBuffer;
import org.lwjgl.system.MemoryStack;
import org.lwjgl.vulkan.VK;
import org.lwjgl.vulkan.VkApplicationInfo;
import org.lwjgl.vulkan.VkDevice;
import org.lwjgl.vulkan.VkDeviceCreateInfo;
import org.lwjgl.vulkan.VkDeviceQueueCreateInfo;
import org.lwjgl.vulkan.VkExtensionProperties;
import org.lwjgl.vulkan.VkInstance;
import org.lwjgl.vulkan.VkInstanceCreateInfo;
import org.lwjgl.vulkan.VkPhysicalDevice;
import org.lwjgl.vulkan.VkQueue;
import org.lwjgl.vulkan.VkQueueFamilyProperties;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.RenderBackend;

final class VulkanContext implements GraphicsContext {
  private final long window;
  private VkInstance instance;
  private long surface;
  private VkPhysicalDevice physicalDevice;
  private VkDevice device;
  private VkQueue graphicsQueue;
  private int graphicsQueueFamilyIndex;

  private VulkanContext(long window) {
    this.window = window;
  }

  static VulkanContext create(String title, int width, int height) {
    selectWaylandOnLinux();
    if (!glfwInit()) {
      throw new IllegalStateException("GLFW initialization failed");
    }
    long window;
    try {
      if (!glfwVulkanSupported()) {
        throw new IllegalStateException("GLFW reports Vulkan is not supported");
      }
      validateWaylandOnLinux();
      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      window = glfwCreateWindow(width, height, title, NULL, NULL);
      if (window == NULL) {
        throw new IllegalStateException("GLFW window creation failed");
      }
    } catch (RuntimeException error) {
      glfwTerminate();
      throw error;
    }
    var context = new VulkanContext(window);
    try {
      context.createInstance();
      context.createSurface();
      context.pickPhysicalDeviceAndQueue();
      context.createDevice();
      System.out.printf(
          "GLFW %s, Vulkan queue family %d, platform %d%n",
          glfwGetVersionString(), context.graphicsQueueFamilyIndex, glfwGetPlatform());
      return context;
    } catch (RuntimeException error) {
      context.close();
      throw error;
    }
  }

  @Override
  public long window() {
    return window;
  }

  @Override
  public RenderBackend backend() {
    return RenderBackend.VULKAN;
  }

  NativePointer instancePointer() {
    return NativePointer.ofAddress(instance.address());
  }

  NativePointer physicalDevicePointer() {
    return NativePointer.ofAddress(physicalDevice.address());
  }

  NativePointer devicePointer() {
    return NativePointer.ofAddress(device.address());
  }

  NativePointer graphicsQueuePointer() {
    return NativePointer.ofAddress(graphicsQueue.address());
  }

  NativePointer getInstanceProcAddrPointer() {
    ensureVulkanFunctionProvider();
    return NativePointer.ofAddress(
        VK.getFunctionProvider().getFunctionAddress("vkGetInstanceProcAddr"));
  }

  NativePointer getDeviceProcAddrPointer() {
    ensureVulkanFunctionProvider();
    return NativePointer.ofAddress(
        VK.getFunctionProvider().getFunctionAddress("vkGetDeviceProcAddr"));
  }

  int graphicsQueueFamilyIndex() {
    return graphicsQueueFamilyIndex;
  }

  NativePointer surfacePointer() {
    return NativePointer.ofAddress(surface);
  }

  VkInstance instance() {
    return instance;
  }

  long surface() {
    return surface;
  }

  VkPhysicalDevice physicalDevice() {
    return physicalDevice;
  }

  VkDevice device() {
    return device;
  }

  VkQueue graphicsQueue() {
    return graphicsQueue;
  }

  void waitIdle() {
    if (device != null) {
      check(vkDeviceWaitIdle(device), "vkDeviceWaitIdle");
    }
  }

  private void createInstance() {
    try (var stack = MemoryStack.stackPush()) {
      var required = glfwGetRequiredInstanceExtensions();
      if (required == null) {
        throw new IllegalStateException("GLFW did not return Vulkan instance extensions");
      }
      var available = instanceExtensions(stack);
      var extensions = new LinkedHashSet<String>();
      for (int i = 0; i < required.remaining(); i++) {
        extensions.add(required.getStringUTF8(i));
      }
      var enablePortability = available.contains(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      if (enablePortability) {
        extensions.add(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      }
      if (available.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        // Enable the extension when present so validation layers can attach during local runs.
        extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }
      var extensionBuffer = stringBuffer(stack, extensions);
      var app =
          VkApplicationInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_APPLICATION_INFO)
              .pApplicationName(stack.UTF8("lwjgl-map"))
              .pEngineName(stack.UTF8("maplibre-native-ffi"))
              .apiVersion(VK_API_VERSION_1_0);
      var createInfo =
          VkInstanceCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
              .pApplicationInfo(app)
              .ppEnabledExtensionNames(extensionBuffer);
      if (enablePortability) {
        createInfo.flags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR);
      }
      var out = stack.mallocPointer(1);
      check(vkCreateInstance(createInfo, null, out), "vkCreateInstance");
      instance = new VkInstance(out.get(0), createInfo);
      System.out.println("Enabled Vulkan instance extensions: " + extensions);
    }
  }

  private void createSurface() {
    try (var stack = MemoryStack.stackPush()) {
      var out = stack.mallocLong(1);
      check(glfwCreateWindowSurface(instance, window, null, out), "glfwCreateWindowSurface");
      surface = out.get(0);
    }
  }

  private void pickPhysicalDeviceAndQueue() {
    try (var stack = MemoryStack.stackPush()) {
      var count = stack.mallocInt(1);
      check(vkEnumeratePhysicalDevices(instance, count, null), "vkEnumeratePhysicalDevices(count)");
      if (count.get(0) == 0) {
        throw new IllegalStateException("No Vulkan physical devices found");
      }
      var devices = stack.mallocPointer(count.get(0));
      check(vkEnumeratePhysicalDevices(instance, count, devices), "vkEnumeratePhysicalDevices");
      for (int i = 0; i < devices.capacity(); i++) {
        var candidate = new VkPhysicalDevice(devices.get(i), instance);
        var queueFamily = findGraphicsPresentQueueFamily(stack, candidate);
        if (queueFamily >= 0) {
          physicalDevice = candidate;
          graphicsQueueFamilyIndex = queueFamily;
          return;
        }
      }
      throw new IllegalStateException("No Vulkan device has a graphics queue that can present");
    }
  }

  private int findGraphicsPresentQueueFamily(MemoryStack stack, VkPhysicalDevice candidate) {
    var count = stack.mallocInt(1);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, null);
    var families = VkQueueFamilyProperties.calloc(count.get(0), stack);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, families);
    var present = stack.mallocInt(1);
    for (int i = 0; i < families.capacity(); i++) {
      var family = families.get(i);
      if ((family.queueFlags() & VK_QUEUE_GRAPHICS_BIT) == 0) {
        continue;
      }
      check(
          vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, present),
          "vkGetPhysicalDeviceSurfaceSupportKHR");
      if (present.get(0) != 0) {
        return i;
      }
    }
    return -1;
  }

  private void createDevice() {
    try (var stack = MemoryStack.stackPush()) {
      var extensions = new LinkedHashSet<String>();
      extensions.add(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      var deviceExtensions = deviceExtensions(stack, physicalDevice);
      if (!deviceExtensions.contains(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        throw new IllegalStateException("Selected Vulkan device does not support VK_KHR_swapchain");
      }
      if (deviceExtensions.contains(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
        extensions.add(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
      }
      var priorities = stack.floats(1.0f);
      var queueInfo =
          VkDeviceQueueCreateInfo.calloc(1, stack)
              .sType(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO)
              .queueFamilyIndex(graphicsQueueFamilyIndex)
              .pQueuePriorities(priorities);
      var createInfo =
          VkDeviceCreateInfo.calloc(stack)
              .sType(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
              .pQueueCreateInfos(queueInfo)
              .ppEnabledExtensionNames(stringBuffer(stack, extensions));
      var out = stack.mallocPointer(1);
      check(vkCreateDevice(physicalDevice, createInfo, null, out), "vkCreateDevice");
      device = new VkDevice(out.get(0), physicalDevice, createInfo);
      var queueOut = stack.mallocPointer(1);
      vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, queueOut);
      graphicsQueue = new VkQueue(queueOut.get(0), device);
      System.out.println("Enabled Vulkan device extensions: " + extensions);
    }
  }

  private static Set<String> instanceExtensions(MemoryStack stack) {
    var count = stack.mallocInt(1);
    check(
        vkEnumerateInstanceExtensionProperties((String) null, count, null),
        "vkEnumerateInstanceExtensionProperties(count)");
    var props = VkExtensionProperties.calloc(count.get(0), stack);
    check(
        vkEnumerateInstanceExtensionProperties((String) null, count, props),
        "vkEnumerateInstanceExtensionProperties");
    var names = new LinkedHashSet<String>();
    for (var prop : props) {
      names.add(prop.extensionNameString());
    }
    return names;
  }

  private static void ensureVulkanFunctionProvider() {
    if (VK.getFunctionProvider() == null) {
      VK.create();
    }
  }

  private static Set<String> deviceExtensions(MemoryStack stack, VkPhysicalDevice device) {
    var count = stack.mallocInt(1);
    check(
        vkEnumerateDeviceExtensionProperties(device, (String) null, count, null),
        "vkEnumerateDeviceExtensionProperties(count)");
    var props = VkExtensionProperties.calloc(count.get(0), stack);
    check(
        vkEnumerateDeviceExtensionProperties(device, (String) null, count, props),
        "vkEnumerateDeviceExtensionProperties");
    var names = new LinkedHashSet<String>();
    for (var prop : props) {
      names.add(prop.extensionNameString());
    }
    return names;
  }

  private static PointerBuffer stringBuffer(MemoryStack stack, Set<String> values) {
    var buffer = stack.mallocPointer(values.size());
    for (var value : values) {
      buffer.put(stack.UTF8(value));
    }
    return buffer.flip();
  }

  private static void selectWaylandOnLinux() {
    if (!isLinux()) {
      return;
    }
    if (System.getenv("WAYLAND_DISPLAY") != null && !System.getenv("WAYLAND_DISPLAY").isBlank()) {
      glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    }
  }

  private static void validateWaylandOnLinux() {
    if (!isLinux()) {
      return;
    }
    if (System.getenv("WAYLAND_DISPLAY") == null || System.getenv("WAYLAND_DISPLAY").isBlank()) {
      System.err.println(
          "WAYLAND_DISPLAY is not set; Linux runtime support for this example targets Wayland.");
      return;
    }
    if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
      throw new IllegalStateException(
          "GLFW did not select Wayland; selected platform=" + glfwGetPlatform());
    }
  }

  private static boolean isLinux() {
    return System.getProperty("os.name").toLowerCase(Locale.ROOT).contains("linux");
  }

  private static void check(int status, String operation) {
    if (status != VK_SUCCESS) {
      throw new IllegalStateException(operation + " failed with Vulkan status " + status);
    }
  }

  @Override
  public void close() {
    if (device != null) {
      vkDeviceWaitIdle(device);
      vkDestroyDevice(device, null);
      device = null;
    }
    if (surface != NULL) {
      vkDestroySurfaceKHR(instance, surface, null);
      surface = NULL;
    }
    if (instance != null) {
      vkDestroyInstance(instance, null);
      instance = null;
    }
    if (window != NULL) {
      glfwDestroyWindow(window);
    }
    glfwTerminate();
  }
}
