package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor

internal object TestVulkanDriver {
  init {
    System.loadLibrary("binding_test_graphics")
  }

  external fun create(): LongArray?

  external fun destroy(context: Long)
}

internal fun attachAndroidVulkan(
  map: MapHandle,
  width: Int,
  height: Int,
  depth: UInt,
): OwnedTextureTestSession {
  val values =
    checkNotNull(TestVulkanDriver.create()) { "Vulkan test driver initialization failed" }
  val descriptor =
    VulkanContextDescriptor(
      NativePointer.ofAddress(values[1]),
      NativePointer.ofAddress(values[2]),
      NativePointer.ofAddress(values[3]),
      NativePointer.ofAddress(values[4]),
      values[5].toUInt(),
      NativePointer.ofAddress(values[6]),
      NativePointer.ofAddress(values[7]),
    )
  return attachOwnedTextureFixture(
    map,
    width,
    height,
    depth,
    attach = { target, w, h, options ->
      target.vulkanOwnedTextureAttach(
        VulkanOwnedTextureDescriptor(RenderTargetExtent(w.toUInt(), h.toUInt(), 1.0), descriptor),
        options,
      )
    },
    frameSize = { frame ->
      frame.withGetVulkanTexture { OwnedTextureFrameSize(it.width.toInt(), it.height.toInt()) }
    },
    releaseGraphics = { TestVulkanDriver.destroy(values[0]) },
  )
}
