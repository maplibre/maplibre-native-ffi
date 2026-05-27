package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for OpenGL native surface render targets. */
public final class OpenGLSurfaceDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private OpenGLContextDescriptor context = new WglContextDescriptor();
  private NativePointer surface = NativePointer.NULL;

  public RenderTargetExtent extent() {
    return extent;
  }

  public OpenGLSurfaceDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public OpenGLContextDescriptor context() {
    return context;
  }

  public OpenGLSurfaceDescriptor context(OpenGLContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }

  public NativePointer surface() {
    return surface;
  }

  public OpenGLSurfaceDescriptor surface(NativePointer surface) {
    this.surface = Objects.requireNonNull(surface, "surface");
    return this;
  }
}
