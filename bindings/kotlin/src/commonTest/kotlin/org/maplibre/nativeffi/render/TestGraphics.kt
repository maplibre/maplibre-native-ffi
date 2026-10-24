package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglClientApi
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglContextProviderFlag
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.RenderBackendFlag
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.WebglContextDescriptor
import org.maplibre.nativeffi.generated.WebglContextKind
import org.maplibre.nativeffi.generated.WebgpuContextDescriptor
import org.maplibre.nativeffi.generated.WebgpuOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.WglContextDescriptor

/**
 * The handles of a test graphics context, as addresses. Only its backend's fields are set. A WebGL
 * context is an `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, and a WebGPU device is an emdawnwebgpu handle:
 * the page creates them, and the binding imports them through `WebGraphics`.
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
  val webglContext: Int = 0,
  val webgpuDevice: Long = 0,
)

/**
 * A canvas that a session's core worker takes over, named by [selector]. [close] removes it once
 * that session has closed.
 */
internal class TestCanvas(val selector: String, private val remove: () -> Unit) : AutoCloseable {
  override fun close() = remove()
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
   * Creates a canvas of [width] x [height] that a WebGL session transfers to its core worker. Only
   * the browser fixtures have one; elsewhere this throws.
   */
  fun transferredCanvas(width: Int, height: Int): TestCanvas
}

/** The graphics backend a build renders with, and its context from the test graphics library. */
internal enum class TestBackend(val graphicsBackend: UInt) {
  METAL(1u),
  VULKAN(2u),
  EGL(3u),
  WGL(4u),
  /**
   * WebGL 2 in the browser. tests/graphics leaves WebGL to JavaScript, so this value, past the
   * tests/graphics values, names the browser fixtures: the page creates the context, and the
   * binding imports it through `WebGraphics`.
   */
  WEBGL(5u),
  /** WebGPU in the browser, from the browser fixtures, as for [WEBGL]. */
  WEBGPU(6u);

