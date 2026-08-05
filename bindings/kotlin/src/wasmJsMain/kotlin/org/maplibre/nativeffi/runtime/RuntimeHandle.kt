package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeOfflineRegionList
import org.maplibre.nativeffi.internal.lifecycle.NativeOfflineRegionSnapshot
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime
import org.maplibre.nativeffi.internal.lifecycle.NativeWakeSource
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.NativeCall
import org.maplibre.nativeffi.internal.wasm.OfflineMarshal
import org.maplibre.nativeffi.internal.wasm.ResourceProviderBridge
import org.maplibre.nativeffi.internal.wasm.ResourceTransformBridge
import org.maplibre.nativeffi.internal.wasm.RuntimeEventMarshal
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionStatus
import org.maplibre.nativeffi.internal.wasm.generated.MlnResourceProvider
import org.maplibre.nativeffi.internal.wasm.generated.MlnResourceTransform
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEvent
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeOptions
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Bytes one C API handle occupies. Handles are 64-bit whatever a pointer is on this target. */
private const val HANDLE_BYTES = 8

/** Bytes a `size_t` and a `bool` occupy on wasm32. */
private const val SIZE_BYTES = 4
private const val BOOL_BYTES = 1

/**
 * An owned runtime, and the thread the module runs it on.
 *
 * A browser page cannot own a thread and MapLibre blocks, so every call here is placed on the
 * dispatcher's owner thread rather than run on the page. That thread is what created the runtime,
 * which is what makes it the runtime's owner thread as far as the C API is concerned; a call from
 * anywhere else reports an owner-thread status. Parking the Kotlin stack on the answer is what lets
 * this keep the ordinary synchronous shape the other platforms have.
 *
 * Offline snapshots and lists are the one exception. The C API documents them as carrying no thread
 * affinity, so reading one runs on the page rather than costing a round trip per region.
 */
