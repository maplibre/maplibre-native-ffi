package org.maplibre.nativeffi;

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

  public MetalBorrowedTextureDescriptor setSize(int width, int height) {
    if (width <= 0 || height <= 0) {
      throw new IllegalArgumentException("width and height must be positive");
    }
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public MetalBorrowedTextureDescriptor setScaleFactor(double scaleFactor) {
    if (!Double.isFinite(scaleFactor) || scaleFactor <= 0.0) {
      throw new IllegalArgumentException("scaleFactor must be finite and positive");
    }
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer texture() {
    return texture;
  }

  public MetalBorrowedTextureDescriptor setTexture(NativePointer texture) {
    this.texture = Objects.requireNonNull(texture, "texture");
    return this;
  }
}
