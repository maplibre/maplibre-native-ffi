package org.maplibre.nativeffi.render

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame

/** Explicit handle for a Vulkan session-owned texture frame. */
@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class VulkanOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val framePointer: CPointer<mln_vulkan_owned_texture_frame>,
  private val scope: FrameScope,
  private val frameValue: VulkanOwnedTextureFrame,
) : AutoCloseable {
  private val core =
    OwnedTextureFrameHandleCore(
      "VulkanOwnedTextureFrameHandle",
      "Vulkan owned texture frame handle is closed",
    )
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(core) { it.reportLeak() }

  public actual fun frame(): VulkanOwnedTextureFrame {
    core.ensureOpen()
    return frameValue
  }

  public actual val isClosed: Boolean
    get() = core.isClosed()

  public actual override fun close() {
    core.close(
      releaseNative = { session.releaseVulkanFrame(framePointer) },
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
