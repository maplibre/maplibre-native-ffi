package org.maplibre.nativeffi.render

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_frame

/** Explicit handle for an OpenGL session-owned texture frame. */
@OptIn(ExperimentalForeignApi::class)
public class OpenGLOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val framePointer: CPointer<mln_opengl_owned_texture_frame>,
  private val scope: FrameScope,
  private val frameValue: OpenGLOwnedTextureFrame,
) : AutoCloseable {
  private var closed = false

  public fun frame(): OpenGLOwnedTextureFrame {
    ensureOpen()
    return frameValue
  }

  public fun isClosed(): Boolean = closed

  override fun close() {
    if (closed) return
    session.releaseOpenGLFrame(framePointer)
    closed = true
    try {
      scope.close()
    } finally {
      nativeHeap.free(framePointer.rawValue)
    }
  }

  private fun ensureOpen() {
    check(!closed) { "OpenGL owned texture frame handle is closed" }
  }
}
