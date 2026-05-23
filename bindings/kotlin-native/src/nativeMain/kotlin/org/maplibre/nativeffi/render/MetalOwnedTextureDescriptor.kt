package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal session-owned texture render targets. */
public class MetalOwnedTextureDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: MetalContextDescriptor = MetalContextDescriptor(),
) {
  public fun extent(extent: RenderTargetExtent): MetalOwnedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: MetalContextDescriptor): MetalOwnedTextureDescriptor = apply {
    this.context = context
  }
}
