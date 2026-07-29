package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.LatLngBounds

/** Geographic constraint applied to the map camera center. */
public sealed interface BoundsConstraint {
  /** Keeps the camera center inside [bounds]. */
  public data class Bounded(public val bounds: LatLngBounds) : BoundsConstraint

  /**
   * Leaves the camera center unconstrained, so the map pans freely across the antimeridian. This
   * differs from world bounds of -90/-180 to 90/180, which clamp longitude to that range.
   */
  public data object Unbounded : BoundsConstraint
}
