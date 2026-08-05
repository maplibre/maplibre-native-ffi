package org.maplibre.nativeffi.runtime

/**
 * Releases a runtime owner thread parked in [RuntimeHandle.pump]. A wake source is usable from any
 * thread and stays usable after its runtime closes.
 */
public expect class WakeSource : AutoCloseable {
  public val isClosed: Boolean

  /**
   * Sets the runtime's wake flag and releases the parked owner thread. A signal raised while the
   * owner thread runs makes the next [RuntimeHandle.pump] return without parking. Signalling after
   * the runtime closes succeeds and does nothing.
   */
  public fun signal()

  override fun close()
}
