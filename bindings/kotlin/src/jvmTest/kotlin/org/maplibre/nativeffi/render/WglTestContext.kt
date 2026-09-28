package org.maplibre.nativeffi.render

import org.lwjgl.glfw.GLFW.*
import org.lwjgl.glfw.GLFWNativeWGL.glfwGetWGLContext
import org.lwjgl.glfw.GLFWNativeWin32.glfwGetWin32Window
import org.lwjgl.system.windows.User32
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.WglContextDescriptor

internal fun attachJvmWgl(
  map: MapHandle,
  width: Int,
  height: Int,
  depth: UInt,
): OwnedTextureTestSession {
  check(glfwInit()) { "GLFW initialization failed" }
  glfwDefaultWindowHints()
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3)
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE)
  val window = glfwCreateWindow(width, height, "Kotlin binding tests", 0, 0)
  if (window == 0L) {
    glfwTerminate()
    error("WGL window creation failed")
  }
  glfwMakeContextCurrent(window)
  val hwnd = glfwGetWin32Window(window)
  val dc = User32.GetDC(hwnd)
  val release = {
    User32.ReleaseDC(hwnd, dc)
    glfwDestroyWindow(window)
    glfwTerminate()
  }
  if (dc == 0L) {
    release()
    error("WGL device context creation failed")
  }
  val descriptor =
    WglContextDescriptor(
      NativePointer.ofAddress(dc),
      NativePointer.ofAddress(glfwGetWGLContext(window)),
      NativePointer.NULL_POINTER,
    )
  return attachOwnedTextureFixture(
    map,
    width,
    height,
    depth,
    attach = { target, w, h, options ->
      target.openglOwnedTextureAttach(
        OpenglOwnedTextureDescriptor(
          RenderTargetExtent(w.toUInt(), h.toUInt(), 1.0),
          OpenglContextDescriptor(
            OpenglContextOwnership.SHARED,
            OpenglContextDescriptorData.Wgl(descriptor),
          ),
        ),
        options,
      )
    },
    frameSize = { frame ->
      frame.withGetOpenglTexture { OwnedTextureFrameSize(it.width.toInt(), it.height.toInt()) }
    },
    releaseGraphics = release,
  )
}
