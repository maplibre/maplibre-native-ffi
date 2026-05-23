package org.maplibre.nativeffi.map

/** Tile level-of-detail algorithm used by map tile options. */
public enum class TileLodMode(internal val nativeValue: UInt) {
  DEFAULT(0U),
  DISTANCE(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): TileLodMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
