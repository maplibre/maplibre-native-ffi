package org.maplibre.nativeffi.render

/** Mutable WGL context descriptor for OpenGL render targets on Windows. */
public class WglContextDescriptor(
  deviceContext: NativePointer,
  shareContext: NativePointer,
  getProcAddress: NativePointer,
) : OpenGLContextDescriptor {
  public var deviceContext: NativePointer = deviceContext

  public var shareContext: NativePointer = shareContext

  public var getProcAddress: NativePointer = getProcAddress
}
