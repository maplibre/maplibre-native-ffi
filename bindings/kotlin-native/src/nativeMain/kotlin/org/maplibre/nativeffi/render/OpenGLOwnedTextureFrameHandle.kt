package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_frame

/** Explicit handle for an OpenGL session-owned texture frame. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
public class OpenGLOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val framePointer: CPointer<mln_opengl_owned_texture_frame>,
  private val scope: FrameScope,
  private val frameValue: OpenGLOwnedTextureFrame,
) : AutoCloseable {
  private val leakReport = LeakReport("OpenGLOwnedTextureFrameHandle")
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(leakReport) { it.report() }
  private var closed = false

  public fun frame(): OpenGLOwnedTextureFrame {
    ensureOpen()
    return frameValue
  }

  public val isClosed: Boolean
    get() = closed

  override fun close() {
    FrameReleasePolicy.close(
      isClosed = { closed },
      releaseNative = { session.releaseOpenGLFrame(framePointer) },
      ownerClosed = { session.isClosed },
      closeLocal = ::closeLocal,
    )
  }

  private fun closeLocal() {
    if (closed) return
    closed = true
    leakReport.markClosed()
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

  private fun ensureOpen() {
    check(!closed) { "OpenGL owned texture frame handle is closed" }
  }

  private class LeakReport(private val typeName: String) {
    private val closed = AtomicInt(0)

    fun markClosed() {
      closed.store(1)
    }

    fun report() {
      if (closed.load() == 0) {
        println(
          "Leaked $typeName; close frame handles explicitly on the render session owner thread."
        )
      }
    }
  }
}
