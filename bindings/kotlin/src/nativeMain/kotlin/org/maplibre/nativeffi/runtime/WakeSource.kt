package org.maplibre.nativeffi.runtime

import cnames.structs.mln_wake_source
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_wake_source_destroy
import org.maplibre.nativeffi.internal.c.mln_wake_source_signal
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.status.Status

/** Owned wake source. Signal and close it from any thread. */
@OptIn(ExperimentalForeignApi::class)
public actual class WakeSource internal constructor(handle: CPointer<mln_wake_source>?) :
  AutoCloseable {
  // HandleState serializes close against a concurrent signal, so this handle is
  // usable from any thread.
  private val state = HandleState("WakeSource", handle)

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual fun signal() {
    Status.check(mln_wake_source_signal(state.requireLive()))
  }

  public actual override fun close() {
    state.closeOnce { source ->
      mln_wake_source_destroy(source)
      MaplibreStatus.OK.nativeCode
    }
  }
}
