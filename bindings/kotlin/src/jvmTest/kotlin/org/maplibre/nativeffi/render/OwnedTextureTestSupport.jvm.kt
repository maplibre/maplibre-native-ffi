package org.maplibre.nativeffi.render

import org.lwjgl.PointerBuffer
import org.lwjgl.egl.EGL
import org.lwjgl.egl.EGL14
import org.lwjgl.egl.EGL15
import org.lwjgl.system.MemoryStack
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.map.MapHandle

private const val EGL_PLATFORM_SURFACELESS_MESA = 0x31DD

internal actual object OwnedTextureTestSupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession? {
    if (RenderBackend.OPENGL !in Maplibre.supportedRenderBackends()) return null
    if (OpenGLContextProvider.EGL !in Maplibre.supportedOpenGLContextProviders()) return null
    ensureEglFunctionProvider()
    return MemoryStack.stackPush().use { stack -> createEglSession(map, width, height, stack) }
  }
}

private class JvmEglOwnedTextureSession(
  private val display: Long,
  private val config: Long,
  private val surface: Long,
  private val context: Long,
  override val session: RenderSessionHandle,
) : OwnedTextureTestSession {
  override fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionHandle {
    val descriptor =
      EglContextDescriptor(
        NativePointer.ofAddress(display),
        NativePointer.ofAddress(config),
        NativePointer.ofAddress(context),
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

private fun createEglSession(
  map: MapHandle,
  width: Int,
  height: Int,
  stack: MemoryStack,
): OwnedTextureTestSession? {
  val display = initializedDisplay(stack)
  if (display == null) {
    check(!isLinuxHost()) { "EGL pbuffer fixture failed on Linux" }
    return null
  }

  var surface = EGL14.EGL_NO_SURFACE
  var context = EGL14.EGL_NO_CONTEXT
  try {
    check(EGL14.eglBindAPI(EGL14.EGL_OPENGL_ES_API)) { "EGL bind OpenGL ES API failed" }

    val configAttributes =
      stack.ints(
        EGL14.EGL_SURFACE_TYPE,
        EGL14.EGL_PBUFFER_BIT,
        EGL14.EGL_RENDERABLE_TYPE,
        EGL15.EGL_OPENGL_ES3_BIT,
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
    val configs = stack.mallocPointer(1)
    val configCount = stack.mallocInt(1)
    check(
      EGL14.eglChooseConfig(display, configAttributes, configs, configCount) && configCount[0] > 0
    ) {
      "no EGL config supports OpenGL ES 3 pbuffer rendering"
    }
    val config = configs[0]

    val contextAttributes = stack.ints(EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, EGL14.EGL_NONE)
    context = EGL14.eglCreateContext(display, config, EGL14.EGL_NO_CONTEXT, contextAttributes)
    check(context != EGL14.EGL_NO_CONTEXT) { "EGL context creation failed" }

    val surfaceAttributes =
      stack.ints(EGL14.EGL_WIDTH, width, EGL14.EGL_HEIGHT, height, EGL14.EGL_NONE)
    surface = EGL14.eglCreatePbufferSurface(display, config, surfaceAttributes)
    check(surface != EGL14.EGL_NO_SURFACE) { "EGL pbuffer creation failed" }
    check(EGL14.eglMakeCurrent(display, surface, surface, context)) { "EGL make current failed" }

    val descriptor =
      EglContextDescriptor(
        NativePointer.ofAddress(display),
        NativePointer.ofAddress(config),
        NativePointer.ofAddress(context),
        NativePointer.NULL_POINTER,
      )
    val session =
      map.attachOpenGLOwnedTexture(
        OpenGLOwnedTextureDescriptor(RenderTargetExtent(width, height, 1.0), descriptor)
      )
    return JvmEglOwnedTextureSession(display, config, surface, context, session)
  } catch (error: Throwable) {
    releaseEgl(display, surface, context)
    throw error
  }
}

private fun initializedDisplay(stack: MemoryStack): Long? {
  val surfaceless = surfacelessDisplay()
  if (surfaceless != EGL14.EGL_NO_DISPLAY && initializeDisplay(surfaceless, stack)) {
    return surfaceless
  }
  val fallback = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY)
  if (
    fallback != EGL14.EGL_NO_DISPLAY &&
      fallback != surfaceless &&
      initializeDisplay(fallback, stack)
  ) {
    return fallback
  }
  return null
}

private fun surfacelessDisplay(): Long {
  return try {
    EGL15.eglGetPlatformDisplay(
      EGL_PLATFORM_SURFACELESS_MESA,
      EGL14.EGL_DEFAULT_DISPLAY,
      null as PointerBuffer?,
    )
  } catch (_: Throwable) {
    EGL14.EGL_NO_DISPLAY
  }
}

private fun initializeDisplay(display: Long, stack: MemoryStack): Boolean {
  val major = stack.mallocInt(1)
  val minor = stack.mallocInt(1)
  if (EGL14.eglInitialize(display, major, minor)) return true
  EGL14.eglTerminate(display)
  return false
}

private fun releaseEgl(display: Long, surface: Long, context: Long) {
  EGL14.eglMakeCurrent(display, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT)
  if (surface != EGL14.EGL_NO_SURFACE) {
    EGL14.eglDestroySurface(display, surface)
  }
  if (context != EGL14.EGL_NO_CONTEXT) {
    EGL14.eglDestroyContext(display, context)
  }
  EGL14.eglTerminate(display)
}

@Suppress("SENSELESS_COMPARISON")
private fun ensureEglFunctionProvider() {
  if (EGL.getFunctionProvider() == null) {
    EGL.create()
  }
}

private fun isLinuxHost(): Boolean =
  System.getProperty("os.name").orEmpty().lowercase().contains("linux")
