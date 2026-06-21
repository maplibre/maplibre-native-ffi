package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Explicit handle for a Metal session-owned texture frame. */
public actual class MetalOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val frameSegment: NativeAccess.OwnedTextureFrameSegment,
  private val scope: FrameScope,
  private val frameValue: MetalOwnedTextureFrame,
) : AutoCloseable {
  private val core =
    OwnedTextureFrameHandleCore(
      "MetalOwnedTextureFrameHandle",
      "Metal owned texture frame handle is closed",
    )

  public actual fun frame(): MetalOwnedTextureFrame {
    core.ensureOpen()
    return frameValue
  }

  public actual val isClosed: Boolean
    get() = core.isClosed()

  public actual override fun close() {
    core.close(
      releaseNative = { session.releaseMetalFrame(frameSegment.segment) },
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
