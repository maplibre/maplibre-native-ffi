package org.maplibre.nativeffi.map

/** Mutable descriptor for tile prefetch and level-of-detail controls. */
public class TileOptions {
  public var prefetchZoomDelta: Int? = null
    private set

  public var lodMinRadius: Double? = null
    private set

  public var lodScale: Double? = null
    private set

  public var lodPitchThreshold: Double? = null
    private set

  public var lodZoomShift: Double? = null
    private set

  public var lodMode: TileLodMode? = null
    private set

  public fun hasPrefetchZoomDelta(): Boolean = prefetchZoomDelta != null

  public fun prefetchZoomDelta(prefetchZoomDelta: Int): TileOptions {
    require(prefetchZoomDelta >= 0) { "prefetchZoomDelta must be non-negative" }
    this.prefetchZoomDelta = prefetchZoomDelta
    return this
  }

  public fun clearPrefetchZoomDelta(): TileOptions = apply { prefetchZoomDelta = null }

  public fun hasLodMinRadius(): Boolean = lodMinRadius != null

  public fun lodMinRadius(lodMinRadius: Double): TileOptions = apply {
    this.lodMinRadius = lodMinRadius
  }

  public fun clearLodMinRadius(): TileOptions = apply { lodMinRadius = null }

  public fun hasLodScale(): Boolean = lodScale != null

  public fun lodScale(lodScale: Double): TileOptions = apply { this.lodScale = lodScale }

  public fun clearLodScale(): TileOptions = apply { lodScale = null }

  public fun hasLodPitchThreshold(): Boolean = lodPitchThreshold != null

  public fun lodPitchThreshold(lodPitchThreshold: Double): TileOptions = apply {
    this.lodPitchThreshold = lodPitchThreshold
  }

  public fun clearLodPitchThreshold(): TileOptions = apply { lodPitchThreshold = null }

  public fun hasLodZoomShift(): Boolean = lodZoomShift != null

  public fun lodZoomShift(lodZoomShift: Double): TileOptions = apply {
    this.lodZoomShift = lodZoomShift
  }

  public fun clearLodZoomShift(): TileOptions = apply { lodZoomShift = null }

  public fun hasLodMode(): Boolean = lodMode != null

  public fun lodMode(lodMode: TileLodMode): TileOptions = apply {
    require(lodMode != TileLodMode.UNKNOWN) { "lodMode must be a known value" }
    this.lodMode = lodMode
  }

  public fun clearLodMode(): TileOptions = apply { lodMode = null }
}
