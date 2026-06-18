package org.maplibre.nativeffi.log

import kotlin.jvm.JvmInline

/** Severity for a Maplibre Native log record. */
@JvmInline
public value class LogSeverity(public val nativeValue: Int) {
  public companion object {
    public val INFO: LogSeverity = LogSeverity(1)
    public val WARNING: LogSeverity = LogSeverity(2)
    public val ERROR: LogSeverity = LogSeverity(3)

    internal fun fromNative(nativeValue: UInt): LogSeverity = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): LogSeverity = LogSeverity(nativeValue)
  }

  public val nativeMask: Int
    get() {
      require(isKnown) { "Unknown log severity cannot be used as an input: $nativeValue" }
      return 1 shl nativeValue.toInt()
    }

  internal val isKnown: Boolean
    get() = this == INFO || this == WARNING || this == ERROR
}
