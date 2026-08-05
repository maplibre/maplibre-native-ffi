package org.maplibre.nativeffi.log

/**
 * Receives process-global Maplibre Native log records.
 *
 * Native code may invoke this callback on logging or worker threads. The callback should return
 * quickly and avoid calling Maplibre APIs. The binding copies each record before invoking Kotlin
 * code and contains callback exceptions so they do not unwind into native code.
 */
public fun interface LogCallback {
  /**
   * Returns true when the callback consumed the record, false to let native logging handle it.
   *
   * The browser binding ignores this result and always reports "not consumed" to native, so a
   * record a callback sees also reaches MapLibre's platform logger. MapLibre needs the decision on
   * the thread that produced the record, and that thread cannot enter the page's WebAssembly
   * instance where a host's callback body lives, so the browser delivers records asynchronously and
   * answers for them in advance. A browser host that wants one sink filters at its own sink or
   * narrows the severity mask.
   */
  public fun log(record: LogRecord): Boolean
}
