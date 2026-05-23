package org.maplibre.nativeffi.geo

/** Immutable geometry tree used by Maplibre descriptors and copied results. */
public sealed interface Geometry {
  public data object Empty : Geometry

  public data class Point(public val coordinate: LatLng) : Geometry

  public data class LineString(public val coordinates: List<LatLng>) : Geometry

  public data class Polygon(public val rings: List<List<LatLng>>) : Geometry

  public data class MultiPoint(public val coordinates: List<LatLng>) : Geometry

  public data class MultiLineString(public val lines: List<List<LatLng>>) : Geometry

  public data class MultiPolygon(public val polygons: List<List<List<LatLng>>>) : Geometry

  public data class Collection(public val geometries: List<Geometry>) : Geometry

  public companion object {
    public const val MAX_COLLECTION_DEPTH: Int = 64

    public fun empty(): Empty = Empty

    public fun point(coordinate: LatLng): Point = Point(coordinate)

    public fun lineString(coordinates: List<LatLng>): LineString = LineString(coordinates.toList())

    public fun polygon(rings: List<List<LatLng>>): Polygon = Polygon(rings.map { it.toList() })

    public fun multiPoint(coordinates: List<LatLng>): MultiPoint = MultiPoint(coordinates.toList())

    public fun multiLineString(lines: List<List<LatLng>>): MultiLineString =
      MultiLineString(lines.map { it.toList() })

    public fun multiPolygon(polygons: List<List<List<LatLng>>>): MultiPolygon =
      MultiPolygon(polygons.map { polygon -> polygon.map { it.toList() } })

    public fun collection(geometries: List<Geometry>): Collection = Collection(geometries.toList())
  }
}
