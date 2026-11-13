package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.EglContextDescriptor
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MetalBorrowedTexture
import org.maplibre.nativeffi.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.NativePointer
import org.maplibre.nativeffi.OpenGlBorrowedTexture
import org.maplibre.nativeffi.OpenGlBorrowedTextureDescriptor
import org.maplibre.nativeffi.OpenGlClientApi
import org.maplibre.nativeffi.OpenGlContextDescriptor
import org.maplibre.nativeffi.OpenGlContextDescriptorData
import org.maplibre.nativeffi.OpenGlContextOwnership
import org.maplibre.nativeffi.OpenGlContextProviderFlag
import org.maplibre.nativeffi.RenderBackendFlag
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.VulkanBorrowedTexture
import org.maplibre.nativeffi.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.VulkanContextDescriptor
import org.maplibre.nativeffi.WebGlContextDescriptor
import org.maplibre.nativeffi.WebGpuBorrowedTexture
import org.maplibre.nativeffi.WebGpuBorrowedTextureDescriptor
import org.maplibre.nativeffi.WebGpuContextDescriptor
import org.maplibre.nativeffi.WglContextDescriptor
import org.maplibre.nativeffi.openGlSupportedContextProviderMask
import org.maplibre.nativeffi.supportedRenderBackendMask

/**
 * The handles of a test graphics context, as addresses. Only its backend's fields are set. A WebGL
 * context and a WebGPU device are `mln_web_object` handles: the page creates them, and lends them
 * to the module as `WebObject`s.
 */
internal class TestGraphicsContext(
  val metalDevice: Long = 0,
  val vulkanInstance: Long = 0,
  val vulkanPhysicalDevice: Long = 0,
  val vulkanDevice: Long = 0,
  val vulkanQueue: Long = 0,
  val vulkanQueueFamilyIndex: UInt = 0u,
  val vulkanGetInstanceProcAddr: Long = 0,
  val vulkanGetDeviceProcAddr: Long = 0,
  val eglDisplay: Long = 0,
  val eglConfig: Long = 0,
  val eglContext: Long = 0,
  val wglDeviceContext: Long = 0,
  val wglContext: Long = 0,
  val webGlContext: UInt = 0u,
  val webGpuDevice: UInt = 0u,
)

/**
 * A texture of a test graphics context that a borrowed ring can name, and its size. Only its
 * backend's fields are set. A WebGL texture and a WebGPU texture are `mln_web_object` handles: the
 * page creates them on its own context or device. [close] destroys it.
 */
internal class TestTexture(
  val width: UInt,
  val height: UInt,
  /** The MTLPixelFormat or VkFormat. */
  val format: UInt = 0u,
  /** The GPUTextureFormat. */
  val webGpuFormat: String = "",
  val metalTexture: Long = 0,
  val vulkanImage: ULong = 0uL,
  val vulkanImageView: ULong = 0uL,
  val vulkanFinalLayout: UInt = 0u,
  val openGlTexture: UInt = 0u,
  val openGlTarget: UInt = 0u,
  val webGpuTexture: UInt = 0u,
  private val destroy: () -> Unit,
) : AutoCloseable {
  override fun close() = destroy()
}

/**
 * The test graphics library over this platform's FFI: tests/graphics over FFM on the JVM, JNI on
 * Android, and cinterop on Kotlin/Native, and on wasm the page's own WebGL context or WebGPU
 * device, which the suite imports into the browser module. See
 * tests/graphics/include/mln_test_graphics.h.
 */
internal expect object TestGraphicsLibrary {
  /**
   * Creates a context for one [TestBackend.graphicsBackend] value, or throws with the reason. It
   * suspends where the browser answers a WebGPU device request later.
   */
  suspend fun create(backend: UInt): Long

  fun context(graphics: Long): TestGraphicsContext

  /**
   * Makes an EGL, WGL, or WebGL context current on the calling thread, or throws with the reason.
   */
  fun makeCurrent(graphics: Long)

  /**
   * Creates a texture of [width] x [height] on [graphics] that a borrowed ring can render into, or
   * throws with the reason. Call it on the thread where an EGL, WGL, or WebGL context is current.
   */
  fun createTexture(graphics: Long, width: Int, height: Int): TestTexture
}

