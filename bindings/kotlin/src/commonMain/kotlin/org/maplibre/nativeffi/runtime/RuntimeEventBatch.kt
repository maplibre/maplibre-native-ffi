package org.maplibre.nativeffi.runtime

/**
 * Events that one [RuntimeHandle.drainEvents] call returned, in queue order.
 *
 * Every value in the batch is copied out of runtime-owned storage before the drain returns, so a
 * batch and the events in it stay readable after later drains and after the source map or runtime
 * is closed.
 *
 * @property events the drained events in queue order.
 * @property remainingCount events still queued after this batch, as an unsigned 64-bit count. A
 *   nonzero value means another drain reports more events, and an unbounded drain always reports 0.
 */
public data class RuntimeEventBatch(
  public val events: List<RuntimeEvent>,
  public val remainingCount: Long,
)
