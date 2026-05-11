package org.maplibre.nativeffi.render;

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

  public MetalOwnedTextureDescriptor size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public MetalOwnedTextureDescriptor scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public MetalOwnedTextureDescriptor device(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }
}
