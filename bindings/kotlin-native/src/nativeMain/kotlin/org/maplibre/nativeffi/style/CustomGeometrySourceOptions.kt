package org.maplibre.nativeffi.style

/** Mutable descriptor for custom geometry sources. */
public class CustomGeometrySourceOptions(public val callback: CustomGeometrySourceCallback) {
  public var minZoom: Double? = null
    private set

  public var maxZoom: Double? = null
    private set

  public var tolerance: Double? = null
    private set

  public var tileSize: UInt? = null
    private set

  public var buffer: UInt? = null
    private set

  public var clip: Boolean? = null
    private set

  public var wrap: Boolean? = null
    private set

  public fun hasMinZoom(): Boolean = minZoom != null

  public fun minZoom(minZoom: Double): CustomGeometrySourceOptions = apply {
    this.minZoom = minZoom
  }

  public fun clearMinZoom(): CustomGeometrySourceOptions = apply { minZoom = null }

  public fun hasMaxZoom(): Boolean = maxZoom != null

  public fun maxZoom(maxZoom: Double): CustomGeometrySourceOptions = apply {
    this.maxZoom = maxZoom
  }

  public fun clearMaxZoom(): CustomGeometrySourceOptions = apply { maxZoom = null }

  public fun hasTolerance(): Boolean = tolerance != null

  public fun tolerance(tolerance: Double): CustomGeometrySourceOptions = apply {
    this.tolerance = tolerance
  }

  public fun clearTolerance(): CustomGeometrySourceOptions = apply { tolerance = null }

  public fun hasTileSize(): Boolean = tileSize != null

  public fun tileSize(tileSize: UInt): CustomGeometrySourceOptions = apply {
    this.tileSize = tileSize
  }

  public fun clearTileSize(): CustomGeometrySourceOptions = apply { tileSize = null }

  public fun hasBuffer(): Boolean = buffer != null

  public fun buffer(buffer: UInt): CustomGeometrySourceOptions = apply { this.buffer = buffer }

  public fun clearBuffer(): CustomGeometrySourceOptions = apply { buffer = null }

  public fun hasClip(): Boolean = clip != null

  public fun clip(clip: Boolean): CustomGeometrySourceOptions = apply { this.clip = clip }

  public fun clearClip(): CustomGeometrySourceOptions = apply { clip = null }

  public fun hasWrap(): Boolean = wrap != null

  public fun wrap(wrap: Boolean): CustomGeometrySourceOptions = apply { this.wrap = wrap }

  public fun clearWrap(): CustomGeometrySourceOptions = apply { wrap = null }
}
