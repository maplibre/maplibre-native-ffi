package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable descriptor for OpenGL caller-owned texture render targets. */
public final class OpenGLBorrowedTextureDescriptor {
  private RenderTargetExtent extent = new RenderTargetExtent();
  private OpenGLContextDescriptor context = new WglContextDescriptor();
  private int texture;
  private int target;

  public RenderTargetExtent extent() {
    return extent;
  }

  public OpenGLBorrowedTextureDescriptor extent(RenderTargetExtent extent) {
    this.extent = Objects.requireNonNull(extent, "extent");
    return this;
  }

  public OpenGLContextDescriptor context() {
    return context;
  }

  public OpenGLBorrowedTextureDescriptor context(OpenGLContextDescriptor context) {
    this.context = Objects.requireNonNull(context, "context");
    return this;
  }

  public int texture() {
    return texture;
  }

  public OpenGLBorrowedTextureDescriptor texture(int texture) {
    this.texture = texture;
    return this;
  }

  public int target() {
    return target;
  }

  public OpenGLBorrowedTextureDescriptor target(int target) {
    this.target = target;
    return this;
  }
}
