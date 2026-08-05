package org.maplibre.nativeffi.style

import kotlin.jvm.JvmInline

/**
 * Tile URL coordinate scheme for vector, raster, and raster DEM sources.
 *
 * Unknown native values remain available through [nativeValue].
 */
@JvmInline
public value class TileScheme(public val nativeValue: Int) {
  public companion object {
    public val XYZ: TileScheme = TileScheme(0)
    public val TMS: TileScheme = TileScheme(1)

    internal fun fromNative(nativeValue: UInt): TileScheme = TileScheme(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): TileScheme = TileScheme(nativeValue)
  }
}
