package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint

/** Screen-space geometry used for rendered feature queries. */
public sealed interface RenderedQueryGeometry {
  public data class Point(public val point: ScreenPoint) : RenderedQueryGeometry

  public data class Box(public val box: ScreenBox) : RenderedQueryGeometry

  public class LineString(points: List<ScreenPoint>) : RenderedQueryGeometry {
    public val points: List<ScreenPoint> = points.toList()

    override fun equals(other: Any?): Boolean = other is LineString && points == other.points

    override fun hashCode(): Int = points.hashCode()

    override fun toString(): String = "LineString(points=$points)"
  }
}
