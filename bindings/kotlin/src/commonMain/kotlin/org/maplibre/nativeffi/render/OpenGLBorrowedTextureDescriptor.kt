package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL caller-owned texture render targets. */
public class OpenGLBorrowedTextureDescriptor(
  extent: RenderTargetExtent,
  context: OpenGLContextDescriptor,
  texture: Int,
  target: Int,
) {
  public var extent: RenderTargetExtent = extent

  public var context: OpenGLContextDescriptor = context

  public var texture: Int = texture

  public var target: Int = target
}
