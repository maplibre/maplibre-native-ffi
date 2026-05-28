package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL native surface render targets. */
public class OpenGLSurfaceDescriptor(
  public var extent: RenderTargetExtent = RenderTargetExtent(),
  public var context: OpenGLContextDescriptor = WglContextDescriptor(),
  public var surface: NativePointer = NativePointer.NULL,
) {
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
