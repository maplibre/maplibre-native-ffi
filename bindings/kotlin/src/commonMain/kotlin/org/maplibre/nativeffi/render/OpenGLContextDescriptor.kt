package org.maplibre.nativeffi.render

/** OpenGL platform context provider data for OpenGL render targets. */
public sealed interface OpenGLContextDescriptor {
  /**
   * Whether the render session shares its driver thread and graphics objects with the host. A
   * private EGL owned texture target uses dedicated ownership and grants readback without frame
   * acquisition.
   */
  public var ownership: OpenGLContextOwnership
}
