package org.maplibre.nativeffi.style

/** Style source type values returned by native style metadata. */
public enum class SourceType(public val nativeValue: UInt) {
  UNKNOWN(0U),
  VECTOR(1U),
  RASTER(2U),
  RASTER_DEM(3U),
  GEOJSON(4U),
  IMAGE(5U),
  VIDEO(6U),
  ANNOTATIONS(7U),
  CUSTOM_VECTOR(8U);

  public companion object {
    public fun fromNative(nativeValue: UInt): SourceType =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
