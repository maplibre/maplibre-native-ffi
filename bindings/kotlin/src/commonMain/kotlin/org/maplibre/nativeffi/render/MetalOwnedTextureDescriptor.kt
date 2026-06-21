package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal session-owned texture render targets. */
public class MetalOwnedTextureDescriptor(
  extent: RenderTargetExtent,
  context: MetalContextDescriptor,
) {
  public var extent: RenderTargetExtent = extent

  public var context: MetalContextDescriptor = context
}
