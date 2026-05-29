package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL native surface render targets. */
public class OpenGLSurfaceDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: OpenGLContextDescriptor = WglContextDescriptor(),
  surface: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent
    private set

  public var context: OpenGLContextDescriptor = context
    private set

  public var surface: NativePointer = surface
    private set

  public fun extent(extent: RenderTargetExtent): OpenGLSurfaceDescriptor = apply {
    this.extent = extent
  }

  public fun context(context: OpenGLContextDescriptor): OpenGLSurfaceDescriptor = apply {
    this.context = context
  }

  public fun surface(surface: NativePointer): OpenGLSurfaceDescriptor = apply {
    this.surface = surface
  }
}
