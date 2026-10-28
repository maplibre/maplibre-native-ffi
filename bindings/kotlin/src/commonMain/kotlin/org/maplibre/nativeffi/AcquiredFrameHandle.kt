// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A rendered frame that a render session lends until its release.
 *
 * See `mln_acquired_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class AcquiredFrameHandle
internal constructor(
  handle: Long,
  @Suppress("unused") private val parent: RenderSessionHandle,
) {
  internal val binding =
    HandleState("AcquiredFrameHandle", handle, GeneratedOwnerDisposal::acquiredFrame)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /**
   * Consumes an acquired frame without consumer GPU synchronization, so it quarantines the frame's
   * slot of the texture ring.
   *
   * See `mln_acquired_frame_discard` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun discard(): Unit =
    nativeClose(this, binding) { check(C.mln_acquired_frame_discard(handle, diagnostic)) }

  /**
   * Copies OpenGL-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_open_gl_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun <T> withOpenGlTexture(block: (OpenGlTextureFrame) -> T): T =
    nativeCall(this, binding) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(64, 8)
        check(C.mln_acquired_frame_get_open_gl_texture(handle, out, diagnostic))
        block(readOpenGlTextureFrame(out, scope))
      }
    }

  /**
   * Copies the producer synchronization for an acquired texture frame.
   *
   * See `mln_acquired_frame_get_producer_sync` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun <T> withProducerSync(block: (GpuSync) -> T): T =
    nativeCall(this, binding) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(16, 8)
        check(C.mln_acquired_frame_get_producer_sync(handle, out, diagnostic))
        block(readGpuSync(out, scope))
      }
    }

  /**
   * Copies common metadata for an acquired frame.
   *
   * See `mln_acquired_frame_get_result` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getResult(): RenderFrameResult =
    nativeCall(this, binding) {
      val out = sized(264, 8)
      check(C.mln_acquired_frame_get_result(handle, out, diagnostic))
      readRenderFrameResult(out)
    }

  /**
   * Releases an acquired frame after the host's GPU work that reads it.
   *
   * See `mln_acquired_frame_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun release(consumerCompletion: GpuSync?): Unit =
    nativeClose(this, binding) {
      check(
        C.mln_acquired_frame_release(
          handle,
          consumerCompletion?.let { writeGpuSync(it) } ?: 0L,
          diagnostic,
        ),
      )
    }

  override fun toString(): String =
    "AcquiredFrameHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
