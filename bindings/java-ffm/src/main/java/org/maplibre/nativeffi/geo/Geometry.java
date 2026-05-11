package org.maplibre.nativeffi.geo;

import java.util.List;
import java.util.Objects;

/** Immutable geometry tree used by Maplibre descriptors and copied results. */
public sealed interface Geometry
    permits Geometry.Empty,
        Geometry.Point,
        Geometry.LineString,
        Geometry.Polygon,
        Geometry.MultiPoint,
        Geometry.MultiLineString,
        Geometry.MultiPolygon,
        Geometry.Collection {
  int MAX_COLLECTION_DEPTH = 64;

  static Empty empty() {
    return Empty.INSTANCE;
  }

  static Point point(LatLng coordinate) {
    return new Point(coordinate);
  }

  static LineString lineString(List<LatLng> coordinates) {
    return new LineString(coordinates);
  }

  static Polygon polygon(List<List<LatLng>> rings) {
    return new Polygon(rings);
  }

  static MultiPoint multiPoint(List<LatLng> coordinates) {
    return new MultiPoint(coordinates);
  }

  static MultiLineString multiLineString(List<List<LatLng>> lines) {
    return new MultiLineString(lines);
  }

  static MultiPolygon multiPolygon(List<List<List<LatLng>>> polygons) {
    return new MultiPolygon(polygons);
  }

  static Collection collection(List<Geometry> geometries) {
    return new Collection(geometries);
  }

  /** Singleton empty geometry. */
  final class Empty implements Geometry {
    public static final Empty INSTANCE = new Empty();

    private Empty() {}

    @Override
    public boolean equals(Object other) {
      return other instanceof Empty;
    }

    @Override
    public int hashCode() {
      return 0;
    }

    @Override
    public String toString() {
      return "Geometry.Empty";
    }
  }

  record Point(LatLng coordinate) implements Geometry {
    public Point {
      Objects.requireNonNull(coordinate, "coordinate");
    }
  }

  record LineString(List<LatLng> coordinates) implements Geometry {
    public LineString {
      coordinates = List.copyOf(coordinates);
    }
  }

  record Polygon(List<List<LatLng>> rings) implements Geometry {
    public Polygon {
      rings = copyNested(rings);
    }
  }

  record MultiPoint(List<LatLng> coordinates) implements Geometry {
    public MultiPoint {
      coordinates = List.copyOf(coordinates);
    }
  }

  record MultiLineString(List<List<LatLng>> lines) implements Geometry {
    public MultiLineString {
      lines = copyNested(lines);
    }
  }

  record MultiPolygon(List<List<List<LatLng>>> polygons) implements Geometry {
    public MultiPolygon {
      polygons = copyDeep(polygons);
    }
  }

  record Collection(List<Geometry> geometries) implements Geometry {
    public Collection {
      geometries = List.copyOf(geometries);
    }
  }

  private static <T> List<List<T>> copyNested(List<List<T>> values) {
    return List.copyOf(values.stream().map(List::copyOf).toList());
  }

  private static <T> List<List<List<T>>> copyDeep(List<List<List<T>>> values) {
    return List.copyOf(
        values.stream().map(polygons -> polygons.stream().map(List::copyOf).toList()).toList());
  }
}
