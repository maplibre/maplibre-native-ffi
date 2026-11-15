package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.EglContextDescriptor
import org.maplibre.nativeffi.GpuDeviceHandle
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MetalBorrowedTexture
import org.maplibre.nativeffi.MetalBorrowedTexturesTarget
import org.maplibre.nativeffi.MetalDeviceDescriptor
import org.maplibre.nativeffi.NativePointer
import org.maplibre.nativeffi.OpenGlBorrowedTexture
import org.maplibre.nativeffi.OpenGlBorrowedTexturesTarget
import org.maplibre.nativeffi.OpenGlContextDescriptor
import org.maplibre.nativeffi.OpenGlContextProviderFlag
import org.maplibre.nativeffi.QueueLock
import org.maplibre.nativeffi.RenderBackend
import org.maplibre.nativeffi.RenderBackendFlag
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.RenderTarget
import org.maplibre.nativeffi.VulkanBorrowedTexture
import org.maplibre.nativeffi.VulkanBorrowedTexturesTarget
import org.maplibre.nativeffi.VulkanContextDescriptor
import org.maplibre.nativeffi.VulkanDeviceDescriptor
import org.maplibre.nativeffi.WebGlContextDescriptor
import org.maplibre.nativeffi.WebGpuBorrowedTexture
import org.maplibre.nativeffi.WebGpuBorrowedTexturesTarget
import org.maplibre.nativeffi.WglContextDescriptor
import org.maplibre.nativeffi.openGlSupportedContextProviderMask
import org.maplibre.nativeffi.supportedRenderBackendMask
import org.maplibre.nativeffi.testBackendName

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

  /** The render backend that a GPU device for this backend's tests asks for. */
  val renderBackend: RenderBackend
    get() =
      when (this) {
        METAL -> RenderBackend.METAL
        VULKAN -> RenderBackend.VULKAN
        EGL,
        WGL,
        WEBGL -> RenderBackend.OPEN_GL
        WEBGPU -> RenderBackend.WEB_GPU
      }

  companion object {
    /**
     * The backend under test: the one that [testBackendName] names, which the loaded library must
     * carry, or where nothing names one, the one backend that the library carries.
     */
    fun underTest(): TestBackend {
      val backends = supportedRenderBackendMask()
      val opengl = {
        val providers = openGlSupportedContextProviderMask()
        when {
          OpenGlContextProviderFlag.WGL in providers -> WGL
          OpenGlContextProviderFlag.WEB_GL in providers -> WEBGL
          else -> EGL
        }
      }
      val backend =
        when (val named = testBackendName()) {
          "metal" -> METAL
          "vulkan" -> VULKAN
          "webgpu" -> WEBGPU
          "opengl" -> opengl()
          null -> {
            val carried =
              listOf(
                  RenderBackendFlag.METAL to { METAL },
                  RenderBackendFlag.VULKAN to { VULKAN },
                  RenderBackendFlag.WEB_GPU to { WEBGPU },
                  RenderBackendFlag.OPEN_GL to opengl,
                )
                .filter { (flag, _) -> flag in backends }
            check(carried.size == 1) {
              "the loaded library renders with $backends; MLN_FFI_TEST_BACKEND must name the " +
                "backend under test"
            }
            carried.single().second()
          }
          else -> error("no tests drive the $named backend")
        }
      check(
        when (backend.renderBackend) {
          RenderBackend.METAL -> RenderBackendFlag.METAL
          RenderBackend.VULKAN -> RenderBackendFlag.VULKAN
          RenderBackend.WEB_GPU -> RenderBackendFlag.WEB_GPU
          else -> RenderBackendFlag.OPEN_GL
        } in backends,
      ) {
        "the loaded library lacks the backend under test, $backend: $backends"
      }
      return backend
    }
  }
}

/**
 * A canvas of the page's that a session presents to, in a browser, which has no readback. [close]
 * ends the module's reference to it.
 */
internal interface TestCanvas : AutoCloseable {
  /** The canvas's `mln_web_object` handle, for a web canvas target. */
  val handle: UInt

  /** The RGBA bytes of the canvas's center pixel, as [capture] last read it, or null before. */
  val centerPixel: List<Int>?

  /** Reads the canvas's center pixel. Call it in the task that presented the frame. */
  fun capture()
}

/**
 * Creates a [TestCanvas] of [width] x [height] in a browser, or returns null elsewhere, where a
 * session on a device of its own renders offscreen and reads back.
 */
