package org.maplibre.nativeffi.resource

/** Resource usage copied from a native resource request. */
public enum class ResourceUsage(public val nativeValue: Int) {
  ONLINE(0),
  OFFLINE(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ResourceUsage = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ResourceUsage =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
