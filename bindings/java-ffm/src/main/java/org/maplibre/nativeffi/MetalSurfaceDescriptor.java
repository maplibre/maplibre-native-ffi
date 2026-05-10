package org.maplibre.nativeffi;

import java.util.Objects;

/** Mutable descriptor for Metal native surface render targets. */
public final class MetalSurfaceDescriptor {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;
  private NativePointer layer = NativePointer.NULL;
  private NativePointer device = NativePointer.NULL;

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public MetalSurfaceDescriptor setSize(int width, int height) {
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

  public MetalSurfaceDescriptor setScaleFactor(double scaleFactor) {
    if (!Double.isFinite(scaleFactor) || scaleFactor <= 0.0) {
      throw new IllegalArgumentException("scaleFactor must be finite and positive");
    }
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer layer() {
    return layer;
  }

  public MetalSurfaceDescriptor setLayer(NativePointer layer) {
    this.layer = Objects.requireNonNull(layer, "layer");
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public MetalSurfaceDescriptor setDevice(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }
}
