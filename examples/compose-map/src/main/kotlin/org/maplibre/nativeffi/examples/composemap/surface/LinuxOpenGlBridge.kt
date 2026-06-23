package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import java.util.concurrent.ExecutionException
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicReference
import org.lwjgl.egl.EGL
import org.lwjgl.egl.EGL10.EGL_ALPHA_SIZE
import org.lwjgl.egl.EGL10.EGL_BLUE_SIZE
import org.lwjgl.egl.EGL10.EGL_DEPTH_SIZE
import org.lwjgl.egl.EGL10.EGL_EXTENSIONS
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
import org.lwjgl.egl.EGL10.eglGetDisplay
import org.lwjgl.egl.EGL10.eglGetError
import org.lwjgl.egl.EGL10.eglInitialize
import org.lwjgl.egl.EGL10.eglMakeCurrent
import org.lwjgl.egl.EGL10.eglQueryString
import org.lwjgl.egl.EGL10.eglTerminate
import org.lwjgl.egl.EGL12.eglBindAPI
import org.lwjgl.egl.EGL13.EGL_RENDERABLE_TYPE
import org.lwjgl.egl.EGL14.EGL_DEFAULT_DISPLAY
import org.lwjgl.egl.EGL14.EGL_OPENGL_API
import org.lwjgl.egl.EGL14.EGL_OPENGL_BIT
import org.lwjgl.opengl.EXTMemoryObject.GL_DEDICATED_MEMORY_OBJECT_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_OPTIMAL_TILING_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_TEXTURE_TILING_EXT
import org.lwjgl.opengl.EXTMemoryObject.glCreateMemoryObjectsEXT
import org.lwjgl.opengl.EXTMemoryObject.glDeleteMemoryObjectsEXT
import org.lwjgl.opengl.EXTMemoryObject.glMemoryObjectParameteriEXT
import org.lwjgl.opengl.EXTMemoryObject.glTexStorageMem2DEXT
import org.lwjgl.opengl.EXTMemoryObjectFD.GL_HANDLE_TYPE_OPAQUE_FD_EXT
import org.lwjgl.opengl.EXTMemoryObjectFD.glImportMemoryFdEXT
import org.lwjgl.opengl.GL
import org.lwjgl.opengl.GL11.GL_LINEAR
import org.lwjgl.opengl.GL11.GL_NO_ERROR
import org.lwjgl.opengl.GL11.GL_RGBA8
import org.lwjgl.opengl.GL11.GL_TEXTURE_2D
import org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER
import org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER
import org.lwjgl.opengl.GL11.GL_TEXTURE_WRAP_S
import org.lwjgl.opengl.GL11.GL_TEXTURE_WRAP_T
import org.lwjgl.opengl.GL11.GL_TRUE
import org.lwjgl.opengl.GL11.glBindTexture
import org.lwjgl.opengl.GL11.glDeleteTextures
import org.lwjgl.opengl.GL11.glFinish
import org.lwjgl.opengl.GL11.glGenTextures
import org.lwjgl.opengl.GL11.glGetError
import org.lwjgl.opengl.GL11.glTexParameteri
import org.lwjgl.opengl.GL12.GL_CLAMP_TO_EDGE
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL

internal class LinuxOpenGlBridge : NativeSurfaceBridge {
  private var vulkan: LinuxVulkanContext? = null
  private val producerThreadRef = AtomicReference<Thread?>()
  private val producerExecutor = Executors.newSingleThreadExecutor { task ->
    Thread(task, "compose-map-linux-egl-producer").also {
      it.isDaemon = true
      producerThreadRef.set(it)
    }
  }
  private val egl: LinuxEglContext
  private var exportedTexture: LinuxExportedVulkanTexture? = null
  private var producerTexture: LinuxEglImportedTexture? = null
  private var consumerTexture: LinuxOpenGlImportedTexture? = null
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val backend: ProducerBackend = ProducerBackend.OPENGL

  override val consumerBackend: ConsumerBackend = ConsumerBackend.OPENGL

