package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan session-owned texture render targets. */
public class VulkanOwnedTextureDescriptor(
  extent: RenderTargetExtent,
  context: VulkanContextDescriptor,
) {
  public var extent: RenderTargetExtent = extent

  public var context: VulkanContextDescriptor = context
}
