package org.maplibre.nativejni.render;

import java.util.Objects;

/** Mutable descriptor for OpenGL session-owned texture render targets. */
public final class OpenGLOwnedTextureDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private OpenGLContextDescriptor context = new WglContextDescriptor();

  public RenderTargetExtent extent() {
    return extent;
  }

  public OpenGLOwnedTextureDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public OpenGLContextDescriptor context() {
    return context;
  }

  public OpenGLOwnedTextureDescriptor context(OpenGLContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }
}
