@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.COpaquePointerVar
import kotlinx.cinterop.IntVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.toCValues
import kotlinx.cinterop.toLong
import kotlinx.cinterop.value
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.internal.egl.EGL_ALPHA_SIZE
import org.maplibre.nativeffi.internal.egl.EGL_BLUE_SIZE
import org.maplibre.nativeffi.internal.egl.EGL_CONTEXT_CLIENT_VERSION
import org.maplibre.nativeffi.internal.egl.EGL_DEFAULT_DISPLAY
import org.maplibre.nativeffi.internal.egl.EGL_DEPTH_SIZE
import org.maplibre.nativeffi.internal.egl.EGL_GREEN_SIZE
import org.maplibre.nativeffi.internal.egl.EGL_HEIGHT
import org.maplibre.nativeffi.internal.egl.EGL_NONE
import org.maplibre.nativeffi.internal.egl.EGL_NO_CONTEXT
import org.maplibre.nativeffi.internal.egl.EGL_NO_DISPLAY
import org.maplibre.nativeffi.internal.egl.EGL_NO_SURFACE
import org.maplibre.nativeffi.internal.egl.EGL_OPENGL_ES3_BIT
import org.maplibre.nativeffi.internal.egl.EGL_OPENGL_ES_API
import org.maplibre.nativeffi.internal.egl.EGL_PBUFFER_BIT
import org.maplibre.nativeffi.internal.egl.EGL_PLATFORM_SURFACELESS_MESA
import org.maplibre.nativeffi.internal.egl.EGL_RED_SIZE
import org.maplibre.nativeffi.internal.egl.EGL_RENDERABLE_TYPE
import org.maplibre.nativeffi.internal.egl.EGL_STENCIL_SIZE
import org.maplibre.nativeffi.internal.egl.EGL_SURFACE_TYPE
import org.maplibre.nativeffi.internal.egl.EGL_WIDTH
import org.maplibre.nativeffi.internal.egl.eglBindAPI
import org.maplibre.nativeffi.internal.egl.eglChooseConfig
import org.maplibre.nativeffi.internal.egl.eglCreateContext
import org.maplibre.nativeffi.internal.egl.eglCreatePbufferSurface
import org.maplibre.nativeffi.internal.egl.eglDestroyContext
import org.maplibre.nativeffi.internal.egl.eglDestroySurface
import org.maplibre.nativeffi.internal.egl.eglGetDisplay
import org.maplibre.nativeffi.internal.egl.eglGetPlatformDisplay
import org.maplibre.nativeffi.internal.egl.eglInitialize
import org.maplibre.nativeffi.internal.egl.eglMakeCurrent
import org.maplibre.nativeffi.internal.egl.eglTerminate
import org.maplibre.nativeffi.map.MapHandle

internal actual object OwnedTextureTestSupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession? {
    if (RenderBackend.OPENGL !in Maplibre.supportedRenderBackends()) return null
    if (OpenGLContextProvider.EGL !in Maplibre.supportedOpenGLContextProviders()) return null
    return createEglSession(map, width, height)
  }
}

private class LinuxEglOwnedTextureSession(
  private val display: COpaquePointer,
  private val config: COpaquePointer,
  private val surface: COpaquePointer,
  private val context: COpaquePointer,
  override val session: RenderSessionHandle,
) : OwnedTextureTestSession {
  override fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionHandle {
    val descriptor =
      EglContextDescriptor(
        NativePointer.ofAddress(display.toLong()),
        NativePointer.ofAddress(config.toLong()),
        NativePointer.ofAddress(context.toLong()),
        NativePointer.NULL_POINTER,
      )
    return session
      .map()
      .attachOpenGLOwnedTexture(
        OpenGLOwnedTextureDescriptor(RenderTargetExtent(width, height, 1.0), descriptor)
      )
  }

  override fun acquireFrame(): OwnedTextureTestFrame {
    val handle = session.acquireOpenGLOwnedTextureFrame()
    val frame = handle.frame()
    return object : OwnedTextureTestFrame {
      override val width: Int
        get() = frame.width()

      override val height: Int
        get() = frame.height()

      override val isClosed: Boolean
        get() = handle.isClosed

      override fun close() {
        handle.close()
      }
    }
  }

  override fun close() {
    try {
      session.close()
    } finally {
      releaseEgl(display, surface, context)
    }
  }
}

