package org.maplibre.nativejni.query;

import java.util.Objects;

/** Mutable selector for render-session feature-state operations. */
public final class FeatureStateSelector {
  private final String sourceId;
  private String sourceLayerId;
  private String featureId;
  private String stateKey;

  public FeatureStateSelector(String sourceId) {
    this.sourceId = Objects.requireNonNull(sourceId, "sourceId");
  }

  public String sourceId() {
    return sourceId;
  }

  public boolean hasSourceLayerId() {
    return sourceLayerId != null;
  }

  public String sourceLayerId() {
    return sourceLayerId;
  }

  public FeatureStateSelector sourceLayerId(String sourceLayerId) {
    this.sourceLayerId = Objects.requireNonNull(sourceLayerId, "sourceLayerId");
    return this;
  }

  public FeatureStateSelector clearSourceLayerId() {
    sourceLayerId = null;
    return this;
  }

  public boolean hasFeatureId() {
    return featureId != null;
  }

  public String featureId() {
    return featureId;
  }

  public FeatureStateSelector featureId(String featureId) {
    this.featureId = Objects.requireNonNull(featureId, "featureId");
    return this;
  }

  public FeatureStateSelector clearFeatureId() {
    featureId = null;
    stateKey = null;
    return this;
  }

  public boolean hasStateKey() {
    return stateKey != null;
  }

  public String stateKey() {
    return stateKey;
  }

  public FeatureStateSelector stateKey(String stateKey) {
    if (featureId == null) {
      throw new IllegalStateException("stateKey requires featureId");
    }
    this.stateKey = Objects.requireNonNull(stateKey, "stateKey");
    return this;
  }

  public FeatureStateSelector clearStateKey() {
    stateKey = null;
    return this;
  }
}
