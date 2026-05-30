package org.maplibre.nativeffi.map

/** Map rendering modes used when creating a map. */
public enum class MapMode(public val nativeValue: Int) {
  CONTINUOUS(0),
  STATIC(1),
  TILE(2),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): MapMode = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): MapMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
