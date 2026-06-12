package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.GLFW_CLIENT_API;
import static org.lwjgl.glfw.GLFW.GLFW_CONTEXT_CREATION_API;
import static org.lwjgl.glfw.GLFW.GLFW_CONTEXT_VERSION_MAJOR;
import static org.lwjgl.glfw.GLFW.GLFW_CONTEXT_VERSION_MINOR;
import static org.lwjgl.glfw.GLFW.GLFW_EGL_CONTEXT_API;
import static org.lwjgl.glfw.GLFW.GLFW_OPENGL_API;
import static org.lwjgl.glfw.GLFW.GLFW_OPENGL_ES_API;
import static org.lwjgl.glfw.GLFW.GLFW_RESIZABLE;
import static org.lwjgl.glfw.GLFW.GLFW_TRUE;
import static org.lwjgl.glfw.GLFW.glfwCreateWindow;
import static org.lwjgl.glfw.GLFW.glfwDefaultWindowHints;
import static org.lwjgl.glfw.GLFW.glfwDestroyWindow;
import static org.lwjgl.glfw.GLFW.glfwGetVersionString;
import static org.lwjgl.glfw.GLFW.glfwInit;
import static org.lwjgl.glfw.GLFW.glfwMakeContextCurrent;
import static org.lwjgl.glfw.GLFW.glfwSwapBuffers;
import static org.lwjgl.glfw.GLFW.glfwTerminate;
import static org.lwjgl.glfw.GLFW.glfwWindowHint;
import static org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLConfig;
import static org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLContext;
import static org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLDisplay;
import static org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLSurface;
import static org.lwjgl.glfw.GLFWNativeWGL.glfwGetWGLContext;
import static org.lwjgl.glfw.GLFWNativeWin32.glfwGetWin32Window;
import static org.lwjgl.system.MemoryUtil.NULL;

import java.util.Locale;
import org.lwjgl.PointerBuffer;
import org.lwjgl.opengl.GL;
import org.lwjgl.opengles.GLES;
import org.lwjgl.system.MemoryStack;
import org.lwjgl.system.windows.User32;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.render.EglContextDescriptor;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.OpenGLContextDescriptor;
import org.maplibre.nativeffi.render.OpenGLContextProvider;
import org.maplibre.nativeffi.render.RenderBackend;
import org.maplibre.nativeffi.render.WglContextDescriptor;

final class OpenGLContext implements GraphicsContext {
  private final boolean gles;
  private final long window;
  private long hdc;
  private long eglDisplay;
  private long eglConfig;
  private long eglSurface;
  private long eglContext;
  private boolean closed;

  private OpenGLContext(boolean gles, long window) {
    this.gles = gles;
    this.window = window;
  }

  static OpenGLContext create(String title, int width, int height) {
    if (isLinux()) {
      return createEgl(title, width, height);
    }
    if (isWindows()) {
      return createWgl(title, width, height);
    }
    throw new IllegalStateException("LWJGL OpenGL context is only supported on Linux and Windows");
  }

  private static OpenGLContext createEgl(String title, int width, int height) {
    if (!Maplibre.supportedOpenGLContextProviders().contains(OpenGLContextProvider.EGL)) {
      throw new IllegalStateException("Native library does not support EGL");
    }
    if (!glfwInit()) {
      throw new IllegalStateException("GLFW initialization failed");
    }
    long window = NULL;
    var context = new OpenGLContext(true, NULL);
    try (var stack = MemoryStack.stackPush()) {
      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      window = glfwCreateWindow(width, height, title, NULL, NULL);
      if (window == NULL) {
        throw new IllegalStateException("GLFW EGL window creation failed");
      }
      context = new OpenGLContext(true, window);
      context.makeCurrent();
      GLES.createCapabilities();
      context.eglDisplay = glfwGetEGLDisplay();
      context.eglContext = glfwGetEGLContext(window);
      context.eglSurface = glfwGetEGLSurface(window);
      PointerBuffer configs = stack.mallocPointer(1);
      if (!glfwGetEGLConfig(window, configs)) {
        throw new IllegalStateException("GLFW did not expose an EGL config");
      }
      context.eglConfig = configs.get(0);
      if (context.eglDisplay == NULL || context.eglContext == NULL || context.eglSurface == NULL) {
        throw new IllegalStateException("GLFW did not expose EGL handles");
      }
      System.out.printf("GLFW %s, OpenGL EGL/GLES%n", glfwGetVersionString());
      return context;
    } catch (RuntimeException error) {
      context.close();
      if (window != NULL && context.window != window) {
        glfwDestroyWindow(window);
        glfwTerminate();
      }
      throw error;
    }
  }

