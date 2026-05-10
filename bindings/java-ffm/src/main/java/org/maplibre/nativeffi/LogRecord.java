package org.maplibre.nativeffi;

/** Copied MapLibre Native log record delivered to a Java log callback. */
public record LogRecord(
    LogSeverity severity,
    int rawSeverity,
    LogEvent event,
    int rawEvent,
    long code,
    String message) {}
