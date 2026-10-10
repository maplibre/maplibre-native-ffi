// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedEventBatchOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  /**
   * Borrows the event and message view stored by an owned event batch.
   *
   * See `mln_event_batch_get` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun get(): EventBatchView =
    nativeCall(this, binding, "mln_event_batch_get", Access.READ) {
      val out = sized(w(24, 40), w(4, 8))
      check(C.mln_event_batch_get(handle, out, diagnostic))
      readEventBatchView(out)
    }

  /**
   * Releases an owned event batch. A null handle is a no-op.
   *
   * See `mln_event_batch_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding, "mln_event_batch_release") { C.mln_event_batch_release(handle) }
}
