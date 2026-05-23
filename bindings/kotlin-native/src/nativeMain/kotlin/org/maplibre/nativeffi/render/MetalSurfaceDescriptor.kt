package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal native surface render targets. */
public class MetalSurfaceDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: MetalContextDescriptor = MetalContextDescriptor(),
  public var layer: NativePointer = NativePointer.NULL,
) {
  public fun extent(extent: RenderTargetExtent): MetalSurfaceDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: MetalContextDescriptor): MetalSurfaceDescriptor = apply {
    this.context = context
  }

  public fun layer(layer: NativePointer): MetalSurfaceDescriptor = apply { this.layer = layer }
}
