package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import java.nio.file.Files
import java.nio.file.Path
import kotlin.io.path.Path
import kotlin.io.path.absolute
import kotlin.io.path.exists
import kotlin.io.path.pathString
import org.lwjgl.egl.EGL
import org.lwjgl.egl.EGL10.EGL_ALPHA_SIZE
import org.lwjgl.egl.EGL10.EGL_BLUE_SIZE
import org.lwjgl.egl.EGL10.EGL_DEPTH_SIZE
import org.lwjgl.egl.EGL10.EGL_EXTENSIONS
import org.lwjgl.egl.EGL10.EGL_FALSE
import org.lwjgl.egl.EGL10.EGL_GREEN_SIZE
import org.lwjgl.egl.EGL10.EGL_HEIGHT
import org.lwjgl.egl.EGL10.EGL_NONE
import org.lwjgl.egl.EGL10.EGL_NO_CONTEXT
import org.lwjgl.egl.EGL10.EGL_NO_DISPLAY
import org.lwjgl.egl.EGL10.EGL_NO_SURFACE
import org.lwjgl.egl.EGL10.EGL_PBUFFER_BIT
import org.lwjgl.egl.EGL10.EGL_RED_SIZE
import org.lwjgl.egl.EGL10.EGL_STENCIL_SIZE
import org.lwjgl.egl.EGL10.EGL_SUCCESS
import org.lwjgl.egl.EGL10.EGL_SURFACE_TYPE
import org.lwjgl.egl.EGL10.EGL_WIDTH
import org.lwjgl.egl.EGL10.eglChooseConfig
import org.lwjgl.egl.EGL10.eglCreateContext
import org.lwjgl.egl.EGL10.eglCreatePbufferSurface
import org.lwjgl.egl.EGL10.eglDestroyContext
import org.lwjgl.egl.EGL10.eglDestroySurface
import org.lwjgl.egl.EGL10.eglGetError
import org.lwjgl.egl.EGL10.eglInitialize
import org.lwjgl.egl.EGL10.eglMakeCurrent
import org.lwjgl.egl.EGL10.eglQueryString
import org.lwjgl.egl.EGL10.eglTerminate
import org.lwjgl.egl.EGL12.EGL_OPENGL_ES_API
import org.lwjgl.egl.EGL12.eglBindAPI
import org.lwjgl.egl.EGL13.EGL_CONTEXT_CLIENT_VERSION
import org.lwjgl.egl.EGL13.EGL_OPENGL_ES2_BIT
import org.lwjgl.egl.EGL13.EGL_RENDERABLE_TYPE
import org.lwjgl.egl.EGL15.EGL_OPENGL_ES3_BIT
import org.lwjgl.opengles.APPLETextureFormatBGRA8888.GL_BGRA8_EXT
import org.lwjgl.opengles.GLES
import org.lwjgl.opengles.GLES20.GL_CLAMP_TO_EDGE
import org.lwjgl.opengles.GLES20.GL_LINEAR
import org.lwjgl.opengles.GLES20.GL_NO_ERROR
import org.lwjgl.opengles.GLES20.GL_TEXTURE_2D
import org.lwjgl.opengles.GLES20.GL_TEXTURE_MAG_FILTER
import org.lwjgl.opengles.GLES20.GL_TEXTURE_MIN_FILTER
import org.lwjgl.opengles.GLES20.GL_TEXTURE_WRAP_S
import org.lwjgl.opengles.GLES20.GL_TEXTURE_WRAP_T
import org.lwjgl.opengles.GLES20.glBindTexture
import org.lwjgl.opengles.GLES20.glDeleteTextures
import org.lwjgl.opengles.GLES20.glFinish
import org.lwjgl.opengles.GLES20.glGenTextures
import org.lwjgl.opengles.GLES20.glGetError
import org.lwjgl.opengles.GLES20.glTexParameteri
import org.lwjgl.opengles.GLESCapabilities
import org.lwjgl.opengles.OESEGLImage.glEGLImageTargetTexture2DOES
import org.lwjgl.system.Configuration
import org.lwjgl.system.JNI.callPPI
import org.lwjgl.system.JNI.callPPP
import org.lwjgl.system.JNI.callPPPPP
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.system.MemoryUtil.memAddress

