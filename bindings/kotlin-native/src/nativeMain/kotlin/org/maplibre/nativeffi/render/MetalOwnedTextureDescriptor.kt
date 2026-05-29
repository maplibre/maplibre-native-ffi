package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal session-owned texture render targets. */
public class MetalOwnedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: MetalContextDescriptor = MetalContextDescriptor(),
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var context: MetalContextDescriptor = context
    private set

  public fun extent(extent: RenderTargetExtent): MetalOwnedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: MetalContextDescriptor): MetalOwnedTextureDescriptor = apply {
    this.context = context
  }
}
