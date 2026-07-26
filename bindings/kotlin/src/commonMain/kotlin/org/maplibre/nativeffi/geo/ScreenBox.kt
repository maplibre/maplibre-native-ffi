package org.maplibre.nativeffi.geo

/**
 * Screen-space box in logical map pixels.
 *
 * Corners may be given in any order, and may extend past the viewport. Rendered queries normalize
 * the corners and clip the box to the viewport, so a box that over-covers the viewport queries
 * everything visible.
 */
public data class ScreenBox(public val min: ScreenPoint, public val max: ScreenPoint)
