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
   * A non-zero timeout blocks the calling thread and ignores interruption. Call it outside any lock
   * that a thread signalling a [WakeSource] takes.
   */
  public fun pump(timeoutMillis: Long)

  /**
   * Acquires a [WakeSource] that releases this runtime's parked owner thread. The returned source
   * is usable from any thread, and the caller closes it.
   */
  public fun acquireWakeSource(): WakeSource

  public fun startAmbientCacheOperation(operation: AmbientCacheOperation): OperationHandle<Unit>

  /**
   * Starts a change to this runtime's maximum ambient cache size. Lowering it evicts cached ambient
   * resources; offline regions are unaffected.
   */
  public fun startSetMaximumAmbientCacheSize(size: Long): OperationHandle<Unit>

  public fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OperationHandle<OfflineRegionInfo>

  public fun startOfflineRegion(id: Long): OperationHandle<OfflineRegionInfo?>

  public fun startOfflineRegions(): OperationHandle<List<OfflineRegionInfo>>

  public fun startMergeOfflineRegionsDatabase(
    path: String
  ): OperationHandle<List<OfflineRegionInfo>>

  public fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OperationHandle<OfflineRegionInfo>

  public fun startOfflineRegionStatus(id: Long): OperationHandle<OfflineRegionStatus>

  public fun startSetOfflineRegionObserved(id: Long, observed: Boolean): OperationHandle<Unit>

  public fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OperationHandle<Unit>

  public fun startInvalidateOfflineRegion(id: Long): OperationHandle<Unit>

  public fun startDeleteOfflineRegion(id: Long): OperationHandle<Unit>

  public fun takeCreateOfflineRegionResult(
    operation: OperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo

  public fun takeOfflineRegionResult(
    operation: OperationHandle<OfflineRegionInfo?>
  ): OfflineRegionInfo?

  public fun takeOfflineRegionsResult(
    operation: OperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo>

  public fun takeMergeOfflineRegionsDatabaseResult(
    operation: OperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo>

  public fun takeUpdateOfflineRegionMetadataResult(
    operation: OperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo

  public fun takeOfflineRegionStatusResult(
    operation: OperationHandle<OfflineRegionStatus>
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
