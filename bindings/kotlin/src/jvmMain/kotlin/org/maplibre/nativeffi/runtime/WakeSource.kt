package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeWakeSource
import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Wake source backed by the JVM FFM bridge. */
public actual class WakeSource private constructor(private val source: NativeWakeSource) :
  AutoCloseable {
  // Signal and close are both any-thread, so the gate orders them against each other. Without it a
  // signal that passed the live check could reach native after a concurrent close retired the id,
  // reporting the C API's stale-handle status where every other binding reports success or its own
  // closed error.
  private val nativeCallGate = Any()
  private val core = HandleStateCore("WakeSource", source.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun signal() {
    NativeAccess.ensureLoaded()
    synchronized(nativeCallGate) {
      core.requireLive()
      NativeAccess.signalWakeSource(source)
    }
  }

  public actual override fun close() {
    synchronized(nativeCallGate) {
      core.closeOnce(
        destroy = {
          NativeAccess.destroyWakeSource(source)
          MaplibreStatus.OK.nativeCode
        }
      )
    }
  }

  internal companion object {
    fun fromNative(source: NativeWakeSource): WakeSource = WakeSource(source)
  }
}