internal class MacOpenGlMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher =
    NativeSurfaceRendererDispatcher("compose-map-mac-opengl-renderer")
  private val egl: MacAngleEglContext =
    try {
      rendererDispatcher.run { MacAngleEglContext.create() }
    } catch (error: Throwable) {
      rendererDispatcher.close()
      throw error
    }
  private var metalTexture = NativeHandle(0)
  private var pixelFormat = 0L
  private var importedTexture: MacAngleImportedTexture? = null
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val backend: ProducerBackend = ProducerBackend.OPENGL

  override val consumerBackend: ConsumerBackend = ConsumerBackend.METAL

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
    )

  override fun resize(extent: SurfaceExtent) {
    val metalDevice = if (extent.isEmpty) null else SkikoHost.requireMetalDevice()
    rendererDispatcher.run { resizeOnRendererThread(extent, metalDevice) }
  }

  private fun resizeOnRendererThread(extent: SurfaceExtent, metalDevice: SkikoMetalDevice? = null) {
    if (extent == currentExtent && importedTexture != null) {
      return
    }
    recreateTexture(extent, metalDevice)
    currentExtent = extent
    generation += 1
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame = rendererDispatcher.run {
    if (importedTexture == null || extent != currentExtent) {
      resizeOnRendererThread(extent)
    }
    NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    rendererDispatcher.run { egl.waitIdle() }
  }

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is OpenGlTextureTarget || metalTexture.address == 0L) {
      return false
    }
    return SkikoHost.drawMetalTexture(
      scope,
      MetalTextureTarget(
        texture = metalTexture,
        pixelFormat = pixelFormat,
        origin = TextureOrigin.BOTTOM_LEFT,
        extent = target.extent,
        generation = target.generation,
      ),
    )
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run {
      MacMetalBridgeNative.runInAutoreleasePool(action)
    }

  override fun <T> withRendererAccess(action: () -> T): T = rendererDispatcher.run(action)

  override fun close() {
    try {
      rendererDispatcher.run {
        disposeTexture()
        egl.close()
      }
    } finally {
      rendererDispatcher.close()
    }
  }

  private fun target(generation: Long): NativeSurfaceTarget =
    checkNotNull(importedTexture) { "ANGLE texture is not initialized" }.target(generation)

  private fun recreateTexture(extent: SurfaceExtent, metalDevice: SkikoMetalDevice? = null) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }

    val oldTexture = metalTexture
    val newTextureAddress =
      MacMetalBridgeNative.createMetalTexture(
        metalDevice = (metalDevice ?: SkikoHost.requireMetalDevice()).ptr,
        oldTexture = oldTexture.address,
        width = extent.physicalWidth,
        height = extent.physicalHeight,
      )
    val newTexture = NativeHandle(newTextureAddress)
    if (newTexture == oldTexture && importedTexture != null) {
      return
    }

    importedTexture?.close()
    importedTexture = null
    if (newTexture != oldTexture) {
      releaseMetalTexture(oldTexture)
    }
    metalTexture = newTexture
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(newTexture.address)
    try {
      importedTexture = egl.createImportedTexture(newTexture, extent)
    } catch (error: RuntimeException) {
      disposeTexture()
      throw error
    }
  }

  private fun disposeTexture() {
    importedTexture?.close()
    importedTexture = null
    releaseMetalTexture(metalTexture)
    metalTexture = NativeHandle(0)
    pixelFormat = 0
  }

  private fun releaseMetalTexture(texture: NativeHandle) {
    if (texture.address == 0L) {
      return
    }
    SkikoHost.forgetMetalTexture(texture)
    MacMetalBridgeNative.disposeMetalTexture(texture.address)
  }
}

internal class MacAngleEglContext private constructor(private val angleRoot: Path) : AutoCloseable {
  private var display = EGL_NO_DISPLAY
  private var config = NULL
  private var surface = EGL_NO_SURFACE
  private var shareContext = EGL_NO_CONTEXT
  private var glesCapabilities: GLESCapabilities? = null
  private var eglCreateImage = NULL
  private var eglDestroyImage = NULL
  private var eglGetProcAddress = NULL

  val handles: EglContextHandles
    get() =
      EglContextHandles(
        display = NativeHandle(display),
        config = NativeHandle(config),
        shareContext = NativeHandle(shareContext),
        getProcAddress = NativeHandle(eglGetProcAddress),
      )

  fun createImportedTexture(
    metalTexture: NativeHandle,
    extent: SurfaceExtent,
  ): MacAngleImportedTexture = MacAngleImportedTexture.create(this, metalTexture, extent)

  fun makeCurrent() {
    check(
      display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE && shareContext != EGL_NO_CONTEXT
    ) {
      "ANGLE EGL context is not initialized"
    }
    check(eglMakeCurrent(display, surface, surface, shareContext), "eglMakeCurrent")
    glesCapabilities?.let { GLES.setCapabilities(it) }
      ?: run { glesCapabilities = GLES.createCapabilities() }
  }

  fun waitIdle() {
    if (shareContext == EGL_NO_CONTEXT) {
      return
    }
    makeCurrent()
    glFinish()
  }

