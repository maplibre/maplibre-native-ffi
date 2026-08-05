package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.geo.LatLngBounds

/**
 * Copied metadata for one style source.
 *
 * [url] is present when the source retains a URL. [tileJson] is present after MapLibre has parsed a
 * tiled source description. The value remains valid after its source or map closes.
 */
public data class SourceInfo(
  public val type: SourceType,
  public val volatileSource: Boolean,
  public val attribution: String?,
  public val url: String?,
  public val tileJson: TileJson?,
  public val tileSize: Int?,
  public val vectorEncoding: VectorTileEncoding?,
  public val rasterDemEncoding: RasterDemEncoding?,
)

/**
 * Parsed TileJSON fields that MapLibre retains for a tiled style source.
 *
 * This value contains normalized retained fields rather than the original TileJSON document.
 */
public data class TileJson(
  public val tileUrls: List<String>,
  public val minZoom: Double,
  public val maxZoom: Double,
  public val scheme: TileScheme,
  public val bounds: LatLngBounds?,
)
