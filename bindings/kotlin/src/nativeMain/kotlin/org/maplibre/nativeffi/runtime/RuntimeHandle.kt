package org.maplibre.nativeffi.runtime

import cnames.structs.mln_offline_region_list
import cnames.structs.mln_offline_region_snapshot
import cnames.structs.mln_runtime
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.WeakReference
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toLong
import kotlinx.cinterop.value
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE
import org.maplibre.nativeffi.internal.c.mln_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_runtime_clear_resource_transform
import org.maplibre.nativeffi.internal.c.mln_runtime_create
import org.maplibre.nativeffi.internal.c.mln_runtime_destroy
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_operation_discard
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
import org.maplibre.nativeffi.internal.c.mln_runtime_poll_event
import org.maplibre.nativeffi.internal.c.mln_runtime_run_ambient_cache_operation_start
import org.maplibre.nativeffi.internal.c.mln_runtime_run_once
import org.maplibre.nativeffi.internal.c.mln_runtime_set_resource_provider
import org.maplibre.nativeffi.internal.c.mln_runtime_set_resource_transform
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.RuntimeStructs
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned native runtime handle. Close it on the owner thread. */
@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class RuntimeHandle
internal constructor(
  handle: CPointer<mln_runtime>,
  private val destroyer: (CPointer<mln_runtime>) -> Int = ::mln_runtime_destroy,
) : AutoCloseable {
  private val state = HandleState("RuntimeHandle", handle)
  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()
  private var resourceTransformState: ResourceTransformState? = null
  private var resourceProviderState: ResourceProviderState? = null

  public actual fun runOnce() {
    Status.check(mln_runtime_run_once(state.requireLive()))
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_run_ambient_cache_operation_start(
        state.requireLive(),
        operation.nativeValue.toUInt(),
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.AMBIENT_CACHE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_create_start(
        state.requireLive(),
        RuntimeStructs.offlineRegionDefinition(definition, this),
        RuntimeStructs.metadata(metadata, this),
        metadata.size.toULong(),
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.REGION_CREATE,
      OfflineOperationResultKind.REGION,
    )
  }

  public actual fun startOfflineRegion(id: Long): OfflineOperationHandle<OfflineRegionInfo?> =
    memScoped {
      val outOperationId = alloc<ULongVar>()
      Status.check(
        mln_runtime_offline_region_get_start(state.requireLive(), id, outOperationId.ptr)
      )
      offlineOperation(
        outOperationId.value,
        OfflineOperationKind.REGION_GET,
        OfflineOperationResultKind.OPTIONAL_REGION,
      )
    }

  public actual fun startOfflineRegions(): OfflineOperationHandle<List<OfflineRegionInfo>> =
    memScoped {
      val outOperationId = alloc<ULongVar>()
      Status.check(mln_runtime_offline_regions_list_start(state.requireLive(), outOperationId.ptr))
      offlineOperation(
        outOperationId.value,
        OfflineOperationKind.REGIONS_LIST,
        OfflineOperationResultKind.REGION_LIST,
      )
    }

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OfflineOperationHandle<List<OfflineRegionInfo>> = memScoped {
    MemoryUtil.requireValidCString(path)
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_regions_merge_database_start(
        state.requireLive(),
        path,
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.REGIONS_MERGE_DATABASE,
      OfflineOperationResultKind.REGION_LIST,
    )
  }

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_update_metadata_start(
        state.requireLive(),
        id,
        RuntimeStructs.metadata(metadata, this),
        metadata.size.toULong(),
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.REGION_UPDATE_METADATA,
      OfflineOperationResultKind.REGION,
    )
  }

  public actual fun startOfflineRegionStatus(
    id: Long
  ): OfflineOperationHandle<OfflineRegionStatus> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_get_status_start(state.requireLive(), id, outOperationId.ptr)
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.REGION_GET_STATUS,
      OfflineOperationResultKind.REGION_STATUS,
    )
  }

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OfflineOperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_set_observed_start(
        state.requireLive(),
        id,
        observed,
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.REGION_SET_OBSERVED,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OfflineOperationHandle<Unit> = memScoped {
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_set_download_state_start(
        state.requireLive(),
        id,
        downloadState.nativeValue.toUInt(),
        outOperationId.ptr,
      )
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.REGION_SET_DOWNLOAD_STATE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startInvalidateOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    memScoped {
      val outOperationId = alloc<ULongVar>()
      Status.check(
        mln_runtime_offline_region_invalidate_start(state.requireLive(), id, outOperationId.ptr)
      )
      offlineOperation(
        outOperationId.value,
        OfflineOperationKind.REGION_INVALIDATE,
        OfflineOperationResultKind.NONE,
      )
    }

  public actual fun startDeleteOfflineRegion(id: Long): OfflineOperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_offline_region_delete_start(state.requireLive(), id, outOperationId.ptr)
    )
    offlineOperation(
      outOperationId.value,
      OfflineOperationKind.REGION_DELETE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun takeCreateOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo = memScoped {
    val outRegion = alloc<CPointerVarOf<CPointer<mln_offline_region_snapshot>>>()
    outRegion.value = null
    val operationId =
      operation.requireLive(
        this@RuntimeHandle,
        OfflineOperationKind.REGION_CREATE,
        OfflineOperationResultKind.REGION,
      )
    Status.check(
      mln_runtime_offline_region_create_take_result(state.requireLive(), operationId, outRegion.ptr)
    )
    operation.markConsumed()
    RuntimeStructs.offlineRegionSnapshot(requireNotNull(outRegion.value))
  }

  public actual fun takeOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo?>
  ): OfflineRegionInfo? = memScoped {
    val outRegion = alloc<CPointerVarOf<CPointer<mln_offline_region_snapshot>>>()
    val outFound = alloc<BooleanVar>()
    outRegion.value = null
    val operationId =
      operation.requireLive(
        this@RuntimeHandle,
        OfflineOperationKind.REGION_GET,
        OfflineOperationResultKind.OPTIONAL_REGION,
      )
    Status.check(
      mln_runtime_offline_region_get_take_result(
        state.requireLive(),
        operationId,
        outRegion.ptr,
        outFound.ptr,
      )
    )
    operation.markConsumed()
    if (!outFound.value) null
    else RuntimeStructs.offlineRegionSnapshot(requireNotNull(outRegion.value))
  }

  public actual fun takeOfflineRegionsResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> = memScoped {
    val outRegions = alloc<CPointerVarOf<CPointer<mln_offline_region_list>>>()
    outRegions.value = null
    val operationId =
      operation.requireLive(
        this@RuntimeHandle,
        OfflineOperationKind.REGIONS_LIST,
        OfflineOperationResultKind.REGION_LIST,
      )
    Status.check(
      mln_runtime_offline_regions_list_take_result(state.requireLive(), operationId, outRegions.ptr)
    )
    operation.markConsumed()
    RuntimeStructs.offlineRegionList(requireNotNull(outRegions.value))
  }

  public actual fun takeMergeOfflineRegionsDatabaseResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> = memScoped {
    val outRegions = alloc<CPointerVarOf<CPointer<mln_offline_region_list>>>()
    outRegions.value = null
    val operationId =
      operation.requireLive(
        this@RuntimeHandle,
        OfflineOperationKind.REGIONS_MERGE_DATABASE,
        OfflineOperationResultKind.REGION_LIST,
      )
    Status.check(
      mln_runtime_offline_regions_merge_database_take_result(
        state.requireLive(),
        operationId,
        outRegions.ptr,
      )
    )
    operation.markConsumed()
    RuntimeStructs.offlineRegionList(requireNotNull(outRegions.value))
  }

  public actual fun takeUpdateOfflineRegionMetadataResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo = memScoped {
    val outRegion = alloc<CPointerVarOf<CPointer<mln_offline_region_snapshot>>>()
    outRegion.value = null
    val operationId =
      operation.requireLive(
        this@RuntimeHandle,
        OfflineOperationKind.REGION_UPDATE_METADATA,
        OfflineOperationResultKind.REGION,
      )
    Status.check(
      mln_runtime_offline_region_update_metadata_take_result(
        state.requireLive(),
        operationId,
        outRegion.ptr,
      )
    )
    operation.markConsumed()
    RuntimeStructs.offlineRegionSnapshot(requireNotNull(outRegion.value))
  }

  public actual fun takeOfflineRegionStatusResult(
    operation: OfflineOperationHandle<OfflineRegionStatus>
  ): OfflineRegionStatus = memScoped {
    val outStatus = alloc<mln_offline_region_status>()
    outStatus.size = sizeOf<mln_offline_region_status>().toUInt()
    val operationId =
      operation.requireLive(
        this@RuntimeHandle,
        OfflineOperationKind.REGION_GET_STATUS,
        OfflineOperationResultKind.REGION_STATUS,
      )
    Status.check(
      mln_runtime_offline_region_get_status_take_result(
        state.requireLive(),
        operationId,
        outStatus.ptr,
      )
    )
    operation.markConsumed()
    RuntimeStructs.offlineRegionStatus(outStatus)
  }

  private fun <T> offlineOperation(
    operationId: ULong,
    kind: OfflineOperationKind,
    resultKind: OfflineOperationResultKind,
  ): OfflineOperationHandle<T> = OfflineOperationHandle(this, operationId, kind, resultKind)

  internal fun discardOfflineOperation(operation: OfflineOperationHandle<*>) {
    if (operation.isClosed) return
    val id = operation.requireLive(this)
    val runtime =
      try {
        state.requireLive()
      } catch (error: InvalidStateException) {
        operation.markConsumed()
        throw error
      }
    Status.check(mln_runtime_offline_operation_discard(runtime, id))
    operation.markConsumed()
  }

  public actual fun setResourceProvider(callback: ResourceProviderCallback) {
    setResourceProvider(callback) { replacement ->
      mln_runtime_set_resource_provider(state.requireLive(), replacement.descriptor())
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

  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    setResourceTransform(callback) { replacement ->
      mln_runtime_set_resource_transform(state.requireLive(), replacement.descriptor())
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
    Status.check(mln_runtime_clear_resource_transform(state.requireLive()))
    val previous = resourceTransformState
    resourceTransformState = null
    previous?.close()
  }

  public actual fun pollEvent(): RuntimeEvent? = memScoped {
    val event = alloc<mln_runtime_event>()
    event.size = sizeOf<mln_runtime_event>().toUInt()
    val hasEvent = alloc<BooleanVar>()
    hasEvent.value = false
    Status.check(mln_runtime_poll_event(state.requireLive(), event.ptr, hasEvent.ptr))
    if (!hasEvent.value) {
      return@memScoped null
    }

    runtimeEvent(event)
  }

  public actual override fun close() {
    resourceProviderState?.checkCanClose()
    resourceTransformState?.checkCanClose()
    state.closeOnce(destroyer) {
      resourceProviderState?.close()
      resourceTransformState?.close()
      resourceProviderState = null
      resourceTransformState = null
    }
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  internal fun nativeHandle(): CPointer<mln_runtime> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  internal fun retainChild(): HandleStateCore.ChildRetention = state.retainChild()

  internal fun resourceProviderStateForTesting(): ResourceProviderState? = resourceProviderState

  internal fun resourceTransformStateForTesting(): ResourceTransformState? = resourceTransformState

  internal fun copyEventForTesting(event: mln_runtime_event): RuntimeEvent = runtimeEvent(event)

  internal fun applyEventSideEffectsForTesting(
    eventType: RuntimeEventType,
    sourceType: RuntimeEventSourceType,
    sourceAddress: Long?,
  ): MapHandle? = applyEventSideEffects(eventType, sourceType, sourceAddress)

  private fun runtimeEvent(event: mln_runtime_event): RuntimeEvent {
    val eventType = RuntimeEventType.fromNative(event.type)
    val sourceType = RuntimeEventSourceType.fromNative(event.source_type)
    val sourceAddress = event.source?.rawValue?.toLong()
    val mapSource = applyEventSideEffects(eventType, sourceType, sourceAddress)
    return RuntimeEvent(
      eventType,
      sourceType,
      if (sourceType == RuntimeEventSourceType.RUNTIME) this else null,
      mapSource,
      event.code,
      RuntimeStructs.payload(event),
      RuntimeStructs.message(event),
    )
  }

  private fun applyEventSideEffects(
    eventType: RuntimeEventType,
    sourceType: RuntimeEventSourceType,
    sourceAddress: Long?,
  ): MapHandle? {
    val mapSource =
      if (sourceType == RuntimeEventSourceType.MAP && sourceAddress != null)
        liveMaps[sourceAddress]?.value
      else null
    if (eventType == RuntimeEventType.MAP_STYLE_LOADED) {
      mapSource?.releaseDetachedCustomGeometrySources()
    }
    return mapSource
  }

  internal fun registerMap(map: MapHandle) {
    liveMaps[map.nativeAddress()] = WeakReference(map)
  }

  internal fun unregisterMap(map: MapHandle) {
    val address = map.nativeAddress()
    if (liveMaps[address]?.value === map) {
      liveMaps.remove(address)
    }
  }

  public actual companion object {
    public actual fun create(options: RuntimeOptions): RuntimeHandle =
      create(options, Maplibre.cVersion(), ::mln_runtime_create)

    internal fun createForTesting(
      options: RuntimeOptions = RuntimeOptions(),
      actualAbiVersion: Long = Maplibre.EXPECTED_C_ABI_VERSION,
      creator:
        (CPointer<mln_runtime_options>, CPointer<CPointerVarOf<CPointer<mln_runtime>>>) -> Int,
    ): RuntimeHandle = create(options, actualAbiVersion, creator)

    private fun create(
      options: RuntimeOptions,
      actualAbiVersion: Long,
      creator:
        (CPointer<mln_runtime_options>, CPointer<CPointerVarOf<CPointer<mln_runtime>>>) -> Int,
    ): RuntimeHandle = memScoped {
      Maplibre.checkCompatibleCAbi(actualAbiVersion)
      val nativeOptions = alloc<mln_runtime_options>()
      mln_runtime_options_default().place(nativeOptions.ptr)
      options.assetPath?.let { nativeOptions.asset_path = MemoryUtil.cString(this, it) }
      options.cachePath?.let { nativeOptions.cache_path = MemoryUtil.cString(this, it) }
      options.maximumCacheSize?.let {
        Status.requireArgument(it >= 0) { "maximumCacheSize must be non-negative" }
        nativeOptions.flags = nativeOptions.flags or MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE
        nativeOptions.maximum_cache_size = it.toULong()
      }

      val outRuntime = alloc<CPointerVarOf<CPointer<mln_runtime>>>()
      outRuntime.value = null
      Status.check(creator(nativeOptions.ptr, outRuntime.ptr))
      RuntimeHandle(requireNotNull(outRuntime.value) { "mln_runtime_create returned null" })
    }
  }
}
