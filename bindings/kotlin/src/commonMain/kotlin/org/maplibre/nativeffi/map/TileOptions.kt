package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.internal.status.Status

/** Mutable descriptor for tile prefetch and level-of-detail controls. */
public class TileOptions {
  public var prefetchZoomDelta: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "prefetchZoomDelta must be non-negative" } }
      field = value
    }

  public var lodMinRadius: Double? = null

  public var lodScale: Double? = null

  public var lodPitchThreshold: Double? = null

  public var lodZoomShift: Double? = null

  public var lodMode: TileLodMode? = null
}