  internal fun createMetalImage(metalTexture: NativeHandle): Long {
    check(eglCreateImage != NULL) { "EGL_KHR_image_base is not available" }
    MemoryStack.stackPush().use { stack ->
      val attributes = stack.ints(EGL_NONE)
      val image =
        callPPPPP(
          display,
          EGL_NO_CONTEXT,
          EGL_METAL_TEXTURE_ANGLE,
          metalTexture.address,
          memAddress(attributes),
          eglCreateImage,
        )
      if (image == NULL) {
        throw NativeSurfaceBridgeException(
          "eglCreateImageKHR(EGL_METAL_TEXTURE_ANGLE) failed with ${eglError()}"
        )
      }
      return image
    }
  }

  internal fun destroyMetalImage(image: Long) {
    if (image != NULL && eglDestroyImage != NULL) {
      check(callPPI(display, image, eglDestroyImage) != EGL_FALSE, "eglDestroyImageKHR")
    }
  }

  override fun close() {
    runCatching { waitIdle() }
    GLES.setCapabilities(null)
    if (display != EGL_NO_DISPLAY) {
      eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)
    }
    if (surface != EGL_NO_SURFACE) {
      eglDestroySurface(display, surface)
      surface = EGL_NO_SURFACE
    }
    if (shareContext != EGL_NO_CONTEXT) {
      eglDestroyContext(display, shareContext)
      shareContext = EGL_NO_CONTEXT
    }
    if (display != EGL_NO_DISPLAY) {
      eglTerminate(display)
      display = EGL_NO_DISPLAY
    }
    glesCapabilities = null
  }

  private fun create() {
    MacAngleLibraries.load(angleRoot)
    display = createMetalDisplay()
    val major = IntArray(1)
    val minor = IntArray(1)
    check(eglInitialize(display, major, minor), "eglInitialize")
    val displayCapabilities = EGL.createDisplayCapabilities(display, major[0], minor[0])
    eglCreateImage = displayCapabilities.eglCreateImageKHR
    eglDestroyImage = displayCapabilities.eglDestroyImageKHR
    eglGetProcAddress = EGL.getFunctionProvider().getFunctionAddress("eglGetProcAddress")

    val extensions = eglQueryString(display, EGL_EXTENSIONS).orEmpty()
    check("metal_texture_client_buffer" in extensions) {
      "ANGLE EGL display does not expose EGL_ANGLE_metal_texture_client_buffer"
    }
    check(displayCapabilities.EGL_KHR_image_base || displayCapabilities.EGL_KHR_image) {
      "ANGLE EGL display does not expose EGL_KHR_image_base"
    }
    check(eglBindAPI(EGL_OPENGL_ES_API), "eglBindAPI(EGL_OPENGL_ES_API)")
    chooseConfig()
    createShareContext()
    createPbuffer()
    makeCurrent()
  }

  private fun createMetalDisplay(): Long =
    MemoryStack.stackPush().use { stack ->
      val attributes =
        stack.ints(
          EGL_PLATFORM_ANGLE_TYPE_ANGLE,
          EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
          EGL_NONE,
        )
      val function = EGL.getCapabilities().eglGetPlatformDisplayEXT
      check(function != NULL) { "EGL_EXT_platform_base is not available" }
      val result = callPPP(EGL_PLATFORM_ANGLE_ANGLE, NULL, memAddress(attributes), function)
      check(result != EGL_NO_DISPLAY) { "eglGetPlatformDisplayEXT(ANGLE Metal) failed" }
      result
    }

  private fun chooseConfig() {
    MemoryStack.stackPush().use { stack ->
      val attributes =
        stack.ints(
          EGL_SURFACE_TYPE,
          EGL_PBUFFER_BIT,
          EGL_RENDERABLE_TYPE,
          EGL_OPENGL_ES3_BIT or EGL_OPENGL_ES2_BIT,
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
      val configs = stack.mallocPointer(1)
      val count = stack.mallocInt(1)
      check(eglChooseConfig(display, attributes, configs, count), "eglChooseConfig")
      check(count[0] > 0 && configs[0] != NULL) {
        "No ANGLE EGL config supports GLES3 pbuffer rendering"
      }
      config = configs[0]
    }
  }

  private fun createShareContext() {
    MemoryStack.stackPush().use { stack ->
      val attributes = stack.ints(EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE)
      shareContext = eglCreateContext(display, config, EGL_NO_CONTEXT, attributes)
      check(shareContext != EGL_NO_CONTEXT) { "eglCreateContext(GLES3) failed with ${eglError()}" }
    }
  }

  private fun createPbuffer() {
    MemoryStack.stackPush().use { stack ->
      val attributes = stack.ints(EGL_WIDTH, 8, EGL_HEIGHT, 8, EGL_NONE)
      surface = eglCreatePbufferSurface(display, config, attributes)
      check(surface != EGL_NO_SURFACE) { "eglCreatePbufferSurface failed with ${eglError()}" }
    }
  }

  private fun check(status: Boolean, operation: String) {
    check(status) { "$operation failed with ${eglError()}" }
  }

  private fun eglError(): String {
    val error = eglGetError()
    return if (error == EGL_SUCCESS) "EGL_SUCCESS" else "EGL error 0x${error.toString(16)}"
  }

  companion object {
    private const val EGL_PLATFORM_ANGLE_ANGLE = 0x3202
    private const val EGL_PLATFORM_ANGLE_TYPE_ANGLE = 0x3203
    private const val EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE = 0x3209
    private const val EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE = 0x320A
    private const val EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE = 0x3489
    private const val EGL_METAL_TEXTURE_ANGLE = 0x34A7

    fun create(): MacAngleEglContext {
      val context = MacAngleEglContext(resolveAngleRoot())
      try {
        context.create()
        return context
      } catch (error: RuntimeException) {
        context.close()
        throw error
      }
    }

    private fun resolveAngleRoot(): Path {
      val fromEnvironment = System.getenv("MLN_FFI_EGL_ROOT")?.let(::Path)
      if (fromEnvironment != null && fromEnvironment.exists()) {
        return fromEnvironment.absolute()
      }
      val fromCheckout =
        Path("third_party/angle/chromium-7151_rev1/macos-arm64").absolute().normalize()
      if (fromCheckout.exists()) {
        return fromCheckout
      }
      throw NativeSurfaceBridgeException(
        "ANGLE root is unavailable; set MLN_FFI_EGL_ROOT to the directory containing libEGL.dylib"
      )
    }
  }
}

