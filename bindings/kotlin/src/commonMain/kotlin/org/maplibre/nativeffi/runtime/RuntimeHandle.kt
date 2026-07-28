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
   * Advances this runtime, optionally parking the owner thread first.
   *
   * One call is the whole pump step: park if asked, then drain the owner-thread task queues. Follow
   * it with [pollEvent] until the queue is empty, then render whatever the drained events asked
   * for.
   *
   * [timeoutMillis] selects where the loop's cadence comes from. Zero never blocks, which is what a
   * host driven by a frame callback it does not own passes. A positive value parks for up to that
   * long, which is how a host that owns its pump thread takes its cadence from the runtime's own
   * work. A negative value parks until a wake arrives.
   *
   * Draining is drain, not slice: a single call can span a whole style parse, so measure it rather
   * than budgeting it as a per-frame slice.
   *
   * A wake is a latch, not a work predicate. A return does not promise work arrived, and a pump
   * that finds nothing is expected. Style, tile, offline, and resource responses latch a wake, as
   * does [WakeSource.signal]; an unread runtime event also prevents parking. Timers and ready I/O
   * that queue no owner-thread work do not, so pass a timeout to bound latency regardless.
   *
   * A non-zero timeout blocks the calling thread without releasing it to an interruption. Do not
   * use one while holding a lock that a thread signalling a [WakeSource] also takes.
   */
  public fun pump(timeoutMillis: Long)

  /**
   * Acquires a [WakeSource] that releases this runtime's parked owner thread from any thread. The
   * caller closes the returned source.
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
