package org.maplibre.nativeffi.render

/** Mutable EGL context descriptor for OpenGL render targets on Linux. */
public class EglContextDescriptor(
  public var display: NativePointer = NativePointer.NULL,
  public var config: NativePointer = NativePointer.NULL,
  public var shareContext: NativePointer = NativePointer.NULL,
  public var getProcAddress: NativePointer = NativePointer.NULL,
) : OpenGLContextDescriptor {
  public fun display(display: NativePointer): EglContextDescriptor = apply {
    this.display = display
  }

  public fun config(config: NativePointer): EglContextDescriptor = apply { this.config = config }

  public fun shareContext(shareContext: NativePointer): EglContextDescriptor = apply {
    this.shareContext = shareContext
  }

  public fun getProcAddress(getProcAddress: NativePointer): EglContextDescriptor = apply {
    this.getProcAddress = getProcAddress
  }
}
