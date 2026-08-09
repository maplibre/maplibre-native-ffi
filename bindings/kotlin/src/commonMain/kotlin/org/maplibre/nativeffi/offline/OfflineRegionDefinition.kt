package org.maplibre.nativeffi.offline

import org.maplibre.nativeffi.geo.LatLngBounds

/** Offline region definition copied into native storage at creation time. */
public sealed interface OfflineRegionDefinition {
  public data class TilePyramid(
    public val styleUrl: String,
    public val bounds: LatLngBounds,
    public val minZoom: Double,
    public val maxZoom: Double,
    public val pixelRatio: Float,
    public val includeIdeographs: Boolean,
  ) : OfflineRegionDefinition

  public class GeometryRegion(
    public val styleUrl: String,
    geometry: ByteArray,
    public val minZoom: Double,
    public val maxZoom: Double,
    public val pixelRatio: Float,
    public val includeIdeographs: Boolean,
  ) : OfflineRegionDefinition {
    private val geometryBytes: ByteArray = geometry.copyOf()

    public val geometry: ByteArray
      get() = geometryBytes.copyOf()

    internal val geometryTransit: ByteArray
      get() = geometryBytes

    override fun equals(other: Any?): Boolean =
      other is GeometryRegion &&
        styleUrl == other.styleUrl &&
        geometryBytes.contentEquals(other.geometryBytes) &&
        minZoom == other.minZoom &&
        maxZoom == other.maxZoom &&
        pixelRatio == other.pixelRatio &&
        includeIdeographs == other.includeIdeographs

    override fun hashCode(): Int {
      var result = styleUrl.hashCode()
      result = 31 * result + geometryBytes.contentHashCode()
      result = 31 * result + minZoom.hashCode()
      result = 31 * result + maxZoom.hashCode()
      result = 31 * result + pixelRatio.hashCode()
      result = 31 * result + includeIdeographs.hashCode()
      return result
    }

    override fun toString(): String =
      "GeometryRegion(styleUrl=$styleUrl, geometry=${geometryBytes.contentToString()}, " +
        "minZoom=$minZoom, maxZoom=$maxZoom, pixelRatio=$pixelRatio, " +
        "includeIdeographs=$includeIdeographs)"
  }

  public class Unknown internal constructor(public val rawType: Int, public val rawSize: Int) :
    OfflineRegionDefinition {
    override fun equals(other: Any?): Boolean =
      other is Unknown && rawType == other.rawType && rawSize == other.rawSize

    override fun hashCode(): Int {
      var result = rawType
      result = 31 * result + rawSize
      return result
    }

    override fun toString(): String = "Unknown(rawType=$rawType, rawSize=$rawSize)"
  }
}
