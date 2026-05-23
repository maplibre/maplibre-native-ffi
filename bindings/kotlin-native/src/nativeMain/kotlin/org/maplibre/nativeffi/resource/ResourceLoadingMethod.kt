package org.maplibre.nativeffi.resource

/** Resource loading method copied from a native resource request. */
public enum class ResourceLoadingMethod(internal val nativeValue: UInt) {
  ALL(0U),
  CACHE_ONLY(1U),
  NETWORK_ONLY(2U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): ResourceLoadingMethod =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
