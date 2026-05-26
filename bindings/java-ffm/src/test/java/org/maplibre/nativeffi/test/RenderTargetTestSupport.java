package org.maplibre.nativeffi.test;

import static org.lwjgl.glfw.GLFW.GLFW_CLIENT_API;
import static org.lwjgl.glfw.GLFW.GLFW_FALSE;
import static org.lwjgl.glfw.GLFW.GLFW_OPENGL_API;
import static org.lwjgl.glfw.GLFW.GLFW_VISIBLE;
import static org.lwjgl.glfw.GLFW.glfwCreateWindow;
import static org.lwjgl.glfw.GLFW.glfwDefaultWindowHints;
import static org.lwjgl.glfw.GLFW.glfwDestroyWindow;
import static org.lwjgl.glfw.GLFW.glfwInit;
import static org.lwjgl.glfw.GLFW.glfwMakeContextCurrent;
import static org.lwjgl.glfw.GLFW.glfwWindowHint;
import static org.lwjgl.glfw.GLFWNativeWGL.glfwGetWGLContext;
import static org.lwjgl.glfw.GLFWNativeWin32.glfwGetWin32Window;
import static org.lwjgl.opengl.GL11.GL_NEAREST;
import static org.lwjgl.opengl.GL11.GL_NO_ERROR;
import static org.lwjgl.opengl.GL11.GL_RGBA;
import static org.lwjgl.opengl.GL11.GL_RGBA8;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_2D;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER;
import static org.lwjgl.opengl.GL11.GL_UNSIGNED_BYTE;
import static org.lwjgl.opengl.GL11.glBindTexture;
import static org.lwjgl.opengl.GL11.glDeleteTextures;
import static org.lwjgl.opengl.GL11.glGenTextures;
import static org.lwjgl.opengl.GL11.glGetError;
import static org.lwjgl.opengl.GL11.glGetTexImage;
import static org.lwjgl.opengl.GL11.glTexImage2D;
import static org.lwjgl.opengl.GL11.glTexParameteri;
import static org.lwjgl.system.MemoryUtil.NULL;
import static org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
import static org.lwjgl.vulkan.KHRPortabilityEnumeration.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
import static org.lwjgl.vulkan.KHRPortabilitySubset.VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
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
import static org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceFeatures;
import static org.lwjgl.vulkan.VK10.vkGetPhysicalDeviceQueueFamilyProperties;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.nio.ByteBuffer;
import java.util.LinkedHashSet;
import java.util.Locale;
import java.util.Set;
import org.lwjgl.BufferUtils;
import org.lwjgl.PointerBuffer;
import org.lwjgl.opengl.GL;
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
import org.lwjgl.vulkan.VkPhysicalDeviceFeatures;
import org.lwjgl.vulkan.VkQueue;
import org.lwjgl.vulkan.VkQueueFamilyProperties;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.render.MetalContextDescriptor;
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLContextProvider;
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.RenderBackend;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.render.VulkanContextDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.WglContextDescriptor;

public final class RenderTargetTestSupport implements AutoCloseable {
  private final RenderSessionHandle session;
  private final AutoCloseable context;
  private boolean closed;

  private RenderTargetTestSupport(RenderSessionHandle session, AutoCloseable context) {
    this.session = session;
    this.context = context;
  }

  public static RenderTargetTestSupport attachOwnedTexture(
      MapHandle map, RenderTargetExtent extent) {
    var backends = Maplibre.supportedRenderBackends();
    if (backends.contains(RenderBackend.METAL)) {
      return attachMetalOwnedTexture(map, extent);
    }
    if (backends.contains(RenderBackend.VULKAN)) {
      return attachVulkanOwnedTexture(map, extent);
    }
    if (backends.contains(RenderBackend.OPENGL)) {
      return attachOpenGLOwnedTexture(map, extent);
    }
    throw new IllegalStateException("Native library does not support Metal, Vulkan, or OpenGL");
  }

  public static RenderTargetTestSupport attachMetalOwnedTexture(
      MapHandle map, RenderTargetExtent extent) {
    var context = MetalTestContext.create();
    try {
      return new RenderTargetTestSupport(
          map.attachMetalOwnedTexture(
              new MetalOwnedTextureDescriptor().extent(extent).context(context.descriptor())),
          context);
    } catch (RuntimeException | Error error) {
      closeContextAfterAttachFailure(context, error);
      throw error;
    }
  }

  public static RenderTargetTestSupport attachVulkanOwnedTexture(
      MapHandle map, RenderTargetExtent extent) {
    var context = VulkanTestContext.create();
    try {
      return new RenderTargetTestSupport(
          map.attachVulkanOwnedTexture(
              new VulkanOwnedTextureDescriptor().extent(extent).context(context.descriptor())),
          context);
    } catch (RuntimeException | Error error) {
      closeContextAfterAttachFailure(context, error);
      throw error;
    }
  }

