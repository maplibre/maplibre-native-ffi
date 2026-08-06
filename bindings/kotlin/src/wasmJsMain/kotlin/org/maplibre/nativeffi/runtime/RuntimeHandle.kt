package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.internal.callback.CallbackRing
import org.maplibre.nativeffi.internal.callback.QueuedResourceProviders
import org.maplibre.nativeffi.internal.callback.ResourceRewriteRules
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeOfflineRegionList
import org.maplibre.nativeffi.internal.lifecycle.NativeOfflineRegionSnapshot
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime
import org.maplibre.nativeffi.internal.lifecycle.NativeWakeSource
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.internal.wasm.OfflineMarshal
import org.maplibre.nativeffi.internal.wasm.RuntimeEventMarshal
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionStatus
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEvent
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeOptions
import org.maplibre.nativeffi.internal.wasm.generated.mln_offline_region_list_count
import org.maplibre.nativeffi.internal.wasm.generated.mln_offline_region_list_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_offline_region_list_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_offline_region_snapshot_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_offline_region_snapshot_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_clear_http_header_transform
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_create
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_operation_discard
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_create_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_create_take_result
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_delete_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_get_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_get_status_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_get_status_take_result
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_get_take_result
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_invalidate_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_set_download_state_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_set_observed_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_update_metadata_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_region_update_metadata_take_result
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_regions_list_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_regions_list_take_result
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_regions_merge_database_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_offline_regions_merge_database_take_result
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_poll_event
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_pump
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_run_ambient_cache_operation_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_set_maximum_ambient_cache_size_start
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_wake_source_acquire
import org.maplibre.nativeffi.internal.wasm.generated.mln_wake_source_destroy
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.QueuedResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderRoute
import org.maplibre.nativeffi.resource.ResourceTransformCallback
import org.maplibre.nativeffi.resource.ResourceUrlRewriteRule

/** Bytes one C API handle occupies. Handles are 64-bit whatever a pointer is on this target. */
private const val HANDLE_BYTES = 8

/** What the failure for closing a parent that still has children calls this wrapper. */
private const val TYPE_NAME = "RuntimeHandle"

/** Bytes a `size_t` and a `bool` occupy on wasm32. */
private const val SIZE_BYTES = 4
private const val BOOL_BYTES = 1

/**
 * An owned runtime, on the thread this binding runs.
 *
 * Kotlin/Wasm runs on the Emscripten pthread that the module's `main()` imported it into, where
 * blocking is legal, so every call here is an ordinary synchronous C call made from the runtime's
 * owner thread. [pump] is the one that parks, and it is also where this binding delivers the
 * callbacks that MapLibre's own threads produced.
 */