private fun createEglSession(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession {
  val display = initializedDisplay()
  var surface: COpaquePointer? = EGL_NO_SURFACE
  var context: COpaquePointer? = EGL_NO_CONTEXT
  try {
    check(eglBindAPI(EGL_OPENGL_ES_API.toUInt()) != 0u) { "EGL bind OpenGL ES API failed" }
    val config = chooseConfig(display)
    memScoped {
      val contextAttributes = intArrayOf(EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE)
      context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes.toCValues())
      check(context != EGL_NO_CONTEXT && context != null) { "EGL context creation failed" }

      val surfaceAttributes = intArrayOf(EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE)
      surface = eglCreatePbufferSurface(display, config, surfaceAttributes.toCValues())
      check(surface != EGL_NO_SURFACE && surface != null) { "EGL pbuffer creation failed" }
      check(eglMakeCurrent(display, surface, surface, context) != 0u) { "EGL make current failed" }
    }
    val liveSurface = surface ?: error("EGL pbuffer creation failed")
    val liveContext = context ?: error("EGL context creation failed")
    val descriptor =
      EglContextDescriptor(
        NativePointer.ofAddress(display.toLong()),
        NativePointer.ofAddress(config.toLong()),
        NativePointer.ofAddress(liveContext.toLong()),
        NativePointer.NULL_POINTER,
      )
    val session =
      map.attachOpenGLOwnedTexture(
        OpenGLOwnedTextureDescriptor(RenderTargetExtent(width, height, 1.0), descriptor)
      )
    return LinuxEglOwnedTextureSession(display, config, liveSurface, liveContext, session)
  } catch (error: Throwable) {
    releaseEgl(display, surface, context)
    throw error
  }
}

private fun initializedDisplay(): COpaquePointer {
  val surfaceless =
    eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA.toUInt(), EGL_DEFAULT_DISPLAY, null)
  if (surfaceless != EGL_NO_DISPLAY && surfaceless != null && initializeDisplay(surfaceless)) {
    return surfaceless
  }
  val fallback = eglGetDisplay(EGL_DEFAULT_DISPLAY)
  check(fallback != EGL_NO_DISPLAY && fallback != null && initializeDisplay(fallback)) {
    "EGL pbuffer fixture failed on Linux"
  }
  return fallback
}

private fun initializeDisplay(display: COpaquePointer): Boolean = memScoped {
  val major = alloc<IntVar>()
  val minor = alloc<IntVar>()
  if (eglInitialize(display, major.ptr, minor.ptr) != 0u) return true
  eglTerminate(display)
  return false
}

private fun chooseConfig(display: COpaquePointer): COpaquePointer = memScoped {
  val attributes =
    intArrayOf(
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
      EGL_NONE,
    )
  val configs = alloc<COpaquePointerVar>()
  val count = alloc<IntVar>()
  check(
    eglChooseConfig(display, attributes.toCValues(), configs.ptr, 1, count.ptr) != 0u &&
      count.value > 0
  ) {
    "no EGL config supports OpenGL ES 3 pbuffer rendering"
  }
  return configs.value ?: error("EGL choose config returned no config")
}

private fun releaseEgl(
  display: COpaquePointer,
  surface: COpaquePointer?,
  context: COpaquePointer?,
) {
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)
  if (surface != null && surface != EGL_NO_SURFACE) {
    eglDestroySurface(display, surface)
  }
  if (context != null && context != EGL_NO_CONTEXT) {
    eglDestroyContext(display, context)
  }
  eglTerminate(display)
}
