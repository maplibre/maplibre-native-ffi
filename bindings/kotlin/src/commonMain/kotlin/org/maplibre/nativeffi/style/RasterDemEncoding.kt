package org.maplibre.nativeffi.style

import kotlin.jvm.JvmInline

/** DEM raster encoding for raster DEM style sources, including unknown native values. */
@JvmInline
public value class RasterDemEncoding(public val nativeValue: Int) {
  public companion object {
    public val MAPBOX: RasterDemEncoding = RasterDemEncoding(0)
    public val TERRARIUM: RasterDemEncoding = RasterDemEncoding(1)

    internal fun fromNative(nativeValue: UInt): RasterDemEncoding =
      RasterDemEncoding(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): RasterDemEncoding = RasterDemEncoding(nativeValue)
  }
}