/** The graphics backend a build renders with, and its context from the test graphics library. */
internal enum class TestBackend(val graphicsBackend: UInt) {
  METAL(1u),
  VULKAN(2u),
  EGL(3u),
  WGL(4u),
  /**
   * WebGL 2 in the browser. tests/graphics leaves WebGL to JavaScript, so this value, past the
   * tests/graphics values, names the browser fixtures: the page creates the context, and the page
   * lends it to the module as a `WebObject`.
   */
  WEBGL(5u),
  /** WebGPU in the browser, from the browser fixtures, as for [WEBGL]. */
  WEBGPU(6u);

  companion object {
    /** The one backend the loaded library renders with. */
    fun ofBuild(): TestBackend {
      val backends = supportedRenderBackendMask()
      return when {
        RenderBackendFlag.METAL in backends -> METAL
        RenderBackendFlag.VULKAN in backends -> VULKAN
        RenderBackendFlag.WEB_GPU in backends -> WEBGPU
        RenderBackendFlag.OPEN_GL in backends -> {
          val providers = openGlSupportedContextProviderMask()
          when {
            OpenGlContextProviderFlag.WGL in providers -> WGL
            OpenGlContextProviderFlag.WEB_GL in providers -> WEBGL
            else -> EGL
          }
        }
        else -> error("the loaded library renders with no backend the tests drive: $backends")
      }
    }
  }
}

/** The dispatcher of the one thread that every render test drives its graphics from. */
internal expect val testGraphicsThread: CoroutineDispatcher

/**
 * The graphics thread every render test drives its sessions from, and the build's context on it.
 *
 * The context is created on first use and kept for the process: an EGL, WGL, or WebGL context stays
 * current on this one thread, and every caller-driven session fixes its graphics thread to it.
 */
internal object TestGraphics {
  val thread: CoroutineDispatcher
    get() = testGraphicsThread

  val backend: TestBackend by lazy { TestBackend.ofBuild() }

  private class Shared(val graphics: Long, val context: TestGraphicsContext)

  // Read and written on [thread] only.
  private var shared: Shared? = null

  /** The build's context, made current on [thread] when its backend has a current context. */
  suspend fun context(): TestGraphicsContext = shared().context

  private suspend fun shared(): Shared =
    withContext(thread) {
      shared
        ?: run {
          val graphics = TestGraphicsLibrary.create(backend.graphicsBackend)
          if (
            backend == TestBackend.EGL || backend == TestBackend.WGL || backend == TestBackend.WEBGL
          ) {
            TestGraphicsLibrary.makeCurrent(graphics)
          }
          Shared(graphics, TestGraphicsLibrary.context(graphics)).also { shared = it }
        }
    }

  /**
   * Creates [count] textures of [width] x [height] on the build's context, for a borrowed ring.
   * Destroy them with [destroyTextures] once every session that renders into them has retired.
   */
  suspend fun createTextures(width: Int, height: Int, count: Int): List<TestTexture> =
    withContext(thread) {
      val graphics = shared().graphics
      val textures = mutableListOf<TestTexture>()
      try {
        repeat(count) { textures += TestGraphicsLibrary.createTexture(graphics, width, height) }
      } catch (failure: Throwable) {
        textures.forEach { it.close() }
        throw failure
      }
      textures
    }

  /** Destroys [textures] from [createTextures]. */
  suspend fun destroyTextures(textures: List<TestTexture>) {
    withContext(NonCancellable + thread) { textures.forEach { it.close() } }
  }

