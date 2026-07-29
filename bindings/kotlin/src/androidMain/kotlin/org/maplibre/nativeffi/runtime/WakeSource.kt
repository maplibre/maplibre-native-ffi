package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status

/** Owned wake source backed by the Android JNI bridge. */
public actual class WakeSource internal constructor(private val sourceId: Long) : AutoCloseable {
  // Signal and close are both any-thread, so the gate orders them against each other. Without it a
  // signal that passed the live check could reach native after a concurrent close retired the id,
  // reporting the C API's stale-handle status where every other binding reports success or its own
  // closed error.
  private val nativeCallGate = Any()
  private val core = HandleStateCore("WakeSource", sourceId)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun signal() {
    NativeAccess.ensureLoaded()
    synchronized(nativeCallGate) {
      core.requireLive()
      Status.check(MaplibreNativeC.mln_wake_source_signal(sourceId))
    }
  }

  public actual override fun close() {
    synchronized(nativeCallGate) {
      core.closeOnce(
        destroy = {
          MaplibreNativeC.mln_wake_source_destroy(sourceId)
          MaplibreStatus.OK.nativeCode
        }
      )
    }
  }
}
