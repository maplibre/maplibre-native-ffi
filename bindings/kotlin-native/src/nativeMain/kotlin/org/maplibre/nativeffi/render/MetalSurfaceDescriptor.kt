package org.maplibre.nativeffi.render

/** Mutable descriptor for Metal native surface render targets. */
public class MetalSurfaceDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: MetalContextDescriptor = MetalContextDescriptor(),
  layer: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var context: MetalContextDescriptor = context
    private set

  public var layer: NativePointer = layer
    private set

  public fun extent(extent: RenderTargetExtent): MetalSurfaceDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: MetalContextDescriptor): MetalSurfaceDescriptor = apply {
    this.context = context
  }

  public fun layer(layer: NativePointer): MetalSurfaceDescriptor = apply { this.layer = layer }
}
