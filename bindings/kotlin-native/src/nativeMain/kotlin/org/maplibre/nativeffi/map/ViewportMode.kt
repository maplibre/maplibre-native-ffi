package org.maplibre.nativeffi.map

/** Viewport orientation mode used by map viewport options. */
public enum class ViewportMode(internal val nativeValue: UInt) {
  DEFAULT(0U),
  FLIPPED_Y(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): ViewportMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
