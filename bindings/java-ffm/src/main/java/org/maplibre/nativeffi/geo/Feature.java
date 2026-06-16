package org.maplibre.nativeffi.geo;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativeffi.json.JsonValue;

/** Immutable GeoJSON feature descriptor. */
public record Feature(
    Geometry geometry, List<JsonValue.Member> properties, FeatureIdentifier identifier) {
  public Feature {
    Objects.requireNonNull(geometry, "geometry");
    properties = List.copyOf(properties);
    identifier = identifier == null ? FeatureIdentifier.nullValue() : identifier;
  }
}