  /**
   * Submits an attach of a borrowed ring of [textures], from [createTextures], on the build's
   * backend. Call it on [thread], where the build's OpenGL context is current.
   */
  suspend fun attachBorrowedTexture(
    map: MapHandle,
    textures: List<TestTexture>,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    val context = context()
    val texture = textures.first()
    val logicalSize = LogicalSize(texture.width, texture.height)
    val openGlContext =
      when (backend) {
        TestBackend.EGL ->
          OpenGlContextDescriptorData.Egl(
            EglContextDescriptor(
              pointer(context.eglDisplay),
              pointer(context.eglConfig),
              pointer(context.eglContext),
              OpenGlClientApi.GLES,
              NativePointer.NULL_POINTER,
            ),
          )
        TestBackend.WGL ->
          OpenGlContextDescriptorData.Wgl(
            WglContextDescriptor(
              pointer(context.wglDeviceContext),
              pointer(context.wglContext),
              NativePointer.NULL_POINTER,
            ),
          )
        TestBackend.WEBGL ->
          OpenGlContextDescriptorData.WebGl(
            WebGlContextDescriptor(context.webGlContext),
          )
        TestBackend.METAL,
        TestBackend.VULKAN,
        TestBackend.WEBGPU -> return attachBackendBorrowedTexture(map, textures, context, options)
      }
    return map.attachOpenGlBorrowedTexture(
      OpenGlBorrowedTextureDescriptor(
        logicalSize,
        physicalWidth = texture.width,
        physicalHeight = texture.height,
        context = OpenGlContextDescriptor(OpenGlContextOwnership.SHARED, openGlContext),
        textures = textures.map { OpenGlBorrowedTexture(it.openGlTexture) },
        target = texture.openGlTarget,
      ),
      options,
    )
  }

  /** Reads the size of the backend texture behind [frame] through its typed view. */
  fun frameTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
    when (backend) {
      TestBackend.EGL,
      TestBackend.WGL,
      TestBackend.WEBGL -> frame.withOpenGlTexture { it.width to it.height }
      TestBackend.METAL,
      TestBackend.VULKAN,
      TestBackend.WEBGPU -> backendTextureSize(frame)
    }

  fun metalBorrowedTexture(textures: List<TestTexture>): MetalBorrowedTextureDescriptor {
    val texture = textures.first()
    return MetalBorrowedTextureDescriptor(
      LogicalSize(texture.width, texture.height),
      physicalWidth = texture.width,
      physicalHeight = texture.height,
      textures = textures.map { MetalBorrowedTexture(pointer(it.metalTexture)) },
      commandQueue = NativePointer.NULL_POINTER,
    )
  }

  fun vulkanBorrowedTexture(
    textures: List<TestTexture>,
    context: TestGraphicsContext,
  ): VulkanBorrowedTextureDescriptor {
    val texture = textures.first()
    return VulkanBorrowedTextureDescriptor(
      LogicalSize(texture.width, texture.height),
      physicalWidth = texture.width,
      physicalHeight = texture.height,
      context =
        VulkanContextDescriptor(
          pointer(context.vulkanInstance),
          pointer(context.vulkanPhysicalDevice),
          pointer(context.vulkanDevice),
          pointer(context.vulkanQueue),
          context.vulkanQueueFamilyIndex,
          pointer(context.vulkanGetInstanceProcAddr),
          pointer(context.vulkanGetDeviceProcAddr),
        ),
      textures = textures.map { VulkanBorrowedTexture(it.vulkanImage, it.vulkanImageView) },
      format = texture.format,
      finalLayout = texture.vulkanFinalLayout,
    )
  }

  fun webGpuBorrowedTexture(
    textures: List<TestTexture>,
    context: TestGraphicsContext,
  ): WebGpuBorrowedTextureDescriptor {
    val texture = textures.first()
    return WebGpuBorrowedTextureDescriptor(
      LogicalSize(texture.width, texture.height),
      physicalWidth = texture.width,
      physicalHeight = texture.height,
      context = WebGpuContextDescriptor(context.webGpuDevice),
      textures = textures.map { WebGpuBorrowedTexture(it.webGpuTexture) },
      format = texture.webGpuFormat,
    )
  }

  private fun pointer(address: Long): NativePointer = NativePointer.ofAddress(address)
}

/**
 * Submits a borrowed-texture attach on a backend whose attach only some targets declare. Each
 * target attaches the backends that its source sets declare and rejects the others.
 */
internal expect fun TestGraphics.attachBackendBorrowedTexture(
  map: MapHandle,
  textures: List<TestTexture>,
  context: TestGraphicsContext,
  options: RenderSessionAttachOptions,
): RenderSessionHandle

/** Reads the texture size of [frame] on a backend whose view only some targets declare. */
internal expect fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt>
