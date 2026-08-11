package org.maplibre.nativeffi.render

/** Mutable EGL context descriptor for OpenGL render targets. */
public class EglContextDescriptor(
  display: NativePointer,
  config: NativePointer,
  shareContext: NativePointer,
  getProcAddress: NativePointer,
  clientApi: OpenGLClientApi = OpenGLClientApi.UNSPECIFIED,
  ownership: OpenGLContextOwnership = OpenGLContextOwnership.SHARED,
) : OpenGLContextDescriptor {
  public var display: NativePointer = display

  public var config: NativePointer = config

  /**
   * Borrowed EGLContext whose share group the session context joins. Required under shared
   * ownership, where the session also takes its client API from this context. A dedicated session
   * joins no share group, so it is null there and names [clientApi] instead.
   */
  public var shareContext: NativePointer = shareContext

  public var getProcAddress: NativePointer = getProcAddress

  /**
   * Client API the session creates its context for. Required under dedicated ownership. A shared
   * session queries [shareContext] for it, so this is ignored there.
   */
  public var clientApi: OpenGLClientApi = clientApi

  public override var ownership: OpenGLContextOwnership = ownership
}
