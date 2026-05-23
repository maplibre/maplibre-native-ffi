package org.maplibre.nativeffi.log

/** Severity for a Maplibre Native log record. */
public enum class LogSeverity(public val nativeValue: UInt) {
  INFO(1U),
  WARNING(2U),
  ERROR(3U),
  UNKNOWN(UInt.MAX_VALUE);

  public val nativeMask: UInt
    get() {
      require(this != UNKNOWN) { "UNKNOWN log severity cannot be used as an input" }
      return 1U shl nativeValue.toInt()
    }

  public companion object {
    public fun fromNative(nativeValue: UInt): LogSeverity =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
