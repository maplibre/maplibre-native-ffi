// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedAcquiredFrameOperations internal constructor() {
  internal abstract val binding: HandleState

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
      val out = sized(256, 8)
      check(C.mln_acquired_frame_get_result(handle, out, diagnostic))
      readRenderFrameResult(out)
    }

  /**
   * Releases an acquired frame after the host's GPU work that reads it.
   *
   * See `mln_acquired_frame_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun release(consumerCompletion: GpuSync? = null): Unit =
    nativeClose(this, binding) {
      check(
        C.mln_acquired_frame_release(
          handle,
          consumerCompletion?.let { writeGpuSync(it) } ?: 0L,
          diagnostic,
        ),
      )
    }
}
