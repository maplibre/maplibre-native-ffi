package org.maplibre.nativeffi;

/**
 * Receives process-global MapLibre Native log records.
 *
 * <p>Native code may invoke this callback on logging or worker threads. The callback should return
 * quickly and avoid calling MapLibre APIs. The binding copies each record before invoking Java code
 * and contains callback exceptions so they do not unwind into native code.
 */
@FunctionalInterface
public interface LogCallback {
  /**
   * Handles a copied log record.
   *
   * @return true when the callback consumed the record, false to let native logging handle it
   */
  boolean log(LogRecord record);
}
