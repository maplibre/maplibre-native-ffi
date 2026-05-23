package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan session-owned texture render targets. */
public class VulkanOwnedTextureDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: VulkanContextDescriptor = VulkanContextDescriptor(),
) {
  public fun extent(extent: RenderTargetExtent): VulkanOwnedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: VulkanContextDescriptor): VulkanOwnedTextureDescriptor = apply {
    this.context = context
  }
}