public actual class RuntimeHandle private constructor(private val handle: NativeRuntime) :
  AutoCloseable {
  private val core = HandleStateCore("RuntimeHandle", handle.raw)

  // The Kotlin end of each synchronous callback this runtime has registered. Native holds the
  // module's thunk for as long as the registration lasts, and these hold the callback the thunk
  // reaches, so each one outlives its registration by exactly the call that clears it.
  private var resourceProvider: ResourceProviderBridge? = null
  private var resourceTransform: ResourceTransformBridge? = null

  /**
   * Checks this handle is live and then runs [body], without holding a use count across it.
   *
   * `withLive` would hold one, and every call here parks the Kotlin stack while the owner thread
   * works. A close arriving during that park would drain a count that cannot be released until the
   * park ends, which is the invariant `yieldWhileClosing` refuses to spin on. The window this
   * leaves is the one the C API already closes: a handle destroyed between the check and the call
   * is a stale handle, and native reports invalid argument for it.
   */
  private inline fun <T> live(body: () -> T): T {
    core.requireLive()
    return body()
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun pump(timeoutMillis: Long) {
    live {
      // The one call here where a non-zero timeout parks the thread it runs on rather than
      // returning promptly. That is what a pump is for, and the thread it parks is the module's
      // own worker: the page keeps servicing its event loop while this stack waits on the answer.
      Dispatcher.call(
        "mln_runtime_pump",
        2,
        { slots ->
          slots.setLong(0, handle.raw)
          slots.setLong(1, timeoutMillis)
        },
        { Status.check(Heap.loadInt(it)) },
      )
    }
  }

  public actual fun acquireWakeSource(): WakeSource = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      // Native refuses an out-parameter that is not the null handle, which the zeroed scratch
      // already satisfies.
      Dispatcher.call(
        "mln_runtime_wake_source_acquire",
        2,
        { slots ->
          slots.setLong(0, handle.raw)
          slots.setPointer(1, out)
        },
        { Status.check(Heap.loadInt(it)) },
      )
      WakeSource.fromNative(NativeWakeSource(Heap.loadLong(out)))
    }
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> =
    startOperation(
      "mln_runtime_run_ambient_cache_operation_start",
      OfflineOperationKind.AMBIENT_CACHE,
      OfflineOperationResultKind.NONE,
      3,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setInt(1, operation.nativeValue)
      slots.setPointer(2, out)
    }

  public actual fun startSetMaximumAmbientCacheSize(size: Long): OfflineOperationHandle<Unit> {
    // The C parameter is unsigned, so a negative Kotlin value would arrive as an enormous budget
    // rather than as the mistake it is.
    Status.requireArgument(size >= 0) { "size must be non-negative" }
    return startOperation(
      "mln_runtime_set_maximum_ambient_cache_size_start",
      OfflineOperationKind.SET_MAXIMUM_AMBIENT_CACHE_SIZE,
      OfflineOperationResultKind.NONE,
      3,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setLong(1, size)
      slots.setPointer(2, out)
    }
  }

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> {
    // Measured before the block is taken. A definition is a descriptor, a style URL, and possibly
    // a whole geometry tree, and the arena carves them out of one allocation rather than taking
    // one each.
    val definitionBytes = OfflineMarshal.measureDefinition(definition)
    return Heap.withScratch(definitionBytes) { scratch ->
      val root = OfflineMarshal.writeDefinition(HeapArena(scratch, definitionBytes), definition)
      withMetadata(metadata) { metadataBytes ->
        startOperation(
          "mln_runtime_offline_region_create_start",
          OfflineOperationKind.REGION_CREATE,
          OfflineOperationResultKind.REGION,
          5,
        ) { slots, out ->
          slots.setLong(0, handle.raw)
          slots.setPointer(1, root)
          slots.setPointer(2, metadataBytes)
          slots.setInt(3, metadata.size)
          slots.setPointer(4, out)
        }
      }
    }
  }

  public actual fun startOfflineRegion(id: Long): OfflineOperationHandle<OfflineRegionInfo?> =
    startOperation(
      "mln_runtime_offline_region_get_start",
      OfflineOperationKind.REGION_GET,
      OfflineOperationResultKind.OPTIONAL_REGION,
      3,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setLong(1, id)
      slots.setPointer(2, out)
    }

  public actual fun startOfflineRegions(): OfflineOperationHandle<List<OfflineRegionInfo>> =
    startOperation(
      "mln_runtime_offline_regions_list_start",
      OfflineOperationKind.REGIONS_LIST,
      OfflineOperationResultKind.REGION_LIST,
      2,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setPointer(1, out)
    }

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OfflineOperationHandle<List<OfflineRegionInfo>> =
    Heap.withScratch(Heap.utf8Size(path)) { scratch ->
      // Crosses as a bare `const char*`, so an embedded NUL would silently merge a database whose
      // path is a prefix of the one the caller named.
      Heap.requireCString(path, "path")
      Heap.storeUtf8(scratch, path)
      startOperation(
        "mln_runtime_offline_regions_merge_database_start",
        OfflineOperationKind.REGIONS_MERGE_DATABASE,
        OfflineOperationResultKind.REGION_LIST,
        3,
      ) { slots, out ->
        slots.setLong(0, handle.raw)
        slots.setPointer(1, scratch)
        slots.setPointer(2, out)
      }
    }

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> =
    withMetadata(metadata) { metadataBytes ->
      startOperation(
        "mln_runtime_offline_region_update_metadata_start",
        OfflineOperationKind.REGION_UPDATE_METADATA,
        OfflineOperationResultKind.REGION,
        5,
      ) { slots, out ->
        slots.setLong(0, handle.raw)
        slots.setLong(1, id)
        slots.setPointer(2, metadataBytes)
        slots.setInt(3, metadata.size)
        slots.setPointer(4, out)
      }
    }

  public actual fun startOfflineRegionStatus(
    id: Long
  ): OfflineOperationHandle<OfflineRegionStatus> =
    startOperation(
      "mln_runtime_offline_region_get_status_start",
      OfflineOperationKind.REGION_GET_STATUS,
      OfflineOperationResultKind.REGION_STATUS,
      3,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setLong(1, id)
      slots.setPointer(2, out)
    }

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OfflineOperationHandle<Unit> =
    startOperation(
      "mln_runtime_offline_region_set_observed_start",
      OfflineOperationKind.REGION_SET_OBSERVED,
      OfflineOperationResultKind.NONE,
      4,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setLong(1, id)
      slots.setInt(2, if (observed) 1 else 0)
      slots.setPointer(3, out)
    }

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OfflineOperationHandle<Unit> {
    // The download state is an open domain on the way out of native, but only the named values
    // mean anything on the way in.
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    return startOperation(
      "mln_runtime_offline_region_set_download_state_start",
      OfflineOperationKind.REGION_SET_DOWNLOAD_STATE,
      OfflineOperationResultKind.NONE,
      4,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setLong(1, id)
      slots.setInt(2, downloadState.nativeValue)
      slots.setPointer(3, out)
    }
  }

  public actual fun startInvalidateOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    startOperation(
      "mln_runtime_offline_region_invalidate_start",
      OfflineOperationKind.REGION_INVALIDATE,
      OfflineOperationResultKind.NONE,
      3,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setLong(1, id)
      slots.setPointer(2, out)
    }

  public actual fun startDeleteOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    startOperation(
      "mln_runtime_offline_region_delete_start",
      OfflineOperationKind.REGION_DELETE,
      OfflineOperationResultKind.NONE,
      3,
    ) { slots, out ->
      slots.setLong(0, handle.raw)
      slots.setLong(1, id)
      slots.setPointer(2, out)
    }

  public actual fun takeCreateOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    takeRegionSnapshot(
      "mln_runtime_offline_region_create_take_result",
      operation.requireLive(
        this,
        OfflineOperationKind.REGION_CREATE,
        OfflineOperationResultKind.REGION,
      ),
      operation::markConsumed,
    )

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
      // The snapshot handle and the found flag share one block, so this costs one scratch
      // acquisition rather than two.
      Heap.withScratch(HANDLE_BYTES + BOOL_BYTES) { out ->
        val found = out + HANDLE_BYTES
        Dispatcher.call(
          "mln_runtime_offline_region_get_take_result",
          4,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setLong(1, operationId)
            slots.setPointer(2, out)
            slots.setPointer(3, found)
          },
          { Status.check(Heap.loadInt(it)) },
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
  ): List<OfflineRegionInfo> =
    takeRegionList(
      "mln_runtime_offline_regions_list_take_result",
      operation.requireLive(
        this,
        OfflineOperationKind.REGIONS_LIST,
        OfflineOperationResultKind.REGION_LIST,
      ),
      operation::markConsumed,
    )

  public actual fun takeMergeOfflineRegionsDatabaseResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> =
    takeRegionList(
      "mln_runtime_offline_regions_merge_database_take_result",
      operation.requireLive(
        this,
        OfflineOperationKind.REGIONS_MERGE_DATABASE,
        OfflineOperationResultKind.REGION_LIST,
      ),
      operation::markConsumed,
    )

  public actual fun takeUpdateOfflineRegionMetadataResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    takeRegionSnapshot(
      "mln_runtime_offline_region_update_metadata_take_result",
      operation.requireLive(
        this,
        OfflineOperationKind.REGION_UPDATE_METADATA,
        OfflineOperationResultKind.REGION,
      ),
      operation::markConsumed,
    )

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
        // An output descriptor states its own size too: native reads it to decide which fields it
        // may write, and refuses a zeroed block outright.
        OfflineMarshal.writeStatusHeader(out)
        Dispatcher.call(
          "mln_runtime_offline_region_get_status_take_result",
          3,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setLong(1, operationId)
            slots.setPointer(2, out)
          },
          { Status.check(Heap.loadInt(it)) },
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
   * Registers a resource provider, which MapLibre invokes from whichever thread reaches the network
   * layer.
   *
   * The callback native holds is the browser module's own thunk rather than a trampoline this
   * binding added, because a trampoline belongs to the agent that added it and a MapLibre worker
   * cannot call one the page installed. The thunk forwards each request to the page and blocks the
   * worker until the page answers, which is safe in that direction only: the page never blocks, so
   * it always reaches the event loop turn that delivers the request.
   */
  public actual fun setResourceProvider(callback: ResourceProviderCallback) {
    resourceProvider?.checkCanClose()
    val replacement = live {
      // The host trampoline goes in before the runtime is told about the thunk, because the thunk
      // reaches the host through a pointer it reads on every request. A request that arrived while
      // that pointer was still absent would pass through and load from the network instead.
      val bridge = ResourceProviderBridge.install(callback)
      try {
        Heap.withScratch(MlnResourceProvider.SIZEOF) { descriptor ->
          MlnResourceProvider.setSize(descriptor, MlnResourceProvider.SIZEOF)
          // The layout generator leaves a function-pointer field to its caller, so the thunk's
          // address is written at the offset the generator declares for it. That offset is one of
          // the few this binding writes by hand.
          Heap.storeInt(
            descriptor + MlnResourceProvider.OFFSET_CALLBACK,
            ResourceProviderBridge.thunk(),
          )
          MlnResourceProvider.setUserData(descriptor, bridge.userData)
          Dispatcher.call(
            "mln_runtime_set_resource_provider",
            2,
            { slots ->
              slots.setLong(0, handle.raw)
              slots.setPointer(1, descriptor)
            },
            { Status.check(Heap.loadInt(it)) },
          )
        }
      } catch (error: Throwable) {
        bridge.close()
        throw error
      }
      bridge
    }
    // The call above returned, so native holds no reference to the provider being replaced and no
    // in-flight request can still invoke it. Only now may its host trampoline go.
    val previous = resourceProvider
    resourceProvider = replacement
    previous?.close()
  }

  public actual fun clearResourceProvider() {
    resourceProvider?.checkCanClose()
    // Native accepts clearing a provider that was never set, so a host that tears down
    // unconditionally does not have to remember whether it registered one.
    live { call("mln_runtime_clear_resource_provider") }
    val previous = resourceProvider
    resourceProvider = null
    previous?.close()
  }

  /** Registers a resource URL transform, which reaches the page the same way a provider does. */
  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    resourceTransform?.checkCanClose()
    val replacement = live {
      val bridge = ResourceTransformBridge.install(callback)
      try {
        Heap.withScratch(MlnResourceTransform.SIZEOF) { descriptor ->
          MlnResourceTransform.setSize(descriptor, MlnResourceTransform.SIZEOF)
          Heap.storeInt(
            descriptor + MlnResourceTransform.OFFSET_CALLBACK,
            ResourceTransformBridge.thunk(),
          )
          MlnResourceTransform.setUserData(descriptor, bridge.userData)
          Dispatcher.call(
            "mln_runtime_set_resource_transform",
            2,
            { slots ->
              slots.setLong(0, handle.raw)
              slots.setPointer(1, descriptor)
            },
            { Status.check(Heap.loadInt(it)) },
          )
        }
      } catch (error: Throwable) {
        bridge.close()
        throw error
      }
      bridge
    }
    val previous = resourceTransform
    resourceTransform = replacement
    previous?.close()
  }

  public actual fun clearResourceTransform() {
    resourceTransform?.checkCanClose()
    live { call("mln_runtime_clear_resource_transform") }
    val previous = resourceTransform
    resourceTransform = null
    previous?.close()
  }

  /**
   * Reports that the browser does not support outgoing HTTP header transforms.
   *
   * This is the documented behaviour rather than a gap in this binding. The fetch transport follows
   * redirects itself, so it cannot strip a transformed header before a cross-origin hop, and the C
   * API reports the same status for the same reason. A resource provider serves those requests
   * instead.
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
    live { call("mln_runtime_clear_http_header_transform") }
  }

  public actual fun pollEvent(): RuntimeEvent? {
    val event = live {
      // The event descriptor and the has-event flag share one block, so a poll costs one scratch
      // acquisition rather than two.
      Heap.withScratch(MlnRuntimeEvent.SIZEOF + BOOL_BYTES) { block ->
        val hasEvent = block + MlnRuntimeEvent.SIZEOF
        RuntimeEventMarshal.writeHeader(block)
        Dispatcher.call(
          "mln_runtime_poll_event",
          3,
          { slots ->
            slots.setLong(0, handle.raw)
            slots.setPointer(1, block)
            slots.setPointer(2, hasEvent)
          },
          { Status.check(Heap.loadInt(it)) },
        )
        // Read here rather than handed back as a view. The message and payload the descriptor
        // points at live in runtime-owned storage that the next poll for this runtime overwrites,
        // so the public event has to be whole before this frame returns.
        if (Heap.loadByte(hasEvent) == 0.toByte()) null
        else RuntimeEventMarshal.readEvent(block, this)
      }
    }
    // After the event is whole, because releasing a registration asks the map which sources it
    // still has and that is a second call on the owner thread. A style that has finished loading is
    // the only announcement a style set by URL makes, so it is where a source the new style dropped
    // stops being one this binding holds a callback for.
    if (event?.type == RuntimeEventType.MAP_STYLE_LOADED) {
      event.mapSource?.releaseDetachedCustomGeometrySources()
    }
    return event
  }

  public actual override fun close() {
    resourceProvider?.checkCanClose()
    resourceTransform?.checkCanClose()
    core.closeOnce(
      destroy = {
        Dispatcher.call(
          "mln_runtime_destroy",
          1,
          { slots -> slots.setLong(0, handle.raw) },
          { Heap.loadInt(it) },
        )
      },
      afterSuccess = {
        // Destroying the runtime is what released its callbacks, so the host trampolines go here
        // rather than being left in the module's function table for the life of the page.
        resourceProvider?.close()
        resourceTransform?.close()
        resourceProvider = null
        resourceTransform = null
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
      // The out-handle, the descriptor, and the two paths share one block. The handle goes first
      // because it is the only member here that needs eight-byte alignment.
      return Heap.withScratch(HANDLE_BYTES + MlnRuntimeOptions.SIZEOF + assetBytes + cacheBytes) {
        scratch ->
        val descriptor = scratch + HANDLE_BYTES
        // The leading size field is how the C API versions a descriptor: it carries the size this
        // binding was generated against so native can tell which fields it may read. No flags are
        // defined yet, and native refuses a non-zero value, so the zeroed scratch is the answer.
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
        // Placed on the owner thread like every other call, and for a stronger reason than the
        // rest: the thread that runs this becomes the runtime's owner thread, so creating on the
        // page would make every later call a wrong-thread one.
        Dispatcher.call(
          "mln_runtime_create",
          2,
          { slots ->
            slots.setPointer(0, descriptor)
            slots.setPointer(1, scratch)
          },
          { Status.check(Heap.loadInt(it)) },
        )
        RuntimeHandle(NativeRuntime(Heap.loadLong(scratch)))
      }
    }
  }

  /**
   * Discards an operation's runtime-owned state.
   *
   * Discarding does not cancel native database work; it drops any stored result and suppresses the
   * completion event. A runtime that is already gone took that state with it, so the wrapper is
   * marked consumed rather than left able to discard an id nothing owns.
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
    Dispatcher.call(
      "mln_runtime_offline_operation_discard",
      2,
      { slots ->
        slots.setLong(0, handle.raw)
        slots.setLong(1, operationId)
      },
      { Status.check(Heap.loadInt(it)) },
    )
    operation.markConsumed()
  }

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  /**
   * The maps this runtime raised events for, so a map-originated event can name its handle.
   *
   * The reference is strong, where Kotlin/Native holds a weak one. Kotlin/Wasm has no finalization
   * and no weak reference, so a weak registry is not available to write; and it would buy nothing
   * if it were, because a map that is never closed is never reclaimed on this target either. What
   * this does mean is that the entry has to be removed when the map closes rather than when it
   * becomes unreachable, which is why [unregisterMap] is called from `MapHandle.close`.
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

  /** One handle argument and nothing else, which is the shape the clearing calls take. */
  private fun call(name: String) {
    Dispatcher.call(
      name,
      1,
      { slots -> slots.setLong(0, handle.raw) },
      { Status.check(Heap.loadInt(it)) },
    )
  }

  /**
   * Starts an offline database operation and wraps the id it was given.
   *
   * [fill] writes every slot, including the trailing out-pointer this owns, so a call site reads as
   * the C signature it is packing for.
   */
  private fun <T> startOperation(
    name: String,
    kind: OfflineOperationKind,
    resultKind: OfflineOperationResultKind,
    slotCount: Int,
    fill: (NativeCall.Slots, HeapPointer) -> Unit,
  ): OfflineOperationHandle<T> = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Dispatcher.call(
        name,
        slotCount,
        { slots -> fill(slots, out) },
        { Status.check(Heap.loadInt(it)) },
      )
      OfflineOperationHandle(this, Heap.loadLong(out), kind, resultKind)
    }
  }

  /** Takes a completed operation's snapshot result and copies the region out of it. */
  private fun takeRegionSnapshot(
    name: String,
    operationId: Long,
    markConsumed: () -> Unit,
  ): OfflineRegionInfo = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Dispatcher.call(
        name,
        3,
        { slots ->
          slots.setLong(0, handle.raw)
          slots.setLong(1, operationId)
          slots.setPointer(2, out)
        },
        { Status.check(Heap.loadInt(it)) },
      )
      try {
        readSnapshot(NativeOfflineRegionSnapshot(Heap.loadLong(out)))
      } finally {
        markConsumed()
      }
    }
  }

  /** Takes a completed operation's list result and copies every region out of it. */
  private fun takeRegionList(
    name: String,
    operationId: Long,
    markConsumed: () -> Unit,
  ): List<OfflineRegionInfo> = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Dispatcher.call(
        name,
        3,
        { slots ->
          slots.setLong(0, handle.raw)
          slots.setLong(1, operationId)
          slots.setPointer(2, out)
        },
        { Status.check(Heap.loadInt(it)) },
      )
      try {
        readList(NativeOfflineRegionList(Heap.loadLong(out)))
      } finally {
        markConsumed()
      }
    }
  }

  /**
   * Copies a snapshot's region out and destroys it.
   *
   * Run on the page rather than dispatched. The C API documents snapshots and lists as carrying no
   * thread affinity and locks its handle table across the read, so a round trip would buy nothing.
   */
  private fun readSnapshot(snapshot: NativeOfflineRegionSnapshot): OfflineRegionInfo =
    try {
      Heap.withScratch(MlnOfflineRegionInfo.SIZEOF) { info ->
        OfflineMarshal.writeRegionInfoHeader(info)
        NativeCall.call(
          "mln_offline_region_snapshot_get",
          2,
          { slots ->
            slots.setLong(0, snapshot.raw)
            slots.setPointer(1, info)
          },
          { Status.check(Heap.loadInt(it)) },
        )
        OfflineMarshal.readRegionInfo(info)
      }
    } finally {
      // The pointers the info carries belong to this snapshot, so it outlives the copy above and
      // no longer than that.
      NativeCall.call(
        "mln_offline_region_snapshot_destroy",
        1,
        { slots -> slots.setLong(0, snapshot.raw) },
        {},
      )
    }

  /** Copies every region out of a list and destroys it, on the page for the reason above. */
  private fun readList(list: NativeOfflineRegionList): List<OfflineRegionInfo> =
    try {
      // The info descriptor goes first, and the count after it. A descriptor holds 64-bit fields,
      // and the heap views these reads go through index by width rather than by byte, so one
      // placed at an address the allocator did not align would be read at the wrong offsets
      // entirely rather than merely slowly.
      Heap.withScratch(MlnOfflineRegionInfo.SIZEOF + SIZE_BYTES) { info ->
        val count = info + MlnOfflineRegionInfo.SIZEOF
        NativeCall.call(
          "mln_offline_region_list_count",
          2,
          { slots ->
            slots.setLong(0, list.raw)
            slots.setPointer(1, count)
          },
          { Status.check(Heap.loadInt(it)) },
        )
        List(Heap.loadInt(count)) { index ->
          OfflineMarshal.writeRegionInfoHeader(info)
          NativeCall.call(
            "mln_offline_region_list_get",
            3,
            { slots ->
              slots.setLong(0, list.raw)
              slots.setInt(1, index)
              slots.setPointer(2, info)
            },
            { Status.check(Heap.loadInt(it)) },
          )
          OfflineMarshal.readRegionInfo(info)
        }
      }
    } finally {
      NativeCall.call(
        "mln_offline_region_list_destroy",
        1,
        { slots -> slots.setLong(0, list.raw) },
        {},
      )
    }

  /** Places [metadata] in its own block and runs [body] with where it starts. */
  private fun <T> withMetadata(metadata: ByteArray, body: (HeapPointer) -> T): T {
    // The C API spells absent metadata as the null pointer with a zero size, and scratch of zero
    // bytes cannot be acquired anyway.
    if (metadata.isEmpty()) return body(HeapPointer(0))
    return Heap.withScratch(metadata.size) { scratch ->
      Heap.storeBytes(scratch, metadata)
      body(scratch)
    }
  }
}
