// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*

public expect abstract class GeneratedRuntimeOperations internal constructor() {
  internal val bindingCallbacks: org.maplibre.nativeffi.internal.callback.CallbackOwner

  public fun mapCreate(options: MapOptions): Deferred<org.maplibre.nativeffi.generated.MapHandle>

  public fun barrier(): Deferred<Unit>

  public fun clearHttpHeaderTransform(): Deferred<Unit>

  public fun clearResourceProvider(): Deferred<Unit>

  public fun clearResourceTransform(): Deferred<Unit>

  public fun dispose(): Unit

  public fun drainEvents(): org.maplibre.nativeffi.generated.EventBatchHandle

  public fun getEventMask(): RuntimeEventMask

  public fun offlineRegionCreate(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo>

  public fun offlineRegionDelete(regionId: Long): Deferred<Unit>

  public fun offlineRegionGet(regionId: Long): Deferred<OfflineRegionInfo?>

  public fun offlineRegionGetStatus(regionId: Long): Deferred<OfflineRegionStatus>

  public fun offlineRegionInvalidate(regionId: Long): Deferred<Unit>

  public fun offlineRegionSetDownloadState(
    regionId: Long,
    state: OfflineRegionDownloadState,
  ): Deferred<Unit>

  public fun offlineRegionSetObserved(regionId: Long, observed: Boolean): Deferred<Unit>

  public fun offlineRegionUpdateMetadata(
    regionId: Long,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo>

  public fun offlineRegionsList(): Deferred<List<OfflineRegionInfo>>

  public fun offlineRegionsMergeDatabase(
    sideDatabasePath: String
  ): Deferred<List<OfflineRegionInfo>>

  public fun release(): Deferred<Unit>

  public fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit>

  public fun setEventMask(mask: RuntimeEventMask): Unit

  public fun setHttpHeaderTransform(transform: HttpHeaderTransform): Deferred<Unit>

  public fun setMaximumAmbientCacheSize(size: ULong): Deferred<Unit>

  public fun setResourceProvider(provider: ResourceProvider): Deferred<Unit>

  public fun setResourceTransform(transform: ResourceTransform): Deferred<Unit>
}
