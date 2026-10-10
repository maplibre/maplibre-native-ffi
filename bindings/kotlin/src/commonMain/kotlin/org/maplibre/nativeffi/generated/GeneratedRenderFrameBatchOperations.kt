// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedRenderFrameBatchOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  /**
   * Returns the number of records in an owned frame-result batch.
   *
   * See `mln_render_frame_batch_count` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun count(): ULong =
    nativeCall(this, binding, "mln_render_frame_batch_count") {
      val out = allocate(w(4, 8), w(4, 8))
      check(C.mln_render_frame_batch_count(handle, out, diagnostic))
      readSize(out)
    }

  /**
   * Copies one frame-result record.
   *
   * See `mln_render_frame_batch_get` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun get(indexValue: ULong): RenderFrameResult =
    nativeCall(this, binding, "mln_render_frame_batch_get") {
      val out = sized(48, 8)
      check(C.mln_render_frame_batch_get(handle, indexValue.toLong(), out, diagnostic))
      readRenderFrameResult(out)
    }

  /**
   * Releases a frame-result batch.
   *
   * See `mln_render_frame_batch_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding, "mln_render_frame_batch_release") {
      C.mln_render_frame_batch_release(handle)
    }
}
