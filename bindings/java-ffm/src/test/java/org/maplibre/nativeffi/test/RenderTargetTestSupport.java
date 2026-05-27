package org.maplibre.nativeffi.test;

import static org.lwjgl.egl.EGL10.EGL_ALPHA_SIZE;
import static org.lwjgl.egl.EGL10.EGL_BLUE_SIZE;
import static org.lwjgl.egl.EGL10.EGL_DEPTH_SIZE;
import static org.lwjgl.egl.EGL10.EGL_GREEN_SIZE;
import static org.lwjgl.egl.EGL10.EGL_HEIGHT;
import static org.lwjgl.egl.EGL10.EGL_NONE;
import static org.lwjgl.egl.EGL10.EGL_NO_CONTEXT;
import static org.lwjgl.egl.EGL10.EGL_NO_DISPLAY;
import static org.lwjgl.egl.EGL10.EGL_NO_SURFACE;
import static org.lwjgl.egl.EGL10.EGL_PBUFFER_BIT;
import static org.lwjgl.egl.EGL10.EGL_RED_SIZE;
import static org.lwjgl.egl.EGL10.EGL_STENCIL_SIZE;
import static org.lwjgl.egl.EGL10.EGL_SURFACE_TYPE;
import static org.lwjgl.egl.EGL10.EGL_WIDTH;
import static org.lwjgl.egl.EGL10.eglChooseConfig;
import static org.lwjgl.egl.EGL10.eglCreateContext;
import static org.lwjgl.egl.EGL10.eglCreatePbufferSurface;
import static org.lwjgl.egl.EGL10.eglDestroyContext;
import static org.lwjgl.egl.EGL10.eglDestroySurface;
import static org.lwjgl.egl.EGL10.eglGetDisplay;
import static org.lwjgl.egl.EGL10.eglInitialize;
import static org.lwjgl.egl.EGL10.eglMakeCurrent;
import static org.lwjgl.egl.EGL10.eglTerminate;
import static org.lwjgl.egl.EGL12.EGL_RENDERABLE_TYPE;
import static org.lwjgl.egl.EGL13.EGL_CONTEXT_CLIENT_VERSION;
import static org.lwjgl.egl.EGL14.EGL_DEFAULT_DISPLAY;
import static org.lwjgl.egl.EGL14.EGL_OPENGL_ES_API;
import static org.lwjgl.egl.EGL14.eglBindAPI;
import static org.lwjgl.egl.EGL15.EGL_OPENGL_ES3_BIT;
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
import static org.lwjgl.opengl.GL11.GL_FRONT;
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
import static org.lwjgl.opengl.GL11.glReadBuffer;
import static org.lwjgl.opengl.GL11.glReadPixels;
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
import org.lwjgl.egl.EGL;
import org.lwjgl.opengl.GL;
import org.lwjgl.opengles.GLES;
import org.lwjgl.opengles.GLES20;
import org.lwjgl.opengles.GLES30;
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
import org.maplibre.nativeffi.render.EglContextDescriptor;
import org.maplibre.nativeffi.render.MetalContextDescriptor;
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLContextProvider;
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor;
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

  public static RenderTargetTestSupport attachOpenGLSurface(
      MapHandle map, RenderTargetExtent extent) {
    var context = OpenGLTestContext.create(extent.width(), extent.height());
    try {
      return new RenderTargetTestSupport(
          map.attachOpenGLSurface(
              new OpenGLSurfaceDescriptor()
                  .extent(extent)
                  .context(context.descriptor())
                  .surface(context.surface())),
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

  public byte[] readOpenGLSurfaceRgba(int width, int height) {
    if (closed) {
      throw new IllegalStateException("RenderTargetTestSupport is closed");
    }
    if (context instanceof OpenGLTestContext openglContext) {
      return openglContext.readSurfaceRgba(width, height);
    }
    throw new IllegalStateException("Render target is not an OpenGL surface");
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
    protected long eglDisplay;
    protected long eglConfig;
    protected long eglSurface;
    protected long eglContext;
    protected boolean egl;
    protected boolean closed;

    static OpenGLTestContext create(int width, int height) {
      var osName = System.getProperty("os.name").toLowerCase(Locale.ROOT);
      if (osName.contains("windows")) {
        return createWgl(width, height);
      }
      if (osName.contains("linux")) {
        return createEgl(width, height);
      }
      throw new IllegalStateException(
          "OpenGL test context is only available on Windows WGL or Linux EGL");
    }

    private static OpenGLTestContext createWgl(int width, int height) {
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

    private static OpenGLTestContext createEgl(int width, int height) {
      if (!Maplibre.supportedOpenGLContextProviders().contains(OpenGLContextProvider.EGL)) {
        throw new IllegalStateException("Native library does not support EGL");
      }

      var context = new OpenGLTestContext();
      context.egl = true;
      context.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      if (context.eglDisplay == EGL_NO_DISPLAY) {
        throw new IllegalStateException("EGL display unavailable");
      }
      try (var stack = MemoryStack.stackPush()) {
        var major = stack.mallocInt(1);
        var minor = stack.mallocInt(1);
        if (!eglInitialize(context.eglDisplay, major, minor)) {
          throw new IllegalStateException("EGL initialization failed");
        }
        EGL.createDisplayCapabilities(context.eglDisplay, major.get(0), minor.get(0));
        if (!eglBindAPI(EGL_OPENGL_ES_API)) {
          throw new IllegalStateException("EGL OpenGL ES API binding failed");
        }

        var configAttributes =
            stack.ints(
                EGL_SURFACE_TYPE,
                EGL_PBUFFER_BIT,
                EGL_RENDERABLE_TYPE,
                EGL_OPENGL_ES3_BIT,
                EGL_RED_SIZE,
                8,
                EGL_GREEN_SIZE,
                8,
                EGL_BLUE_SIZE,
                8,
                EGL_ALPHA_SIZE,
                8,
                EGL_DEPTH_SIZE,
                24,
                EGL_STENCIL_SIZE,
                8,
                EGL_NONE);
        var configs = stack.mallocPointer(1);
        var configCount = stack.mallocInt(1);
        if (!eglChooseConfig(context.eglDisplay, configAttributes, configs, configCount)
            || configCount.get(0) == 0) {
          throw new IllegalStateException("EGL config unavailable");
        }
        context.eglConfig = configs.get(0);

        var contextAttributes = stack.ints(EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE);
        context.eglContext =
            eglCreateContext(
                context.eglDisplay, context.eglConfig, EGL_NO_CONTEXT, contextAttributes);
        if (context.eglContext == EGL_NO_CONTEXT) {
          throw new IllegalStateException("EGL context creation failed");
        }

        var surfaceAttributes = stack.ints(EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE);
        context.eglSurface =
            eglCreatePbufferSurface(context.eglDisplay, context.eglConfig, surfaceAttributes);
        if (context.eglSurface == EGL_NO_SURFACE) {
          throw new IllegalStateException("EGL pbuffer creation failed");
        }

        context.makeCurrent();
        return context;
      } catch (RuntimeException error) {
        context.close();
        throw error;
      }
    }

    final org.maplibre.nativeffi.render.OpenGLContextDescriptor descriptor() {
      if (egl) {
        return new EglContextDescriptor(
            NativePointer.ofAddress(eglDisplay),
            NativePointer.ofAddress(eglConfig),
            NativePointer.ofAddress(eglContext));
      }
      return new WglContextDescriptor(NativePointer.ofAddress(hdc), NativePointer.ofAddress(hglrc));
    }

    final NativePointer surface() {
      if (egl) {
        return NativePointer.ofAddress(eglSurface);
      }
      return NativePointer.ofAddress(hdc);
    }

    final void makeCurrent() {
      if (closed) {
        throw new IllegalStateException("OpenGL test context is closed");
      }
      if (egl) {
        if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
          throw new IllegalStateException("EGL make-current failed");
        }
        GLES.createCapabilities();
      } else {
        glfwMakeContextCurrent(window);
        GL.createCapabilities();
      }
    }

    final void checkGlError(String operation) {
      var error = egl ? GLES20.glGetError() : glGetError();
      if (error != GL_NO_ERROR) {
        throw new IllegalStateException(
            operation + " failed with OpenGL error 0x%x".formatted(error));
      }
    }

    final byte[] readSurfaceRgba(int width, int height) {
      makeCurrent();
      var pixels = BufferUtils.createByteBuffer(width * height * 4);
      if (egl) {
        GLES20.glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      } else {
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      }
      checkGlError("read OpenGL surface");
      var bytes = new byte[pixels.capacity()];
      pixels.get(0, bytes);
      return bytes;
    }

    @Override
    public void close() {
      if (closed) {
        return;
      }
      closed = true;
      if (egl) {
        if (eglDisplay != EGL_NO_DISPLAY) {
          eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
          if (eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(eglDisplay, eglSurface);
            eglSurface = EGL_NO_SURFACE;
          }
          if (eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(eglDisplay, eglContext);
            eglContext = EGL_NO_CONTEXT;
          }
          eglTerminate(eglDisplay);
          eglDisplay = EGL_NO_DISPLAY;
        }
        GLES.setCapabilities(null);
      } else if (window != NULL) {
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
      context.eglDisplay = base.eglDisplay;
      context.eglConfig = base.eglConfig;
      context.eglSurface = base.eglSurface;
      context.eglContext = base.eglContext;
      context.egl = base.egl;
      base.window = NULL;
      base.hdc = NULL;
      base.hglrc = NULL;
      base.eglDisplay = EGL_NO_DISPLAY;
      base.eglConfig = NULL;
      base.eglSurface = EGL_NO_SURFACE;
      base.eglContext = EGL_NO_CONTEXT;
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
      bindTexture(texture);
      readTexture(pixels);
      bindTexture(0);
      checkGlError("read OpenGL borrowed texture");
      var bytes = new byte[pixels.capacity()];
      pixels.get(0, bytes);
      return bytes;
    }

    @Override
    public void close() {
      if (texture != 0) {
        makeCurrent();
        if (egl) {
          GLES20.glDeleteTextures(texture);
        } else {
          glDeleteTextures(texture);
        }
        texture = 0;
      }
      super.close();
    }

    private void createTexture() {
      makeCurrent();
      texture = egl ? GLES20.glGenTextures() : glGenTextures();
      bindTexture(texture);
      texParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      texParameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      texImage2D();
      bindTexture(0);
      checkGlError("create OpenGL borrowed texture");
    }

    private void bindTexture(int texture) {
      if (egl) {
        GLES20.glBindTexture(GL_TEXTURE_2D, texture);
      } else {
        glBindTexture(GL_TEXTURE_2D, texture);
      }
    }

    private void texParameteri(int parameterName, int value) {
      if (egl) {
        GLES20.glTexParameteri(GL_TEXTURE_2D, parameterName, value);
      } else {
        glTexParameteri(GL_TEXTURE_2D, parameterName, value);
      }
    }

    private void texImage2D() {
      if (egl) {
        GLES30.glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            (ByteBuffer) null);
      } else {
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
      }
    }

    private void readTexture(ByteBuffer pixels) {
      if (egl) {
        var framebuffer = GLES20.glGenFramebuffers();
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer);
        try {
          GLES20.glFramebufferTexture2D(
              GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
          if (GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER)
              != GLES20.GL_FRAMEBUFFER_COMPLETE) {
            throw new IllegalStateException("OpenGL ES borrowed texture framebuffer incomplete");
          }
          GLES20.glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        } finally {
          GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
          GLES20.glDeleteFramebuffers(framebuffer);
        }
      } else {
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      }
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