  init {
    try {
      egl = runOnProducerThread { LinuxEglContext.create() }
    } catch (error: Throwable) {
      producerExecutor.shutdown()
      throw error
    }
  }

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
    )

  override fun resize(extent: SurfaceExtent) {
    // The consumer import issues OpenGL calls against Skiko's context, so texture creation is
    // applied lazily by acquireFrame inside the Compose draw callback.
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (producerTexture == null || consumerTexture == null || extent != currentExtent) {
      recreateTexture(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    runOnProducerThread { egl.waitIdle() }
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    runOnProducerThread(action)

  override fun <T> withRendererAccess(action: () -> T): T = runOnProducerThread(action)

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is OpenGlTextureTarget) {
      return false
    }
    val texture = consumerTexture ?: return false
    return SkikoHost.drawOpenGlTexture(scope, texture.target(target.generation))
  }

  override fun close() {
    try {
      disposeTexture(consumerContextCurrent = false)
    } finally {
      try {
        runOnProducerThread { egl.close() }
      } finally {
        producerExecutor.shutdown()
        vulkan?.close()
        vulkan = null
      }
    }
  }

  private fun target(generation: Long): NativeSurfaceTarget =
    checkNotNull(producerTexture) { "Linux EGL texture is not initialized" }.target(generation)

  private fun recreateTexture(extent: SurfaceExtent) {
    if (extent.isEmpty) {
      disposeTexture(consumerContextCurrent = true)
      currentExtent = SurfaceExtent.Empty
      generation += 1
      return
    }

    disposeTexture(consumerContextCurrent = true)
    val context =
      vulkan ?: LinuxVulkanContext.create(currentOpenGlDeviceUuids()).also { vulkan = it }
    val exported = context.createExportedTexture(extent)
    var producer: LinuxEglImportedTexture? = null
    var consumer: LinuxOpenGlImportedTexture? = null
    try {
      producer = runOnProducerThread {
        LinuxEglImportedTexture.create(egl, exported.exportFd(), exported, extent)
      }
      consumer =
        LinuxOpenGlImportedTexture.create(
          exported.exportFd(),
          exported.memorySize(),
          extent,
          origin = TextureOrigin.BOTTOM_LEFT,
        )
      exportedTexture = exported
      producerTexture = producer
      consumerTexture = consumer
      currentExtent = extent
      generation += 1
    } catch (error: RuntimeException) {
      if (producer != null) {
        runOnProducerThread { producer.close() }
      }
      consumer?.close()
      exported.close()
      throw error
    }
  }

  private fun disposeTexture(consumerContextCurrent: Boolean = true) {
    val oldProducerTexture = producerTexture
    producerTexture = null
    if (oldProducerTexture != null) {
      runOnProducerThread { oldProducerTexture.close() }
    }
    consumerTexture?.let { texture ->
      if (consumerContextCurrent) {
        texture.close()
      } else {
        SkikoHost.withLinuxOpenGlContext { texture.close() }
      }
    }
    consumerTexture = null
    exportedTexture?.close()
    exportedTexture = null
  }

  private fun <T> runOnProducerThread(action: () -> T): T {
    if (Thread.currentThread() == producerThreadRef.get()) {
      return action()
    }
    return try {
      producerExecutor.submit<T> { action() }.get()
    } catch (error: ExecutionException) {
      throw error.cause ?: error
    }
  }
}

internal class LinuxEglContext private constructor() : AutoCloseable {
  private var display = EGL_NO_DISPLAY
  private var config = NULL
  private var surface = EGL_NO_SURFACE
  private var shareContext = EGL_NO_CONTEXT
  private var eglGetProcAddress = NULL
  private var initialized = false

  val handles: EglContextHandles
    get() =
      EglContextHandles(
        display = NativeHandle(display),
        config = NativeHandle(config),
        shareContext = NativeHandle(shareContext),
        getProcAddress = NativeHandle(eglGetProcAddress),
      )

  fun makeCurrent() {
    check(
      display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE && shareContext != EGL_NO_CONTEXT
    ) {
      "Linux EGL context is not initialized"
    }
    check(eglMakeCurrent(display, surface, surface, shareContext), "eglMakeCurrent")
    ensureLwjglOpenGlCapabilities()
  }

  fun waitIdle() {
    if (shareContext == EGL_NO_CONTEXT) {
      return
    }
    makeCurrent()
    glFinish()
  }

  override fun close() {
    if (initialized) {
      runCatching { waitIdle() }
      initialized = false
    }
    GL.setCapabilities(null)
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
  }

