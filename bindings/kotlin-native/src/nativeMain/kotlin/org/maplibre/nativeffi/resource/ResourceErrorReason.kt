package org.maplibre.nativeffi.resource

/** Resource error reason copied from native events. */
public enum class ResourceErrorReason(public val nativeValue: Int) {
  NONE(0),
  NOT_FOUND(1),
  SERVER(2),
  CONNECTION(3),
  RATE_LIMIT(4),
  OTHER(5),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ResourceErrorReason =
      fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ResourceErrorReason =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
