package org.maplibre.nativejni.style;

import java.util.Objects;
import java.util.Optional;

/** Copied fixed metadata for one style source. */
public record SourceInfo(
    SourceType type, int nativeType, boolean volatileSource, Optional<String> attribution) {
  public SourceInfo {
    Objects.requireNonNull(type, "type");
    attribution = Objects.requireNonNull(attribution, "attribution");
  }
}
