package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.NewRenderSession
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.attachVulkanOwnedTexture
import org.maplibre.nativeffi.withVulkanTexture

internal actual fun TestGraphics.attachBackendOwnedTexture(
  map: MapHandle,
  extent: LogicalExtent,
  context: TestGraphicsContext,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  when (backend) {
    TestBackend.VULKAN -> map.attachVulkanOwnedTexture(vulkanOwnedTexture(extent, context), options)
    else -> error("this target declares no attach for $backend")
  }

internal actual fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
  when (backend) {
    TestBackend.VULKAN -> frame.withVulkanTexture { it.width to it.height }
    else -> error("this target declares no texture view for $backend")
  }
