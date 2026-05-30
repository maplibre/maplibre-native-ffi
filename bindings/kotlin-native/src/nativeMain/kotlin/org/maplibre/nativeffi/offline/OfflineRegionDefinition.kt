package org.maplibre.nativeffi.offline

import org.maplibre.nativeffi.geo.Geometry
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

  public data class GeometryRegion(
    public val styleUrl: String,
    public val geometry: Geometry,
    public val minZoom: Double,
    public val maxZoom: Double,
    public val pixelRatio: Float,
    public val includeIdeographs: Boolean,
  ) : OfflineRegionDefinition
}
