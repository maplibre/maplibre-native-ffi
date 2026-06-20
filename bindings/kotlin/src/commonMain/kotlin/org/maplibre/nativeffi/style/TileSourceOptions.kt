package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.status.Status

/** Mutable descriptor for vector, raster, and raster DEM style tile sources. */
public class TileSourceOptions {
  public var minZoom: Double? = null

  public var maxZoom: Double? = null

  public var attribution: String? = null

  public var scheme: TileScheme? = null

  public var bounds: LatLngBounds? = null

  public var tileSize: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "tileSize must be non-negative" } }
      field = value
    }

  public var vectorEncoding: VectorTileEncoding? = null

  public var rasterDemEncoding: RasterDemEncoding? = null
}
