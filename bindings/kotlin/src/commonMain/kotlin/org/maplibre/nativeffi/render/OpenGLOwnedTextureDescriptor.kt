package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL session-owned texture render targets. */
public class OpenGLOwnedTextureDescriptor(
  extent: RenderTargetExtent,
  context: OpenGLContextDescriptor,
) {
  public var extent: RenderTargetExtent = extent

  public var context: OpenGLContextDescriptor = context
}
