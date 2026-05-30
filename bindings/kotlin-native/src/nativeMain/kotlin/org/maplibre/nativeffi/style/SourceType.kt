package org.maplibre.nativeffi.style

/** Style source type values returned by native style metadata. */
public enum class SourceType(public val nativeValue: Int) {
  UNKNOWN(0),
  VECTOR(1),
  RASTER(2),
  RASTER_DEM(3),
  GEOJSON(4),
  IMAGE(5),
  VIDEO(6),
  ANNOTATIONS(7),
  CUSTOM_VECTOR(8);

  public companion object {
    internal fun fromNative(nativeValue: UInt): SourceType = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): SourceType =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
