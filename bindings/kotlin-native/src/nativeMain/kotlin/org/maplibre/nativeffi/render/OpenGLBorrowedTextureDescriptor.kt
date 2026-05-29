package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL caller-owned texture render targets. */
public class OpenGLBorrowedTextureDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: OpenGLContextDescriptor = WglContextDescriptor(),
  texture: Int = 0,
  target: Int = 0,
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var context: OpenGLContextDescriptor = context
    private set

  public var texture: Int = texture
    private set

  public var target: Int = target
    private set

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
