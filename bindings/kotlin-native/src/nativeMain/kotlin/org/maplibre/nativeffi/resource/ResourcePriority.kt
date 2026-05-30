package org.maplibre.nativeffi.resource

/** Resource request priority copied from a native resource request. */
public enum class ResourcePriority(public val nativeValue: Int) {
  REGULAR(0),
  LOW(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ResourcePriority = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ResourcePriority =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
