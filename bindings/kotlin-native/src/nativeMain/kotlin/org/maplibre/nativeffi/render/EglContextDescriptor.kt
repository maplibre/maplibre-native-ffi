package org.maplibre.nativeffi.render

/** Mutable EGL context descriptor for OpenGL render targets. */
public class EglContextDescriptor(
  display: NativePointer,
  config: NativePointer,
  shareContext: NativePointer,
  getProcAddress: NativePointer,
) : OpenGLContextDescriptor {
  public var display: NativePointer = display

  public var config: NativePointer = config

  public var shareContext: NativePointer = shareContext

  public var getProcAddress: NativePointer = getProcAddress
}
