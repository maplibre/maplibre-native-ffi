package org.maplibre.nativeffi.render

/**
 * Mutable descriptor for a Metal `CAMetalLayer` surface.
 *
 * The session writes the layer's `drawableSize` from [extent]'s physical size at attach, resize,
 * and set-target. A `drawableSize` the host already set is overwritten.
 */
public class MetalSurfaceDescriptor(
  extent: RenderTargetExtent,
  context: MetalContextDescriptor,
  layer: NativePointer,
) {
  public var extent: RenderTargetExtent = extent

  public var context: MetalContextDescriptor = context

  /** Borrowed `CAMetalLayer`. Required. The session retains it. */
  public var layer: NativePointer = layer
}
