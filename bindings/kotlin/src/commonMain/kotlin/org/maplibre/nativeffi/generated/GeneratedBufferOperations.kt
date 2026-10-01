// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedBufferOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  public fun destroy(): Unit =
    nativeClose(this, binding, "mln_buffer_destroy") { C.mln_buffer_destroy(handle) }

  public fun get(): ByteArray =
    nativeCall(this, binding, "mln_buffer_get", Access.READ) {
      val out = allocate(2 * NativeMemory.addressSize, NativeMemory.addressSize)
      check(C.mln_buffer_get(handle, out, diagnostic))
      readView(out)
    }
}
