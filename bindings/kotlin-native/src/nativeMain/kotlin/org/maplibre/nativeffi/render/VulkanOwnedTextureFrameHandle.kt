package org.maplibre.nativeffi.render

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame

/** Explicit handle for a Vulkan session-owned texture frame. */
@OptIn(ExperimentalForeignApi::class)
public class VulkanOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val framePointer: CPointer<mln_vulkan_owned_texture_frame>,
  private val scope: FrameScope,
  private val frameValue: VulkanOwnedTextureFrame,
) : AutoCloseable {
  private var closed = false

  public fun frame(): VulkanOwnedTextureFrame {
    ensureOpen()
    return frameValue
  }

  public fun isClosed(): Boolean = closed

  override fun close() {
    if (closed) return
    session.releaseVulkanFrame(framePointer)
    closed = true
    try {
      scope.close()
    } finally {
      nativeHeap.free(framePointer.rawValue)
    }
  }

  private fun ensureOpen() {
    check(!closed) { "Vulkan owned texture frame handle is closed" }
  }
}
