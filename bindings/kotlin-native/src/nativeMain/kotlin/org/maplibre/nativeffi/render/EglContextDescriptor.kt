package org.maplibre.nativeffi.render

/** Mutable EGL context descriptor for OpenGL render targets. */
public class EglContextDescriptor() : OpenGLContextDescriptor {
  public var display: NativePointer = NativePointer.NULL

  public var config: NativePointer = NativePointer.NULL

  public var shareContext: NativePointer = NativePointer.NULL

  public var getProcAddress: NativePointer = NativePointer.NULL

  public constructor(
    display: NativePointer,
    config: NativePointer,
    shareContext: NativePointer,
    getProcAddress: NativePointer,
  ) : this() {
    this.display = display
    this.config = config
    this.shareContext = shareContext
    this.getProcAddress = getProcAddress
  }
}