  private static OpenGLContext createWgl(String title, int width, int height) {
    if (!Maplibre.supportedOpenGLContextProviders().contains(OpenGLContextProvider.WGL)) {
      throw new IllegalStateException("Native library does not support WGL");
    }
    if (!glfwInit()) {
      throw new IllegalStateException("GLFW initialization failed");
    }
    var context = new OpenGLContext(false, NULL);
    try {
      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      var window = glfwCreateWindow(width, height, title, NULL, NULL);
      if (window == NULL) {
        throw new IllegalStateException("GLFW WGL window creation failed");
      }
      context = new OpenGLContext(false, window);
      context.makeCurrent();
      GL.createCapabilities();
      var hwnd = glfwGetWin32Window(window);
      var hglrc = glfwGetWGLContext(window);
      context.hdc = getDc(hwnd);
      if (context.hdc == NULL || hglrc == NULL) {
        throw new IllegalStateException("GLFW did not expose WGL handles");
      }
      System.out.printf("GLFW %s, OpenGL WGL%n", glfwGetVersionString());
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
    return RenderBackend.OPENGL;
  }

  boolean isGles() {
    return gles;
  }

  OpenGLContextDescriptor descriptor() {
    if (gles) {
      return new EglContextDescriptor(
          NativePointer.ofAddress(eglDisplay),
          NativePointer.ofAddress(eglConfig),
          NativePointer.ofAddress(eglContext));
    }
    return new WglContextDescriptor(
        NativePointer.ofAddress(hdc), NativePointer.ofAddress(glfwGetWGLContext(window)));
  }

  NativePointer surfacePointer() {
    return NativePointer.ofAddress(gles ? eglSurface : hdc);
  }

  void makeCurrent() {
    if (closed) {
      throw new IllegalStateException("OpenGL context is closed");
    }
    glfwMakeContextCurrent(window);
    if (gles) {
      GLES.createCapabilities();
    } else {
      GL.createCapabilities();
    }
  }

  void swapBuffers() {
    glfwSwapBuffers(window);
  }

  @Override
  public void resize(Viewport viewport) {
    makeCurrent();
  }

  @Override
  public void close() {
    if (closed) {
      return;
    }
    closed = true;
    if (window != NULL) {
      glfwMakeContextCurrent(window);
      if (gles) {
        org.lwjgl.opengles.GLES20.glFinish();
        GLES.setCapabilities(null);
      } else {
        org.lwjgl.opengl.GL11.glFinish();
        GL.setCapabilities(null);
      }
      glfwMakeContextCurrent(NULL);
      if (!gles && hdc != NULL) {
        releaseDc(glfwGetWin32Window(window), hdc);
        hdc = NULL;
      }
      glfwDestroyWindow(window);
    }
    glfwTerminate();
  }

  private static boolean isLinux() {
    return System.getProperty("os.name").toLowerCase(Locale.ROOT).contains("linux");
  }

  private static boolean isWindows() {
    return System.getProperty("os.name").toLowerCase(Locale.ROOT).contains("windows");
  }

  private static long getDc(long hwnd) {
    return User32.GetDC(hwnd);
  }

  private static void releaseDc(long hwnd, long hdc) {
    if (!User32.ReleaseDC(hwnd, hdc)) {
      throw new IllegalStateException("ReleaseDC failed");
    }
  }
}
