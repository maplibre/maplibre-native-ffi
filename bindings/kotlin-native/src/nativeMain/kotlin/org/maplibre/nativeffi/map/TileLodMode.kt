package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/** Tile level-of-detail algorithm used by map tile options. */
@JvmInline
public value class TileLodMode(public val nativeValue: Int) {
  public companion object {
    public val DEFAULT: TileLodMode = TileLodMode(0)
    public val DISTANCE: TileLodMode = TileLodMode(1)

    internal fun fromNative(nativeValue: UInt): TileLodMode = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): TileLodMode = TileLodMode(nativeValue)
  }

  internal val isKnown: Boolean
    get() = this == DEFAULT || this == DISTANCE
}
