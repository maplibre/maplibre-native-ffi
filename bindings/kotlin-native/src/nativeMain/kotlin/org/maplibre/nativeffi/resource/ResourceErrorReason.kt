package org.maplibre.nativeffi.resource

/** Resource error reason copied from native events. */
public enum class ResourceErrorReason(internal val nativeValue: UInt) {
  NONE(0U),
  NOT_FOUND(1U),
  SERVER(2U),
  CONNECTION(3U),
  RATE_LIMIT(4U),
  OTHER(5U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): ResourceErrorReason =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
