package org.maplibre.nativeffi.log

/** Severity for a Maplibre Native log record. */
public enum class LogSeverity(public val nativeValue: Int) {
  INFO(1),
  WARNING(2),
  ERROR(3),
  UNKNOWN(-1);

  public val nativeMask: Int
    get() {
      require(this != UNKNOWN) { "UNKNOWN log severity cannot be used as an input" }
      return 1 shl nativeValue.toInt()
    }

  public companion object {
    internal fun fromNative(nativeValue: UInt): LogSeverity = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): LogSeverity =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
