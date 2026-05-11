package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for Metal caller-owned texture render targets. */
public final class MetalBorrowedTextureDescriptor {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;
  private NativePointer texture = NativePointer.NULL;

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public MetalBorrowedTextureDescriptor size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public MetalBorrowedTextureDescriptor scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
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
