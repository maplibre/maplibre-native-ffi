package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
import java.util.OptionalLong;
import org.maplibre.nativejni.runtime.RuntimeOptions;

/** Internal materializers for runtime options, events, and offline operation data. */
public final class RuntimeStructs {
  private RuntimeStructs() {}

  public record RuntimeOptionsValue(
      String assetPath, String cachePath, boolean hasMaximumCacheSize, long maximumCacheSize) {}

  public static RuntimeOptionsValue runtimeOptions(RuntimeOptions options) {
    Objects.requireNonNull(options, "options");
    OptionalLong maximumCacheSize = options.maximumCacheSize();
    return new RuntimeOptionsValue(
        options.assetPath(),
        options.cachePath(),
        maximumCacheSize.isPresent(),
        maximumCacheSize.orElse(0));
  }
}
