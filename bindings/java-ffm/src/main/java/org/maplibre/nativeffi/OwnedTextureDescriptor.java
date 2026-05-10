package org.maplibre.nativeffi;

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

  public OwnedTextureDescriptor setSize(int width, int height) {
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

  public OwnedTextureDescriptor setScaleFactor(double scaleFactor) {
    if (!Double.isFinite(scaleFactor) || scaleFactor <= 0.0) {
      throw new IllegalArgumentException("scaleFactor must be finite and positive");
    }
    this.scaleFactor = scaleFactor;
    return this;
  }
}
