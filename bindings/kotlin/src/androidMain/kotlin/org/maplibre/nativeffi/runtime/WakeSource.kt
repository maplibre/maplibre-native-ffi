package org.maplibre.nativeffi.runtime

import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status

/** Owned wake source backed by the Android JNI bridge. */
public actual class WakeSource internal constructor(private val sourceAddress: Long) :
  AutoCloseable {
  // HandleStateCore serializes close against a concurrent signal, which is what
  // makes this handle usable from any thread.
  private val core = HandleStateCore("WakeSource", sourceAddress)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun signal() {
    NativeAccess.ensureLoaded()
    core.requireLive()
    Status.check(MaplibreNativeC.mln_wake_source_signal(wakeSource(sourceAddress)))
  }

  public actual override fun close() {
    core.closeOnce(
      destroy = {
        MaplibreNativeC.mln_wake_source_destroy(wakeSource(sourceAddress))
        MaplibreStatus.OK.nativeCode
      }
    )
  }
}

private fun wakeSource(address: Long): MaplibreNativeC.mln_wake_source =
  MaplibreNativeC.mln_wake_source(WakeSourceAddressPointer(address))

private class WakeSourceAddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}
