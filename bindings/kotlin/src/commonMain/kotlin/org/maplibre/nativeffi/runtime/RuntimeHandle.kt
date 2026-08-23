package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.Deferred
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

  /** Completes after every command accepted before this call reaches a terminal disposition. */
  public fun barrier(): Deferred<Unit>

  public fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit>

  /**
   * Starts a change to this runtime's maximum ambient cache size. Lowering it evicts cached ambient
   * resources; offline regions are unaffected.
   */
  public fun setMaximumAmbientCacheSize(size: Long): Deferred<Unit>

  public fun createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo>

  public fun offlineRegion(id: Long): Deferred<OfflineRegionInfo?>

  public fun offlineRegions(): Deferred<List<OfflineRegionInfo>>

  public fun mergeOfflineRegionsDatabase(path: String): Deferred<List<OfflineRegionInfo>>

  public fun updateOfflineRegionMetadata(id: Long, metadata: ByteArray): Deferred<OfflineRegionInfo>

  public fun offlineRegionStatus(id: Long): Deferred<OfflineRegionStatus>

  public fun setOfflineRegionObserved(id: Long, observed: Boolean): Deferred<Unit>

  public fun setOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): Deferred<Unit>

  public fun invalidateOfflineRegion(id: Long): Deferred<Unit>

  public fun deleteOfflineRegion(id: Long): Deferred<Unit>

  public fun setResourceProvider(callback: ResourceProviderCallback): Deferred<CommandCompletion>

  public fun clearResourceProvider(): Deferred<CommandCompletion>

  public fun setResourceTransform(callback: ResourceTransformCallback): Deferred<CommandCompletion>

  public fun clearResourceTransform(): Deferred<CommandCompletion>

  public fun setHttpHeaderTransform(
    callback: HttpHeaderTransformCallback
  ): Deferred<CommandCompletion>

  public fun clearHttpHeaderTransform(): Deferred<CommandCompletion>

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
   * Releases this runtime and its callback state, and reports the end of native teardown.
   *
   * The call consumes the handle before it returns, and it throws when this runtime still has a
   * live or pending child. The returned [Deferred] completes after every earlier accepted
   * submission, including released maps' teardown, has finished and the runtime's threads and
   * resources are gone, so a host that awaits it may exit the process without racing native
   * teardown. Closing an already closed runtime returns that same [Deferred].
   */
  public fun close(): Deferred<Unit>

  public companion object {
    /** Creates a runtime. */
    public fun create(options: RuntimeOptions): RuntimeHandle
  }
}
