package org.maplibre.nativeffi.map

/** Map rendering modes used when creating a map. */
public enum class MapMode(public val nativeValue: UInt) {
  CONTINUOUS(0U),
  STATIC(1U),
  TILE(2U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): MapMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
