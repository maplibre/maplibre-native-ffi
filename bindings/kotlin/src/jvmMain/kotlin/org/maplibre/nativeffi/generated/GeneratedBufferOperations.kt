// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual abstract class GeneratedBufferOperations internal actual constructor() {
  internal abstract fun bindingBufferHandle(): Long

  internal abstract fun <T> bindingReadBuffer(block: (Long) -> T): T

  internal abstract fun bindingCloseBuffer(call: (Long) -> Unit)

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
        Arena.ofConfined().use { arena -> MapLibreNativeC.mln_buffer_destroy(owner) }
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
        Arena.ofConfined().use { arena ->
          val output = mln_buffer_view.allocate(arena)
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_buffer_get(bindingBufferHandle(), output, diagnostic)
          }
          GeneratedValues.readBytes(output)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
