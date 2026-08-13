package org.maplibre.nativeffi.runtime

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.WeakReference
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.interpretCPointer
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toLong
import kotlinx.cinterop.value
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.internal.c.mln_event_batch_get
import org.maplibre.nativeffi.internal.c.mln_event_batch_release
import org.maplibre.nativeffi.internal.c.mln_notification_source_close
import org.maplibre.nativeffi.internal.c.mln_notification_source_create
import org.maplibre.nativeffi.internal.c.mln_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_runtime_clear_http_header_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_clear_resource_provider
import org.maplibre.nativeffi.internal.c.mln_runtime_clear_resource_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_create
import org.maplibre.nativeffi.internal.c.mln_runtime_destroy
import org.maplibre.nativeffi.internal.c.mln_runtime_drain_events
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_batch_view
import org.maplibre.nativeffi.internal.c.mln_runtime_get_event_mask
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_create_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_create_take_result
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_delete_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_get_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_get_status_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_get_status_take_result
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_get_take_result
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_invalidate_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_set_download_state_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_set_observed_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_update_metadata_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_region_update_metadata_take_result
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_regions_list_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_regions_list_take_result
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_regions_merge_database_start
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_regions_merge_database_take_result
import org.maplibre.nativeffi.internal.c.mln_runtime_options
import org.maplibre.nativeffi.internal.c.mln_runtime_options_default
import org.maplibre.nativeffi.internal.c.mln_runtime_pump
import org.maplibre.nativeffi.internal.c.mln_runtime_run_ambient_cache_operation_start
import org.maplibre.nativeffi.internal.c.mln_runtime_set_event_mask
import org.maplibre.nativeffi.internal.c.mln_runtime_set_http_header_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_set_maximum_ambient_cache_size_start
import org.maplibre.nativeffi.internal.c.mln_runtime_set_resource_provider
import org.maplibre.nativeffi.internal.c.mln_runtime_set_resource_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_wake_source_acquire
import org.maplibre.nativeffi.internal.callback.HttpHeaderTransformState
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime
import org.maplibre.nativeffi.internal.lifecycle.asHandle
import org.maplibre.nativeffi.internal.lifecycle.offlineRegionListHandle
import org.maplibre.nativeffi.internal.lifecycle.offlineRegionSnapshotHandle
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.lifecycle.runtimeHandle
import org.maplibre.nativeffi.internal.lifecycle.wakeSourceHandle
import org.maplibre.nativeffi.internal.memory.MemoryUtil
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

