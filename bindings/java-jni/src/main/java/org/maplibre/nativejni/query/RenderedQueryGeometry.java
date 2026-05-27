package org.maplibre.nativejni.query;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativejni.geo.ScreenBox;
import org.maplibre.nativejni.geo.ScreenPoint;

/** Screen-space geometry used for rendered feature queries. */
public sealed interface RenderedQueryGeometry
    permits RenderedQueryGeometry.Point,
        RenderedQueryGeometry.Box,
        RenderedQueryGeometry.LineString {
  static Point point(ScreenPoint point) {
    return new Point(point);
  }

  static Box box(ScreenBox box) {
    return new Box(box);
  }

  static LineString lineString(List<ScreenPoint> points) {
    return new LineString(points);
  }

  record Point(ScreenPoint point) implements RenderedQueryGeometry {
    public Point {
      Objects.requireNonNull(point, "point");
    }
  }

  record Box(ScreenBox box) implements RenderedQueryGeometry {
    public Box {
      Objects.requireNonNull(box, "box");
    }
  }

  record LineString(List<ScreenPoint> points) implements RenderedQueryGeometry {
    public LineString {
      points = List.copyOf(points);
    }
  }
}
