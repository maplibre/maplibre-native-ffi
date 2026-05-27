package org.maplibre.nativejni.query;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.json.JsonValue;

/** Copied result from a feature extension query. */
public sealed interface FeatureExtensionResult
    permits FeatureExtensionResult.Value,
        FeatureExtensionResult.FeatureCollection,
        FeatureExtensionResult.Unknown {
  record Value(JsonValue value) implements FeatureExtensionResult {
    public Value {
      Objects.requireNonNull(value, "value");
    }
  }

  record FeatureCollection(List<Feature> features) implements FeatureExtensionResult {
    public FeatureCollection {
      features = List.copyOf(features);
    }
  }

  record Unknown(int rawType) implements FeatureExtensionResult {}
}
