// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedEventBatchOperations internal actual constructor() {
  internal abstract fun bindingEventBatchHandle(): ULong

  internal abstract fun <T> bindingReadEventBatch(block: (ULong) -> T): T

  internal abstract fun bindingCloseEventBatch(call: (ULong) -> Int)

  public actual fun get(): RuntimeEventBatchView {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingEventBatchHandle().toLong(),
        "mln_event_batch_get",
      )
      return bindingReadEventBatch {
        memScoped {
          val arena = this
          val output = arena.alloc<mln_runtime_event_batch_view>()
          output.size = sizeOf<mln_runtime_event_batch_view>().toUInt()
          BindingStatus.check(mln_event_batch_get(bindingEventBatchHandle(), output.ptr))
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
        memScoped {
          val arena = this
          mln_event_batch_release(owner)
          0
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
