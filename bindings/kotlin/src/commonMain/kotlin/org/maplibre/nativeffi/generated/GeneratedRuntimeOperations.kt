// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedRuntimeOperations internal constructor() {
  internal abstract val binding: HandleStateCore
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  public fun mapCreate(options: MapOptions): Deferred<MapHandle> =
    nativeSubmitOwned(
      this,
      binding,
      "mln_map_create",
      { MapHandle(it, this@GeneratedRuntimeOperations as RuntimeHandle) },
      GeneratedOwnerDisposal::map,
      { it.dispose() },
    ) {
      check(C.mln_map_create(handle, writeMapOptions(options), completion, diagnostic))
    }

  public fun barrier(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_barrier") {
      check(C.mln_runtime_barrier(handle, completion, diagnostic))
    }

  public fun clearHttpHeaderTransform(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_clear_http_header_transform") {
      check(C.mln_runtime_clear_http_header_transform(handle, completion, diagnostic))
    }

  public fun clearResourceProvider(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_clear_resource_provider") {
      check(C.mln_runtime_clear_resource_provider(handle, completion, diagnostic))
    }

  public fun clearResourceTransform(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_clear_resource_transform") {
      check(C.mln_runtime_clear_resource_transform(handle, completion, diagnostic))
    }

  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_runtime_dispose") {
      check(C.mln_runtime_dispose(handle, diagnostic))
    }

  public fun drainEvents(): EventBatchHandle =
    nativeCall(this, binding, "mln_runtime_drain_events") {
      val out = allocate(8)
      check(C.mln_runtime_drain_events(handle, out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::eventBatch) { EventBatchHandle(it) }
    }

  public fun getEventMask(): RuntimeEventMask =
    nativeCall(this, binding, "mln_runtime_get_event_mask") {
      val out = allocate(8)
      check(C.mln_runtime_get_event_mask(handle, out, diagnostic))
      RuntimeEventMask(readU64(out))
    }

  public fun offlineRegionCreate(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_region_create",
      { result -> readOfflineRegionInfo(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_runtime_offline_region_create(
          handle,
          writeOfflineRegionDefinition(definition),
          bytes(metadata),
          metadata.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  public fun offlineRegionDelete(regionId: Long): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_offline_region_delete") {
      check(C.mln_runtime_offline_region_delete(handle, regionId, completion, diagnostic))
    }

  public fun offlineRegionGet(regionId: Long): Deferred<OfflineRegionInfo?> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_region_get",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readOfflineRegionInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_runtime_offline_region_get(handle, regionId, completion, diagnostic))
    }

  public fun offlineRegionGetStatus(regionId: Long): Deferred<OfflineRegionStatus> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_region_get_status",
      { result -> readOfflineRegionStatus(CompletionBridge.value(result)) },
    ) {
      check(C.mln_runtime_offline_region_get_status(handle, regionId, completion, diagnostic))
    }

  public fun offlineRegionInvalidate(regionId: Long): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_offline_region_invalidate") {
      check(C.mln_runtime_offline_region_invalidate(handle, regionId, completion, diagnostic))
    }

  public fun offlineRegionSetDownloadState(
    regionId: Long,
    state: OfflineRegionDownloadState,
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_offline_region_set_download_state") {
      check(
        C.mln_runtime_offline_region_set_download_state(
          handle,
          regionId,
          state.rawValue.toInt(),
          completion,
          diagnostic,
        )
      )
    }

  public fun offlineRegionSetObserved(regionId: Long, observed: Boolean): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_offline_region_set_observed") {
      check(
        C.mln_runtime_offline_region_set_observed(
          handle,
          regionId,
          observed,
          completion,
          diagnostic,
        )
      )
    }

  public fun offlineRegionUpdateMetadata(
    regionId: Long,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_region_update_metadata",
      { result -> readOfflineRegionInfo(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_runtime_offline_region_update_metadata(
          handle,
          regionId,
          bytes(metadata),
          metadata.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  public fun offlineRegionsList(): Deferred<List<OfflineRegionInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_regions_list",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          w(96, 112).toLong(),
        ) {
          readOfflineRegionInfo(it)
        }
      },
    ) {
      check(C.mln_runtime_offline_regions_list(handle, completion, diagnostic))
    }

  public fun offlineRegionsMergeDatabase(
    sideDatabasePath: String
  ): Deferred<List<OfflineRegionInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_regions_merge_database",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          w(96, 112).toLong(),
        ) {
          readOfflineRegionInfo(it)
        }
      },
    ) {
      check(
        C.mln_runtime_offline_regions_merge_database(
          handle,
          cString(sideDatabasePath),
          completion,
          diagnostic,
        )
      )
    }

  public fun release(): Deferred<Unit> =
    nativeRetire(this, binding, "mln_runtime_release") {
      check(C.mln_runtime_release(handle, completion, diagnostic))
    }

  public fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_run_ambient_cache_operation") {
      check(
        C.mln_runtime_run_ambient_cache_operation(
          handle,
          operation.rawValue.toInt(),
          completion,
          diagnostic,
        )
      )
    }

  public fun setEventMask(mask: RuntimeEventMask): Unit =
    nativeCall(this, binding, "mln_runtime_set_event_mask") {
      check(C.mln_runtime_set_event_mask(handle, mask.rawValue.toLong(), diagnostic))
    }

  public fun setHttpHeaderTransform(transform: HttpHeaderTransform): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_set_http_header_transform", bindingCallbacks) {
      check(
        C.mln_runtime_set_http_header_transform(
          handle,
          writeHttpHeaderTransform(transform),
          completion,
          diagnostic,
        )
      )
    }

  public fun setMaximumAmbientCacheSize(size: ULong): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_set_maximum_ambient_cache_size") {
      check(
        C.mln_runtime_set_maximum_ambient_cache_size(handle, size.toLong(), completion, diagnostic)
      )
    }

  public fun setResourceProvider(provider: ResourceProvider): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_set_resource_provider", bindingCallbacks) {
      check(
        C.mln_runtime_set_resource_provider(
          handle,
          writeResourceProvider(provider),
          completion,
          diagnostic,
        )
      )
    }

  public fun setResourceTransform(transform: ResourceTransform): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_set_resource_transform", bindingCallbacks) {
      check(
        C.mln_runtime_set_resource_transform(
          handle,
          writeResourceTransform(transform),
          completion,
          diagnostic,
        )
      )
    }
}
