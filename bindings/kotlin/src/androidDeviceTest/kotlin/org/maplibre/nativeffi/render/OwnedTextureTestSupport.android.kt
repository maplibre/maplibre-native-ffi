package org.maplibre.nativeffi.render

import android.opengl.EGL14
import android.opengl.EGLConfig
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.map.MapHandle

private const val EGL_OPENGL_ES3_BIT = 0x00000040

internal actual object OwnedTextureTestSupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession? {
    if (RenderBackend.OPENGL !in Maplibre.supportedRenderBackends()) return null
    return createEglSession(map, width, height)
  }
}

private class AndroidEglOwnedTextureSession(
  private val display: android.opengl.EGLDisplay,
  private val config: EGLConfig,
  private val surface: android.opengl.EGLSurface,
  private val context: android.opengl.EGLContext,
  override val session: RenderSessionHandle,
) : OwnedTextureTestSession {
  override fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionHandle {
    val descriptor =
      EglContextDescriptor(
        NativePointer.ofAddress(display.nativeHandle),
        NativePointer.ofAddress(config.nativeHandle),
        NativePointer.ofAddress(context.nativeHandle),
        NativePointer.NULL,
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
  val display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY)
  check(display != EGL14.EGL_NO_DISPLAY) { "EGL display is unavailable" }
  val version = IntArray(2)
  check(EGL14.eglInitialize(display, version, 0, version, 1)) { "EGL initialize failed" }

  var surface = EGL14.EGL_NO_SURFACE
  var context = EGL14.EGL_NO_CONTEXT
  try {
    check(EGL14.eglBindAPI(EGL14.EGL_OPENGL_ES_API)) { "EGL bind OpenGL ES API failed" }
    val config = chooseConfig(display)
    val contextAttributes = intArrayOf(EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, EGL14.EGL_NONE)
    context = EGL14.eglCreateContext(display, config, EGL14.EGL_NO_CONTEXT, contextAttributes, 0)
    check(context != EGL14.EGL_NO_CONTEXT) { "EGL context creation failed" }

    val surfaceAttributes =
      intArrayOf(EGL14.EGL_WIDTH, width, EGL14.EGL_HEIGHT, height, EGL14.EGL_NONE)
    surface = EGL14.eglCreatePbufferSurface(display, config, surfaceAttributes, 0)
    check(surface != EGL14.EGL_NO_SURFACE) { "EGL pbuffer creation failed" }
    check(EGL14.eglMakeCurrent(display, surface, surface, context)) { "EGL make current failed" }

    val descriptor =
      EglContextDescriptor(
        NativePointer.ofAddress(display.nativeHandle),
        NativePointer.ofAddress(config.nativeHandle),
        NativePointer.ofAddress(context.nativeHandle),
        NativePointer.NULL,
      )
    val session =
      map.attachOpenGLOwnedTexture(
        OpenGLOwnedTextureDescriptor(RenderTargetExtent(width, height, 1.0), descriptor)
      )
    return AndroidEglOwnedTextureSession(display, config, surface, context, session)
  } catch (error: Throwable) {
    releaseEgl(display, surface, context)
    throw error
  }
}

private fun releaseEgl(
  display: android.opengl.EGLDisplay,
  surface: android.opengl.EGLSurface,
  context: android.opengl.EGLContext,
) {
  EGL14.eglMakeCurrent(display, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT)
  if (surface != EGL14.EGL_NO_SURFACE) {
    EGL14.eglDestroySurface(display, surface)
  }
  if (context != EGL14.EGL_NO_CONTEXT) {
    EGL14.eglDestroyContext(display, context)
  }
  EGL14.eglTerminate(display)
  EGL14.eglReleaseThread()
}

private fun chooseConfig(display: android.opengl.EGLDisplay): EGLConfig {
  val attributes =
    intArrayOf(
      EGL14.EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES3_BIT,
      EGL14.EGL_SURFACE_TYPE,
      EGL14.EGL_PBUFFER_BIT,
      EGL14.EGL_RED_SIZE,
      8,
      EGL14.EGL_GREEN_SIZE,
      8,
      EGL14.EGL_BLUE_SIZE,
      8,
      EGL14.EGL_ALPHA_SIZE,
      8,
      EGL14.EGL_DEPTH_SIZE,
      24,
      EGL14.EGL_STENCIL_SIZE,
      8,
      EGL14.EGL_NONE,
    )
  val configs = arrayOfNulls<EGLConfig>(1)
  val count = IntArray(1)
  check(
    EGL14.eglChooseConfig(display, attributes, 0, configs, 0, configs.size, count, 0) &&
      count[0] > 0
  ) {
    "no EGL config supports OpenGL ES 3 pbuffer rendering"
  }
  return configs[0] ?: error("EGL choose config returned no config")
}
