package org.maplibre.nativeffi.runtime

/**
 * Releases a runtime owner thread parked in [RuntimeHandle.pump].
 *
 * A wake source is usable from any thread, which a host's task submission and shutdown paths rely
 * on. It stays usable after its runtime closes, and signalling it then does nothing.
 */
public expect class WakeSource : AutoCloseable {
  public val isClosed: Boolean

  /**
   * Latches a wake and releases the parked owner thread.
   *
   * A signal raised while the owner thread runs stays latched, so the next [RuntimeHandle.pump]
   * consumes it and returns without parking. Signalling after the runtime closes succeeds and does
   * nothing.
   */
  public fun signal()

  override fun close()
}
