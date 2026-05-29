package org.maplibre.nativeffi.render

/** Mutable descriptor for Vulkan session-owned texture render targets. */
public class VulkanOwnedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: VulkanContextDescriptor = VulkanContextDescriptor(),
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var context: VulkanContextDescriptor = context
    private set

  public fun extent(extent: RenderTargetExtent): VulkanOwnedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: VulkanContextDescriptor): VulkanOwnedTextureDescriptor = apply {
    this.context = context
  }
}
