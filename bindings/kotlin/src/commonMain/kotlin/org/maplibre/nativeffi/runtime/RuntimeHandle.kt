package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned runtime handle. Platform actuals own the native runtime carrier. */
public expect class RuntimeHandle : AutoCloseable {
  public val isClosed: Boolean

  /**
   * Advances this runtime: parks the owner thread when [timeoutMillis] allows it, then drains the
   * owner-thread task queues. Drain the queued runtime events with [drainEvents] afterwards.
   *
   * [timeoutMillis] bounds the park. Zero drains and returns, a positive value parks for up to that
   * many milliseconds, and a negative value parks until a wake arrives. A parking call returns as
   * soon as the runtime's wake flag is set, and returns without parking while unread runtime events
   * are queued. Timers and ready I/O set the flag only when they queue owner-thread work, so pass a
   * bounded timeout to cap how long a call waits.
   *
   * [budgetMillis] bounds the drain. A negative value drains without a bound, and zero or a
   * positive value stops the drain at the first task boundary after that many milliseconds. The
   * first queued task always runs, so a bounded pump always makes progress, and tasks left behind
   * set the wake flag so the next pump returns without parking and continues them.
   *
   * A non-zero timeout blocks the calling thread and ignores interruption. Call it outside any lock
   * that a thread signalling a [WakeSource] takes.
   */
  public fun pump(timeoutMillis: Long, budgetMillis: Long = -1)

  /**
   * Acquires a [WakeSource] that releases this runtime's parked owner thread. The returned source
   * is usable from any thread, and the caller closes it.
   */
  public fun acquireWakeSource(): WakeSource

  public fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit>

  /**
   * Starts a change to this runtime's maximum ambient cache size. Lowering it evicts cached ambient
   * resources; offline regions are unaffected.
   */
  public fun startSetMaximumAmbientCacheSize(size: Long): OfflineOperationHandle<Unit>

  public fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo>

  public fun startOfflineRegion(id: Long): OfflineOperationHandle<OfflineRegionInfo?>

  public fun startOfflineRegions(): OfflineOperationHandle<List<OfflineRegionInfo>>

  public fun startMergeOfflineRegionsDatabase(
    path: String
  ): OfflineOperationHandle<List<OfflineRegionInfo>>

  public fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo>

  public fun startOfflineRegionStatus(id: Long): OfflineOperationHandle<OfflineRegionStatus>

  public fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OfflineOperationHandle<Unit>

  public fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OfflineOperationHandle<Unit>

  public fun startInvalidateOfflineRegion(id: Long): OfflineOperationHandle<Unit>

  public fun startDeleteOfflineRegion(id: Long): OfflineOperationHandle<Unit>

  public fun takeCreateOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo

  public fun takeOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo?>
  ): OfflineRegionInfo?

  public fun takeOfflineRegionsResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo>

  public fun takeMergeOfflineRegionsDatabaseResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo>

  public fun takeUpdateOfflineRegionMetadataResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo

  public fun takeOfflineRegionStatusResult(
    operation: OfflineOperationHandle<OfflineRegionStatus>
  ): OfflineRegionStatus

  public fun setResourceProvider(callback: ResourceProviderCallback)

  public fun clearResourceProvider()

  public fun setResourceTransform(callback: ResourceTransformCallback)

  public fun clearResourceTransform()

  public fun setHttpHeaderTransform(callback: HttpHeaderTransformCallback)

  public fun clearHttpHeaderTransform()

  /**
   * Drains this runtime's queued events into one batch, in queue order.
   *
   * [maxEvents] bounds the drain. Zero drains every queued event; a positive value drains at most
   * that many events and reports the rest in [RuntimeEventBatch.remainingCount]. A negative value
   * throws [org.maplibre.nativeffi.error.InvalidArgumentException].
   *
   * The two subscription masks decide which events reach the queue. Draining is a queue operation
   * that runs no owner-thread work, so call [pump] first and drain what the pump produced.
   */
  public fun drainEvents(maxEvents: Int = 0): RuntimeEventBatch

  /**
   * Runtime-originated event types that this runtime queues, [RuntimeEventMask.ALL] until a host
   * narrows it.
   *
   * The setter reads the [RuntimeEventMask.ALL_RUNTIME_EVENTS] bits and ignores the rest, so a host
   * reads this mask, changes one bit, and writes it back. Narrowing gates later events and keeps
   * queued ones.
   */
  public var eventMask: RuntimeEventMask

  override fun close()

  public companion object {
    public fun create(options: RuntimeOptions): RuntimeHandle
  }
}
