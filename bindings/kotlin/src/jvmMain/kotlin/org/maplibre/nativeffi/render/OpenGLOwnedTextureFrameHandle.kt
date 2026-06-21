package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Explicit handle for an OpenGL session-owned texture frame. */
public actual class OpenGLOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val frameSegment: NativeAccess.OwnedTextureFrameSegment,
  private val scope: FrameScope,
  private val frameValue: OpenGLOwnedTextureFrame,
) : AutoCloseable {
  private val core =
    OwnedTextureFrameHandleCore(
      "OpenGLOwnedTextureFrameHandle",
      "OpenGL owned texture frame handle is closed",
    )

  public actual fun frame(): OpenGLOwnedTextureFrame {
    core.ensureOpen()
    return frameValue
  }

  public actual val isClosed: Boolean
    get() = core.isClosed()

  public actual override fun close() {
    core.close(
      releaseNative = { session.releaseOpenGLFrame(frameSegment.segment) },
      ownerClosed = { session.isClosed },
      releaseLocal = ::releaseLocal,
    )
  }

  private fun releaseLocal() {
    try {
      scope.close()
    } finally {
      try {
        frameSegment.close()
      } finally {
        session.finishFrameBorrow()
      }
    }
  }
}