  companion object {
    /** The one backend the loaded library renders with. */
    fun ofBuild(): TestBackend {
      val backends = GeneratedApi.supportedRenderBackendMask()
      return when {
        RenderBackendFlag.METAL in backends -> METAL
        RenderBackendFlag.VULKAN in backends -> VULKAN
        RenderBackendFlag.WEBGPU in backends -> WEBGPU
        RenderBackendFlag.OPENGL in backends -> {
          val providers = GeneratedApi.openglSupportedContextProviderMask()
          when {
            OpenglContextProviderFlag.WGL in providers -> WGL
            OpenglContextProviderFlag.WEBGL in providers -> WEBGL
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

  /**
   * The driver of the owned texture that REND-01 reads back. WebGL reads it back from a core worker
   * over a transferred canvas, and the other render tests drive the page's existing context, so the
   * suite covers both WebGL context kinds. Every other backend reads it back on [thread].
   */
  val readbackDriver: RenderDriverKind
    get() =
      if (backend == TestBackend.WEBGL) RenderDriverKind.CORE_WORKER
      else RenderDriverKind.CALLER_GRAPHICS_THREAD

  // Read and written on [thread] only.
  private var shared: TestGraphicsContext? = null

  /** The build's context, made current on [thread] when its backend has a current context. */
  suspend fun context(): TestGraphicsContext =
    withContext(thread) {
      shared
        ?: run {
          val graphics = TestGraphicsLibrary.create(backend.graphicsBackend)
          if (
            backend == TestBackend.EGL || backend == TestBackend.WGL || backend == TestBackend.WEBGL
          ) {
            TestGraphicsLibrary.makeCurrent(graphics)
          }
          TestGraphicsLibrary.context(graphics).also { shared = it }
        }
    }

  /**
   * The canvas that an owned texture of [width] x [height] takes over when [driver] drives it, or
   * null when it renders with the build's context instead. Close it after the texture's session.
   */
  fun ownedTextureCanvas(width: Int, height: Int, driver: RenderDriverKind): TestCanvas? =
    if (driver == RenderDriverKind.CORE_WORKER) TestGraphicsLibrary.transferredCanvas(width, height)
    else null

  /**
   * Submits an owned-texture attach of [width] x [height] on the build's backend. A texture with a
   * [canvas] from [ownedTextureCanvas] renders on a core worker over that canvas, and any other on
   * [thread] with the build's context.
   */
  suspend fun attachOwnedTexture(
    map: MapHandle,
    width: Int,
    height: Int,
    options: RenderSessionAttachOptions,
    canvas: TestCanvas? = null,
  ): RenderSessionAttachment {
    val extent = LogicalExtent(width.toUInt(), height.toUInt(), 1.0)
    if (canvas != null) {
      return map.attachOpenglOwnedTexture(
        OpenglOwnedTextureDescriptor(
          extent,
          OpenglContextDescriptor(
            OpenglContextOwnership.DEDICATED,
            OpenglContextDescriptorData.Webgl(
              WebglContextDescriptor(
                WebglContextKind.TRANSFERRED_CANVAS,
                canvasSelector = canvas.selector,
              ),
            ),
          ),
        ),
        options,
      )
    }
    val context = context()
    return when (backend) {
      TestBackend.METAL ->
        map.attachMetalOwnedTexture(
          MetalOwnedTextureDescriptor(
            extent,
            MetalContextDescriptor(pointer(context.metalDevice)),
            // MTLPixelFormatBGRA8Unorm.
            pixelFormat = 80uL,
          ),
          options,
        )
      TestBackend.VULKAN ->
        map.attachVulkanOwnedTexture(
          VulkanOwnedTextureDescriptor(
            extent,
            VulkanContextDescriptor(
              pointer(context.vulkanInstance),
              pointer(context.vulkanPhysicalDevice),
              pointer(context.vulkanDevice),
              pointer(context.vulkanQueue),
              context.vulkanQueueFamilyIndex,
              pointer(context.vulkanGetInstanceProcAddr),
              pointer(context.vulkanGetDeviceProcAddr),
            ),
          ),
          options,
        )
      TestBackend.EGL ->
        map.attachOpenglOwnedTexture(
          OpenglOwnedTextureDescriptor(
            extent,
            OpenglContextDescriptor(
              OpenglContextOwnership.SHARED,
              OpenglContextDescriptorData.Egl(
                EglContextDescriptor(
                  pointer(context.eglDisplay),
                  pointer(context.eglConfig),
                  pointer(context.eglContext),
                  OpenglClientApi.GLES,
                  NativePointer.NULL_POINTER,
                ),
              ),
            ),
          ),
          options,
        )
      TestBackend.WGL ->
        map.attachOpenglOwnedTexture(
          OpenglOwnedTextureDescriptor(
            extent,
            OpenglContextDescriptor(
              OpenglContextOwnership.SHARED,
              OpenglContextDescriptorData.Wgl(
                WglContextDescriptor(
                  pointer(context.wglDeviceContext),
                  pointer(context.wglContext),
                  NativePointer.NULL_POINTER,
                ),
              ),
            ),
          ),
          options,
        )
      TestBackend.WEBGL ->
        map.attachOpenglOwnedTexture(
          OpenglOwnedTextureDescriptor(
            extent,
            OpenglContextDescriptor(
              OpenglContextOwnership.SHARED,
              OpenglContextDescriptorData.Webgl(
                WebglContextDescriptor(WebglContextKind.EXISTING, context.webglContext),
              ),
            ),
          ),
          options,
        )
      TestBackend.WEBGPU ->
        map.attachWebgpuOwnedTexture(
          WebgpuOwnedTextureDescriptor(
            extent,
            // Sessions use no instance, and null names the device's default queue, as a browser
            // host without a queue of its own hands over.
            WebgpuContextDescriptor(
              NativePointer.NULL_POINTER,
              pointer(context.webgpuDevice),
              NativePointer.NULL_POINTER,
            ),
          ),
          options,
        )
    }
  }

  /** Reads the size of the backend texture behind [frame] through its typed view. */
  fun frameTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
    when (backend) {
      TestBackend.METAL -> frame.withMetalTexture { it.width to it.height }
      TestBackend.VULKAN -> frame.withVulkanTexture { it.width to it.height }
      TestBackend.EGL,
      TestBackend.WGL,
      TestBackend.WEBGL -> frame.withOpenglTexture { it.width to it.height }
      TestBackend.WEBGPU -> frame.withWebgpuTexture { it.width to it.height }
    }

  private fun pointer(address: Long): NativePointer = NativePointer.ofAddress(address)
}
