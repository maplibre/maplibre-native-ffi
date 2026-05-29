package org.maplibre.nativeffi.render

/** Mutable descriptor for OpenGL native surface render targets. */
public class OpenGLSurfaceDescriptor(
  extent: RenderTargetExtent = RenderTargetExtent(),
  context: OpenGLContextDescriptor = WglContextDescriptor(),
  surface: NativePointer = NativePointer.NULL,
) {
  public var extent: RenderTargetExtent = extent

  public var context: OpenGLContextDescriptor = context

  public var surface: NativePointer = surface
}
