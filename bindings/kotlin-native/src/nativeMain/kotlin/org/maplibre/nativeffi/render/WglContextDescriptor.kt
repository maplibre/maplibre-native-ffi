package org.maplibre.nativeffi.render

/** Mutable WGL context descriptor for OpenGL render targets on Windows. */
public class WglContextDescriptor() : OpenGLContextDescriptor {
  public var deviceContext: NativePointer = NativePointer.NULL

  public var shareContext: NativePointer = NativePointer.NULL

  public var getProcAddress: NativePointer = NativePointer.NULL

  public constructor(
    deviceContext: NativePointer,
    shareContext: NativePointer,
    getProcAddress: NativePointer,
  ) : this() {
    this.deviceContext = deviceContext
    this.shareContext = shareContext
    this.getProcAddress = getProcAddress
  }
}
