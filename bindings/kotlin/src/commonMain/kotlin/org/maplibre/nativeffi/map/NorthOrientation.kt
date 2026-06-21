package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/** North orientation used by map viewport options. */
@JvmInline
public value class NorthOrientation(public val nativeValue: Int) {
  public companion object {
    public val UP: NorthOrientation = NorthOrientation(0)
    public val RIGHT: NorthOrientation = NorthOrientation(1)
    public val DOWN: NorthOrientation = NorthOrientation(2)
    public val LEFT: NorthOrientation = NorthOrientation(3)

    internal fun fromNative(nativeValue: UInt): NorthOrientation = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): NorthOrientation = NorthOrientation(nativeValue)
  }

  internal val isKnown: Boolean
    get() = this == UP || this == RIGHT || this == DOWN || this == LEFT
}
