package org.maplibre.nativeffi.query;

import java.util.Objects;
import java.util.Optional;
import org.maplibre.nativeffi.geo.Feature;
import org.maplibre.nativeffi.json.JsonValue;

/** One feature copied from a rendered or source feature query result. */
public record QueriedFeature(
    Feature feature,
    Optional<String> sourceId,
    Optional<String> sourceLayerId,
    Optional<JsonValue> state) {
  public QueriedFeature {
    Objects.requireNonNull(feature, "feature");
    sourceId = sourceId == null ? Optional.empty() : sourceId;
    sourceLayerId = sourceLayerId == null ? Optional.empty() : sourceLayerId;
    state = state == null ? Optional.empty() : state;
  }
}
