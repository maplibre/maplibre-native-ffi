package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.newSingleThreadContext
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.GeneratedApi
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
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.WglContextDescriptor

/** The handles of a tests/graphics context, as addresses. Only its backend's fields are set. */
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
)

/**
 * The tests/graphics library over this platform's FFI: FFM on the JVM, JNI on Android, and cinterop
 * on Kotlin/Native. See tests/graphics/README.md.
 */
internal expect object TestGraphicsLibrary {
  /** Creates a context for one MLN_TEST_GRAPHICS_BACKEND_* value, or throws with the reason. */
  fun create(backend: UInt): Long

  fun context(graphics: Long): TestGraphicsContext

  /** Makes an EGL or WGL context current on the calling thread, or throws with the reason. */
  fun makeCurrent(graphics: Long)
}

/** The graphics backend a build renders with, and its context from tests/graphics. */
internal enum class TestBackend(val graphicsBackend: UInt) {
  METAL(1u),
  VULKAN(2u),
  EGL(3u),
  WGL(4u);

  companion object {
    /** The one backend the loaded library renders with. */
    fun ofBuild(): TestBackend {
      val backends = GeneratedApi.supportedRenderBackendMask()
      return when {
        RenderBackendFlag.METAL in backends -> METAL
        RenderBackendFlag.VULKAN in backends -> VULKAN
        RenderBackendFlag.OPENGL in backends ->
          if (OpenglContextProviderFlag.WGL in GeneratedApi.openglSupportedContextProviderMask())
            WGL
          else EGL
        else -> error("the loaded library renders with no backend the tests drive: $backends")
      }
    }
  }
}

/**
 * The graphics thread every render test drives its sessions from, and the build's context on it.
 *
 * The context is created on first use and kept for the process: an EGL or WGL context stays current
 * on this one thread, and every caller-driven session fixes its graphics thread to it.
 */
@OptIn(DelicateCoroutinesApi::class, ExperimentalCoroutinesApi::class)
internal object TestGraphics {
  val thread: CoroutineDispatcher = newSingleThreadContext("maplibre-test-graphics")

  val backend: TestBackend by lazy { TestBackend.ofBuild() }

  // Read and written on [thread] only.
  private var shared: TestGraphicsContext? = null

  /** The build's context, made current on [thread] when its backend has a current context. */
  suspend fun context(): TestGraphicsContext =
    withContext(thread) {
      shared
        ?: run {
          val graphics = TestGraphicsLibrary.create(backend.graphicsBackend)
          if (backend == TestBackend.EGL || backend == TestBackend.WGL) {
            TestGraphicsLibrary.makeCurrent(graphics)
          }
          TestGraphicsLibrary.context(graphics).also { shared = it }
        }
    }

  /** Submits an owned-texture attach of [width] x [height] on the build's backend. */
  suspend fun attachOwnedTexture(
    map: MapHandle,
    width: Int,
    height: Int,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    val context = context()
    val extent = RenderTargetExtent(width.toUInt(), height.toUInt(), 1.0)
    return when (backend) {
      TestBackend.METAL ->
        map.metalOwnedTextureAttach(
          MetalOwnedTextureDescriptor(extent, MetalContextDescriptor(pointer(context.metalDevice))),
          options,
        )
      TestBackend.VULKAN ->
        map.vulkanOwnedTextureAttach(
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
        map.openglOwnedTextureAttach(
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
                )
              ),
            ),
          ),
          options,
        )
      TestBackend.WGL ->
        map.openglOwnedTextureAttach(
          OpenglOwnedTextureDescriptor(
            extent,
            OpenglContextDescriptor(
              OpenglContextOwnership.SHARED,
              OpenglContextDescriptorData.Wgl(
                WglContextDescriptor(
                  pointer(context.wglDeviceContext),
                  pointer(context.wglContext),
                  NativePointer.NULL_POINTER,
                )
              ),
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
      TestBackend.WGL -> frame.withOpenglTexture { it.width to it.height }
    }

  private fun pointer(address: Long): NativePointer = NativePointer.ofAddress(address)
}