/** Owned native runtime handle. Close it on the owner thread. */
@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class RuntimeHandle
internal constructor(
  handle: NativeRuntime,
  private var notificationSource: ULong,
  private val destroyer: (ULong) -> Int = ::mln_runtime_destroy,
) : AutoCloseable {
  private val state = HandleState("RuntimeHandle", handle)
  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()
  private var resourceTransformState: ResourceTransformState? = null
  private var httpHeaderTransformState: HttpHeaderTransformState? = null
  private var resourceProviderState: ResourceProviderState? = null

  public actual fun pump(timeoutMillis: Long) {
    Status.check(mln_runtime_pump(state.requireLive().rawHandleValue, timeoutMillis))
  }

  public actual fun acquireWakeSource(): WakeSource = memScoped {
    val outSource = alloc<ULongVar>()
    outSource.value = 0uL
    Status.check(mln_runtime_wake_source_acquire(state.requireLive().rawHandleValue, outSource.ptr))
    WakeSource(outSource.value.asHandle("mln_runtime_wake_source_acquire", ::wakeSourceHandle))
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_run_ambient_cache_operation_start(
        state.requireLive().rawHandleValue,
        operation.nativeValue.toUInt(),
        outOperationId.ptr,
      )
    )
    offlineOperation(outOperationId.value, OperationKind.AMBIENT_CACHE, OperationResultKind.NONE)
  }

  public actual fun startSetMaximumAmbientCacheSize(size: Long): OperationHandle<Unit> = memScoped {
    Status.requireArgument(size >= 0) { "size must be non-negative" }
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_set_maximum_ambient_cache_size_start(
        state.requireLive().rawHandleValue,
        size.toULong(),
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.SET_MAXIMUM_AMBIENT_CACHE_SIZE,
      OperationResultKind.NONE,
    )
  }

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OperationHandle<OfflineRegionInfo> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_create_start(
        state.requireLive().rawHandleValue,
        RuntimeStructs.offlineRegionDefinition(definition, this),
        RuntimeStructs.metadata(metadata, this),
        metadata.size.toULong(),
        outOperationId.ptr,
      )
    )
    offlineOperation(outOperationId.value, OperationKind.REGION_CREATE, OperationResultKind.REGION)
  }

  public actual fun startOfflineRegion(id: Long): OperationHandle<OfflineRegionInfo?> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_get_start(
        state.requireLive().rawHandleValue,
        id,
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.REGION_GET,
      OperationResultKind.OPTIONAL_REGION,
    )
  }

  public actual fun startOfflineRegions(): OperationHandle<List<OfflineRegionInfo>> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_regions_list_start(state.requireLive().rawHandleValue, outOperationId.ptr)
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.REGIONS_LIST,
      OperationResultKind.REGION_LIST,
    )
  }

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OperationHandle<List<OfflineRegionInfo>> = memScoped {
    MemoryUtil.requireValidCString(path)
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_regions_merge_database_start(
        state.requireLive().rawHandleValue,
        path,
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.REGIONS_MERGE_DATABASE,
      OperationResultKind.REGION_LIST,
    )
  }

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OperationHandle<OfflineRegionInfo> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_update_metadata_start(
        state.requireLive().rawHandleValue,
        id,
        RuntimeStructs.metadata(metadata, this),
        metadata.size.toULong(),
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.REGION_UPDATE_METADATA,
      OperationResultKind.REGION,
    )
  }

  public actual fun startOfflineRegionStatus(id: Long): OperationHandle<OfflineRegionStatus> =
    memScoped {
      val outOperationId = alloc<ULongVar>()
      Status.check(
        mln_runtime_offline_region_get_status_start(
          state.requireLive().rawHandleValue,
          id,
          outOperationId.ptr,
        )
      )
      offlineOperation(
        outOperationId.value,
        OperationKind.REGION_GET_STATUS,
        OperationResultKind.REGION_STATUS,
      )
    }

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_set_observed_start(
        state.requireLive().rawHandleValue,
        id,
        observed,
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.REGION_SET_OBSERVED,
      OperationResultKind.NONE,
    )
  }

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OperationHandle<Unit> = memScoped {
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_set_download_state_start(
        state.requireLive().rawHandleValue,
        id,
        downloadState.nativeValue.toUInt(),
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.REGION_SET_DOWNLOAD_STATE,
      OperationResultKind.NONE,
    )
  }

  public actual fun startInvalidateOfflineRegion(id: Long): OperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_invalidate_start(
        state.requireLive().rawHandleValue,
        id,
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OperationKind.REGION_INVALIDATE,
      OperationResultKind.NONE,
    )
  }

  public actual fun startDeleteOfflineRegion(id: Long): OperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_delete_start(
        state.requireLive().rawHandleValue,
        id,
        outOperationId.ptr,
      )
    )
    offlineOperation(outOperationId.value, OperationKind.REGION_DELETE, OperationResultKind.NONE)
  }

  public actual fun takeCreateOfflineRegionResult(
    operation: OperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    operation.withResultUse(OperationKind.REGION_CREATE, OperationResultKind.REGION) { operationId
      ->
      memScoped {
        val outRegion = alloc<ULongVar>()
        outRegion.value = 0uL
        Status.check(mln_runtime_offline_region_create_take_result(operationId, outRegion.ptr))
        operation.markResultConsumed()
        RuntimeStructs.offlineRegionSnapshot(
          outRegion.value.asHandle("mln_offline_region_snapshot", ::offlineRegionSnapshotHandle)
        )
      }
    }

  public actual fun takeOfflineRegionResult(
    operation: OperationHandle<OfflineRegionInfo?>
  ): OfflineRegionInfo? =
    operation.withResultUse(OperationKind.REGION_GET, OperationResultKind.OPTIONAL_REGION) {
      operationId ->
      memScoped {
        val outRegion = alloc<ULongVar>()
        val outFound = alloc<BooleanVar>()
        outRegion.value = 0uL
        Status.check(
          mln_runtime_offline_region_get_take_result(operationId, outRegion.ptr, outFound.ptr)
        )
        operation.markResultConsumed()
        if (!outFound.value) null
        else
          RuntimeStructs.offlineRegionSnapshot(
            outRegion.value.asHandle("mln_offline_region_snapshot", ::offlineRegionSnapshotHandle)
          )
      }
    }

  public actual fun takeOfflineRegionsResult(
    operation: OperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> =
    operation.withResultUse(OperationKind.REGIONS_LIST, OperationResultKind.REGION_LIST) {
      operationId ->
      memScoped {
        val outRegions = alloc<ULongVar>()
        outRegions.value = 0uL
        Status.check(mln_runtime_offline_regions_list_take_result(operationId, outRegions.ptr))
        operation.markResultConsumed()
        RuntimeStructs.offlineRegionList(
          outRegions.value.asHandle("mln_offline_region_list", ::offlineRegionListHandle)
        )
      }
    }

  public actual fun takeMergeOfflineRegionsDatabaseResult(
    operation: OperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> =
    operation.withResultUse(
      OperationKind.REGIONS_MERGE_DATABASE,
      OperationResultKind.REGION_LIST,
    ) { operationId ->
      memScoped {
        val outRegions = alloc<ULongVar>()
        outRegions.value = 0uL
        Status.check(
          mln_runtime_offline_regions_merge_database_take_result(operationId, outRegions.ptr)
        )
        operation.markResultConsumed()
        RuntimeStructs.offlineRegionList(
          outRegions.value.asHandle("mln_offline_region_list", ::offlineRegionListHandle)
        )
      }
    }

  public actual fun takeUpdateOfflineRegionMetadataResult(
    operation: OperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    operation.withResultUse(OperationKind.REGION_UPDATE_METADATA, OperationResultKind.REGION) {
      operationId ->
      memScoped {
        val outRegion = alloc<ULongVar>()
        outRegion.value = 0uL
        Status.check(
          mln_runtime_offline_region_update_metadata_take_result(operationId, outRegion.ptr)
        )
        operation.markResultConsumed()
        RuntimeStructs.offlineRegionSnapshot(
          outRegion.value.asHandle("mln_offline_region_snapshot", ::offlineRegionSnapshotHandle)
        )
      }
    }

  public actual fun takeOfflineRegionStatusResult(
    operation: OperationHandle<OfflineRegionStatus>
  ): OfflineRegionStatus =
    operation.withResultUse(OperationKind.REGION_GET_STATUS, OperationResultKind.REGION_STATUS) {
      operationId ->
      memScoped {
        val outStatus = alloc<mln_offline_region_status>()
        outStatus.size = sizeOf<mln_offline_region_status>().toUInt()
        Status.check(mln_runtime_offline_region_get_status_take_result(operationId, outStatus.ptr))
        operation.markResultConsumed()
        RuntimeStructs.offlineRegionStatus(outStatus)
      }
    }

  private fun <T> offlineOperation(
    operationId: ULong,
    kind: OperationKind,
    resultKind: OperationResultKind,
  ): OperationHandle<T> = OperationHandle(this, operationId, kind, resultKind)

  public actual fun setResourceProvider(callback: ResourceProviderCallback) {
    setResourceProvider(callback) { replacement ->
      mln_runtime_set_resource_provider(
        state.requireLive().rawHandleValue,
        replacement.descriptor(),
      )
    }
  }

  internal fun setResourceProviderForTesting(
    callback: ResourceProviderCallback,
    install: (ResourceProviderState) -> Int,
  ) {
    setResourceProvider(callback, install)
  }

  private fun setResourceProvider(
    callback: ResourceProviderCallback,
    install: (ResourceProviderState) -> Int,
  ) {
    resourceProviderState?.checkCanClose()
    val replacement = ResourceProviderState(callback)
    val previous: ResourceProviderState?
    try {
      Status.check(install(replacement))
      previous = resourceProviderState
      resourceProviderState = replacement
    } catch (error: Throwable) {
      replacement.close()
      throw error
    }
    previous?.close()
  }

  public actual fun clearResourceProvider() {
    resourceProviderState?.checkCanClose()
    Status.check(mln_runtime_clear_resource_provider(state.requireLive().rawHandleValue))
    val previous = resourceProviderState
    resourceProviderState = null
    previous?.close()
  }

  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    setResourceTransform(callback) { replacement ->
      mln_runtime_set_resource_transform(
        state.requireLive().rawHandleValue,
        replacement.descriptor(),
      )
    }
  }

  internal fun setResourceTransformForTesting(
    callback: ResourceTransformCallback,
    install: (ResourceTransformState) -> Int,
  ) {
    setResourceTransform(callback, install)
  }

  private fun setResourceTransform(
    callback: ResourceTransformCallback,
    install: (ResourceTransformState) -> Int,
  ) {
    resourceTransformState?.checkCanClose()
    val replacement = ResourceTransformState(callback)
    val previous: ResourceTransformState?
    try {
      Status.check(install(replacement))
      previous = resourceTransformState
      resourceTransformState = replacement
    } catch (error: Throwable) {
      replacement.close()
      throw error
    }
    previous?.close()
  }

  public actual fun clearResourceTransform() {
    resourceTransformState?.checkCanClose()
    Status.check(mln_runtime_clear_resource_transform(state.requireLive().rawHandleValue))
    val previous = resourceTransformState
    resourceTransformState = null
    previous?.close()
  }

  public actual fun setHttpHeaderTransform(callback: HttpHeaderTransformCallback) {
    httpHeaderTransformState?.checkCanClose()
    val replacement = HttpHeaderTransformState(callback)
    try {
      Status.check(
        mln_runtime_set_http_header_transform(
          state.requireLive().rawHandleValue,
          replacement.descriptor(),
        )
      )
    } catch (error: Throwable) {
      replacement.close()
      throw error
    }
    val previous = httpHeaderTransformState
    httpHeaderTransformState = replacement
    previous?.close()
  }

  public actual fun clearHttpHeaderTransform() {
    httpHeaderTransformState?.checkCanClose()
    Status.check(mln_runtime_clear_http_header_transform(state.requireLive().rawHandleValue))
    val previous = httpHeaderTransformState
    httpHeaderTransformState = null
    previous?.close()
  }

  public actual fun drainEvents(maxEvents: Int): RuntimeEventBatch = memScoped {
    Status.requireArgument(maxEvents >= 0) { "maxEvents must be non-negative" }
    val outBatch = alloc<ULongVar>()
    outBatch.value = 0uL
    Status.check(
      mln_runtime_drain_events(
        state.requireLive().rawHandleValue,
        maxEvents.toULong(),
        outBatch.ptr,
      )
    )
    try {
      val view = alloc<mln_runtime_event_batch_view>()
      view.size = sizeOf<mln_runtime_event_batch_view>().toUInt()
      Status.check(mln_event_batch_get(outBatch.value, view.ptr))
      val eventCount = view.event_count
      require(eventCount <= Int.MAX_VALUE.toULong()) { "event count exceeds Int.MAX_VALUE" }
      val remainingCount = view.remaining_count
      require(remainingCount <= Long.MAX_VALUE.toULong()) {
        "remaining count exceeds Long.MAX_VALUE"
      }
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
      RuntimeEventBatch(copied, remainingCount.toLong())
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

  public actual override fun close() {
    resourceProviderState?.checkCanClose()
    resourceTransformState?.checkCanClose()
    httpHeaderTransformState?.checkCanClose()
    state.closeOnce({ runtime -> destroyer(runtime.rawHandleValue) }) {
      resourceProviderState?.close()
      resourceTransformState?.close()
      httpHeaderTransformState?.close()
      resourceProviderState = null
      resourceTransformState = null
      httpHeaderTransformState = null
    }
    if (notificationSource != 0uL) {
      Status.check(mln_notification_source_close(notificationSource))
      notificationSource = 0uL
    }
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  internal fun nativeHandle(): NativeRuntime = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    state.retainChild(childTypeName)

  internal fun resourceProviderStateForTesting(): ResourceProviderState? = resourceProviderState

  internal fun resourceTransformStateForTesting(): ResourceTransformState? = resourceTransformState

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
      val outSource = alloc<ULongVar>()
      outSource.value = 0uL
      Status.check(mln_notification_source_create(outSource.ptr))
      val source = outSource.value
      val nativeOptions = alloc<mln_runtime_options>()
      mln_runtime_options_default().place(nativeOptions.ptr)
      options.assetPath?.let { nativeOptions.asset_path = MemoryUtil.cString(this, it) }
      options.cachePath?.let { nativeOptions.cache_path = MemoryUtil.cString(this, it) }
      nativeOptions.event_mask = options.eventMask.nativeValue.toULong()
      nativeOptions.notification_source = source

      val outRuntime = alloc<ULongVar>()
      outRuntime.value = 0uL
      try {
        Status.check(creator(nativeOptions.ptr, outRuntime.ptr))
        RuntimeHandle(outRuntime.value.asHandle("mln_runtime_create", ::runtimeHandle), source)
      } catch (error: Throwable) {
        if (outRuntime.value != 0uL) {
          mln_runtime_destroy(outRuntime.value)
        }
        mln_notification_source_close(source)
        throw error
      }
    }
  }
}
