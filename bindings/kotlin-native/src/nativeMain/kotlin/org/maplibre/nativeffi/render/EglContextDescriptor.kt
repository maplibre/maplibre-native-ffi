package org.maplibre.nativeffi.render

/** Mutable EGL context descriptor for OpenGL render targets on Linux. */
public class EglContextDescriptor() : OpenGLContextDescriptor {
  public var display: NativePointer = NativePointer.NULL
    private set

  public var config: NativePointer = NativePointer.NULL
    private set

  public var shareContext: NativePointer = NativePointer.NULL
    private set

  public var getProcAddress: NativePointer = NativePointer.NULL
    private set

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
