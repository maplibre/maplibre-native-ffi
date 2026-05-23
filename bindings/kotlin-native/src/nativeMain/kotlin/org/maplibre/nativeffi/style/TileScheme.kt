package org.maplibre.nativeffi.style

/** Tile URL coordinate scheme for vector, raster, and raster DEM sources. */
public enum class TileScheme(public val nativeValue: UInt) {
  XYZ(0U),
  TMS(1U),
}