internal expect fun createTestCanvas(width: Int, height: Int): TestCanvas?

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

  val backend: TestBackend by lazy { TestBackend.underTest() }

  /**
   * Whether sessions render into textures that the test graphics create. mln-rs's browser build
   * renders into canvases only.
   */
  val borrowsTextures: Boolean
    get() = backend != TestBackend.WEBGL && backend != TestBackend.WEBGPU

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
   * Wraps the build's device, or its OpenGL context, in a GPU device that a borrowed ring of
   * [createTextures]' textures renders on. A Vulkan device takes [queueLock], the lock on the queue
   * that the test graphics submit to. Release the device once the sessions that attach on it have
   * attached: each session keeps its device until it ends.
   */
  suspend fun wrapDevice(queueLock: QueueLock = QueueLock()): GpuDeviceHandle {
    val context = context()
    val openGl =
      when (backend) {
        TestBackend.EGL ->
          OpenGlContextDescriptor.Egl(
            EglContextDescriptor(
              pointer(context.eglDisplay),
              pointer(context.eglConfig),
              pointer(context.eglContext),
              NativePointer.NULL_POINTER,
            ),
          )
        TestBackend.WGL ->
          OpenGlContextDescriptor.Wgl(
            WglContextDescriptor(
              pointer(context.wglDeviceContext),
              pointer(context.wglContext),
              NativePointer.NULL_POINTER,
            ),
          )
        TestBackend.WEBGL ->
          OpenGlContextDescriptor.WebGl(WebGlContextDescriptor(context.webGlContext))
        TestBackend.METAL,
        TestBackend.VULKAN,
        TestBackend.WEBGPU -> return wrapBackendDevice(context, queueLock)
      }
    return GpuDeviceHandle.wrapOpenGl(openGl)
  }

  /** The borrowed ring of [textures], from [createTextures], on the build's backend. */
  fun borrowedTarget(textures: List<TestTexture>): RenderTarget {
    val texture = textures.first()
    val logicalSize = LogicalSize(texture.width, texture.height)
    return when (backend) {
      TestBackend.EGL,
      TestBackend.WGL,
      TestBackend.WEBGL ->
        RenderTarget.OpenGlBorrowedTextures(
          OpenGlBorrowedTexturesTarget(
            logicalSize,
            physicalWidth = texture.width,
            physicalHeight = texture.height,
            textures = textures.map { OpenGlBorrowedTexture(it.openGlTexture) },
            target = texture.openGlTarget,
          ),
        )
      TestBackend.METAL ->
        RenderTarget.MetalBorrowedTextures(
          MetalBorrowedTexturesTarget(
            logicalSize,
            physicalWidth = texture.width,
            physicalHeight = texture.height,
            textures = textures.map { MetalBorrowedTexture(pointer(it.metalTexture)) },
          ),
        )
      TestBackend.VULKAN ->
        RenderTarget.VulkanBorrowedTextures(
          VulkanBorrowedTexturesTarget(
            logicalSize,
            physicalWidth = texture.width,
            physicalHeight = texture.height,
            textures = textures.map { VulkanBorrowedTexture(it.vulkanImage, it.vulkanImageView) },
            format = texture.format,
            finalLayout = texture.vulkanFinalLayout,
          ),
        )
      TestBackend.WEBGPU ->
        RenderTarget.WebGpuBorrowedTextures(
          WebGpuBorrowedTexturesTarget(
            logicalSize,
            physicalWidth = texture.width,
            physicalHeight = texture.height,
            textures = textures.map { WebGpuBorrowedTexture(it.webGpuTexture) },
            format = texture.webGpuFormat,
          ),
        )
    }
  }

  /**
   * Submits an attach of a borrowed ring of [textures], from [createTextures], on a device from
   * [wrapDevice] with [queueLock], which the session alone keeps. Call it on [thread], where the
   * build's OpenGL context is current.
   */
  suspend fun attachBorrowedTexture(
    map: MapHandle,
    textures: List<TestTexture>,
    options: RenderSessionAttachOptions,
    queueLock: QueueLock = QueueLock(),
  ): RenderSessionHandle =
    wrapDevice(queueLock).use { device -> map.attach(borrowedTarget(textures), device, options) }

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

  /** The Vulkan device of [context], for `GpuDeviceHandle.wrapVulkan`, with [queueLock]. */
  fun vulkanDevice(context: TestGraphicsContext, queueLock: QueueLock): VulkanDeviceDescriptor =
    VulkanDeviceDescriptor(
      VulkanContextDescriptor(
        pointer(context.vulkanInstance),
        pointer(context.vulkanPhysicalDevice),
        pointer(context.vulkanDevice),
        pointer(context.vulkanQueue),
        context.vulkanQueueFamilyIndex,
        pointer(context.vulkanGetInstanceProcAddr),
        pointer(context.vulkanGetDeviceProcAddr),
      ),
      queueLock,
    )

  /** The Metal device of [context], for `GpuDeviceHandle.wrapMetal`, with its own queue. */
  fun metalDevice(context: TestGraphicsContext): MetalDeviceDescriptor =
    MetalDeviceDescriptor(pointer(context.metalDevice), NativePointer.NULL_POINTER)

  private fun pointer(address: Long): NativePointer = NativePointer.ofAddress(address)
}

/**
 * Wraps the device of [context] on a backend whose wrap only some targets declare: Metal, Vulkan
 * with [queueLock], or WebGPU. Each target wraps the backends that its source sets declare and
 * rejects the others.
 */
internal expect fun TestGraphics.wrapBackendDevice(
  context: TestGraphicsContext,
  queueLock: QueueLock,
): GpuDeviceHandle

/** Reads the texture size of [frame] on a backend whose view only some targets declare. */
internal expect fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt>
