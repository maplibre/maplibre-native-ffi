package org.maplibre.nativejni.render;

import java.util.Objects;

/** Mutable descriptor for Metal native surface render targets. */
public final class MetalSurfaceDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private MetalContextDescriptor context = new MetalContextDescriptor();
  private NativePointer layer = NativePointer.NULL;

  public RenderTargetExtent extent() {
    return extent;
  }

  public MetalSurfaceDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public MetalContextDescriptor context() {
    return context;
  }

  public MetalSurfaceDescriptor context(MetalContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }

  public NativePointer layer() {
    return layer;
  }

  public MetalSurfaceDescriptor layer(NativePointer layer) {
    this.layer = Objects.requireNonNull(layer, "layer");
    return this;
  }
}
