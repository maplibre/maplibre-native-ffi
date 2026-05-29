package org.maplibre.nativeffi.render

/** Mutable WGL context descriptor for OpenGL render targets on Windows. */
public class WglContextDescriptor() : OpenGLContextDescriptor {
  public var deviceContext: NativePointer = NativePointer.NULL
    private set

  public var shareContext: NativePointer = NativePointer.NULL
    private set

  public var getProcAddress: NativePointer = NativePointer.NULL
    private set

  public constructor(
    deviceContext: NativePointer,
    shareContext: NativePointer,
    getProcAddress: NativePointer,
  ) : this() {
    this.deviceContext = deviceContext
    this.shareContext = shareContext
    this.getProcAddress = getProcAddress
  }

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
