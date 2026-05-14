package org.maplibre.nativeffi.render;

/** Mutable logical render target extent. */
public final class RenderTargetExtent {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;

  public RenderTargetExtent() {}

  public RenderTargetExtent(int width, int height, double scaleFactor) {
    this.width = width;
    this.height = height;
    this.scaleFactor = scaleFactor;
  }

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public RenderTargetExtent size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public RenderTargetExtent scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }
}
