package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

/** Explicit handle for a Metal session-owned texture frame. */
public actual class MetalOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val nativeFrame: MaplibreNativeC.mln_metal_owned_texture_frame,
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
      releaseNative = { session.releaseMetalFrame(nativeFrame) },
      ownerClosed = { session.isClosed },
      releaseLocal = ::releaseLocal,
    )
  }

  private fun releaseLocal() {
    try {
      scope.close()
    } finally {
      try {
        nativeFrame.close()
      } finally {
        session.finishFrameBorrow()
      }
    }
  }
}
