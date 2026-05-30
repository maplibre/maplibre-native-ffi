package org.maplibre.nativeffi.map

/** Tile level-of-detail algorithm used by map tile options. */
public enum class TileLodMode(public val nativeValue: Int) {
  DEFAULT(0),
  DISTANCE(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): TileLodMode = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): TileLodMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
