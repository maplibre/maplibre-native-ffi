package org.maplibre.nativeffi.style

/** DEM raster encoding for raster DEM style sources. */
public enum class RasterDemEncoding(public val nativeValue: UInt) {
  MAPBOX(0U),
  TERRARIUM(1U),
}
