package org.maplibre.nativeffi.map

/** North orientation used by map viewport options. */
public enum class NorthOrientation(public val nativeValue: Int) {
  UP(0),
  RIGHT(1),
  DOWN(2),
  LEFT(3),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): NorthOrientation = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): NorthOrientation =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
