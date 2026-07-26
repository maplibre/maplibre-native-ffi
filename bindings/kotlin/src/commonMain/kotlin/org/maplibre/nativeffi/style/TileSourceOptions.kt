package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.status.Status

/**
 * Mutable descriptor for vector, raster, and raster DEM style tile sources.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
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

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: TileSourceOptions.() -> Unit = {}): TileSourceOptions =
    TileSourceOptions()
      .also {
        it.minZoom = minZoom
        it.maxZoom = maxZoom
        it.attribution = attribution
        it.scheme = scheme
        it.bounds = bounds
        it.tileSize = tileSize
        it.vectorEncoding = vectorEncoding
        it.rasterDemEncoding = rasterDemEncoding
      }
      .apply(block)

  private val fields: List<Any?>
    get() =
      listOf(
        minZoom,
        maxZoom,
        attribution,
        scheme,
        bounds,
        tileSize,
        vectorEncoding,
        rasterDemEncoding,
      )

  override fun equals(other: Any?): Boolean = other is TileSourceOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
