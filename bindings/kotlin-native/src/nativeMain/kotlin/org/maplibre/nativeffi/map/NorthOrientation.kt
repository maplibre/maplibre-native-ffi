package org.maplibre.nativeffi.map

/** North orientation used by map viewport options. */
public enum class NorthOrientation(internal val nativeValue: UInt) {
  UP(0U),
  RIGHT(1U),
  DOWN(2U),
  LEFT(3U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): NorthOrientation =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
