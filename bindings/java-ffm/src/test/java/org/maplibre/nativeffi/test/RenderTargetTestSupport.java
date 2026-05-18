package org.maplibre.nativeffi.test;

import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.render.VulkanContextDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;

public final class RenderTargetTestSupport {
  private RenderTargetTestSupport() {}

  public static RenderSessionHandle attachVulkanOwnedTexture(
      MapHandle map, RenderTargetExtent extent) {
    return map.attachVulkanOwnedTexture(
        new VulkanOwnedTextureDescriptor()
            .extent(extent)
            .context(new VulkanContextDescriptor()));
  }
}
