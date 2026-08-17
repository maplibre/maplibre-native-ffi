package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned runtime handle. Platform actuals own the native runtime carrier. */
public expect class RuntimeHandle {
  public val isClosed: Boolean

  /**
   * Installs the receiver callback for this runtime's notification source. Native threads call the
   * callback only to schedule [drainReady] on the receiver.
   */
  public fun setNotificationCallback(callback: () -> Unit)

  /** Clears the receiver callback after every callback invocation has returned. */
  public fun clearNotificationCallback()

  /** Drains one owned copy of the endpoints that are currently ready. */
  public fun drainReady(): List<ReadyEndpoint>

  /** Suspends until every command accepted before this call has reached a terminal disposition. */
  public suspend fun barrier()

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

  public fun setResourceProvider(callback: ResourceProviderCallback): ULong

  public fun clearResourceProvider(): ULong

  public fun setResourceTransform(callback: ResourceTransformCallback): ULong

  public fun clearResourceTransform(): ULong

  public fun setHttpHeaderTransform(callback: HttpHeaderTransformCallback): ULong

  public fun clearHttpHeaderTransform(): ULong

  /**
   * Drains this runtime's queued events into one batch, in queue order.
   *
   * The two subscription masks decide which events reach the queue. Draining copies events that the
   * runtime worker has already published.
   */
  public fun drainEvents(): RuntimeEventBatch

  /**
   * Runtime-originated event types that this runtime queues, [RuntimeEventMask.ALL] until a host
   * narrows it.
   *
   * The setter reads the [RuntimeEventMask.ALL_RUNTIME_EVENTS] bits and ignores the rest, so a host
   * reads this mask, changes one bit, and writes it back. Narrowing gates later events and keeps
   * queued ones.
   */
  public var eventMask: RuntimeEventMask

  /**
   * Suspends until native runtime retirement completes, then releases callbacks and notification
   * resources.
   */
  public fun close()

  public companion object {
    /** Creates a runtime without blocking the caller's coroutine. */
    public suspend fun create(options: RuntimeOptions): RuntimeHandle
  }
}
