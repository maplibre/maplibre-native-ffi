// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedRenderFrameBatchOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  /**
   * Borrows the result view stored by an owned frame-result batch.
   *
   * See `mln_render_frame_batch_get` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun get(): RenderFrameBatchView =
    nativeCall(this, binding, "mln_render_frame_batch_get", Access.READ) {
      val out = sized(w(16, 24), w(4, 8))
      check(C.mln_render_frame_batch_get(handle, out, diagnostic))
      readRenderFrameBatchView(out)
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
