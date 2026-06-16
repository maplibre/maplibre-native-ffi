package org.maplibre.nativejni.log;

/** Copied Maplibre Native log record delivered to a Java log callback. */
public record LogRecord(LogSeverity severity, LogEvent event, long code, String message) {}
