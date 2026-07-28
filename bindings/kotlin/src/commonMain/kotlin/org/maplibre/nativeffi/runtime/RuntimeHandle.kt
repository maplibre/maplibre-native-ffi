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
   * Advances this runtime.
   *
   * The call parks the owner thread when [timeoutMillis] allows it, then drains the owner-thread
   * task queues. Drain the queued runtime events with [pollEvent] afterwards.
   *
   * [timeoutMillis] sets the park bound. Zero drains and returns; hosts pumping from a frame
   * callback pass it. A positive value parks for up to that many milliseconds; hosts that own their
   * pump thread pass one and take their cadence from the runtime's own work. A negative value parks
   * until a wake arrives.
   *
   * The drain runs every task queued when it begins plus every task those enqueue, so a single call
   * can span a full style parse.
   *
   * A wake is a latch. One pump consumes one latch, and work arriving during the drain latches the
   * next wake, so a pump that finds no new work is ordinary. Style, tile, offline, and resource
   * responses latch a wake, as does [WakeSource.signal]. A queued unread runtime event also returns
   * the call without parking. Timers and ready I/O latch a wake when they queue owner-thread work,
   * so pass a bounded timeout to cap park latency.
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