  public static RenderTargetTestSupport attachOpenGLOwnedTexture(
      MapHandle map, RenderTargetExtent extent) {
    var context = OpenGLTestContext.create(8, 8);
    try {
      return new RenderTargetTestSupport(
          map.attachOpenGLOwnedTexture(
              new OpenGLOwnedTextureDescriptor().extent(extent).context(context.descriptor())),
          context);
    } catch (RuntimeException | Error error) {
      closeContextAfterAttachFailure(context, error);
      throw error;
    }
  }

  public static RenderTargetTestSupport attachOpenGLBorrowedTexture(
      MapHandle map, RenderTargetExtent extent) {
    var context = OpenGLBorrowedTextureContext.create(extent.width(), extent.height());
    try {
      return new RenderTargetTestSupport(
          map.attachOpenGLBorrowedTexture(
              new OpenGLBorrowedTextureDescriptor()
                  .extent(extent)
                  .context(context.descriptor())
                  .texture(context.texture())
                  .target(GL_TEXTURE_2D)),
          context);
    } catch (RuntimeException | Error error) {
      closeContextAfterAttachFailure(context, error);
      throw error;
    }
  }

  private static void closeContextAfterAttachFailure(AutoCloseable context, Throwable failure) {
    try {
      context.close();
    } catch (Exception closeError) {
      failure.addSuppressed(closeError);
    }
  }

  public RenderSessionHandle session() {
    if (closed) {
      throw new IllegalStateException("RenderTargetTestSupport is closed");
    }
    return session;
  }

  public byte[] readOpenGLBorrowedTextureRgba() {
    if (closed) {
      throw new IllegalStateException("RenderTargetTestSupport is closed");
    }
    if (context instanceof OpenGLBorrowedTextureContext openglContext) {
      return openglContext.readRgba();
    }
    throw new IllegalStateException("Render target is not an OpenGL borrowed texture");
  }

  @Override
  public void close() throws Exception {
    if (closed) {
      return;
    }
    closed = true;
    try {
      if (!session.isClosed()) {
        session.close();
      }
    } finally {
      context.close();
    }
  }

  private static final class MetalTestContext implements AutoCloseable {
    private final NativePointer device;

    private MetalTestContext(NativePointer device) {
      this.device = device;
    }

    static MetalTestContext create() {
      if (!System.getProperty("os.name").toLowerCase(Locale.ROOT).contains("mac")) {
        throw new IllegalStateException("Metal test context is only available on macOS");
      }
      try {
        var lookup =
            SymbolLookup.libraryLookup(
                "/System/Library/Frameworks/Metal.framework/Metal", Arena.global());
        var symbol =
            lookup
                .find("MTLCreateSystemDefaultDevice")
                .orElseThrow(
                    () -> new IllegalStateException("MTLCreateSystemDefaultDevice missing"));
        var handle =
            Linker.nativeLinker()
                .downcallHandle(symbol, FunctionDescriptor.of(ValueLayout.ADDRESS));
        var device = (MemorySegment) handle.invoke();
        if (device.equals(MemorySegment.NULL)) {
          throw new IllegalStateException("Metal did not return a default device");
        }
        return new MetalTestContext(NativePointer.ofAddress(device.address()));
      } catch (RuntimeException | Error error) {
        throw error;
      } catch (Throwable error) {
        throw new IllegalStateException("Failed to create Metal test context", error);
      }
    }

    MetalContextDescriptor descriptor() {
      return new MetalContextDescriptor(device);
    }

    @Override
    public void close() {}
  }

  private static class OpenGLTestContext implements AutoCloseable {
    protected long window;
    protected long hdc;
    protected long hglrc;
    protected boolean closed;

    static OpenGLTestContext create(int width, int height) {
      if (!System.getProperty("os.name").toLowerCase(Locale.ROOT).contains("windows")) {
        // TODO(linux): Add an EGL pbuffer/context helper for Java OpenGL
        // binding tests on a Linux machine with the CI EGL/llvmpipe stack.
        throw new IllegalStateException("OpenGL test context is only available on Windows WGL");
      }
      if (!Maplibre.supportedOpenGLContextProviders().contains(OpenGLContextProvider.WGL)) {
        throw new IllegalStateException("Native library does not support WGL");
      }
      if (!glfwInit()) {
        throw new IllegalStateException("GLFW initialization failed");
      }

      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
      glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
      var window = glfwCreateWindow(width, height, "MapLibre Java WGL Test", NULL, NULL);
      if (window == NULL) {
        throw new IllegalStateException("GLFW OpenGL window creation failed");
      }

      var context = new OpenGLTestContext();
      context.window = window;
      try {
        context.makeCurrent();
        GL.createCapabilities();
        var hwnd = glfwGetWin32Window(window);
        context.hglrc = glfwGetWGLContext(window);
        context.hdc = getDc(hwnd);
        if (context.hdc == NULL || context.hglrc == NULL) {
          throw new IllegalStateException("GLFW did not expose WGL handles");
        }
        return context;
      } catch (RuntimeException error) {
        context.close();
        throw error;
      }
    }

