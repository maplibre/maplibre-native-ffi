// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import java.lang.foreign.ValueLayout
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual abstract class GeneratedRenderFrameBatchOperations internal actual constructor() {
  internal abstract fun bindingRenderFrameBatchHandle(): Long

  internal abstract fun bindingCloseRenderFrameBatch(call: (Long) -> Unit)

  public actual fun count(): ULong {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderFrameBatchHandle().toLong(),
        "mln_render_frame_batch_count",
      )
      return Arena.ofConfined().use { arena ->
        val output = arena.allocate(ValueLayout.JAVA_LONG)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_render_frame_batch_count(
            bindingRenderFrameBatchHandle(),
            output,
            diagnostic,
          )
        }
        output.get(ValueLayout.JAVA_LONG, 0).toULong()
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun get(indexValue: ULong): RenderFrameResult {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderFrameBatchHandle().toLong(),
        "mln_render_frame_batch_get",
      )
      return Arena.ofConfined().use { arena ->
        val output = mln_render_frame_result.allocate(arena)
        mln_render_frame_result.size(output, mln_render_frame_result.sizeof().toInt())
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_render_frame_batch_get(
            bindingRenderFrameBatchHandle(),
            indexValue.toLong(),
            output,
            diagnostic,
          )
        }
        GeneratedValues.readRenderFrameResult(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun release(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_render_frame_batch_release"
      )
      return bindingCloseRenderFrameBatch { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_render_frame_batch_release",
        )
        Arena.ofConfined().use { arena -> MapLibreNativeC.mln_render_frame_batch_release(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
