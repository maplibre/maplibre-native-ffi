package org.maplibre.nativeffi.geo

/** Immutable geometry tree used by Maplibre descriptors and copied results. */
public sealed interface Geometry {
  public data object Empty : Geometry

  public data class Point(public val coordinate: LatLng) : Geometry

  public class LineString(coordinates: List<LatLng>) : Geometry {
    public val coordinates: List<LatLng> = coordinates.toList()

    override fun equals(other: Any?): Boolean =
      other is LineString && coordinates == other.coordinates

    override fun hashCode(): Int = coordinates.hashCode()

    override fun toString(): String = "LineString(coordinates=$coordinates)"
  }

  public class Polygon(rings: List<List<LatLng>>) : Geometry {
    public val rings: List<List<LatLng>> = rings.map { it.toList() }

    override fun equals(other: Any?): Boolean = other is Polygon && rings == other.rings

    override fun hashCode(): Int = rings.hashCode()

    override fun toString(): String = "Polygon(rings=$rings)"
  }

  public class MultiPoint(coordinates: List<LatLng>) : Geometry {
    public val coordinates: List<LatLng> = coordinates.toList()

    override fun equals(other: Any?): Boolean =
      other is MultiPoint && coordinates == other.coordinates

    override fun hashCode(): Int = coordinates.hashCode()

    override fun toString(): String = "MultiPoint(coordinates=$coordinates)"
  }

  public class MultiLineString(lines: List<List<LatLng>>) : Geometry {
    public val lines: List<List<LatLng>> = lines.map { it.toList() }

    override fun equals(other: Any?): Boolean = other is MultiLineString && lines == other.lines

    override fun hashCode(): Int = lines.hashCode()

    override fun toString(): String = "MultiLineString(lines=$lines)"
  }

  public class MultiPolygon(polygons: List<List<List<LatLng>>>) : Geometry {
    public val polygons: List<List<List<LatLng>>> = polygons.map { polygon ->
      polygon.map { it.toList() }
    }

    override fun equals(other: Any?): Boolean = other is MultiPolygon && polygons == other.polygons

    override fun hashCode(): Int = polygons.hashCode()

    override fun toString(): String = "MultiPolygon(polygons=$polygons)"
  }

  public class Collection(geometries: List<Geometry>) : Geometry {
    public val geometries: List<Geometry> = geometries.toList()

    override fun equals(other: Any?): Boolean =
      other is Collection && geometries == other.geometries

    override fun hashCode(): Int = geometries.hashCode()

    override fun toString(): String = "Collection(geometries=$geometries)"
  }

  public class Unknown internal constructor(public val rawType: Int, public val rawSize: Int) :
    Geometry {
    override fun equals(other: Any?): Boolean =
      other is Unknown && rawType == other.rawType && rawSize == other.rawSize

    override fun hashCode(): Int {
      var result = rawType
      result = 31 * result + rawSize
      return result
    }

    override fun toString(): String = "Unknown(rawType=$rawType, rawSize=$rawSize)"
  }

  public companion object {
    public const val MAX_COLLECTION_DEPTH: Int = 64
  }
}