public actual class RuntimeHandle private constructor(private val handle: NativeRuntime) :
  AutoCloseable {
  private val core = HandleStateCore(TYPE_NAME, handle.raw)
  private val rewriteRules = ResourceRewriteRules()

  /** The wake source the record ring signals, so a queued record releases a parked [pump]. */
  private var ringWake: Long = 0

  init {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Status.check(mln_runtime_wake_source_acquire(handle.raw, out.address))
      ringWake = Heap.loadLong(out)
    }
    CallbackRing.setWake(ringWake)
  }

  /** Checks this handle is live and then runs [body]; native refuses a stale handle itself. */
  private inline fun <T> live(body: () -> T): T {
    core.requireLive()
    return body()
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun pump(timeoutMillis: Long) {
    live {
      Status.check(mln_runtime_pump(handle.raw, timeoutMillis))
      // The one place a host gives this binding its thread back, and so the only place a callback
      // MapLibre raised on another thread reaches host code.
      CallbackRing.drain()
    }
  }

  public actual fun acquireWakeSource(): WakeSource = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Status.check(mln_runtime_wake_source_acquire(handle.raw, out.address))
      WakeSource.fromNative(NativeWakeSource(Heap.loadLong(out)))
    }
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> =
    startOperation(OfflineOperationKind.AMBIENT_CACHE, OfflineOperationResultKind.NONE) { out ->
      mln_runtime_run_ambient_cache_operation_start(handle.raw, operation.nativeValue, out.address)
    }

  public actual fun startSetMaximumAmbientCacheSize(size: Long): OfflineOperationHandle<Unit> {
    // Unsigned in C, so a negative value would arrive as an enormous budget rather than a mistake.
    Status.requireArgument(size >= 0) { "size must be non-negative" }
    return startOperation(
      OfflineOperationKind.SET_MAXIMUM_AMBIENT_CACHE_SIZE,
      OfflineOperationResultKind.NONE,
    ) { out ->
      mln_runtime_set_maximum_ambient_cache_size_start(handle.raw, size, out.address)
    }
  }

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> {
    // A definition is a tree, and the arena carves it out of one allocation rather than many.
    val definitionBytes = OfflineMarshal.measureDefinition(definition)
    return Heap.withScratch(definitionBytes) { scratch ->
      val root = OfflineMarshal.writeDefinition(HeapArena(scratch, definitionBytes), definition)
      withMetadata(metadata) { metadataBytes ->
        startOperation(OfflineOperationKind.REGION_CREATE, OfflineOperationResultKind.REGION) { out
          ->
          mln_runtime_offline_region_create_start(
            handle.raw,
            root.address,
            metadataBytes.address,
            metadata.size,
            out.address,
          )
        }
      }
    }
  }

  public actual fun startOfflineRegion(id: Long): OfflineOperationHandle<OfflineRegionInfo?> =
    startOperation(OfflineOperationKind.REGION_GET, OfflineOperationResultKind.OPTIONAL_REGION) {
      out ->
      mln_runtime_offline_region_get_start(handle.raw, id, out.address)
    }

  public actual fun startOfflineRegions(): OfflineOperationHandle<List<OfflineRegionInfo>> =
    startOperation(OfflineOperationKind.REGIONS_LIST, OfflineOperationResultKind.REGION_LIST) { out
      ->
      mln_runtime_offline_regions_list_start(handle.raw, out.address)
    }

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OfflineOperationHandle<List<OfflineRegionInfo>> =
    Heap.withScratch(Heap.utf8Size(path)) { scratch ->
      // A bare `const char*`, so an embedded NUL would merge a database named by a prefix of this.
      Heap.requireCString(path, "path")
      Heap.storeUtf8(scratch, path)
      startOperation(
        OfflineOperationKind.REGIONS_MERGE_DATABASE,
        OfflineOperationResultKind.REGION_LIST,
      ) { out ->
        mln_runtime_offline_regions_merge_database_start(handle.raw, scratch.address, out.address)
      }
    }

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> =
    withMetadata(metadata) { metadataBytes ->
      startOperation(
        OfflineOperationKind.REGION_UPDATE_METADATA,
        OfflineOperationResultKind.REGION,
      ) { out ->
        mln_runtime_offline_region_update_metadata_start(
          handle.raw,
          id,
          metadataBytes.address,
          metadata.size,
          out.address,
        )
      }
    }

  public actual fun startOfflineRegionStatus(
    id: Long
  ): OfflineOperationHandle<OfflineRegionStatus> =
    startOperation(
      OfflineOperationKind.REGION_GET_STATUS,
      OfflineOperationResultKind.REGION_STATUS,
    ) { out ->
      mln_runtime_offline_region_get_status_start(handle.raw, id, out.address)
    }

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OfflineOperationHandle<Unit> =
    startOperation(OfflineOperationKind.REGION_SET_OBSERVED, OfflineOperationResultKind.NONE) { out
      ->
      mln_runtime_offline_region_set_observed_start(
        handle.raw,
        id,
        if (observed) 1 else 0,
        out.address,
      )
    }

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OfflineOperationHandle<Unit> {
    // An open domain on the way out of native; only the named values mean anything on the way in.
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    return startOperation(
      OfflineOperationKind.REGION_SET_DOWNLOAD_STATE,
      OfflineOperationResultKind.NONE,
    ) { out ->
      mln_runtime_offline_region_set_download_state_start(
        handle.raw,
        id,
        downloadState.nativeValue,
        out.address,
      )
    }
  }

  public actual fun startInvalidateOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    startOperation(OfflineOperationKind.REGION_INVALIDATE, OfflineOperationResultKind.NONE) { out ->
      mln_runtime_offline_region_invalidate_start(handle.raw, id, out.address)
    }

  public actual fun startDeleteOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    startOperation(OfflineOperationKind.REGION_DELETE, OfflineOperationResultKind.NONE) { out ->
      mln_runtime_offline_region_delete_start(handle.raw, id, out.address)
    }

  public actual fun takeCreateOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo {
    val operationId =
      operation.requireLive(
        this,
        OfflineOperationKind.REGION_CREATE,
        OfflineOperationResultKind.REGION,
      )
    return takeRegionSnapshot(operation::markConsumed) { out ->
      mln_runtime_offline_region_create_take_result(handle.raw, operationId, out.address)
    }
  }

  public actual fun takeOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo?>
  ): OfflineRegionInfo? {
    val operationId =
      operation.requireLive(
        this,
        OfflineOperationKind.REGION_GET,
        OfflineOperationResultKind.OPTIONAL_REGION,
      )
    return live {
      Heap.withScratch(HANDLE_BYTES + BOOL_BYTES) { out ->
        val found = out + HANDLE_BYTES
        Status.check(
          mln_runtime_offline_region_get_take_result(
            handle.raw,
            operationId,
            out.address,
            found.address,
          )
        )
        try {
          if (Heap.loadByte(found) == 0.toByte()) {
            null
          } else {
            readSnapshot(NativeOfflineRegionSnapshot(Heap.loadLong(out)))
          }
        } finally {
          operation.markConsumed()
        }
      }
    }
  }

  public actual fun takeOfflineRegionsResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> {
    val operationId =
      operation.requireLive(
        this,
        OfflineOperationKind.REGIONS_LIST,
        OfflineOperationResultKind.REGION_LIST,
      )
    return takeRegionList(operation::markConsumed) { out ->
      mln_runtime_offline_regions_list_take_result(handle.raw, operationId, out.address)
    }
  }

  public actual fun takeMergeOfflineRegionsDatabaseResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> {
    val operationId =
      operation.requireLive(
        this,
        OfflineOperationKind.REGIONS_MERGE_DATABASE,
        OfflineOperationResultKind.REGION_LIST,
      )
    return takeRegionList(operation::markConsumed) { out ->
      mln_runtime_offline_regions_merge_database_take_result(handle.raw, operationId, out.address)
    }
  }

  public actual fun takeUpdateOfflineRegionMetadataResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo {
    val operationId =
      operation.requireLive(
        this,
        OfflineOperationKind.REGION_UPDATE_METADATA,
        OfflineOperationResultKind.REGION,
      )
    return takeRegionSnapshot(operation::markConsumed) { out ->
      mln_runtime_offline_region_update_metadata_take_result(handle.raw, operationId, out.address)
    }
  }

  public actual fun takeOfflineRegionStatusResult(
    operation: OfflineOperationHandle<OfflineRegionStatus>
  ): OfflineRegionStatus {
    val operationId =
      operation.requireLive(
        this,
        OfflineOperationKind.REGION_GET_STATUS,
        OfflineOperationResultKind.REGION_STATUS,
      )
    return live {
      Heap.withScratch(MlnOfflineRegionStatus.SIZEOF) { out ->
        // Native reads an output descriptor's size to decide which fields it may write.
        OfflineMarshal.writeStatusHeader(out)
        Status.check(
          mln_runtime_offline_region_get_status_take_result(handle.raw, operationId, out.address)
        )
        try {
          OfflineMarshal.readStatus(out)
        } finally {
          operation.markConsumed()
        }
      }
    }
  }

  /**
   * Reports that this target answers a resource provider through declared routes.
   *
   * MapLibre needs a pass-through decision on the thread that raised the request, and that thread
   * is a separate JavaScript agent which cannot enter this module.
   */
  public actual fun setResourceProvider(callback: ResourceProviderCallback) {
    throw UnsupportedFeatureException(
      MaplibreStatus.UNSUPPORTED.nativeCode,
      "A resource provider callback that answers on the thread MapLibre raised it on is not " +
        "supported in the browser, where that thread is a separate JavaScript agent. Declare the " +
        "routes to claim with setResourceProvider(routes, callback) instead.",
    )
  }

  /**
   * Registers a queued resource provider that claims the requests matching [routes].
   *
   * A request a route claims is copied by the C API's queued adapter, and [callback] receives it
   * from [pump] on this thread. A request no route claims continues through the native loader.
   */
  public fun setResourceProvider(
    routes: List<ResourceProviderRoute>,
    callback: QueuedResourceProviderCallback,
  ) {
    live { QueuedResourceProviders.set(handle.raw, routes, callback) }
  }

  public actual fun clearResourceProvider() {
    live { QueuedResourceProviders.clear(handle.raw) }
  }

  /** Reports that this target answers a resource transform through a rule table. */
  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    throw UnsupportedFeatureException(
      MaplibreStatus.UNSUPPORTED.nativeCode,
      "A resource transform callback that answers on the thread MapLibre raised it on is not " +
        "supported in the browser, where that thread is a separate JavaScript agent. Declare the " +
        "rewrites with setResourceUrlRewriteRules(rules) instead.",
    )
  }

  /** Registers or replaces the URL rewrite rules that this runtime's resource transform applies. */
  public fun setResourceUrlRewriteRules(rules: List<ResourceUrlRewriteRule>) {
    live { rewriteRules.set(handle.raw, rules) }
  }

  public actual fun clearResourceTransform() {
    live { rewriteRules.clear(handle.raw) }
  }

  /**
   * Reports that the browser does not support outgoing HTTP header transforms.
   *
   * The fetch transport follows redirects itself, so it cannot strip a transformed header before a
   * cross-origin hop, and the C API reports the same status for the same reason.
   */
  public actual fun setHttpHeaderTransform(callback: HttpHeaderTransformCallback) {
    throw UnsupportedFeatureException(
      MaplibreStatus.UNSUPPORTED.nativeCode,
      "An outgoing HTTP header transform is not supported in the browser, whose fetch transport " +
        "follows redirects itself and so cannot keep transformed headers out of a cross-origin " +
        "redirect. Serve those requests with a resource provider instead.",
    )
  }

  public actual fun clearHttpHeaderTransform() {
    live { Status.check(mln_runtime_clear_http_header_transform(handle.raw)) }
  }

  public actual fun pollEvent(): RuntimeEvent? {
    val event = live {
      Heap.withScratch(MlnRuntimeEvent.SIZEOF + BOOL_BYTES) { block ->
        val hasEvent = block + MlnRuntimeEvent.SIZEOF
        RuntimeEventMarshal.writeHeader(block)
        Status.check(mln_runtime_poll_event(handle.raw, block.address, hasEvent.address))
        // Copied rather than viewed: the next poll for this runtime overwrites the storage the
        // descriptor points at.
        if (Heap.loadByte(hasEvent) == 0.toByte()) null
        else RuntimeEventMarshal.readEvent(block, this)
      }
    }
    // A loaded style is the only announcement a style set by URL makes, so it is where a source
    // the new style dropped stops being one this binding holds a callback for.
    if (event?.type == RuntimeEventType.MAP_STYLE_LOADED) {
      event.mapSource?.releaseDetachedCustomGeometrySources()
    }
    return event
  }

  public actual override fun close() {
    core.closeOnce(
      destroy = { mln_runtime_destroy(handle.raw) },
      afterSuccess = {
        // Destroying the runtime released its callbacks. The provider's routes outlive this by the
        // marker that says native reads them no more.
        QueuedResourceProviders.retireFor(handle.raw)
        rewriteRules.release()
        CallbackRing.clearWake(ringWake)
        mln_wake_source_destroy(ringWake)
        ringWake = 0
      },
    )
  }

  public actual companion object {
    public actual fun create(options: RuntimeOptions): RuntimeHandle {
      val assetPath = options.assetPath
      val cachePath = options.cachePath
      // Both cross as bare `const char*`, so a NUL would truncate the path rather than be rejected.
      assetPath?.let { Heap.requireCString(it, "assetPath") }
      cachePath?.let { Heap.requireCString(it, "cachePath") }
      val assetBytes = assetPath?.let { Heap.utf8Size(it) } ?: 0
      val cacheBytes = cachePath?.let { Heap.utf8Size(it) } ?: 0
      // The handle goes first because it is the only member here that needs eight-byte alignment.
      return Heap.withScratch(HANDLE_BYTES + MlnRuntimeOptions.SIZEOF + assetBytes + cacheBytes) {
        scratch ->
        val descriptor = scratch + HANDLE_BYTES
        // The leading size field is how the C API versions a descriptor: it carries the size this
        // binding was generated against, so native can tell which fields it may read.
        MlnRuntimeOptions.setSize(descriptor, MlnRuntimeOptions.SIZEOF)
        var text = descriptor + MlnRuntimeOptions.SIZEOF
        assetPath?.let {
          Heap.storeUtf8(text, it)
          MlnRuntimeOptions.setAssetPath(descriptor, text)
          text += assetBytes
        }
        cachePath?.let {
          Heap.storeUtf8(text, it)
          MlnRuntimeOptions.setCachePath(descriptor, text)
        }
        // The thread that runs this becomes the runtime's owner thread.
        Status.check(mln_runtime_create(descriptor.address, scratch.address))
        val created = NativeRuntime(Heap.loadLong(scratch))
        try {
          RuntimeHandle(created)
        } catch (error: Throwable) {
          // The wrapper never existed, so nothing else will destroy what native just created.
          mln_runtime_destroy(created.raw)
          throw error
        }
      }
    }
  }

  /**
   * Drops an operation's stored result and suppresses its completion event, without cancelling the
   * native database work. A runtime that is already gone took that state with it.
   */
  internal fun discardOfflineOperation(operation: OfflineOperationHandle<*>) {
    if (operation.isClosed) return
    val operationId = operation.requireLive(this)
    try {
      core.requireLive()
    } catch (error: InvalidStateException) {
      operation.markConsumed()
      throw error
    }
    Status.check(mln_runtime_offline_operation_discard(handle.raw, operationId))
    operation.markConsumed()
  }

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  /**
   * The maps this runtime raised events for, so a map-originated event can name its handle.
   *
   * A strong reference, where Kotlin/Native holds a weak one: Kotlin/Wasm has neither finalization
   * nor weak references, so `MapHandle.close` is what removes the entry.
   */
  private val liveMaps = mutableMapOf<Long, MapHandle>()

  internal fun registerMap(map: MapHandle) {
    liveMaps[map.nativeHandleId()] = map
  }

  internal fun unregisterMap(map: MapHandle) {
    // An id names one map for the life of the process, so this key can only be this map's.
    liveMaps.remove(map.nativeHandleId())
  }

  /** Resolves the map a runtime event names, or null once that map has been closed. */
  internal fun liveMap(nativeHandleId: Long): MapHandle? = liveMaps[nativeHandleId]

  /** The native runtime, for the wrappers this runtime owns. */
  internal fun nativeHandle(): NativeRuntime = live { handle }

  /** Starts an offline database operation and wraps the id that [start] writes into its out. */
  private fun <T> startOperation(
    kind: OfflineOperationKind,
    resultKind: OfflineOperationResultKind,
    start: (HeapPointer) -> Int,
  ): OfflineOperationHandle<T> = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Status.check(start(out))
      OfflineOperationHandle(this, Heap.loadLong(out), kind, resultKind)
    }
  }

  /** Takes a completed operation's snapshot result and copies the region out of it. */
  private fun takeRegionSnapshot(
    markConsumed: () -> Unit,
    take: (HeapPointer) -> Int,
  ): OfflineRegionInfo = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Status.check(take(out))
      try {
        readSnapshot(NativeOfflineRegionSnapshot(Heap.loadLong(out)))
      } finally {
        markConsumed()
      }
    }
  }

  /** Takes a completed operation's list result and copies every region out of it. */
  private fun takeRegionList(
    markConsumed: () -> Unit,
    take: (HeapPointer) -> Int,
  ): List<OfflineRegionInfo> = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Status.check(take(out))
      try {
        readList(NativeOfflineRegionList(Heap.loadLong(out)))
      } finally {
        markConsumed()
      }
    }
  }

  /** Copies a snapshot's region out and destroys it. */
  private fun readSnapshot(snapshot: NativeOfflineRegionSnapshot): OfflineRegionInfo =
    try {
      InjectedFaults.beginResultCopy(snapshot.raw, MlnOfflineRegionInfo.SIZEOF)
      Heap.withScratch(MlnOfflineRegionInfo.SIZEOF) { info ->
        OfflineMarshal.writeRegionInfoHeader(info)
        Status.check(mln_offline_region_snapshot_get(snapshot.raw, info.address))
        OfflineMarshal.readRegionInfo(info)
      }
    } finally {
      // The pointers the info carries belong to this snapshot, so it outlives the copy and no more.
      mln_offline_region_snapshot_destroy(snapshot.raw)
    }

  /** Copies every region out of a list and destroys it. */
  private fun readList(list: NativeOfflineRegionList): List<OfflineRegionInfo> =
    try {
      InjectedFaults.beginResultCopy(list.raw, MlnOfflineRegionInfo.SIZEOF + SIZE_BYTES)
      // The descriptor goes first and the count after it, because a descriptor holds 64-bit fields
      // and the heap views index by width: a misplaced one reads at the wrong offsets entirely.
      Heap.withScratch(MlnOfflineRegionInfo.SIZEOF + SIZE_BYTES) { info ->
        val count = info + MlnOfflineRegionInfo.SIZEOF
        Status.check(mln_offline_region_list_count(list.raw, count.address))
        List(Heap.loadInt(count)) { index ->
          OfflineMarshal.writeRegionInfoHeader(info)
          Status.check(mln_offline_region_list_get(list.raw, index, info.address))
          OfflineMarshal.readRegionInfo(info)
        }
      }
    } finally {
      mln_offline_region_list_destroy(list.raw)
    }

  /** Places [metadata] in its own block and runs [body] with where it starts. */
  private fun <T> withMetadata(metadata: ByteArray, body: (HeapPointer) -> T): T {
    // Absent metadata is the null pointer with a zero size, and zero-byte scratch cannot be taken.
    if (metadata.isEmpty()) return body(HeapPointer(0))
    return Heap.withScratch(metadata.size) { scratch ->
      Heap.storeBytes(scratch, metadata)
      body(scratch)
    }
  }
}
