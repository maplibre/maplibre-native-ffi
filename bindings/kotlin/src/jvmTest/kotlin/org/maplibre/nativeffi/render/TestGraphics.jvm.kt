package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.attachMetalBorrowedTexture
import org.maplibre.nativeffi.attachVulkanBorrowedTexture
import org.maplibre.nativeffi.withMetalTexture
import org.maplibre.nativeffi.withVulkanTexture

internal actual fun TestGraphics.attachBackendBorrowedTexture(
  map: MapHandle,
  textures: List<TestTexture>,
  context: TestGraphicsContext,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  when (backend) {
    TestBackend.METAL -> map.attachMetalBorrowedTexture(metalBorrowedTexture(textures), options)
    TestBackend.VULKAN ->
      map.attachVulkanBorrowedTexture(vulkanBorrowedTexture(textures, context), options)
    else -> error("this target declares no attach for $backend")
  }

internal actual fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
  when (backend) {
    TestBackend.METAL -> frame.withMetalTexture { it.width to it.height }
    TestBackend.VULKAN -> frame.withVulkanTexture { it.width to it.height }
    else -> error("this target declares no texture view for $backend")
  }
