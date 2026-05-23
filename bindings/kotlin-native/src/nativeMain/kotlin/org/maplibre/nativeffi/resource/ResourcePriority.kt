package org.maplibre.nativeffi.resource

/** Resource request priority copied from a native resource request. */
public enum class ResourcePriority(public val nativeValue: UInt) {
  REGULAR(0U),
  LOW(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): ResourcePriority =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
