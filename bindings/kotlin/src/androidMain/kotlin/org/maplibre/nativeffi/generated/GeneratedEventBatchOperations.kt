// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual abstract class GeneratedEventBatchOperations internal actual constructor() {
  internal abstract fun bindingEventBatchHandle(): Long

  internal abstract fun <T> bindingReadEventBatch(block: (Long) -> T): T

  internal abstract fun bindingCloseEventBatch(call: (Long) -> Unit)

  public actual fun get(): RuntimeEventBatchView {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingEventBatchHandle().toLong(),
        "mln_event_batch_get",
      )
      return bindingReadEventBatch {
        PointerScope().use { arena ->
          val output = MaplibreNativeC.mln_runtime_event_batch_view()
          output.size(output.sizeof())
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_event_batch_get(bindingEventBatchHandle(), output, diagnostic)
          }
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
        PointerScope().use { arena -> MaplibreNativeC.mln_event_batch_release(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
