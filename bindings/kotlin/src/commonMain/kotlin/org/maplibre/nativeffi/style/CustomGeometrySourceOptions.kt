package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.internal.status.Status

/** Mutable descriptor for custom geometry sources. */
public class CustomGeometrySourceOptions(public val callback: CustomGeometrySourceCallback) {
  public var minZoom: Double? = null

  public var maxZoom: Double? = null

  public var tolerance: Double? = null

  public var tileSize: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "tileSize must be non-negative" } }
      field = value
    }

  public var buffer: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "buffer must be non-negative" } }
      field = value
    }

  public var clip: Boolean? = null

  public var wrap: Boolean? = null
}
