package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL session-owned texture render targets. */
public class OpenGLOwnedTextureDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: OpenGLContextDescriptor = WglContextDescriptor(),
) {
  public fun extent(extent: RenderTargetExtent): OpenGLOwnedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: OpenGLContextDescriptor): OpenGLOwnedTextureDescriptor = apply {
    this.context = context
  }
}
