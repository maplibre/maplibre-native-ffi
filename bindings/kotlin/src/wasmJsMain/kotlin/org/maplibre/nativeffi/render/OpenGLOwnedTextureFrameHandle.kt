package org.maplibre.nativeffi.render

/**
 * Explicit handle for an OpenGL session-owned texture frame.
 *
 * The frame's texture is borrowed from the session and stays valid only until this handle is
 * closed, which is why the handle is explicit rather than a value a host could keep. A browser host
 * has no finalizer to fall back on, so nothing releases the frame if this is not closed — and a
 * session with a frame still acquired refuses to render, resize, detach, or close.
 *
 * The frame's values are held here rather than in the module's heap. Native filled a descriptor in
 * scratch that its acquire call freed, and release matches a frame by its generation and frame id
 * rather than by the address those arrive through, so the descriptor is rebuilt for the release.
 */
public actual class OpenGLOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val scope: FrameScope,
  private val frameValue: OpenGLOwnedTextureFrame,
) : AutoCloseable {
  private val core = OwnedTextureFrameHandleCore("OpenGLOwnedTextureFrameHandle")

  public actual fun frame(): OpenGLOwnedTextureFrame {
    core.ensureOpen()
    return frameValue
  }

  public actual val isClosed: Boolean
    get() = core.isClosed()

  public actual override fun close() {
    core.close(
      releaseNative = { session.releaseOpenGLFrame(frameValue) },
      ownerClosed = { session.isClosed },
      releaseLocal = ::releaseLocal,
    )
  }

  /**
   * Retires the frame locally, after native has released it.
   *
   * The scope closes first so the frame's values stop reading as live, and the session's borrow is
   * given back last whatever that does, because a borrow left standing would block every later
   * session call.
   */
  private fun releaseLocal() {
    try {
      scope.close()
    } finally {
      session.finishFrameBorrow()
    }
  }
}
