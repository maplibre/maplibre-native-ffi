// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedRenderFrameBatchOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  public fun count(): ULong =
    nativeCall(this, binding, "mln_render_frame_batch_count") {
      val out = allocate(8)
      check(C.mln_render_frame_batch_count(handle, out, diagnostic))
      readSize(out)
    }

  public fun get(indexValue: ULong): RenderFrameResult =
    nativeCall(this, binding, "mln_render_frame_batch_get") {
      val out = allocate(48, 8).also { writeU32(it, 48.toUInt()) }
      check(C.mln_render_frame_batch_get(handle, indexValue.toLong(), out, diagnostic))
      readRenderFrameResult(out)
    }

  public fun release(): Unit =
    nativeClose(this, binding, "mln_render_frame_batch_release") {
      C.mln_render_frame_batch_release(handle)
    }
}
