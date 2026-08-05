package org.maplibre.nativeffi.examples.androidmap

import android.opengl.EGL14
import android.opengl.EGLConfig
import android.opengl.EGLContext
import android.opengl.EGLDisplay
import android.opengl.EGLSurface
import android.util.Log
import android.view.Surface
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLContextDescriptor

/**
 * The EGL context. The display, the config, and the share context survive a window that is
 * destroyed and recreated; only the window surface is rebuilt, so a session attached against this
 * context is pointed at the replacement rather than closed.
 */
internal class EglGraphicsContext
private constructor(
  private val display: EGLDisplay,
  private val config: EGLConfig,
  private var shareContext: EGLContext,
  private var windowSurface: EGLSurface,
) : GraphicsContext {
  override val backendName: String = "opengl-egl"

  /**
   * Where a parked session presents while the platform has taken the window away. An EGL session
   * always names a surface, and this one stays valid with no window behind it, so the session has
   * something to make current when it closes. Nothing renders into it.
   */
  private var parkedSurface: EGLSurface = EGL14.EGL_NO_SURFACE

  override val hasSurface: Boolean
    get() = windowSurface != EGL14.EGL_NO_SURFACE

  val descriptor: OpenGLContextDescriptor
    get() =
      EglContextDescriptor(
        NativePointer.ofAddress(display.nativeHandle),
        NativePointer.ofAddress(config.nativeHandle),
        NativePointer.ofAddress(shareContext.nativeHandle),
        NativePointer.NULL,
      )

  /** The EGL surface a session presents through: the host window, or the parking surface. */
  val surfacePointer: NativePointer
    get() = NativePointer.ofAddress((if (hasSurface) windowSurface else parkedSurface).nativeHandle)

  override fun setSurface(surface: Surface): Boolean {
    if (hasSurface) {
      // An EGL window surface follows its window's size, so a size or format change rebuilds
      // nothing here; the session takes the new extent through the descriptor.
      return true
    }
    val next = EGL14.eglCreateWindowSurface(display, config, surface, WINDOW_ATTRIBUTES, 0)
    if (next == EGL14.EGL_NO_SURFACE) {
      // A display that can no longer serve a window surface is what a lost EGL context looks like,
      // so report it and let the caller rebuild.
      Log.w(TAG, eglFailure("creating an EGL window surface"))
      return false
    }
    windowSurface = next
    return true
  }

  override fun releaseSurface(): Boolean {
    if (!hasSurface) {
      return true
    }
    if (!ensureParkedSurface()) {
      // Nowhere for a session to park, so this context cannot outlive the window.
      return false
    }
    EGL14.eglDestroySurface(display, windowSurface)
    windowSurface = EGL14.EGL_NO_SURFACE
    return true
  }

  override fun close() {
    if (windowSurface != EGL14.EGL_NO_SURFACE) {
      EGL14.eglDestroySurface(display, windowSurface)
      windowSurface = EGL14.EGL_NO_SURFACE
    }
    if (parkedSurface != EGL14.EGL_NO_SURFACE) {
      EGL14.eglDestroySurface(display, parkedSurface)
      parkedSurface = EGL14.EGL_NO_SURFACE
    }
    if (shareContext != EGL14.EGL_NO_CONTEXT) {
      EGL14.eglDestroyContext(display, shareContext)
      shareContext = EGL14.EGL_NO_CONTEXT
    }
    EGL14.eglTerminate(display)
    EGL14.eglReleaseThread()
  }

  private fun ensureParkedSurface(): Boolean {
    if (parkedSurface != EGL14.EGL_NO_SURFACE) {
      return true
    }
    val attributes = intArrayOf(EGL14.EGL_WIDTH, 1, EGL14.EGL_HEIGHT, 1, EGL14.EGL_NONE)
    parkedSurface = EGL14.eglCreatePbufferSurface(display, config, attributes, 0)
    if (parkedSurface == EGL14.EGL_NO_SURFACE) {
      Log.w(TAG, eglFailure("creating the EGL parking surface"))
      return false
    }
    return true
  }

  companion object {
    private const val TAG = "MapLibreAndroidMap"

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

      val windowSurface =
        EGL14.eglCreateWindowSurface(display, config, surface, WINDOW_ATTRIBUTES, 0)
      check(windowSurface != EGL14.EGL_NO_SURFACE) { "creating EGL window surface failed" }
      return EglGraphicsContext(display, config, context, windowSurface)
    }

    private fun chooseConfig(display: EGLDisplay): EGLConfig {
      val attributes =
        intArrayOf(
          EGL14.EGL_RENDERABLE_TYPE,
          EGL_OPENGL_ES3_BIT,
          EGL14.EGL_SURFACE_TYPE,
          // One config has to serve both the window surface and the pbuffer parking surface.
          EGL14.EGL_WINDOW_BIT or EGL14.EGL_PBUFFER_BIT,
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
        "no EGL config supports OpenGL ES 3 window and pbuffer rendering"
      }
      return configs[0]!!
    }

    private fun eglCheck(ok: Boolean, operation: String) {
      if (!ok) {
        error(eglFailure(operation))
      }
    }

    private fun eglFailure(operation: String): String =
      "$operation failed with EGL error 0x${EGL14.eglGetError().toString(16)}"

    private val WINDOW_ATTRIBUTES = intArrayOf(EGL14.EGL_NONE)

    private const val EGL_OPENGL_ES3_BIT = 0x00000040
  }
}
