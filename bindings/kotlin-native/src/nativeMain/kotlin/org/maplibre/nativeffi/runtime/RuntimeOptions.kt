package org.maplibre.nativeffi.runtime

/** Mutable descriptor used when creating a [RuntimeHandle]. */
public class RuntimeOptions {
  public var assetPath: String? = null

  public var cachePath: String? = null

  public var maximumCacheSize: Long? = null
    set(value) {
      value?.let { require(it >= 0) { "maximumCacheSize must be non-negative" } }
      field = value
    }
}