    final WglContextDescriptor descriptor() {
      return new WglContextDescriptor(NativePointer.ofAddress(hdc), NativePointer.ofAddress(hglrc));
    }

    final void makeCurrent() {
      if (closed) {
        throw new IllegalStateException("OpenGL test context is closed");
      }
      glfwMakeContextCurrent(window);
      GL.createCapabilities();
    }

    final void checkGlError(String operation) {
      var error = glGetError();
      if (error != GL_NO_ERROR) {
        throw new IllegalStateException(
            operation + " failed with OpenGL error 0x%x".formatted(error));
      }
    }

    @Override
    public void close() {
      if (closed) {
        return;
      }
      closed = true;
      if (window != NULL) {
        if (hdc != NULL) {
          releaseDc(glfwGetWin32Window(window), hdc);
          hdc = NULL;
        }
        GL.setCapabilities(null);
        glfwMakeContextCurrent(NULL);
        glfwDestroyWindow(window);
        window = NULL;
        hglrc = NULL;
      }
    }

    private static long getDc(long hwnd) {
      try {
        var hdc = (MemorySegment) loadGetDc().invoke(MemorySegment.ofAddress(hwnd));
        return hdc.address();
      } catch (Throwable error) {
        throw new IllegalStateException("GetDC failed", error);
      }
    }

    private static void releaseDc(long hwnd, long hdc) {
      try {
        loadReleaseDc().invoke(MemorySegment.ofAddress(hwnd), MemorySegment.ofAddress(hdc));
      } catch (Throwable error) {
        throw new IllegalStateException("ReleaseDC failed", error);
      }
    }

    private static MethodHandle loadGetDc() {
      return loadUser32("GetDC", FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
    }

    private static MethodHandle loadReleaseDc() {
      return loadUser32(
          "ReleaseDC",
          FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
    }

    private static MethodHandle loadUser32(String symbolName, FunctionDescriptor descriptor) {
      try {
        var symbol =
            SymbolLookup.libraryLookup("user32", Arena.global())
                .find(symbolName)
                .orElseThrow(() -> new IllegalStateException(symbolName + " missing"));
        return Linker.nativeLinker().downcallHandle(symbol, descriptor);
      } catch (RuntimeException | Error error) {
        throw error;
      } catch (Throwable error) {
        throw new IllegalStateException("Failed to load " + symbolName, error);
      }
    }
  }

  private static final class OpenGLBorrowedTextureContext extends OpenGLTestContext {
    private int texture;
    private int width;
    private int height;

    static OpenGLBorrowedTextureContext create(int width, int height) {
      var base = OpenGLTestContext.create(width, height);
      var context = new OpenGLBorrowedTextureContext();
      context.window = base.window;
      context.hdc = base.hdc;
      context.hglrc = base.hglrc;
      base.window = NULL;
      base.hdc = NULL;
      base.hglrc = NULL;
      base.closed = true;
      context.width = width;
      context.height = height;
      try {
        context.createTexture();
        return context;
      } catch (RuntimeException error) {
        context.close();
        throw error;
      }
    }

    int texture() {
      return texture;
    }

    byte[] readRgba() {
      makeCurrent();
      var pixels = BufferUtils.createByteBuffer(width * height * 4);
      glBindTexture(GL_TEXTURE_2D, texture);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      glBindTexture(GL_TEXTURE_2D, 0);
      checkGlError("read OpenGL borrowed texture");
      var bytes = new byte[pixels.capacity()];
      pixels.get(0, bytes);
      return bytes;
    }

    @Override
    public void close() {
      if (texture != 0) {
        makeCurrent();
        glDeleteTextures(texture);
        texture = 0;
      }
      super.close();
    }

    private void createTexture() {
      makeCurrent();
      texture = glGenTextures();
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RGBA8,
          width,
          height,
          0,
          GL_RGBA,
          GL_UNSIGNED_BYTE,
          (ByteBuffer) null);
      glBindTexture(GL_TEXTURE_2D, 0);
      checkGlError("create OpenGL borrowed texture");
    }
  }

  private static final class VulkanTestContext implements AutoCloseable {
    private VkInstance instance;
    private VkPhysicalDevice physicalDevice;
    private VkDevice device;
    private VkQueue graphicsQueue;
    private int graphicsQueueFamilyIndex;

