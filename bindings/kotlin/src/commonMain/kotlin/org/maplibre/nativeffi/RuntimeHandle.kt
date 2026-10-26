// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmInline
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A runtime: the native scheduler thread and event store for its maps.
 *
 * See `mln_runtime` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class RuntimeHandle
internal constructor(
  handle: Long,
) : AsyncReleasable, AutoCloseable {
  internal val binding = HandleState("RuntimeHandle", handle, GeneratedOwnerDisposal::runtime)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public override val isClosed: Boolean
    get() = binding.isReleased

  public override fun close() {
    dispose()
  }

  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Stops event delivery and retires the previous handler.
   *
   * See `mln_runtime_clear_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearEventHandler(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_event_handler(handle, completion, diagnostic))
    }

  /**
   * Clears the runtime-scoped outgoing HTTP header transform.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearHttpHeaderTransform(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_http_header_transform(handle, completion, diagnostic))
    }

  /**
   * Clears the runtime-scoped network resource provider.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearResourceProvider(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_resource_provider(handle, completion, diagnostic))
    }

  /**
   * Clears the runtime-scoped URL transform for network resources.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearResourceTransform(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_resource_transform(handle, completion, diagnostic))
    }

  /**
   * Starts creation of a map on the runtime.
   *
   * See `mln_runtime_create_map` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun createMap(options: MapOptions): NewMap =
    nativeCall(this, binding) {
      attach(
        { out, completion ->
          C.mln_runtime_create_map(handle, writeMapOptions(options), out, completion, diagnostic)
        },
        GeneratedOwnerDisposal::map,
        { MapHandle(it, this@RuntimeHandle) },
        null,
        { it.dispose() },
        ::NewMap,
      )
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
        ),
      )
    }

  /**
   * Deletes an offline region.
   *
   * See `mln_runtime_delete_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun deleteOfflineRegion(regionId: Long): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_runtime_delete_offline_region(handle, regionId, completion, diagnostic))
    }

  /**
   * Consumes a runtime handle without observing its asynchronous retirement.
   *
   * See `mln_runtime_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding) { check(C.mln_runtime_dispose(handle, diagnostic)) }

  /**
   * Reports which runtime-scoped event types this runtime queues.
   *
   * See `mln_runtime_get_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun getEventMask(): RuntimeEventFlag =
    nativeCall(this, binding) {
      val out = allocate(8, 8)
      check(C.mln_runtime_get_event_mask(handle, out, diagnostic))
      RuntimeEventFlag(readU64(out))
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
    nativeUnit(this, binding) {
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
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(88, 104),
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
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(88, 104),
        ) {
          readOfflineRegionInfo(it)
        }
      },
    ) {
      check(
        C.mln_runtime_merge_offline_regions(handle, view(sideDatabasePath), completion, diagnostic),
      )
    }

  /**
   * Starts release of a runtime after checking its children.
   *
   * See `mln_runtime_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public override fun release(): Deferred<Unit> =
    nativeRetire(this, binding) { check(C.mln_runtime_release(handle, completion, diagnostic)) }

  /**
   * Starts a MapLibre ambient cache maintenance operation for this runtime.
   *
   * See `mln_runtime_run_ambient_cache_operation` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_runtime_run_ambient_cache_operation(
          handle,
          operation.rawValue.toInt(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Selects how runtime and map events reach the host. Map events arrive on the runtime's native
   * scheduler thread, and offline events may arrive on a MapLibre worker thread.
   *
   * See `mln_runtime_set_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setEventHandler(handler: RuntimeEventHandler): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_runtime_set_event_handler(
          handle,
          writeRuntimeEventHandler(handler),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Selects which runtime-scoped event types this runtime queues.
   *
   * See `mln_runtime_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setEventMask(mask: RuntimeEventFlag): Unit =
    nativeCall(this, binding) {
      check(C.mln_runtime_set_event_mask(handle, mask.rawValue.toLong(), diagnostic))
    }

  /**
   * Starts a change to this runtime's maximum ambient cache size.
   *
   * See `mln_runtime_set_maximum_ambient_cache_size` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setMaximumAmbientCacheSize(size: ULong): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_runtime_set_maximum_ambient_cache_size(handle, size.toLong(), completion, diagnostic),
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
    nativeUnit(this, binding) {
      check(
        C.mln_runtime_set_offline_region_download_state(
          handle,
          regionId,
          state.rawValue.toInt(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Enables or disables runtime events for an offline region.
   *
   * See `mln_runtime_set_offline_region_observed` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setOfflineRegionObserved(regionId: Long, observed: Boolean): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_runtime_set_offline_region_observed(
          handle,
          regionId,
          observed,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Registers or replaces a runtime-scoped network resource provider. The callback runs
   * synchronously on a MapLibre thread.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setResourceProvider(provider: ResourceProvider): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_runtime_set_resource_provider(
          handle,
          writeResourceProvider(provider),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Registers or updates a runtime-scoped URL transform for network resources. The callback runs on
   * a MapLibre network thread.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setResourceTransform(transform: ResourceTransform): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_runtime_set_resource_transform(
          handle,
          writeResourceTransform(transform),
          completion,
          diagnostic,
        ),
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
        ),
      )
    }

  /**
   * This handle's commands without their completions. A discarding call returns once native accepts
   * the command, and native logs a command that fails as a warning.
   */
  public val discarding: RuntimeDiscarding
    get() = RuntimeDiscarding(this)

  override fun toString(): String =
    "RuntimeHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Creates a runtime.
     *
     * Suspends until the call's thread starts. On the JVM, Android, and Kotlin/Native the call
     * waits on the calling thread, which it blocks until then. On Kotlin/Wasm it runs on a thread
     * of its own, so the page can call it. Cancellation does not interrupt the wait: a caller
     * cancelled meanwhile receives the cancellation, and the binding disposes the result.
     *
     * See `mln_runtime_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
     */
    public suspend fun create(options: RuntimeOptions): RuntimeHandle =
      nativeThreadStart({ it.dispose() }) {
        val out = allocate(8, 8)
        threadStart { diagnostic ->
          C.mln_runtime_create(writeRuntimeOptions(options), out, diagnostic)
        }
        adopt(out, GeneratedOwnerDisposal::runtime) { RuntimeHandle(it) }
      }
  }
}

/** The commands of a [RuntimeHandle] that drop their completions. */
@JvmInline
public value class RuntimeDiscarding internal constructor(internal val owner: RuntimeHandle) {
  internal val binding
    get() = owner.binding

  internal val bindingCallbacks
    get() = owner.bindingCallbacks

  /** [RuntimeHandle.clearEventHandler] without its completion. */
  public fun clearEventHandler(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_runtime_clear_event_handler(handle, completion, diagnostic))
    }

  /** [RuntimeHandle.clearHttpHeaderTransform] without its completion. */
  public fun clearHttpHeaderTransform(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_runtime_clear_http_header_transform(handle, completion, diagnostic))
    }

  /** [RuntimeHandle.clearResourceProvider] without its completion. */
  public fun clearResourceProvider(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_runtime_clear_resource_provider(handle, completion, diagnostic))
    }

  /** [RuntimeHandle.clearResourceTransform] without its completion. */
  public fun clearResourceTransform(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_runtime_clear_resource_transform(handle, completion, diagnostic))
    }

  /** [RuntimeHandle.setEventHandler] without its completion. */
  public fun setEventHandler(handler: RuntimeEventHandler): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_runtime_set_event_handler(
          handle,
          writeRuntimeEventHandler(handler),
          completion,
          diagnostic,
        ),
      )
    }

  /** [RuntimeHandle.setResourceProvider] without its completion. */
  public fun setResourceProvider(provider: ResourceProvider): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_runtime_set_resource_provider(
          handle,
          writeResourceProvider(provider),
          completion,
          diagnostic,
        ),
      )
    }

  /** [RuntimeHandle.setResourceTransform] without its completion. */
  public fun setResourceTransform(transform: ResourceTransform): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_runtime_set_resource_transform(
          handle,
          writeResourceTransform(transform),
          completion,
          diagnostic,
        ),
      )
    }
}
