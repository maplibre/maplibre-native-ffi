package org.maplibre.nativeffi.render

/** OpenGL platform context provider data for OpenGL render targets. */
public sealed interface OpenGLContextDescriptor {
  /**
   * Whether the render session shares its thread with host graphics work. WGL and EGL surface
   * targets support both. A texture target hands its texture to the host, so it is shared only.
   */
  public var ownership: OpenGLContextOwnership
}
