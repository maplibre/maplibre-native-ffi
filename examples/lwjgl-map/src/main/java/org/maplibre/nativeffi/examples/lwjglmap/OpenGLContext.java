package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.GLFW_CLIENT_API;
import static org.lwjgl.glfw.GLFW.GLFW_OPENGL_API;
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
import static org.lwjgl.glfw.GLFWNativeWGL.glfwGetWGLContext;
import static org.lwjgl.glfw.GLFWNativeWin32.glfwGetWin32Window;
import static org.lwjgl.system.MemoryUtil.NULL;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.util.Locale;
import org.lwjgl.opengl.GL;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.WglContextDescriptor;

final class OpenGLContext implements AutoCloseable {
  private final long window;
  private long hwnd;
  private long hdc;
  private long hglrc;
  private boolean closed;

  private OpenGLContext(long window) {
    this.window = window;
  }

  static OpenGLContext create(String title, int width, int height) {
    if (!isWindows()) {
      // TODO(linux): Add an EGL/GLFW path after validating the Linux Mesa
      // llvmpipe environment on a Linux machine.
      throw new IllegalStateException("The OpenGL LWJGL example currently supports Windows WGL");
    }
    if (!glfwInit()) {
      throw new IllegalStateException("GLFW initialization failed");
    }
    long window;
    try {
      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      window = glfwCreateWindow(width, height, title, NULL, NULL);
      if (window == NULL) {
        throw new IllegalStateException("GLFW OpenGL window creation failed");
      }
    } catch (RuntimeException error) {
      glfwTerminate();
      throw error;
    }

    var context = new OpenGLContext(window);
    try {
      context.makeCurrent();
      context.hwnd = glfwGetWin32Window(window);
      context.hglrc = glfwGetWGLContext(window);
      context.hdc = getDc(context.hwnd);
      if (context.hwnd == NULL || context.hdc == NULL || context.hglrc == NULL) {
        throw new IllegalStateException(
            "GLFW did not expose WGL window, device, and context handles");
      }
      System.out.printf("GLFW %s, OpenGL WGL%n", glfwGetVersionString());
      return context;
    } catch (RuntimeException error) {
      context.close();
      throw error;
    }
  }

  long window() {
    return window;
  }

  WglContextDescriptor descriptor() {
    return new WglContextDescriptor(NativePointer.ofAddress(hdc), NativePointer.ofAddress(hglrc));
  }

  NativePointer surfacePointer() {
    return NativePointer.ofAddress(hdc);
  }

  void makeCurrent() {
    if (closed) {
      throw new IllegalStateException("OpenGL context is closed");
    }
    glfwMakeContextCurrent(window);
    GL.createCapabilities();
  }

  void swapBuffers() {
    glfwSwapBuffers(window);
  }

  @Override
  public void close() {
    if (closed) {
      return;
    }
    closed = true;
    if (window != NULL) {
      GL.setCapabilities(null);
      glfwMakeContextCurrent(NULL);
      if (hdc != NULL) {
        releaseDc(hwnd, hdc);
        hdc = NULL;
      }
      glfwDestroyWindow(window);
    }
    glfwTerminate();
  }

  private static boolean isWindows() {
    return System.getProperty("os.name").toLowerCase(Locale.ROOT).contains("windows");
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
