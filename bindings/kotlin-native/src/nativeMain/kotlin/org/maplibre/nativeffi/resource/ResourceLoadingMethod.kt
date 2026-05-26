package org.maplibre.nativeffi.resource

/** Resource loading method copied from a native resource request. */
public enum class ResourceLoadingMethod(public val nativeValue: Int) {
  ALL(0),
  CACHE_ONLY(1),
  NETWORK_ONLY(2),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ResourceLoadingMethod =
      fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ResourceLoadingMethod =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
