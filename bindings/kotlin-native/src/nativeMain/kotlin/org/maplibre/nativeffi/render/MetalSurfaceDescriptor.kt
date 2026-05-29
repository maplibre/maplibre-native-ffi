package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal native surface render targets. */
public class MetalSurfaceDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: MetalContextDescriptor = MetalContextDescriptor(),
  layer: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent

  public var context: MetalContextDescriptor = context

  public var layer: NativePointer = layer
}
