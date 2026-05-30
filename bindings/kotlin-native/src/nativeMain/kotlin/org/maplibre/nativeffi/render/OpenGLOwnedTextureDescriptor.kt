package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL session-owned texture render targets. */
public class OpenGLOwnedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: OpenGLContextDescriptor = WglContextDescriptor(),
) {
  public var extent: RenderTargetExtent = extent

  public var context: OpenGLContextDescriptor = context
}
