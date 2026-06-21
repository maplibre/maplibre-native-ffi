package org.maplibre.nativeffi.error

import kotlin.jvm.JvmInline

/** Status categories reported by the native MapLibre C ABI. */
@JvmInline
public value class MaplibreStatus(public val nativeCode: Int) {
  public companion object {
    public val OK: MaplibreStatus = MaplibreStatus(0)
    public val INVALID_ARGUMENT: MaplibreStatus = MaplibreStatus(-1)
    public val INVALID_STATE: MaplibreStatus = MaplibreStatus(-2)
    public val WRONG_THREAD: MaplibreStatus = MaplibreStatus(-3)
    public val UNSUPPORTED: MaplibreStatus = MaplibreStatus(-4)
    public val NATIVE_ERROR: MaplibreStatus = MaplibreStatus(-5)

    /** Returns the Kotlin status category for a C ABI status value. */
    internal fun fromNative(nativeCode: Int): MaplibreStatus = MaplibreStatus(nativeCode)
  }
}
