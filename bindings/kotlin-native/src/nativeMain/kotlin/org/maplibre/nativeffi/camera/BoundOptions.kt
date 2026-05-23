package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.LatLngBounds

/** Mutable map bounds descriptor. */
public class BoundOptions {
  public var bounds: LatLngBounds? = null
    private set

  public var minZoom: Double? = null
    private set

  public var maxZoom: Double? = null
    private set

  public var minPitch: Double? = null
    private set

  public var maxPitch: Double? = null
    private set

  public fun hasBounds(): Boolean = bounds != null

  public fun bounds(bounds: LatLngBounds): BoundOptions = apply { this.bounds = bounds }

  public fun clearBounds(): BoundOptions = apply { bounds = null }

  public fun hasMinZoom(): Boolean = minZoom != null

  public fun minZoom(minZoom: Double): BoundOptions = apply { this.minZoom = minZoom }

  public fun clearMinZoom(): BoundOptions = apply { minZoom = null }

  public fun hasMaxZoom(): Boolean = maxZoom != null

  public fun maxZoom(maxZoom: Double): BoundOptions = apply { this.maxZoom = maxZoom }

  public fun clearMaxZoom(): BoundOptions = apply { maxZoom = null }

  public fun hasMinPitch(): Boolean = minPitch != null

  public fun minPitch(minPitch: Double): BoundOptions = apply { this.minPitch = minPitch }

  public fun clearMinPitch(): BoundOptions = apply { minPitch = null }

  public fun hasMaxPitch(): Boolean = maxPitch != null

  public fun maxPitch(maxPitch: Double): BoundOptions = apply { this.maxPitch = maxPitch }

  public fun clearMaxPitch(): BoundOptions = apply { maxPitch = null }
}
