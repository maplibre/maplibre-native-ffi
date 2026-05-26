package org.maplibre.nativeffi.style

/** DEM raster encoding for raster DEM style sources. */
public enum class RasterDemEncoding(public val nativeValue: Int) {
  MAPBOX(0),
  TERRARIUM(1),
}
