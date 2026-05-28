package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL caller-owned texture render targets. */
public class OpenGLBorrowedTextureDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: OpenGLContextDescriptor = WglContextDescriptor(),
  public var texture: Int = 0,
  public var target: Int = 0,
) {
  public fun extent(extent: RenderTargetExtent): OpenGLBorrowedTextureDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: OpenGLContextDescriptor): OpenGLBorrowedTextureDescriptor = apply {
    this.context = context
  }

  public fun texture(texture: Int): OpenGLBorrowedTextureDescriptor = apply {
    this.texture = texture
  }

  public fun target(target: Int): OpenGLBorrowedTextureDescriptor = apply { this.target = target }
}
