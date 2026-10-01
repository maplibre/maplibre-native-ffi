// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedEventBatchOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  public fun get(): RuntimeEventBatchView =
    nativeCall(this, binding, "mln_event_batch_get", Access.READ) {
      val out = allocate(w(24, 40), w(4, 8)).also { writeU32(it, w(24, 40).toUInt()) }
      check(C.mln_event_batch_get(handle, out, diagnostic))
      readRuntimeEventBatchView(out)
    }

  public fun release(): Unit =
    nativeClose(this, binding, "mln_event_batch_release") { C.mln_event_batch_release(handle) }
}