  private fun create() {
    ensureEglFunctionProvider()
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY)
    check(display != EGL_NO_DISPLAY) { "eglGetDisplay(EGL_DEFAULT_DISPLAY) failed" }
    val major = IntArray(1)
    val minor = IntArray(1)
    check(eglInitialize(display, major, minor), "eglInitialize")
    val displayCapabilities = EGL.createDisplayCapabilities(display, major[0], minor[0])
    eglGetProcAddress = EGL.getFunctionProvider().getFunctionAddress("eglGetProcAddress")
    val extensions = eglQueryString(display, EGL_EXTENSIONS).orEmpty()
    check("EGL_KHR_fence_sync" in extensions || displayCapabilities.EGL_KHR_fence_sync) {
      "Linux EGL display does not expose EGL_KHR_fence_sync"
    }
    check(eglBindAPI(EGL_OPENGL_API), "eglBindAPI(EGL_OPENGL_API)")
    chooseConfig()
    createShareContext()
    createPbuffer()
    makeCurrent()
    initialized = true
  }

  private fun chooseConfig() {
    MemoryStack.stackPush().use { stack ->
      val attributes =
        stack.ints(
          EGL_SURFACE_TYPE,
          EGL_PBUFFER_BIT,
          EGL_RENDERABLE_TYPE,
          EGL_OPENGL_BIT,
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
        "No Linux EGL config supports OpenGL pbuffer rendering"
      }
      config = configs[0]
    }
  }

  private fun createShareContext() {
    MemoryStack.stackPush().use { stack ->
      val attributes = stack.ints(EGL_NONE)
      shareContext = eglCreateContext(display, config, EGL_NO_CONTEXT, attributes)
      check(shareContext != EGL_NO_CONTEXT) { "eglCreateContext(OpenGL) failed with ${eglError()}" }
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
    fun create(): LinuxEglContext {
      val context = LinuxEglContext()
      try {
        context.create()
        return context
      } catch (error: RuntimeException) {
        context.close()
        throw error
      }
    }
  }
}

@Suppress("SENSELESS_COMPARISON")
private fun ensureEglFunctionProvider() {
  if (EGL.getFunctionProvider() == null) {
    EGL.create()
  }
}

internal class LinuxEglImportedTexture
private constructor(
  private val context: LinuxEglContext,
  private val exportedTexture: LinuxExportedVulkanTexture,
  private val extent: SurfaceExtent,
  private val fd: Int,
) : AutoCloseable {
  private var memoryObject = 0
  private var textureName = 0

  fun target(generation: Long): OpenGlTextureTarget =
    OpenGlTextureTarget(
      context = context.handles,
      textureName = textureName,
      textureTarget = GL_TEXTURE_2D,
      format = GL_RGBA8,
      contextProvider = OpenGlContextProvider { context.makeCurrent() },
      extent = extent,
      generation = generation,
    )

  private fun create() {
    context.makeCurrent()
    val capabilities = ensureLwjglOpenGlCapabilities()
    check(capabilities.GL_EXT_memory_object) {
      "Linux EGL context does not expose GL_EXT_memory_object"
    }
    check(capabilities.GL_EXT_memory_object_fd) {
      "Linux EGL context does not expose GL_EXT_memory_object_fd"
    }

    var imported = false
    try {
      memoryObject = glCreateMemoryObjectsEXT()
      glMemoryObjectParameteriEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT, GL_TRUE)
      glImportMemoryFdEXT(
        memoryObject,
        exportedTexture.memorySize(),
        GL_HANDLE_TYPE_OPAQUE_FD_EXT,
        fd,
      )
      checkGl("glImportMemoryFdEXT")
      imported = true

      textureName = glGenTextures()
      glBindTexture(GL_TEXTURE_2D, textureName)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_OPTIMAL_TILING_EXT)
      glTexStorageMem2DEXT(
        GL_TEXTURE_2D,
        1,
        GL_RGBA8,
        extent.physicalWidth,
        extent.physicalHeight,
        memoryObject,
        0,
      )
      glBindTexture(GL_TEXTURE_2D, 0)
      checkGl("glTexStorageMem2DEXT")
    } catch (error: RuntimeException) {
      if (!imported) {
        closeFd(fd)
      }
      throw error
    }
  }

  override fun close() {
    context.makeCurrent()
    glFinish()
    if (textureName != 0) {
      glDeleteTextures(textureName)
      textureName = 0
    }
    if (memoryObject != 0) {
      glDeleteMemoryObjectsEXT(memoryObject)
      memoryObject = 0
    }
  }

  companion object {
    fun create(
      context: LinuxEglContext,
      fd: Int,
      exportedTexture: LinuxExportedVulkanTexture,
      extent: SurfaceExtent,
    ): LinuxEglImportedTexture {
      val texture = LinuxEglImportedTexture(context, exportedTexture, extent, fd)
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