    static VulkanTestContext create() {
      if (VK.getFunctionProvider() == null) {
        VK.create();
      }
      var context = new VulkanTestContext();
      try {
        context.createInstance();
        context.pickPhysicalDeviceAndQueue();
        context.createDevice();
        return context;
      } catch (RuntimeException error) {
        context.close();
        throw error;
      }
    }

    VulkanContextDescriptor descriptor() {
      return new VulkanContextDescriptor(
              NativePointer.ofAddress(instance.address()),
              NativePointer.ofAddress(physicalDevice.address()),
              NativePointer.ofAddress(device.address()),
              NativePointer.ofAddress(graphicsQueue.address()),
              graphicsQueueFamilyIndex)
          .procAddresses(
              NativePointer.ofAddress(
                  VK.getFunctionProvider().getFunctionAddress("vkGetInstanceProcAddr")),
              NativePointer.ofAddress(
                  VK.getFunctionProvider().getFunctionAddress("vkGetDeviceProcAddr")));
    }

    private void createInstance() {
      try (var stack = MemoryStack.stackPush()) {
        var available = instanceExtensions(stack);
        var extensions = new LinkedHashSet<String>();
        var flags = 0;
        if (available.contains(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
          extensions.add(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
          flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
        var app =
            VkApplicationInfo.calloc(stack)
                .sType(VK_STRUCTURE_TYPE_APPLICATION_INFO)
                .pApplicationName(stack.UTF8("maplibre-native-java-tests"))
                .pEngineName(stack.UTF8("maplibre-native-ffi"))
                .apiVersion(VK_API_VERSION_1_0);
        var createInfo =
            VkInstanceCreateInfo.calloc(stack)
                .sType(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
                .pApplicationInfo(app)
                .ppEnabledExtensionNames(stringBuffer(stack, extensions))
                .flags(flags);
        var out = stack.mallocPointer(1);
        check(vkCreateInstance(createInfo, null, out), "vkCreateInstance");
        instance = new VkInstance(out.get(0), createInfo);
      }
    }

    private void pickPhysicalDeviceAndQueue() {
      try (var stack = MemoryStack.stackPush()) {
        var count = stack.mallocInt(1);
        check(
            vkEnumeratePhysicalDevices(instance, count, null), "vkEnumeratePhysicalDevices(count)");
        if (count.get(0) == 0) {
          throw new IllegalStateException("No Vulkan physical devices found");
        }
        var devices = stack.mallocPointer(count.get(0));
        check(vkEnumeratePhysicalDevices(instance, count, devices), "vkEnumeratePhysicalDevices");
        for (int i = 0; i < devices.capacity(); i++) {
          var candidate = new VkPhysicalDevice(devices.get(i), instance);
          var queueFamily = findGraphicsQueueFamily(stack, candidate);
          if (queueFamily >= 0) {
            physicalDevice = candidate;
            graphicsQueueFamilyIndex = queueFamily;
            return;
          }
        }
      }
      throw new IllegalStateException("No Vulkan device has a graphics queue");
    }

    private int findGraphicsQueueFamily(MemoryStack stack, VkPhysicalDevice candidate) {
      var count = stack.mallocInt(1);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, null);
      var families = VkQueueFamilyProperties.calloc(count.get(0), stack);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, count, families);
      for (int i = 0; i < families.capacity(); i++) {
        var family = families.get(i);
        if (family.queueCount() > 0 && (family.queueFlags() & VK_QUEUE_GRAPHICS_BIT) != 0) {
          return i;
        }
      }
      return -1;
    }

    private void createDevice() {
      try (var stack = MemoryStack.stackPush()) {
        var extensions = new LinkedHashSet<String>();
        if (deviceExtensions(stack, physicalDevice)
            .contains(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
          extensions.add(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
        var supportedFeatures = VkPhysicalDeviceFeatures.calloc(stack);
        vkGetPhysicalDeviceFeatures(physicalDevice, supportedFeatures);
        var features =
            VkPhysicalDeviceFeatures.calloc(stack)
                .samplerAnisotropy(supportedFeatures.samplerAnisotropy())
                .wideLines(supportedFeatures.wideLines());
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
                .ppEnabledExtensionNames(stringBuffer(stack, extensions))
                .pEnabledFeatures(features);
        var out = stack.mallocPointer(1);
        check(vkCreateDevice(physicalDevice, createInfo, null, out), "vkCreateDevice");
        device = new VkDevice(out.get(0), physicalDevice, createInfo);
        var queueOut = stack.mallocPointer(1);
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, queueOut);
        graphicsQueue = new VkQueue(queueOut.get(0), device);
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
      if (instance != null) {
        vkDestroyInstance(instance, null);
        instance = null;
      }
    }
  }
}
