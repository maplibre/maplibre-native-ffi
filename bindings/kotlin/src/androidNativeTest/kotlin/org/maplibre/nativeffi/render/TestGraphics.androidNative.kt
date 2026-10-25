package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.NewRenderSession
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.attachVulkanOwnedTexture
import org.maplibre.nativeffi.generated.withVulkanTexture

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
