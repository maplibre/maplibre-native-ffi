package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for EGL native surface render targets (Android / Linux). */
public final class EglSurfaceDescriptor {
  private int width = 256;
  private int height = 256;
  private double scaleFactor = 1.0;
  private NativePointer display = NativePointer.NULL;
  private NativePointer context = NativePointer.NULL;
  private NativePointer surface = NativePointer.NULL;

  public int width() {
    return width;
  }

  public int height() {
    return height;
  }

  public EglSurfaceDescriptor size(int width, int height) {
    this.width = width;
    this.height = height;
    return this;
  }

  public double scaleFactor() {
    return scaleFactor;
  }

  public EglSurfaceDescriptor scaleFactor(double scaleFactor) {
    this.scaleFactor = scaleFactor;
    return this;
  }

  public NativePointer display() {
    return display;
  }

  public EglSurfaceDescriptor display(NativePointer display) {
    this.display = Objects.requireNonNull(display, "display");
    return this;
  }

  public NativePointer context() {
    return context;
  }

  public EglSurfaceDescriptor context(NativePointer context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }

  public NativePointer surface() {
    return surface;
  }

  public EglSurfaceDescriptor surface(NativePointer surface) {
    this.surface = Objects.requireNonNull(surface, "surface");
    return this;
  }
}
