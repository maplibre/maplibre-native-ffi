package org.maplibre.nativeffi.log

/** Copied Maplibre Native log record delivered to a Kotlin log callback. */
public data class LogRecord(
  public val severity: LogSeverity,
  public val rawSeverity: Int,
  public val event: LogEvent,
  public val rawEvent: Int,
  public val code: Long,
  public val message: String,
)
