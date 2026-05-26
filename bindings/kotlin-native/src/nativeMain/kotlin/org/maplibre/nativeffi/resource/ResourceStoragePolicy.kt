package org.maplibre.nativeffi.resource

/** Resource storage policy copied from a native resource request. */
public enum class ResourceStoragePolicy(public val nativeValue: Int) {
  PERMANENT(0),
  VOLATILE(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ResourceStoragePolicy =
      fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ResourceStoragePolicy =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
