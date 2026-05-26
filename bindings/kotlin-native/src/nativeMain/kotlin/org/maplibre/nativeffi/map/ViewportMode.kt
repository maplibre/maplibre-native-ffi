package org.maplibre.nativeffi.map

/** Viewport orientation mode used by map viewport options. */
public enum class ViewportMode(public val nativeValue: Int) {
  DEFAULT(0),
  FLIPPED_Y(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ViewportMode = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ViewportMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
