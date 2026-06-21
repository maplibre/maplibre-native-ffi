package org.maplibre.nativeffi.render

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_frame

/** Explicit handle for an OpenGL session-owned texture frame. */
@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class OpenGLOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val framePointer: CPointer<mln_opengl_owned_texture_frame>,
  private val scope: FrameScope,
  private val frameValue: OpenGLOwnedTextureFrame,
) : AutoCloseable {
  private val core =
    OwnedTextureFrameHandleCore(
      "OpenGLOwnedTextureFrameHandle",
      "OpenGL owned texture frame handle is closed",
    )
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(core) { it.reportLeak() }

  public actual fun frame(): OpenGLOwnedTextureFrame {
    core.ensureOpen()
    return frameValue
  }

  public actual val isClosed: Boolean
    get() = core.isClosed()

  public actual override fun close() {
    core.close(
      releaseNative = { session.releaseOpenGLFrame(framePointer) },
      ownerClosed = { session.isClosed },
      releaseLocal = ::releaseLocal,
    )
  }

  private fun releaseLocal() {
    try {
      scope.close()
    } finally {
      try {
        nativeHeap.free(framePointer.rawValue)
      } finally {
        session.finishFrameBorrow()
      }
    }
  }
}
