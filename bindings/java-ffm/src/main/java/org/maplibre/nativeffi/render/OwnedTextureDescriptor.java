package org.maplibre.nativeffi.render;

/** Mutable descriptor for session-owned offscreen texture render targets. */
public final class OwnedTextureDescriptor {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public OwnedTextureDescriptor size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public OwnedTextureDescriptor scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }
}
