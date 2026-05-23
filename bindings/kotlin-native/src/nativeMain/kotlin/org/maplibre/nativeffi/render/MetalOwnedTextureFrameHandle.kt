package org.maplibre.nativeffi.render

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame

/** Explicit handle for a Metal session-owned texture frame. */
@OptIn(ExperimentalForeignApi::class)
public class MetalOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val framePointer: CPointer<mln_metal_owned_texture_frame>,
  private val scope: FrameScope,
  private val frameValue: MetalOwnedTextureFrame,
) : AutoCloseable {
  private var closed = false

  public fun frame(): MetalOwnedTextureFrame {
    ensureOpen()
    return frameValue
  }

  public fun isClosed(): Boolean = closed

  override fun close() {
    if (closed) return
    session.releaseMetalFrame(framePointer)
    closed = true
    try {
      scope.close()
    } finally {
      nativeHeap.free(framePointer.rawValue)
    }
  }

  private fun ensureOpen() {
    check(!closed) { "Metal owned texture frame handle is closed" }
  }
}
