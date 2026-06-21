package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

/** Explicit handle for a Vulkan session-owned texture frame. */
public actual class VulkanOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val nativeFrame: MaplibreNativeC.mln_vulkan_owned_texture_frame,
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
      releaseNative = { session.releaseVulkanFrame(nativeFrame) },
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
