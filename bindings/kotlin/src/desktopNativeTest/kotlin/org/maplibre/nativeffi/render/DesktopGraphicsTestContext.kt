@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import cnames.structs.mln_test_graphics
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.OpenglClientApi
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.RenderBackendFlag
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.internal.graphics.MLN_TEST_GRAPHICS_BACKEND_EGL
import org.maplibre.nativeffi.internal.graphics.MLN_TEST_GRAPHICS_BACKEND_VULKAN
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_context
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_create
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_destroy
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_get_context
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_last_error
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_make_current

internal fun attachDesktopOwnedTexture(
  map: MapHandle,
  width: Int,
  height: Int,
  depth: UInt,
): OwnedTextureTestSession {
  val backends = GeneratedApi.supportedRenderBackendMask()
  val vulkan = RenderBackendFlag.VULKAN in backends
  check(vulkan || RenderBackendFlag.OPENGL in backends) { "No desktop test driver for $backends" }
  val backend = if (vulkan) MLN_TEST_GRAPHICS_BACKEND_VULKAN else MLN_TEST_GRAPHICS_BACKEND_EGL
  val graphics =
    checkNotNull(mln_test_graphics_create(backend.toUInt())) {
      "Test graphics driver initialization failed: ${lastGraphicsError()}"
    }
  if (!vulkan && !mln_test_graphics_make_current(graphics)) {
    val reason = lastGraphicsError()
    mln_test_graphics_destroy(graphics)
    error("EGL test context creation failed: $reason")
  }
  val state = TestGraphicsContext.of(graphics)
  return attachOwnedTextureFixture(
    map,
    width,
    height,
    depth,
    attach = { target, w, h, options ->
      val extent = RenderTargetExtent(w.toUInt(), h.toUInt(), 1.0)
      if (vulkan)
        target.vulkanOwnedTextureAttach(
          VulkanOwnedTextureDescriptor(
            extent,
            VulkanContextDescriptor(
              NativePointer.ofAddress(state.vulkanInstance),
              NativePointer.ofAddress(state.vulkanPhysicalDevice),
              NativePointer.ofAddress(state.vulkanDevice),
              NativePointer.ofAddress(state.vulkanQueue),
              state.vulkanQueueFamilyIndex,
              NativePointer.ofAddress(state.vulkanGetInstanceProcAddr),
              NativePointer.ofAddress(state.vulkanGetDeviceProcAddr),
            ),
          ),
          options,
        )
      else
        target.openglOwnedTextureAttach(
          OpenglOwnedTextureDescriptor(
            extent,
            OpenglContextDescriptor(
              OpenglContextOwnership.SHARED,
              OpenglContextDescriptorData.Egl(
                EglContextDescriptor(
                  NativePointer.ofAddress(state.eglDisplay),
                  NativePointer.ofAddress(state.eglConfig),
                  NativePointer.ofAddress(state.eglContext),
                  OpenglClientApi.GLES,
                  NativePointer.NULL_POINTER,
                )
              ),
            ),
          ),
          options,
        )
    },
    frameSize = { frame ->
      if (vulkan)
        frame.withGetVulkanTexture { OwnedTextureFrameSize(it.width.toInt(), it.height.toInt()) }
      else frame.withGetOpenglTexture { OwnedTextureFrameSize(it.width.toInt(), it.height.toInt()) }
    },
    releaseGraphics = { mln_test_graphics_destroy(graphics) },
  )
}

private fun lastGraphicsError(): String = mln_test_graphics_last_error()?.toKString().orEmpty()

/** The handles of a tests/graphics context, as addresses. */
private class TestGraphicsContext(
  val vulkanInstance: Long,
  val vulkanPhysicalDevice: Long,
  val vulkanDevice: Long,
  val vulkanQueue: Long,
  val vulkanQueueFamilyIndex: UInt,
  val vulkanGetInstanceProcAddr: Long,
  val vulkanGetDeviceProcAddr: Long,
  val eglDisplay: Long,
  val eglConfig: Long,
  val eglContext: Long,
) {
  companion object {
    fun of(graphics: CPointer<mln_test_graphics>): TestGraphicsContext = memScoped {
      val context = alloc<mln_test_graphics_context>()
      check(mln_test_graphics_get_context(graphics, context.ptr)) {
        "Test graphics context lookup failed: ${lastGraphicsError()}"
      }
      TestGraphicsContext(
        context.vulkan_instance.rawValue.toLong(),
        context.vulkan_physical_device.rawValue.toLong(),
        context.vulkan_device.rawValue.toLong(),
        context.vulkan_queue.rawValue.toLong(),
        context.vulkan_queue_family_index,
        context.vulkan_get_instance_proc_addr.rawValue.toLong(),
        context.vulkan_get_device_proc_addr.rawValue.toLong(),
        context.egl_display.rawValue.toLong(),
        context.egl_config.rawValue.toLong(),
        context.egl_context.rawValue.toLong(),
      )
    }
  }
}
