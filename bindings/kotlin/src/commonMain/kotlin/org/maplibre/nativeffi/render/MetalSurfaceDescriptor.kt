package org.maplibre.nativeffi.render

/**
 * Mutable descriptor for Metal native surface render targets.
 *
 * The session writes the layer's `drawableSize` from [extent]'s physical size at attach, resize,
 * and set-target.
 */
public class MetalSurfaceDescriptor(
  extent: RenderTargetExtent,
  context: MetalContextDescriptor,
  layer: NativePointer,
) {
  public var extent: RenderTargetExtent = extent

  public var context: MetalContextDescriptor = context

  /** Borrowed `CAMetalLayer`. The session retains it. */
  public var layer: NativePointer = layer
}
