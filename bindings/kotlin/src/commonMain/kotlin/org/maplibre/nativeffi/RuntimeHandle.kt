// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import org.maplibre.nativeffi.internal.JvmSynthetic
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A runtime owns maps and the network and cache settings that they share.
 *
 * See `mln_runtime` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class RuntimeHandle
private constructor(
  handle: Long,
) : AsyncReleasable, AutoCloseable {
  @get:JvmSynthetic
  internal val binding = HandleState("RuntimeHandle", handle, GeneratedOwnerDisposal::runtime)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * release, so events drained later still match this handle.
   */
  @get:JvmName("getId")
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public override val isClosed: Boolean
    get() = binding.isReleased

  /**
   * Calls [dispose], for the standard library's `use`, which can't wait. [useReleasing] waits for
   * [release] instead.
   */
  public override fun close() {
    dispose()
  }

  @get:JvmSynthetic internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Stops sending the events of the runtime and its maps to a callback.
   *
   * See `mln_runtime_clear_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearEventHandler(): Completion<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_event_handler(handle, completion, diagnostic))
    }

  /**
   * Stops adding headers to the runtime's HTTP requests.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearHttpHeaderTransform(): Completion<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_http_header_transform(handle, completion, diagnostic))
    }

  /**
   * Removes the runtime's resource provider, so the library loads every request itself.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearResourceProvider(): Completion<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_resource_provider(handle, completion, diagnostic))
    }

  /**
   * Stops rewriting the URLs of the runtime's network requests.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun clearResourceTransform(): Completion<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_runtime_clear_resource_transform(handle, completion, diagnostic))
    }

  /**
   * Creates a map on `runtime`.
   *
   * See `mln_runtime_create_map` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun createMap(options: MapOptions): MapHandle =
    nativeCall(this, binding) {
      attach(
        { out, completion ->
          C.mln_runtime_create_map(handle, writeMapOptions(options), out, completion, diagnostic)
        },
        GeneratedOwnerDisposal::map,
        { created, ready -> MapHandle.wrap(created, this@RuntimeHandle, ready) },
        null,
        { it.dispose() },
      )
    }

  /**
   * Creates an offline region in the runtime's cache file.
   *
   * `metadata` holds any bytes to store with the region, such as its name. The region doesn't
   * download until `RuntimeHandle.setOfflineRegionDownloadState()` makes its download active.
   *
   * See `mln_runtime_create_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Completion<OfflineRegionInfo> =
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
   * Deletes the offline region `regionId` from the runtime's cache file.
   *
   * See `mln_runtime_delete_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the cache file has no region `regionId`.
   */
  public fun deleteOfflineRegion(regionId: Long): Completion<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_runtime_delete_offline_region(handle, regionId, completion, diagnostic))
    }

  /**
   * Releases a runtime in any state, without waiting.
   *
   * The runtime finishes its teardown after its maps are released.
   *
   * See `mln_runtime_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding) { check(C.mln_runtime_dispose(handle, diagnostic)) }

  /**
   * Reads the offline region `regionId`.
   *
   * The result is null when the cache file has no region with that ID.
   *
   * See `mln_runtime_get_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getOfflineRegion(regionId: Long): Completion<OfflineRegionInfo?> =
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
   * Reads the download progress of the offline region `regionId`.
   *
   * While the download is active, the counts cover what it has visited. Once it's inactive, they
   * cover every resource the region holds, including those that only earlier versions of its style
   * listed, so the completed counts can exceed the required counts.
   *
   * See `mln_runtime_get_offline_region_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the cache file has no region `regionId`.
   */
  public fun getOfflineRegionStatus(regionId: Long): Completion<OfflineRegionStatus> =
    nativeSubmit(
      this,
      binding,
      { result -> readOfflineRegionStatus(CompletionBridge.value(result)) },
    ) {
      check(C.mln_runtime_get_offline_region_status(handle, regionId, completion, diagnostic))
    }

  /**
   * Makes maps check each resource of the offline region `regionId` with the server before they use
   * it.
   *
   * The region keeps its resources. Starting its download again fetches the invalidated resources
   * for offline use. If the style or a TileJSON document is then missing or invalid, the download
   * reports an offline region response error and goes on from the stored copy. Resources that the
   * current style no longer lists stay in the region until it's deleted.
   *
   * See `mln_runtime_invalidate_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the cache file has no region `regionId`.
   */
  public fun invalidateOfflineRegion(regionId: Long): Completion<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_runtime_invalidate_offline_region(handle, regionId, completion, diagnostic))
    }

  /**
   * Lists the offline regions in the runtime's cache file.
   *
   * See `mln_runtime_list_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun listOfflineRegions(): Completion<List<OfflineRegionInfo>> =
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
   * Copies the offline regions of another cache file into the runtime's cache file.
   *
   * A region whose definition and metadata match one that the runtime's file already has isn't
   * copied again. The result lists the runtime's copy of each region in the other file.
   *
   * See `mln_runtime_merge_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when `sideDatabasePath` isn't a readable database file.
   * - `Status.NATIVE_ERROR` when the merge would exceed the tile limit that
   *   `RuntimeEventType.OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED` describes.
   */
  public fun mergeOfflineRegions(sideDatabasePath: String): Completion<List<OfflineRegionInfo>> =
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
   * Releases a runtime.
   *
   * Release the runtime's maps first.
   *
   * See `mln_runtime_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public override fun release(): Completion<Unit> =
    nativeRetire(this, binding) { check(C.mln_runtime_release(handle, completion, diagnostic)) }

  /**
   * Runs a maintenance action on the runtime's cache database.
   *
   * See `mln_runtime_run_cache_database_operation` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun runCacheDatabaseOperation(operation: CacheDatabaseOperation): Completion<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_runtime_run_cache_database_operation(
          handle,
          operation.rawValue.toInt(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Sends the events of the runtime and its maps to a callback.
   *
   * The callback replaces the one set before.
   *
   * See `mln_runtime_set_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setEventHandler(handler: RuntimeEventHandler): Completion<CommandCompletion> =
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
   * Limits the size of the runtime's ambient cache.
   *
   * `size` is in bytes. Offline regions don't count toward it.
   *
   * See `mln_runtime_set_maximum_ambient_cache_size` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  @JvmName("setMaximumAmbientCacheSize")
  public fun setMaximumAmbientCacheSize(size: ULong): Completion<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_runtime_set_maximum_ambient_cache_size(handle, size.toLong(), completion, diagnostic),
      )
    }

  /**
   * Starts or pauses the download of the offline region `regionId`.
   *
   * Setting the state that the download already has does nothing. A download stops when the runtime
   * closes.
   *
   * See `mln_runtime_set_offline_region_download_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the cache file has no region `regionId`.
   */
  public fun setOfflineRegionDownloadState(
    regionId: Long,
    state: OfflineRegionDownloadState,
  ): Completion<Unit> =
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
   * Selects the events that the runtime reports about the download of the offline region
   * `regionId`.
   *
   * `events` holds bits of `RuntimeEventFlag.ALL_RUNTIME_EVENTS` and replaces the region's previous
   * selection. `RuntimeEventFlag.NONE` stops the region's events.
   *
   * See `mln_runtime_set_offline_region_observed` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the cache file has no region `regionId`.
   */
  public fun setOfflineRegionObserved(regionId: Long, events: RuntimeEventFlag): Completion<Unit> =
    nativeUnit(this, binding) {
      check(
        C.mln_runtime_set_offline_region_observed(
          handle,
          regionId,
          events.rawValue.toLong(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Serves the runtime's network requests through a callback.
   *
   * The provider receives the network requests that its routes match, and the library loads the
   * rest as usual. A provider can serve URL schemes that the library can't load itself. The
   * provider replaces the one set before.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setResourceProvider(provider: ResourceProvider): Completion<CommandCompletion> =
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
   * Rewrites the URLs of the runtime's network requests.
   *
   * The transform replaces the one set before.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setResourceTransform(transform: ResourceTransform): Completion<CommandCompletion> =
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
   * Replaces the metadata of the offline region `regionId`.
   *
   * See `mln_runtime_update_offline_region_metadata` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the cache file has no region `regionId`.
   */
  public fun updateOfflineRegionMetadata(
    regionId: Long,
    metadata: ByteArray,
  ): Completion<OfflineRegionInfo> =
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
    get() = RuntimeDiscarding.wrap(this)

  override fun toString(): String =
    "RuntimeHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Creates a runtime.
     *
     * A runtime holds maps and the resource loading, cache, and offline regions that they share. In
     * a browser, create runtimes on a Web Worker, not on the main thread.
     *
     * The result completes once the call's thread starts. On the JVM, Android, and Kotlin/Native
     * the call waits on the calling thread, which it blocks until then, and returns a completed
     * result. On Kotlin/Wasm it runs on a thread of its own, so the page can call it.
     *
     * See `mln_runtime_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
     */
    public fun create(options: RuntimeOptions? = null): Completion<RuntimeHandle> =
      nativeThreadStart {
        val out = allocate(8, w(8, 8, 4))
        threadStart(
          { diagnostic ->
            C.mln_runtime_create(options?.let { writeRuntimeOptions(it) } ?: 0L, out, diagnostic)
          },
        ) {
          adopt(out, GeneratedOwnerDisposal::runtime) { RuntimeHandle.wrap(it) }
        }
      }

    /** Creates the owner of `handle`. */
    @JvmSynthetic internal fun wrap(handle: Long): RuntimeHandle = RuntimeHandle(handle)
  }
}

/** The commands of a [RuntimeHandle] that drop their completions. */
public class RuntimeDiscarding
private constructor(@get:JvmSynthetic internal val owner: RuntimeHandle) {
  @get:JvmSynthetic
  internal val binding
    get() = owner.binding

  @get:JvmSynthetic
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

  internal companion object {
    /** Creates the discarding view of `owner`. */
    @JvmSynthetic
    internal fun wrap(owner: RuntimeHandle): RuntimeDiscarding = RuntimeDiscarding(owner)
  }
}
