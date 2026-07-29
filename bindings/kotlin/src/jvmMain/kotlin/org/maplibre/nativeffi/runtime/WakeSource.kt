package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeWakeSource
import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Wake source backed by the JVM FFM bridge. */
public actual class WakeSource private constructor(private val source: NativeWakeSource) :
  AutoCloseable {
  private val core = HandleStateCore("WakeSource", source.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun signal() {
    NativeAccess.ensureLoaded()
    core.requireLive()
    NativeAccess.signalWakeSource(source)
  }

  public actual override fun close() {
    core.closeOnce(
      destroy = {
        NativeAccess.destroyWakeSource(source)
        MaplibreStatus.OK.nativeCode
      }
    )
  }

  internal companion object {
    fun fromNative(source: NativeWakeSource): WakeSource = WakeSource(source)
  }
}
