// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

public actual abstract class GeneratedRenderFrameBatchOperations internal actual constructor() {
  internal abstract fun bindingRenderFrameBatchHandle(): Long

  internal abstract fun bindingCloseRenderFrameBatch(call: (Long) -> Int)

  public actual fun count(): ULong {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderFrameBatchHandle().toLong(),
        "mln_render_frame_batch_count",
      )
      return PointerScope().use { arena ->
        val output = SizeTPointer(1L)
        BindingStatus.check(
          MaplibreNativeC.mln_render_frame_batch_count(bindingRenderFrameBatchHandle(), output)
        )
        output.get().toULong()
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
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_render_frame_result()
        output.size(output.sizeof())
        BindingStatus.check(
          MaplibreNativeC.mln_render_frame_batch_get(
            bindingRenderFrameBatchHandle(),
            indexValue.toLong(),
            output,
          )
        )
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
        PointerScope().use { arena ->
          MaplibreNativeC.mln_render_frame_batch_release(owner)
          0
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
