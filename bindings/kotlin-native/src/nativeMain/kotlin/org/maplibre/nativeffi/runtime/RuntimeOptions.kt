package org.maplibre.nativeffi.runtime

/** Mutable descriptor used when creating a [RuntimeHandle]. */
public class RuntimeOptions {
  public var assetPath: String? = null
    private set

  public var cachePath: String? = null
    private set

  public var maximumCacheSize: Long? = null
    private set

  public fun assetPath(assetPath: String): RuntimeOptions = apply { this.assetPath = assetPath }

  public fun clearAssetPath(): RuntimeOptions = apply { assetPath = null }

  public fun cachePath(cachePath: String): RuntimeOptions = apply { this.cachePath = cachePath }

  public fun clearCachePath(): RuntimeOptions = apply { cachePath = null }

  public fun maximumCacheSize(maximumCacheSize: Long): RuntimeOptions = apply {
    this.maximumCacheSize = maximumCacheSize
  }

  public fun clearMaximumCacheSize(): RuntimeOptions = apply { maximumCacheSize = null }

  public fun hasMaximumCacheSize(): Boolean = maximumCacheSize != null
}
