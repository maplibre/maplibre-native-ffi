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

  /**
   * Starts an ordered runtime barrier.
   *
   * See `mln_runtime_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun barrier(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_barrier") {
      check(C.mln_runtime_barrier(handle, completion, diagnostic))
    }

  /**
   * Clears the runtime-scoped outgoing HTTP header transform.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearHttpHeaderTransform(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_clear_http_header_transform") {
      check(C.mln_runtime_clear_http_header_transform(handle, completion, diagnostic))
    }

  /**
   * Clears the runtime-scoped network resource provider.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearResourceProvider(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_clear_resource_provider") {
      check(C.mln_runtime_clear_resource_provider(handle, completion, diagnostic))
    }

  /**
   * Clears the runtime-scoped URL transform for network resources.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearResourceTransform(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_clear_resource_transform") {
      check(C.mln_runtime_clear_resource_transform(handle, completion, diagnostic))
    }

  /**
   * Creates a map on the runtime worker.
   *
   * See `mln_runtime_create_map` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun createMap(options: MapOptions): Deferred<MapHandle> =
    nativeSubmitOwned(
      this,
      binding,
      "mln_runtime_create_map",
      { MapHandle(it, this@GeneratedRuntimeOperations as RuntimeHandle) },
      GeneratedOwnerDisposal::map,
      { it.dispose() },
    ) {
      check(C.mln_runtime_create_map(handle, writeMapOptions(options), completion, diagnostic))
    }

  /**
   * Starts creating an offline region.
   *
   * See `mln_runtime_create_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_create_offline_region",
      { result -> readOfflineRegionInfo(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_runtime_create_offline_region(
          handle,
          writeOfflineRegionDefinition(definition),
          bytes(metadata),
          metadata.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Deletes an offline region.
   *
   * See `mln_runtime_delete_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun deleteOfflineRegion(regionId: Long): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_delete_offline_region") {
      check(C.mln_runtime_delete_offline_region(handle, regionId, completion, diagnostic))
    }

  /**
   * Consumes a runtime handle without observing its asynchronous retirement.
   *
   * See `mln_runtime_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_runtime_dispose") {
      check(C.mln_runtime_dispose(handle, diagnostic))
    }

  /**
   * Drains this runtime's queued events into a new owned batch.
   *
   * See `mln_runtime_drain_events` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun drainEvents(): EventBatchHandle =
    nativeCall(this, binding, "mln_runtime_drain_events") {
      val out = allocate(8, 8)
      check(C.mln_runtime_drain_events(handle, out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::eventBatch) { EventBatchHandle(it) }
    }

  /**
   * Reports which runtime-scoped event types this runtime queues.
   *
   * See `mln_runtime_get_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun getEventMask(): RuntimeEventMask =
    nativeCall(this, binding, "mln_runtime_get_event_mask") {
      val out = allocate(8, 8)
      check(C.mln_runtime_get_event_mask(handle, out, diagnostic))
      RuntimeEventMask(readU64(out))
    }

  /**
   * Starts getting one offline region by ID.
   *
   * See `mln_runtime_get_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getOfflineRegion(regionId: Long): Deferred<OfflineRegionInfo?> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_get_offline_region",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readOfflineRegionInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_runtime_get_offline_region(handle, regionId, completion, diagnostic))
    }

  /**
   * Starts getting the current download status for an offline region.
   *
   * See `mln_runtime_get_offline_region_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getOfflineRegionStatus(regionId: Long): Deferred<OfflineRegionStatus> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_get_offline_region_status",
      { result -> readOfflineRegionStatus(CompletionBridge.value(result)) },
    ) {
      check(C.mln_runtime_get_offline_region_status(handle, regionId, completion, diagnostic))
    }

  /**
   * Invalidates cached resources for an offline region.
   *
   * See `mln_runtime_invalidate_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun invalidateOfflineRegion(regionId: Long): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_invalidate_offline_region") {
      check(C.mln_runtime_invalidate_offline_region(handle, regionId, completion, diagnostic))
    }

  /**
   * Starts listing the offline regions in the runtime database.
   *
   * See `mln_runtime_list_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun listOfflineRegions(): Deferred<List<OfflineRegionInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_list_offline_regions",
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
      check(C.mln_runtime_list_offline_regions(handle, completion, diagnostic))
    }

  /**
   * Starts merging offline regions from another MapLibre offline database.
   *
   * See `mln_runtime_merge_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun mergeOfflineRegions(sideDatabasePath: String): Deferred<List<OfflineRegionInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_merge_offline_regions",
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
        C.mln_runtime_merge_offline_regions(
          handle,
          cString(sideDatabasePath),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Releases a runtime after synchronous child preflight.
   *
   * See `mln_runtime_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun release(): Deferred<Unit> =
    nativeRetire(this, binding, "mln_runtime_release") {
      check(C.mln_runtime_release(handle, completion, diagnostic))
    }

  /**
   * Starts a MapLibre ambient cache maintenance operation for this runtime.
   *
   * See `mln_runtime_run_ambient_cache_operation` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
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

  /**
   * Selects which runtime-scoped event types this runtime queues.
   *
   * See `mln_runtime_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setEventMask(mask: RuntimeEventMask): Unit =
    nativeCall(this, binding, "mln_runtime_set_event_mask") {
      check(C.mln_runtime_set_event_mask(handle, mask.rawValue.toLong(), diagnostic))
    }

  /**
   * Registers or replaces the runtime-scoped outgoing HTTP header transform.
   *
   * See `mln_runtime_set_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
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

  /**
   * Starts a change to this runtime's maximum ambient cache size.
   *
   * See `mln_runtime_set_maximum_ambient_cache_size` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setMaximumAmbientCacheSize(size: ULong): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_set_maximum_ambient_cache_size") {
      check(
        C.mln_runtime_set_maximum_ambient_cache_size(handle, size.toLong(), completion, diagnostic)
      )
    }

  /**
   * Sets an offline region's native download state.
   *
   * See `mln_runtime_set_offline_region_download_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setOfflineRegionDownloadState(
    regionId: Long,
    state: OfflineRegionDownloadState,
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_set_offline_region_download_state") {
      check(
        C.mln_runtime_set_offline_region_download_state(
          handle,
          regionId,
          state.rawValue.toInt(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Enables or disables runtime events for an offline region.
   *
   * See `mln_runtime_set_offline_region_observed` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setOfflineRegionObserved(regionId: Long, observed: Boolean): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_set_offline_region_observed") {
      check(
        C.mln_runtime_set_offline_region_observed(
          handle,
          regionId,
          observed,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Registers or replaces a runtime-scoped network resource provider.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
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

  /**
   * Registers or updates a runtime-scoped URL transform for network resources.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
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

  /**
   * Starts updating opaque binary metadata for an offline region.
   *
   * See `mln_runtime_update_offline_region_metadata` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun updateOfflineRegionMetadata(
    regionId: Long,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_update_offline_region_metadata",
      { result -> readOfflineRegionInfo(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_runtime_update_offline_region_metadata(
          handle,
          regionId,
          bytes(metadata),
          metadata.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }
}
