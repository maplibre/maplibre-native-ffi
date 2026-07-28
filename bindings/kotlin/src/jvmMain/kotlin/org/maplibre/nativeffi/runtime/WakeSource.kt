package org.maplibre.nativeffi.runtime

import java.lang.foreign.MemorySegment
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Wake source backed by the JVM FFM bridge. */
public actual class WakeSource private constructor(private val source: MemorySegment) :
  AutoCloseable {
  // Held across the native signal and across close, so a close on another
  // thread cannot destroy the source between the live check and the call.
  private val nativeCallGate = Any()
  private val core = HandleStateCore("WakeSource", source.address())

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
    fun fromNative(source: MemorySegment): WakeSource = WakeSource(source)
  }
}
