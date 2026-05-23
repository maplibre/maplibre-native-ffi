package org.maplibre.nativeffi.resource

/** Resource storage policy copied from a native resource request. */
public enum class ResourceStoragePolicy(public val nativeValue: UInt) {
  PERMANENT(0U),
  VOLATILE(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): ResourceStoragePolicy =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
