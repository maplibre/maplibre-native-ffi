package org.maplibre.nativeffi.style

/** Tile URL coordinate scheme for vector, raster, and raster DEM sources. */
public enum class TileScheme(public val nativeValue: Int) {
  XYZ(0),
  TMS(1),
}
