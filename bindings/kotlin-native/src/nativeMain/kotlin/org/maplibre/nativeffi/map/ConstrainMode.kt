package org.maplibre.nativeffi.map

/** Constraint mode used by map viewport options. */
public enum class ConstrainMode(public val nativeValue: Int) {
  NONE(0),
  HEIGHT_ONLY(1),
  WIDTH_AND_HEIGHT(2),
  SCREEN(3),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ConstrainMode = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ConstrainMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
