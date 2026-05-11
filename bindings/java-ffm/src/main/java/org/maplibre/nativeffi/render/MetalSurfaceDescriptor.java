package org.maplibre.nativeffi.render;

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

  public MetalSurfaceDescriptor size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public MetalSurfaceDescriptor scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer layer() {
    return layer;
  }

  public MetalSurfaceDescriptor layer(NativePointer layer) {
    this.layer = Objects.requireNonNull(layer, "layer");
    return this;
  }

  public NativePointer device() {
    return device;
  }

  public MetalSurfaceDescriptor device(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }
}
