package org.maplibre.nativeffi.runtime

/** Source kind for a copied runtime event. */
public enum class RuntimeEventSourceType(public val nativeValue: Int) {
  RUNTIME(0),
  MAP(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): RuntimeEventSourceType =
      fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): RuntimeEventSourceType =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
