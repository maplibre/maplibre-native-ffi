package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL native surface render targets. */
public class OpenGLSurfaceDescriptor(
  extent: RenderTargetExtent,
  context: OpenGLContextDescriptor,
  surface: NativePointer,
) {
  public var extent: RenderTargetExtent = extent

  public var context: OpenGLContextDescriptor = context

  public var surface: NativePointer = surface
}
