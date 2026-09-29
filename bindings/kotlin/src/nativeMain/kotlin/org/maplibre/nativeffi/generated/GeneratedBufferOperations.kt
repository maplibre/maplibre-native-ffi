// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedBufferOperations internal actual constructor() {
  internal abstract fun bindingBufferHandle(): ULong

  internal abstract fun <T> bindingReadBuffer(block: (ULong) -> T): T

  internal abstract fun bindingCloseBuffer(call: (ULong) -> Unit)

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
        memScoped {
          val arena = this
          mln_buffer_destroy(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun get(): ByteArray {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingBufferHandle().toLong(),
        "mln_buffer_get",
      )
      return bindingReadBuffer {
        memScoped {
          val arena = this
          val output = arena.alloc<mln_buffer_view>()
          NativeDiagnostics.check { diagnostic ->
            mln_buffer_get(bindingBufferHandle(), output.ptr, diagnostic)
          }
          GeneratedValues.readBytes(output)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
