package org.maplibre.nativeffi.style

import kotlin.jvm.JvmInline

/** Style source type values returned by native style metadata. */
@JvmInline
public value class SourceType(public val nativeValue: Int) {
  public companion object {
    public val UNKNOWN: SourceType = SourceType(0)
    public val VECTOR: SourceType = SourceType(1)
    public val RASTER: SourceType = SourceType(2)
    public val RASTER_DEM: SourceType = SourceType(3)
    public val GEOJSON: SourceType = SourceType(4)
    public val IMAGE: SourceType = SourceType(5)
    public val VIDEO: SourceType = SourceType(6)
    public val ANNOTATIONS: SourceType = SourceType(7)
    public val CUSTOM_VECTOR: SourceType = SourceType(8)

    internal fun fromNative(nativeValue: UInt): SourceType = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): SourceType = SourceType(nativeValue)
  }
}
