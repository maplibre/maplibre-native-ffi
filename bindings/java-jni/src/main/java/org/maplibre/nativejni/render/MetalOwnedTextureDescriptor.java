package org.maplibre.nativejni.render;

import java.util.Objects;

/** Mutable descriptor for Metal session-owned texture render targets. */
public final class MetalOwnedTextureDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private MetalContextDescriptor context = new MetalContextDescriptor();

  public RenderTargetExtent extent() {
    return extent;
  }

  public MetalOwnedTextureDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public MetalContextDescriptor context() {
    return context;
  }

  public MetalOwnedTextureDescriptor context(MetalContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }
}
