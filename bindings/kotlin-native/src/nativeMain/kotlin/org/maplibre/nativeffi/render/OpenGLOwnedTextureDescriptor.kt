package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL session-owned texture render targets. */
public class OpenGLOwnedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: OpenGLContextDescriptor = WglContextDescriptor(),
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var context: OpenGLContextDescriptor = context
    private set

  public fun extent(extent: RenderTargetExtent): OpenGLOwnedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: OpenGLContextDescriptor): OpenGLOwnedTextureDescriptor = apply {
    this.context = context
  }
}
