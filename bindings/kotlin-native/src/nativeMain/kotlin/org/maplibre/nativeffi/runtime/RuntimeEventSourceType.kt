package org.maplibre.nativeffi.runtime

/** Source kind for a copied runtime event. */
public enum class RuntimeEventSourceType(public val nativeValue: UInt) {
  RUNTIME(0U),
  MAP(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): RuntimeEventSourceType =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
