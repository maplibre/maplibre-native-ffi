package org.maplibre.nativeffi.map

/** Constraint mode used by map viewport options. */
public enum class ConstrainMode(internal val nativeValue: UInt) {
  NONE(0U),
  HEIGHT_ONLY(1U),
  WIDTH_AND_HEIGHT(2U),
  SCREEN(3U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): ConstrainMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
