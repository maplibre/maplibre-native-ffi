package org.maplibre.nativeffi.resource

/** Resource usage copied from a native resource request. */
public enum class ResourceUsage(internal val nativeValue: UInt) {
  ONLINE(0U),
  OFFLINE(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): ResourceUsage =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
