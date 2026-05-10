package org.maplibre.nativeffi;

import java.util.Objects;
import java.util.Optional;

/** Copied fixed metadata for one style source. */
public record StyleSourceInfo(
    StyleSourceType type, int nativeType, boolean volatileSource, Optional<String> attribution) {
  public StyleSourceInfo {
    Objects.requireNonNull(type, "type");
    attribution = Objects.requireNonNull(attribution, "attribution");
  }
}
