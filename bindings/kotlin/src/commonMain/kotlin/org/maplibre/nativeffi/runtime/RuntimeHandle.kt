package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned runtime handle. Platform actuals own the native runtime carrier. */
public expect class RuntimeHandle : AutoCloseable {
  public val isClosed: Boolean

  /**
   * Runs one iteration of the owner-thread run loop.
   *
   * An iteration drains every queued task, including tasks those tasks enqueue, plus expired timers
   * and ready I/O. It is one loop iteration rather than one task.
   *
   * This call does not block waiting for work, but its duration is unbounded: a single iteration
   * can span a full style parse. Callers driving a frame loop should measure it rather than budget
   * it as a fixed per-frame slice.
   */
  public fun runOnce()

  public fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit>

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

  /**
   * Removes and returns one queued runtime event, or null when the queue is empty.
   *
   * Polling also advances binding-owned state, so it is more than a read. On a
   * [RuntimeEventType.MAP_STYLE_LOADED] event it releases the custom geometry sources that the
   * newly loaded style dropped, closing their upcall stubs. That release happens only when the
   * event is polled, so a host that stops polling keeps those stubs alive.
   *
   * Returned values are copies and stay valid across later polls.
   */
  public fun pollEvent(): RuntimeEvent?

  override fun close()

  public companion object {
    public fun create(options: RuntimeOptions): RuntimeHandle
  }
}
