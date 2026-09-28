// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

public actual abstract class GeneratedBufferOperations internal actual constructor() {
  internal abstract fun bindingBufferHandle(): Long

  internal abstract fun <T> bindingReadBuffer(block: (Long) -> T): T

  internal abstract fun bindingCloseBuffer(call: (Long) -> Int)

  public actual fun destroy(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_buffer_destroy"
      )
      return bindingCloseBuffer { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_buffer_destroy",
        )
        PointerScope().use { arena ->
          MaplibreNativeC.mln_buffer_destroy(owner)
          0
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun get(): ByteArray {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingBufferHandle().toLong(),
        "mln_buffer_get",
      )
      return bindingReadBuffer {
        PointerScope().use { arena ->
          val output = MaplibreNativeC.mln_buffer_view()
          BindingStatus.check(MaplibreNativeC.mln_buffer_get(bindingBufferHandle(), output))
          GeneratedValues.readBytes(output)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
