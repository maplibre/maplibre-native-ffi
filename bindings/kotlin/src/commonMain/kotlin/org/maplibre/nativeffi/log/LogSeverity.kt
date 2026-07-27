package org.maplibre.nativeffi.log

import kotlin.jvm.JvmInline
import org.maplibre.nativeffi.internal.status.Status

/**
 * Severity for a Maplibre Native log record.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
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
      Status.requireArgument(isKnown) {
        "Unknown log severity cannot be used as an input: $nativeValue"
      }
      return 1 shl nativeValue
    }

  internal val isKnown: Boolean
    get() = this == INFO || this == WARNING || this == ERROR
}
