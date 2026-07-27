package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/**
 * Map rendering modes used when creating a map.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class MapMode(public val nativeValue: Int) {
  public companion object {
    public val CONTINUOUS: MapMode = MapMode(0)
    public val STATIC: MapMode = MapMode(1)
    public val TILE: MapMode = MapMode(2)

    internal fun fromNative(nativeValue: UInt): MapMode = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): MapMode = MapMode(nativeValue)
  }

  internal val isKnown: Boolean
    get() = this == CONTINUOUS || this == STATIC || this == TILE
}
