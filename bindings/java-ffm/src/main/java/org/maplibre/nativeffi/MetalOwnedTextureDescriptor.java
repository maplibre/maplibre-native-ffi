package org.maplibre.nativeffi;

import java.util.Objects;

/** Mutable descriptor for Metal session-owned texture render targets. */
public final class MetalOwnedTextureDescriptor {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;
  private NativePointer device = NativePointer.NULL;

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public MetalOwnedTextureDescriptor setSize(int width, int height) {
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

  public MetalOwnedTextureDescriptor setScaleFactor(double scaleFactor) {
    if (!Double.isFinite(scaleFactor) || scaleFactor <= 0.0) {
      throw new IllegalArgumentException("scaleFactor must be finite and positive");
    }
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public MetalOwnedTextureDescriptor setDevice(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }
}
