package org.maplibre.nativeffi.geo;

import java.util.List;
import java.util.Objects;

/** Immutable GeoJSON descriptor tree. */
public sealed interface GeoJson
    permits GeoJson.GeometryValue, GeoJson.FeatureValue, GeoJson.FeatureCollection {
  static GeometryValue geometry(Geometry geometry) {
    return new GeometryValue(geometry);
  }

  static FeatureValue feature(Feature feature) {
    return new FeatureValue(feature);
  }

  static FeatureCollection featureCollection(List<Feature> features) {
    return new FeatureCollection(features);
  }

  record GeometryValue(Geometry geometry) implements GeoJson {
    public GeometryValue {
      Objects.requireNonNull(geometry, "geometry");
    }
  }

  record FeatureValue(Feature feature) implements GeoJson {
    public FeatureValue {
      Objects.requireNonNull(feature, "feature");
    }
  }

  record FeatureCollection(List<Feature> features) implements GeoJson {
    public FeatureCollection {
      features = List.copyOf(features);
    }
  }
}
