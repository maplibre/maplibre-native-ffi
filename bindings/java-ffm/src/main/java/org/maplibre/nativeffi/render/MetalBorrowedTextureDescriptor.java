package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for Metal caller-owned texture render targets. */
public final class MetalBorrowedTextureDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private NativePointer texture = NativePointer.NULL;

  public RenderTargetExtent extent() {
    return extent;
  }

  public MetalBorrowedTextureDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public NativePointer texture() {
    return texture;
  }

  public MetalBorrowedTextureDescriptor texture(NativePointer texture) {
    this.texture = Objects.requireNonNull(texture, "texture");
    return this;
  }
}
