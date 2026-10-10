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
   * Creates a map on the runtime worker.
   *
   * See `mln_map_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
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
  public fun drainEvents(): EventBatchHandle? =
    nativeCall(this, binding, "mln_runtime_drain_events") {
      val out = allocate(8, 8)
      if (present(C.mln_runtime_drain_events(handle, out, diagnostic), absent = -9))
        adopt(out, GeneratedOwnerDisposal::eventBatch) { EventBatchHandle(it) }
      else null
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
   * Starts creating an offline region.
   *
   * See `mln_runtime_offline_region_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
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

  /**
   * Deletes an offline region.
   *
   * See `mln_runtime_offline_region_delete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun offlineRegionDelete(regionId: Long): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_offline_region_delete") {
      check(C.mln_runtime_offline_region_delete(handle, regionId, completion, diagnostic))
    }

  /**
   * Starts getting one offline region by ID.
   *
   * See `mln_runtime_offline_region_get` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
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

  /**
   * Starts getting the current download status for an offline region.
   *
   * See `mln_runtime_offline_region_get_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun offlineRegionGetStatus(regionId: Long): Deferred<OfflineRegionStatus> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_region_get_status",
      { result -> readOfflineRegionStatus(CompletionBridge.value(result)) },
    ) {
      check(C.mln_runtime_offline_region_get_status(handle, regionId, completion, diagnostic))
    }

  /**
   * Invalidates cached resources for an offline region.
   *
   * See `mln_runtime_offline_region_invalidate` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun offlineRegionInvalidate(regionId: Long): Deferred<Unit> =
    nativeUnit(this, binding, "mln_runtime_offline_region_invalidate") {
      check(C.mln_runtime_offline_region_invalidate(handle, regionId, completion, diagnostic))
    }

  /**
   * Sets an offline region's native download state.
   *
   * See `mln_runtime_offline_region_set_download_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
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

  /**
   * Enables or disables runtime events for an offline region.
   *
   * See `mln_runtime_offline_region_set_observed` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
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

  /**
   * Starts updating opaque binary metadata for an offline region.
   *
   * See `mln_runtime_offline_region_update_metadata` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
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

  /**
   * Starts listing the offline regions in the runtime database.
   *
   * See `mln_runtime_offline_regions_list` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun offlineRegionsList(): Deferred<List<OfflineRegionInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_regions_list",
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(88, 96),
        ) {
          readOfflineRegionInfo(it)
        }
      },
    ) {
      check(C.mln_runtime_offline_regions_list(handle, completion, diagnostic))
    }

  /**
   * Starts merging offline regions from another MapLibre offline database.
   *
   * See `mln_runtime_offline_regions_merge_database` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun offlineRegionsMergeDatabase(
    sideDatabasePath: String
  ): Deferred<List<OfflineRegionInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_runtime_offline_regions_merge_database",
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(88, 96),
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
}
