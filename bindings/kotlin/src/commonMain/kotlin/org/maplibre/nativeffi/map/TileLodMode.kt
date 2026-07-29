package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/**
 * Tile level-of-detail algorithm used by map tile options.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
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
