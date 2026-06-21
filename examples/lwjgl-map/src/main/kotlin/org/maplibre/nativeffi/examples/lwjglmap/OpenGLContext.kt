package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.glfw.GLFW.GLFW_CLIENT_API
import org.lwjgl.glfw.GLFW.GLFW_CONTEXT_CREATION_API
import org.lwjgl.glfw.GLFW.GLFW_CONTEXT_VERSION_MAJOR
import org.lwjgl.glfw.GLFW.GLFW_CONTEXT_VERSION_MINOR
import org.lwjgl.glfw.GLFW.GLFW_EGL_CONTEXT_API
import org.lwjgl.glfw.GLFW.GLFW_OPENGL_API
import org.lwjgl.glfw.GLFW.GLFW_OPENGL_ES_API
import org.lwjgl.glfw.GLFW.GLFW_RESIZABLE
import org.lwjgl.glfw.GLFW.GLFW_TRUE
import org.lwjgl.glfw.GLFW.glfwCreateWindow
import org.lwjgl.glfw.GLFW.glfwDefaultWindowHints
import org.lwjgl.glfw.GLFW.glfwDestroyWindow
import org.lwjgl.glfw.GLFW.glfwGetVersionString
import org.lwjgl.glfw.GLFW.glfwInit
import org.lwjgl.glfw.GLFW.glfwMakeContextCurrent
import org.lwjgl.glfw.GLFW.glfwSwapBuffers
import org.lwjgl.glfw.GLFW.glfwTerminate
import org.lwjgl.glfw.GLFW.glfwWindowHint
import org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLConfig
import org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLContext
import org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLDisplay
import org.lwjgl.glfw.GLFWNativeEGL.glfwGetEGLSurface
import org.lwjgl.glfw.GLFWNativeWGL.glfwGetWGLContext
import org.lwjgl.glfw.GLFWNativeWin32.glfwGetWin32Window
import org.lwjgl.opengl.GL
import org.lwjgl.opengl.GL11
import org.lwjgl.opengles.GLES
import org.lwjgl.opengles.GLES20
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.system.windows.User32
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend

internal class OpenGLContext private constructor(val isGles: Boolean, private val window: Long) :
  GraphicsContext {
  private var hdc = NULL
  private var eglDisplay = NULL
  private var eglConfig = NULL
  private var eglSurface = NULL
  private var eglContext = NULL
  private var closed = false

  override fun window(): Long = window

  override fun backend(): RenderBackend = RenderBackend.OPENGL

  fun hdcAddress(): Long = hdc

  fun eglDisplayAddress(): Long = eglDisplay

  fun eglConfigAddress(): Long = eglConfig

  fun eglContextAddress(): Long = eglContext

  fun wglContextAddress(): Long = glfwGetWGLContext(window)

  fun surfaceAddress(): Long = if (isGles) eglSurface else hdc

  fun makeCurrent() {
    check(!closed) { "OpenGL context is closed" }
    glfwMakeContextCurrent(window)
    if (isGles) {
      GLES.createCapabilities()
    } else {
      GL.createCapabilities()
    }
  }

  fun swapBuffers() {
    glfwSwapBuffers(window)
  }

  override fun resize(viewport: Viewport) {
    makeCurrent()
  }

  override fun close() {
    if (closed) {
      return
    }
    closed = true
    if (window != NULL) {
      glfwMakeContextCurrent(window)
      if (isGles) {
        GLES20.glFinish()
        GLES.setCapabilities(null)
      } else {
        GL11.glFinish()
        GL.setCapabilities(null)
      }
      glfwMakeContextCurrent(NULL)
      if (!isGles && hdc != NULL) {
        releaseDc(glfwGetWin32Window(window), hdc)
        hdc = NULL
      }
      glfwDestroyWindow(window)
    }
    glfwTerminate()
  }

  internal companion object {
    fun create(title: String, width: Int, height: Int): OpenGLContext {
      val providers = Maplibre.supportedOpenGLContextProviders()
      return when {
        OpenGLContextProvider.EGL in providers -> createEgl(title, width, height)
        OpenGLContextProvider.WGL in providers -> createWgl(title, width, height)
        else ->
          error(
            "The loaded MapLibre native library does not support an OpenGL context provider usable by lwjgl-map"
          )
      }
    }

    private fun createEgl(title: String, width: Int, height: Int): OpenGLContext {
      check(OpenGLContextProvider.EGL in Maplibre.supportedOpenGLContextProviders()) {
        "Native library does not support EGL"
      }
      check(glfwInit()) { "GLFW initialization failed" }
      var window = NULL
      var context = OpenGLContext(isGles = true, window = NULL)
      try {
        MemoryStack.stackPush().use { stack ->
          glfwDefaultWindowHints()
          glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API)
          glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API)
          glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3)
          glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0)
          glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE)
          window = glfwCreateWindow(width, height, title, NULL, NULL)
          check(window != NULL) { "GLFW EGL window creation failed" }
          context = OpenGLContext(isGles = true, window = window)
          context.makeCurrent()
          GLES.createCapabilities()
          context.eglDisplay = glfwGetEGLDisplay()
          context.eglContext = glfwGetEGLContext(window)
          context.eglSurface = glfwGetEGLSurface(window)
          val configs = stack.mallocPointer(1)
          check(glfwGetEGLConfig(window, configs)) { "GLFW did not expose an EGL config" }
          context.eglConfig = configs[0]
          check(
            context.eglDisplay != NULL && context.eglContext != NULL && context.eglSurface != NULL
          ) {
            "GLFW did not expose EGL handles"
          }
          System.out.printf("GLFW %s, OpenGL EGL/GLES%n", glfwGetVersionString())
          return context
        }
      } catch (error: RuntimeException) {
        context.close()
        if (window != NULL && context.window != window) {
          glfwDestroyWindow(window)
          glfwTerminate()
        }
        throw error
      }
    }

    private fun createWgl(title: String, width: Int, height: Int): OpenGLContext {
      check(OpenGLContextProvider.WGL in Maplibre.supportedOpenGLContextProviders()) {
        "Native library does not support WGL"
      }
      check(glfwInit()) { "GLFW initialization failed" }
      var context = OpenGLContext(isGles = false, window = NULL)
      try {
        glfwDefaultWindowHints()
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE)
        val window = glfwCreateWindow(width, height, title, NULL, NULL)
        check(window != NULL) { "GLFW WGL window creation failed" }
        context = OpenGLContext(isGles = false, window = window)
        context.makeCurrent()
        GL.createCapabilities()
        val hwnd = glfwGetWin32Window(window)
        val hglrc = glfwGetWGLContext(window)
        context.hdc = getDc(hwnd)
        check(context.hdc != NULL && hglrc != NULL) { "GLFW did not expose WGL handles" }
        System.out.printf("GLFW %s, OpenGL WGL%n", glfwGetVersionString())
        return context
      } catch (error: RuntimeException) {
        context.close()
        throw error
      }
    }

    private fun getDc(hwnd: Long): Long = User32.GetDC(hwnd)

    private fun releaseDc(hwnd: Long, hdc: Long) {
      check(User32.ReleaseDC(hwnd, hdc)) { "ReleaseDC failed" }
    }
  }
}
