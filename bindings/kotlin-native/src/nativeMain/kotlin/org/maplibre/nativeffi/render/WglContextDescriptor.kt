package org.maplibre.nativeffi.render

/** Mutable WGL context descriptor for OpenGL render targets on Windows. */
public class WglContextDescriptor(
  public var deviceContext: NativePointer = NativePointer.NULL,
  public var shareContext: NativePointer = NativePointer.NULL,
  public var getProcAddress: NativePointer = NativePointer.NULL,
) : OpenGLContextDescriptor {
  public fun deviceContext(deviceContext: NativePointer): WglContextDescriptor = apply {
    this.deviceContext = deviceContext
  }

  public fun shareContext(shareContext: NativePointer): WglContextDescriptor = apply {
    this.shareContext = shareContext
  }

  public fun getProcAddress(getProcAddress: NativePointer): WglContextDescriptor = apply {
    this.getProcAddress = getProcAddress
  }
}
