package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for session-owned offscreen texture render targets. */
public final class OwnedTextureDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();

  public RenderTargetExtent extent() {
    return extent;
  }

  public OwnedTextureDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }
}
