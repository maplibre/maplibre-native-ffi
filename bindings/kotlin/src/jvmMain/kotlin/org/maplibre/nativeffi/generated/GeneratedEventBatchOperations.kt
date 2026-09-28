// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

public actual abstract class GeneratedEventBatchOperations internal actual constructor() {
  internal abstract fun bindingEventBatchHandle(): Long

  internal abstract fun <T> bindingReadEventBatch(block: (Long) -> T): T

  internal abstract fun bindingCloseEventBatch(call: (Long) -> Int)

  public actual fun get(): RuntimeEventBatchView {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingEventBatchHandle().toLong(),
        "mln_event_batch_get",
      )
      return bindingReadEventBatch {
        Arena.ofConfined().use { arena ->
          val output = mln_runtime_event_batch_view.allocate(arena)
          mln_runtime_event_batch_view.size(output, mln_runtime_event_batch_view.sizeof().toInt())
          BindingStatus.check(
            MapLibreNativeC.mln_event_batch_get(bindingEventBatchHandle(), output)
          )
          GeneratedValues.readRuntimeEventBatchView(output)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun release(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_event_batch_release"
      )
      return bindingCloseEventBatch { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_event_batch_release",
        )
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_event_batch_release(owner)
          0
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
