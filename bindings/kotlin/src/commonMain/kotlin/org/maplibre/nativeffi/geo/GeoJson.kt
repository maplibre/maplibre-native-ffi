package org.maplibre.nativeffi.geo

/** Immutable GeoJSON descriptor tree. */
public sealed interface GeoJson {
  public data class GeometryValue(public val geometry: Geometry) : GeoJson

  public data class FeatureValue(public val feature: Feature) : GeoJson

  public class FeatureCollection(features: List<Feature>) : GeoJson {
    public val features: List<Feature> = features.toList()

    override fun equals(other: Any?): Boolean =
      other is FeatureCollection && features == other.features

    override fun hashCode(): Int = features.hashCode()

    override fun toString(): String = "FeatureCollection(features=$features)"
  }
}
