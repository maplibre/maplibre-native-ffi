package org.maplibre.nativeffi.log

/**
 * Receives process-global Maplibre Native log records.
 *
 * Native code may invoke this callback on logging or worker threads. The callback should return
 * quickly and avoid calling Maplibre APIs. The binding copies each record before invoking Kotlin
 * code and contains callback exceptions so they do not unwind into native code.
 *
 * Whether a record also reaches MapLibre's platform logger is fixed when the callback is
 * registered, not decided per record, because a host that cannot answer on the producing thread has
 * no way to decide it there.
 */
public fun interface LogCallback {
  /** Receives one log record. */
  public fun log(record: LogRecord)
}
