package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL caller-owned texture render targets. */
public class OpenGLBorrowedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: OpenGLContextDescriptor = WglContextDescriptor(),
  texture: Int = 0,
  target: Int = 0,
) {
  public var extent: RenderTargetExtent = extent

  public var context: OpenGLContextDescriptor = context

  public var texture: Int = texture

  public var target: Int = target
}
