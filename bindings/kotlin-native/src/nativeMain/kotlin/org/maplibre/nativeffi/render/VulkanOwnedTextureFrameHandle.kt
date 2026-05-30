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
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame

/** Explicit handle for a Vulkan session-owned texture frame. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
public class VulkanOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val framePointer: CPointer<mln_vulkan_owned_texture_frame>,
  private val scope: FrameScope,
  private val frameValue: VulkanOwnedTextureFrame,
) : AutoCloseable {
  private val leakReport = LeakReport("VulkanOwnedTextureFrameHandle")
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(leakReport) { it.report() }
  private var closed = false

  public fun frame(): VulkanOwnedTextureFrame {
    ensureOpen()
    return frameValue
  }

  public val isClosed: Boolean
    get() = closed

  override fun close() {
    FrameReleasePolicy.close(
      isClosed = { closed },
      releaseNative = { session.releaseVulkanFrame(framePointer) },
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
      nativeHeap.free(framePointer.rawValue)
    }
  }

  private fun ensureOpen() {
    check(!closed) { "Vulkan owned texture frame handle is closed" }
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
