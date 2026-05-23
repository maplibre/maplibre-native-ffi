package org.maplibre.nativeffi.geo

/** Immutable GeoJSON descriptor tree. */
public sealed interface GeoJson {
  public data class GeometryValue(public val geometry: Geometry) : GeoJson

  public data class FeatureValue(public val feature: Feature) : GeoJson

  public data class FeatureCollection(public val features: List<Feature>) : GeoJson

  public companion object {
    public fun geometry(geometry: Geometry): GeometryValue = GeometryValue(geometry)

    public fun feature(feature: Feature): FeatureValue = FeatureValue(feature)

    public fun featureCollection(features: List<Feature>): FeatureCollection =
      FeatureCollection(features.toList())
  }
}
