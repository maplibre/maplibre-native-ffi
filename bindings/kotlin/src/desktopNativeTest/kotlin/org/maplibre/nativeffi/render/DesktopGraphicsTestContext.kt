@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import kotlinx.cinterop.pointed
import kotlinx.cinterop.rawValue
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
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_create
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_destroy
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
  val graphics =
    checkNotNull(mln_test_graphics_create(vulkan)) {
      "Test graphics driver initialization failed: $backends"
    }
  if (!vulkan && !mln_test_graphics_make_current(graphics)) {
    mln_test_graphics_destroy(graphics)
    error("EGL test context creation failed")
  }
  val state = graphics.pointed
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
              NativePointer.ofAddress(state.instance.rawValue.toLong()),
              NativePointer.ofAddress(state.physical_device.rawValue.toLong()),
              NativePointer.ofAddress(state.device.rawValue.toLong()),
              NativePointer.ofAddress(state.queue.rawValue.toLong()),
              state.queue_family,
              NativePointer.ofAddress(state.get_instance_proc_addr.rawValue.toLong()),
              NativePointer.ofAddress(state.get_device_proc_addr.rawValue.toLong()),
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
                  NativePointer.ofAddress(state.display.rawValue.toLong()),
                  NativePointer.ofAddress(state.config.rawValue.toLong()),
                  NativePointer.ofAddress(state.context.rawValue.toLong()),
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
