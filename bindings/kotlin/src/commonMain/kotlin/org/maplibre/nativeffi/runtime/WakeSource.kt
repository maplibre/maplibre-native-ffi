package org.maplibre.nativeffi.runtime

/**
 * Releases a runtime owner thread parked in [RuntimeHandle.wait].
 *
 * Unlike the other handles here, a wake source is usable from any thread: it is what a host's task
 * submission or shutdown path calls. It stays usable after its runtime closes, and signalling it
 * then does nothing.
 */
public expect class WakeSource : AutoCloseable {
  public val isClosed: Boolean

  /**
   * Latches a wake and releases the parked owner thread.
   *
   * A signal raised while the owner thread runs is latched, so the next [RuntimeHandle.wait]
   * consumes it without blocking. Signalling after the runtime closes succeeds and does nothing.
   */
  public fun signal()

  override fun close()
}
