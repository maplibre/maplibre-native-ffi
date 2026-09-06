package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.WeakReference
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.get
import kotlinx.cinterop.interpretCPointer
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toLong
import kotlinx.cinterop.value
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.mln_completion
import org.maplibre.nativeffi.internal.c.mln_completion_result
import org.maplibre.nativeffi.internal.c.mln_event_batch_get
import org.maplibre.nativeffi.internal.c.mln_event_batch_release
import org.maplibre.nativeffi.internal.c.mln_offline_region_info
import org.maplibre.nativeffi.internal.c.mln_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_runtime_barrier
import org.maplibre.nativeffi.internal.c.mln_runtime_clear_http_header_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_clear_resource_provider
import org.maplibre.nativeffi.internal.c.mln_runtime_clear_resource_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_create
import org.maplibre.nativeffi.internal.c.mln_runtime_drain_events
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_batch_view
import org.maplibre.nativeffi.internal.c.mln_runtime_get_event_mask
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_create
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_delete
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_get
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_get_status
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_invalidate
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_set_download_state
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_set_observed
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_update_metadata
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_regions_list
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_regions_merge_database
import org.maplibre.nativeffi.internal.c.mln_runtime_options
import org.maplibre.nativeffi.internal.c.mln_runtime_options_default
import org.maplibre.nativeffi.internal.c.mln_runtime_release
import org.maplibre.nativeffi.internal.c.mln_runtime_run_ambient_cache_operation
import org.maplibre.nativeffi.internal.c.mln_runtime_set_event_mask
import org.maplibre.nativeffi.internal.c.mln_runtime_set_http_header_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_set_maximum_ambient_cache_size
import org.maplibre.nativeffi.internal.c.mln_runtime_set_resource_provider
import org.maplibre.nativeffi.internal.c.mln_runtime_set_resource_transform
import org.maplibre.nativeffi.internal.callback.HttpHeaderTransformState
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime
import org.maplibre.nativeffi.internal.lifecycle.asHandle
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.lifecycle.runtimeHandle
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.memory.toCSize
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.RuntimeStructs
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned native runtime handle. */
@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class RuntimeHandle internal constructor(handle: NativeRuntime) {
  private val state = HandleState("RuntimeHandle", handle)
  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()

  /** Teardown report of the close that consumed this handle. */
  private val tornDown = AtomicReference<Deferred<Unit>?>(null)

  public actual fun barrier(): Deferred<Unit> = CompletionBridge.unit { completion ->
    mln_runtime_barrier(state.requireLive().rawHandleValue, completion)
  }

  public actual fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit> =
    CompletionBridge.unit { completion ->
      mln_runtime_run_ambient_cache_operation(
        state.requireLive().rawHandleValue,
        operation.nativeValue.toUInt(),
        completion,
      )
    }

  public actual fun setMaximumAmbientCacheSize(size: Long): Deferred<Unit> {
    Status.requireArgument(size >= 0) { "size must be non-negative" }
    return CompletionBridge.unit { completion ->
      mln_runtime_set_maximum_ambient_cache_size(
        state.requireLive().rawHandleValue,
        size.toULong(),
        completion,
      )
    }
  }

  public actual fun createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> = memScoped {
    CompletionBridge.submit(
      { result -> offlineRegionInfo(result)!! },
      { completion ->
        mln_runtime_offline_region_create(
          state.requireLive().rawHandleValue,
          RuntimeStructs.offlineRegionDefinition(definition, this),
          RuntimeStructs.metadata(metadata, this),
          metadata.size.toCSize(),
          completion,
        )
      },
    )
  }

  public actual fun offlineRegion(id: Long): Deferred<OfflineRegionInfo?> =
    CompletionBridge.submit(
      ::offlineRegionInfo,
      { completion ->
        mln_runtime_offline_region_get(state.requireLive().rawHandleValue, id, completion)
      },
    )

  public actual fun offlineRegions(): Deferred<List<OfflineRegionInfo>> =
    CompletionBridge.submit(
      ::offlineRegionList,
      { completion ->
        mln_runtime_offline_regions_list(state.requireLive().rawHandleValue, completion)
      },
    )

  public actual fun mergeOfflineRegionsDatabase(path: String): Deferred<List<OfflineRegionInfo>> =
    memScoped {
      MemoryUtil.requireValidCString(path)
      CompletionBridge.submit(
        ::offlineRegionList,
        { completion ->
          mln_runtime_offline_regions_merge_database(
            state.requireLive().rawHandleValue,
            path,
            completion,
          )
        },
      )
    }

  public actual fun updateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> = memScoped {
    CompletionBridge.submit(
      { result -> offlineRegionInfo(result)!! },
      { completion ->
        mln_runtime_offline_region_update_metadata(
          state.requireLive().rawHandleValue,
          id,
          RuntimeStructs.metadata(metadata, this),
          metadata.size.toCSize(),
          completion,
        )
      },
    )
  }

  public actual fun offlineRegionStatus(id: Long): Deferred<OfflineRegionStatus> =
    CompletionBridge.submit(
      { result ->
        RuntimeStructs.offlineRegionStatus(
          result.pointed.value!!.reinterpret<mln_offline_region_status>().pointed
        )
      },
      { completion ->
        mln_runtime_offline_region_get_status(state.requireLive().rawHandleValue, id, completion)
      },
    )

  public actual fun setOfflineRegionObserved(id: Long, observed: Boolean): Deferred<Unit> =
    CompletionBridge.unit { completion ->
      mln_runtime_offline_region_set_observed(
        state.requireLive().rawHandleValue,
        id,
        observed,
        completion,
      )
    }

  public actual fun setOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): Deferred<Unit> {
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    return CompletionBridge.unit { completion ->
      mln_runtime_offline_region_set_download_state(
        state.requireLive().rawHandleValue,
        id,
        downloadState.nativeValue.toUInt(),
        completion,
      )
    }
  }

  public actual fun invalidateOfflineRegion(id: Long): Deferred<Unit> =
    CompletionBridge.unit { completion ->
      mln_runtime_offline_region_invalidate(state.requireLive().rawHandleValue, id, completion)
    }

  public actual fun deleteOfflineRegion(id: Long): Deferred<Unit> =
    CompletionBridge.unit { completion ->
      mln_runtime_offline_region_delete(state.requireLive().rawHandleValue, id, completion)
    }

  private fun offlineRegionInfo(result: CPointer<mln_completion_result>): OfflineRegionInfo? {
    if (result.pointed.value_count.toULong() == 0uL) return null
    return RuntimeStructs.offlineRegionInfo(
      result.pointed.value!!.reinterpret<mln_offline_region_info>().pointed
    )
  }

  private fun offlineRegionList(result: CPointer<mln_completion_result>): List<OfflineRegionInfo> {
    val count = result.pointed.value_count.toULong()
    if (count == 0uL) return emptyList()
    require(count <= Int.MAX_VALUE.toULong()) { "offline region count exceeds Int.MAX_VALUE" }
    val regions = result.pointed.value!!.reinterpret<mln_offline_region_info>()
    return List(count.toInt()) { index -> RuntimeStructs.offlineRegionInfo(regions[index]) }
  }

  public actual fun setResourceProvider(
    callback: ResourceProviderCallback
  ): Deferred<CommandCompletion> {
    val replacement = ResourceProviderState(callback)
    return command(replacement::close) { completion ->
      mln_runtime_set_resource_provider(
        state.requireLive().rawHandleValue,
        replacement.descriptor(),
        completion,
      )
    }
  }

  public actual fun clearResourceProvider(): Deferred<CommandCompletion> = command { completion ->
    mln_runtime_clear_resource_provider(state.requireLive().rawHandleValue, completion)
  }

  public actual fun setResourceTransform(
    callback: ResourceTransformCallback
  ): Deferred<CommandCompletion> {
    val replacement = ResourceTransformState(callback)
    return command(replacement::close) { completion ->
      mln_runtime_set_resource_transform(
        state.requireLive().rawHandleValue,
        replacement.descriptor(),
        completion,
      )
    }
  }

  public actual fun clearResourceTransform(): Deferred<CommandCompletion> = command { completion ->
    mln_runtime_clear_resource_transform(state.requireLive().rawHandleValue, completion)
  }

  public actual fun setHttpHeaderTransform(
    callback: HttpHeaderTransformCallback
  ): Deferred<CommandCompletion> {
    val replacement = HttpHeaderTransformState(callback)
    return command(replacement::close) { completion ->
      mln_runtime_set_http_header_transform(
        state.requireLive().rawHandleValue,
        replacement.descriptor(),
        completion,
      )
    }
  }

  public actual fun clearHttpHeaderTransform(): Deferred<CommandCompletion> =
    command { completion ->
      mln_runtime_clear_http_header_transform(state.requireLive().rawHandleValue, completion)
    }

  public actual fun drainEvents(): RuntimeEventBatch = memScoped {
    val outBatch = alloc<ULongVar>()
    outBatch.value = 0uL
    Status.check(mln_runtime_drain_events(state.requireLive().rawHandleValue, outBatch.ptr))
    try {
      val view = alloc<mln_runtime_event_batch_view>()
      view.size = sizeOf<mln_runtime_event_batch_view>().toUInt()
      Status.check(mln_event_batch_get(outBatch.value, view.ptr))
      val eventCount = view.event_count.toULong()
      require(eventCount <= Int.MAX_VALUE.toULong()) { "event count exceeds Int.MAX_VALUE" }
      val copied =
        if (eventCount == 0uL) {
          emptyList()
        } else {
          // The stride the batch reports can exceed sizeOf<mln_runtime_event>(), so
          // step through the array by it rather than indexing the typed array.
          val eventSize = view.event_size.toLong()
          val base = view.events!!.rawValue
          val messages = view.messages
          List(eventCount.toInt()) { index ->
            val event = interpretCPointer<mln_runtime_event>(base + index * eventSize)!!.pointed
            copyEvent(event, messages, eventSize)
          }
        }
      RuntimeEventBatch(copied)
    } finally {
      mln_event_batch_release(outBatch.value)
    }
  }

  public actual var eventMask: RuntimeEventMask
    get() = memScoped {
      val outMask = alloc<ULongVar>()
      Status.check(mln_runtime_get_event_mask(state.requireLive().rawHandleValue, outMask.ptr))
      RuntimeEventMask(outMask.value.toLong())
    }
    set(value) {
      Status.check(
        mln_runtime_set_event_mask(state.requireLive().rawHandleValue, value.nativeValue.toULong())
      )
    }

  public actual fun close(): Deferred<Unit> {
    if (!state.beginClose()) return tornDown.load() ?: CompletableDeferred(Unit)
    val handle = state.handleForClose().rawHandleValue
    val completed =
      try {
        CompletionBridge.unitChecked { completion -> mln_runtime_release(handle, completion) }
      } catch (error: Throwable) {
        state.abortClose()
        throw error
      }
    tornDown.store(completed)
    state.completeClose {}
    return completed
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  internal fun nativeHandle(): NativeRuntime = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()

  internal fun copyEventForTesting(
    event: mln_runtime_event,
    messages: CPointer<ByteVar>?,
    eventSize: Long = sizeOf<mln_runtime_event>(),
  ): RuntimeEvent = copyEvent(event, messages, eventSize)

  /** Copies one event of a drained batch out of runtime-owned storage. */
  private fun copyEvent(
    event: mln_runtime_event,
    messages: CPointer<ByteVar>?,
    eventSize: Long,
  ): RuntimeEvent {
    val sourceType = RuntimeEventSourceType.fromNative(event.source_type)
    val sourceId = event.source.toLong()
    return RuntimeEvent(
      RuntimeEventType.fromNative(event.type),
      sourceType,
      sourceId,
      if (sourceType == RuntimeEventSourceType.RUNTIME) this else null,
      mapFor(sourceType, sourceId),
      event.code,
      RuntimeStructs.payload(event, eventSize),
      RuntimeStructs.message(event, messages),
    )
  }

  private fun mapFor(sourceType: RuntimeEventSourceType, sourceId: Long): MapHandle? =
    if (sourceType == RuntimeEventSourceType.MAP && sourceId != 0L) liveMaps[sourceId]?.value
    else null

  internal fun registerMap(map: MapHandle) {
    liveMaps[map.nativeHandleId()] = WeakReference(map)
  }

  internal fun unregisterMap(map: MapHandle) {
    // An id names one map for the life of the process, so this key can only be
    // this map's.
    liveMaps.remove(map.nativeHandleId())
  }

  public actual companion object {
    public actual fun create(options: RuntimeOptions): RuntimeHandle =
      create(options, Maplibre.cVersion(), ::mln_runtime_create)

    internal fun createForTesting(
      options: RuntimeOptions = RuntimeOptions(),
      actualAbiVersion: Long = Maplibre.EXPECTED_C_ABI_VERSION,
      creator: (CPointer<mln_runtime_options>, CPointer<ULongVar>) -> Int,
    ): RuntimeHandle = create(options, actualAbiVersion, creator)

    private fun create(
      options: RuntimeOptions,
      actualAbiVersion: Long,
      creator: (CPointer<mln_runtime_options>, CPointer<ULongVar>) -> Int,
    ): RuntimeHandle = memScoped {
      Maplibre.checkCompatibleCAbi(actualAbiVersion)
      val nativeOptions = alloc<mln_runtime_options>()
      mln_runtime_options_default().place(nativeOptions.ptr)
      options.assetPath?.let { nativeOptions.asset_path = MemoryUtil.cString(this, it) }
      options.cachePath?.let { nativeOptions.cache_path = MemoryUtil.cString(this, it) }
      nativeOptions.event_mask = options.eventMask.nativeValue.toULong()
      val outRuntime = alloc<ULongVar>()
      outRuntime.value = 0uL
      Status.check(creator(nativeOptions.ptr, outRuntime.ptr))
      RuntimeHandle(outRuntime.value.asHandle("mln_runtime_create", ::runtimeHandle))
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private inline fun command(
  crossinline rejected: () -> Unit = {},
  crossinline call: (CPointer<mln_completion>) -> Int,
): Deferred<CommandCompletion> = CompletionBridge.command {
  call(it).also { status -> if (status != 0) rejected() }
}
