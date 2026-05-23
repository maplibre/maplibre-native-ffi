package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.geo.LatLngBounds

/** Mutable descriptor for vector, raster, and raster DEM style tile sources. */
public class TileSourceOptions {
  public var minZoom: Double? = null
    private set

  public var maxZoom: Double? = null
    private set

  public var attribution: String? = null
    private set

  public var scheme: TileScheme? = null
    private set

  public var bounds: LatLngBounds? = null
    private set

  public var tileSize: UInt? = null
    private set

  public var vectorEncoding: VectorTileEncoding? = null
    private set

  public var rasterDemEncoding: RasterDemEncoding? = null
    private set

  public fun hasMinZoom(): Boolean = minZoom != null

  public fun minZoom(minZoom: Double): TileSourceOptions = apply { this.minZoom = minZoom }

  public fun clearMinZoom(): TileSourceOptions = apply { minZoom = null }

  public fun hasMaxZoom(): Boolean = maxZoom != null

  public fun maxZoom(maxZoom: Double): TileSourceOptions = apply { this.maxZoom = maxZoom }

  public fun clearMaxZoom(): TileSourceOptions = apply { maxZoom = null }

  public fun hasAttribution(): Boolean = attribution != null

  public fun attribution(attribution: String): TileSourceOptions = apply {
    this.attribution = attribution
  }

  public fun clearAttribution(): TileSourceOptions = apply { attribution = null }

  public fun hasScheme(): Boolean = scheme != null

  public fun scheme(scheme: TileScheme): TileSourceOptions = apply { this.scheme = scheme }

  public fun clearScheme(): TileSourceOptions = apply { scheme = null }

  public fun hasBounds(): Boolean = bounds != null

  public fun bounds(bounds: LatLngBounds): TileSourceOptions = apply { this.bounds = bounds }

  public fun clearBounds(): TileSourceOptions = apply { bounds = null }

  public fun hasTileSize(): Boolean = tileSize != null

  public fun tileSize(tileSize: UInt): TileSourceOptions = apply { this.tileSize = tileSize }

  public fun clearTileSize(): TileSourceOptions = apply { tileSize = null }

  public fun hasVectorEncoding(): Boolean = vectorEncoding != null

  public fun vectorEncoding(vectorEncoding: VectorTileEncoding): TileSourceOptions = apply {
    this.vectorEncoding = vectorEncoding
  }

  public fun clearVectorEncoding(): TileSourceOptions = apply { vectorEncoding = null }

  public fun hasRasterDemEncoding(): Boolean = rasterDemEncoding != null

  public fun rasterDemEncoding(rasterDemEncoding: RasterDemEncoding): TileSourceOptions = apply {
    this.rasterDemEncoding = rasterDemEncoding
  }

  public fun clearRasterDemEncoding(): TileSourceOptions = apply { rasterDemEncoding = null }
}
