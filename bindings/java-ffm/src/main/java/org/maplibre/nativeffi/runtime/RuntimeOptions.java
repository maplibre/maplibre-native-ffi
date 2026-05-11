package org.maplibre.nativeffi.runtime;

import java.util.OptionalLong;

/** Mutable descriptor used when creating a {@link RuntimeHandle}. */
public final class RuntimeOptions {
  private String assetPath;
  private String cachePath;
  private Long maximumCacheSize;

  public String assetPath() {
    return assetPath;
  }

  public RuntimeOptions assetPath(String assetPath) {
    this.assetPath = assetPath;
    return this;
  }

  public RuntimeOptions clearAssetPath() {
    assetPath = null;
    return this;
  }

  public String cachePath() {
    return cachePath;
  }

  public RuntimeOptions cachePath(String cachePath) {
    this.cachePath = cachePath;
    return this;
  }

  public RuntimeOptions clearCachePath() {
    cachePath = null;
    return this;
  }

  public OptionalLong maximumCacheSize() {
    return maximumCacheSize == null ? OptionalLong.empty() : OptionalLong.of(maximumCacheSize);
  }

  public boolean hasMaximumCacheSize() {
    return maximumCacheSize != null;
  }

  public RuntimeOptions maximumCacheSize(long maximumCacheSize) {
    this.maximumCacheSize = maximumCacheSize;
    return this;
  }

  public RuntimeOptions clearMaximumCacheSize() {
    maximumCacheSize = null;
    return this;
  }
}
