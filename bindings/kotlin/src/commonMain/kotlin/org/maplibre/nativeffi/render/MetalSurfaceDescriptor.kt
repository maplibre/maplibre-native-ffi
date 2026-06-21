package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal native surface render targets. */
public class MetalSurfaceDescriptor(
  extent: RenderTargetExtent,
  context: MetalContextDescriptor,
  layer: NativePointer,
) {
  public var extent: RenderTargetExtent = extent

  public var context: MetalContextDescriptor = context

  public var layer: NativePointer = layer
}
