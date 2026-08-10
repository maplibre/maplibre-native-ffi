package org.maplibre.nativeffi.render

/** Mutable WGL context descriptor for OpenGL render targets on Windows. */
public class WglContextDescriptor(
  deviceContext: NativePointer,
  shareContext: NativePointer,
  getProcAddress: NativePointer,
  ownership: OpenGLContextOwnership = OpenGLContextOwnership.SHARED,
) : OpenGLContextDescriptor {
  public var deviceContext: NativePointer = deviceContext

  /**
   * Borrowed HGLRC whose share group the session context joins. Required under shared ownership. A
   * dedicated session joins no share group, so it is null there.
   */
  public var shareContext: NativePointer = shareContext

  public var getProcAddress: NativePointer = getProcAddress

  public override var ownership: OpenGLContextOwnership = ownership
}
