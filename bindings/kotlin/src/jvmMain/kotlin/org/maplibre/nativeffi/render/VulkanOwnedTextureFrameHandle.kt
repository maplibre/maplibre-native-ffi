package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Explicit handle for a Vulkan session-owned texture frame. */
public actual class VulkanOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val frameSegment: NativeAccess.OwnedTextureFrameSegment,
  private val scope: FrameScope,
  private val frameValue: VulkanOwnedTextureFrame,
) : AutoCloseable {
  private val core =
    OwnedTextureFrameHandleCore(
      "VulkanOwnedTextureFrameHandle",
      "Vulkan owned texture frame handle is closed",
    )

  public actual fun frame(): VulkanOwnedTextureFrame {
    core.ensureOpen()
    return frameValue
  }

  public actual val isClosed: Boolean
    get() = core.isClosed()

  public actual override fun close() {
    core.close(
      releaseNative = { session.releaseVulkanFrame(frameSegment.segment) },
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