internal class MacAngleImportedTexture
private constructor(
  private val context: MacAngleEglContext,
  private val metalTexture: NativeHandle,
  private val extent: SurfaceExtent,
) : AutoCloseable {
  private var image = NULL
  private var texture = 0

  fun target(generation: Long): OpenGlTextureTarget =
    OpenGlTextureTarget(
      context = context.handles,
      textureName = texture,
      textureTarget = GL_TEXTURE_2D,
      format = GL_BGRA8_EXT,
      contextProvider = OpenGlContextProvider { context.makeCurrent() },
      extent = extent,
      generation = generation,
    )

  private fun create() {
    context.makeCurrent()
    image = context.createMetalImage(metalTexture)
    texture = glGenTextures()
    checkGl("glGenTextures")
    glBindTexture(GL_TEXTURE_2D, texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image)
    checkGl("glEGLImageTargetTexture2DOES")
    glBindTexture(GL_TEXTURE_2D, 0)
  }

  override fun close() {
    context.makeCurrent()
    glFinish()
    if (texture != 0) {
      glDeleteTextures(texture)
      texture = 0
    }
    if (image != NULL) {
      context.destroyMetalImage(image)
      image = NULL
    }
  }

  companion object {
    fun create(
      context: MacAngleEglContext,
      metalTexture: NativeHandle,
      extent: SurfaceExtent,
    ): MacAngleImportedTexture {
      val texture = MacAngleImportedTexture(context, metalTexture, extent)
      try {
        texture.create()
        return texture
      } catch (error: RuntimeException) {
        texture.close()
        throw error
      }
    }

    private fun checkGl(operation: String) {
      val error = glGetError()
      check(error == GL_NO_ERROR) { "$operation failed with GL error 0x${error.toString(16)}" }
    }
  }
}

private object MacAngleLibraries {
  private var loadedRoot: Path? = null

  @Synchronized
  fun load(root: Path) {
    val normalizedRoot = root.absolute().normalize()
    if (loadedRoot == normalizedRoot) {
      return
    }
    check(loadedRoot == null) { "ANGLE is already loaded from ${loadedRoot!!.pathString}" }

    val eglLibrary = normalizedRoot.resolve("libEGL.dylib")
    val glesLibrary = normalizedRoot.resolve("libGLESv2.dylib")
    check(Files.isRegularFile(eglLibrary)) { "ANGLE libEGL.dylib not found at $eglLibrary" }
    check(Files.isRegularFile(glesLibrary)) { "ANGLE libGLESv2.dylib not found at $glesLibrary" }

    Configuration.EGL_EXPLICIT_INIT.set(true)
    Configuration.OPENGLES_EXPLICIT_INIT.set(true)
    Configuration.EGL_LIBRARY_NAME.set(eglLibrary.pathString)
    Configuration.OPENGLES_LIBRARY_NAME.set(glesLibrary.pathString)
    Configuration.OPENGLES_CONTEXT_API.set("EGL")

    EGL.create(eglLibrary.pathString)
    GLES.create(glesLibrary.pathString)
    loadedRoot = normalizedRoot
  }
}
