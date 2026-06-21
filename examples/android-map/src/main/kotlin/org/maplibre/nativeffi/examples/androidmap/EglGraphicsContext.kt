package org.maplibre.nativeffi.examples.androidmap

import android.opengl.EGL14
import android.opengl.EGLConfig
import android.opengl.EGLContext
import android.opengl.EGLDisplay
import android.opengl.EGLSurface
import android.view.Surface
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLContextDescriptor

internal class EglGraphicsContext
private constructor(
  private val display: EGLDisplay,
  private val config: EGLConfig,
  private var shareContext: EGLContext,
  private var windowSurface: EGLSurface,
) : GraphicsContext {
  override val backendName: String = "opengl-egl"

  val descriptor: OpenGLContextDescriptor
    get() =
      EglContextDescriptor(
        NativePointer.ofAddress(display.nativeHandle),
        NativePointer.ofAddress(config.nativeHandle),
        NativePointer.ofAddress(shareContext.nativeHandle),
        NativePointer.NULL,
      )

  val surfacePointer: NativePointer
    get() = NativePointer.ofAddress(windowSurface.nativeHandle)

  override fun close() {
    if (windowSurface != EGL14.EGL_NO_SURFACE) {
      EGL14.eglDestroySurface(display, windowSurface)
      windowSurface = EGL14.EGL_NO_SURFACE
    }
    if (shareContext != EGL14.EGL_NO_CONTEXT) {
      EGL14.eglDestroyContext(display, shareContext)
      shareContext = EGL14.EGL_NO_CONTEXT
    }
    EGL14.eglTerminate(display)
    EGL14.eglReleaseThread()
  }

  companion object {
    fun create(surface: Surface): EglGraphicsContext {
      val display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY)
      check(display != EGL14.EGL_NO_DISPLAY) { "EGL display is unavailable" }
      val version = IntArray(2)
      eglCheck(EGL14.eglInitialize(display, version, 0, version, 1), "initialize EGL")
      eglCheck(EGL14.eglBindAPI(EGL14.EGL_OPENGL_ES_API), "bind OpenGL ES EGL API")

      val config = chooseConfig(display)
      val contextAttributes = intArrayOf(EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, EGL14.EGL_NONE)
      val context =
        EGL14.eglCreateContext(display, config, EGL14.EGL_NO_CONTEXT, contextAttributes, 0)
      check(context != EGL14.EGL_NO_CONTEXT) { "creating EGL share context failed" }

      val surfaceAttributes = intArrayOf(EGL14.EGL_NONE)
      val windowSurface =
        EGL14.eglCreateWindowSurface(display, config, surface, surfaceAttributes, 0)
      check(windowSurface != EGL14.EGL_NO_SURFACE) { "creating EGL window surface failed" }
      return EglGraphicsContext(display, config, context, windowSurface)
    }

    private fun chooseConfig(display: EGLDisplay): EGLConfig {
      val attributes =
        intArrayOf(
          EGL14.EGL_RENDERABLE_TYPE,
          EGL_OPENGL_ES3_BIT,
          EGL14.EGL_SURFACE_TYPE,
          EGL14.EGL_WINDOW_BIT,
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
      eglCheck(
        EGL14.eglChooseConfig(display, attributes, 0, configs, 0, configs.size, count, 0),
        "choose EGL config",
      )
      check(count[0] > 0 && configs[0] != null) {
        "no EGL config supports OpenGL ES 3 window rendering"
      }
      return configs[0]!!
    }

    private fun eglCheck(ok: Boolean, operation: String) {
      if (!ok) {
        error("$operation failed with EGL error 0x${EGL14.eglGetError().toString(16)}")
      }
    }

    private const val EGL_OPENGL_ES3_BIT = 0x00000040
  }
}
