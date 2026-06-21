package org.maplibre.nativeffi.geo

import org.maplibre.nativeffi.internal.status.Status

/** Canonical tile identity used by custom geometry source callbacks. */
public data class CanonicalTileId(public val z: Int, public val x: Long, public val y: Long) {
  init {
    Status.requireArgument(z >= 0) { "canonical tile z must be non-negative" }
    Status.requireArgument(x in 0..UInt.MAX_VALUE.toLong()) { "canonical tile x is out of range" }
    Status.requireArgument(y in 0..UInt.MAX_VALUE.toLong()) { "canonical tile y is out of range" }
  }
}
