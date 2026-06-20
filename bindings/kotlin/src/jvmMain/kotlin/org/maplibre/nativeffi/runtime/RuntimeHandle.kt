package org.maplibre.nativeffi.runtime

import java.lang.foreign.MemorySegment
import java.lang.ref.WeakReference
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.loader.NativeAccess.NativeRuntimeEvent
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned runtime handle backed by the JVM FFM bridge. */
public actual class RuntimeHandle private constructor(private val handle: MemorySegment) :
  AutoCloseable {
  private val core = HandleStateCore("RuntimeHandle", handle.address())
  private var resourceProviderState: ResourceProviderState? = null
  private var resourceTransformState: ResourceTransformState? = null
  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runOnce() {
    NativeAccess.ensureLoaded()
    core.requireLive()
    NativeAccess.runRuntimeOnce(handle)
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> {
    NativeAccess.ensureLoaded()
    val operationId =
      NativeAccess.startAmbientCacheOperation(requireLiveHandle(), operation.nativeValue)
    return offlineOperation(
      operationId,
      OfflineOperationKind.AMBIENT_CACHE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> =
    offlineOperation(
      NativeAccess.startCreateOfflineRegion(requireLiveHandle(), definition, metadata),
      OfflineOperationKind.REGION_CREATE,
      OfflineOperationResultKind.REGION,
    )

  public actual fun startOfflineRegion(id: Long): OfflineOperationHandle<OfflineRegionInfo?> =
    offlineOperation(
      NativeAccess.startOfflineRegion(requireLiveHandle(), id),
      OfflineOperationKind.REGION_GET,
      OfflineOperationResultKind.OPTIONAL_REGION,
    )

  public actual fun startOfflineRegions(): OfflineOperationHandle<List<OfflineRegionInfo>> =
    offlineOperation(
      NativeAccess.startOfflineRegions(requireLiveHandle()),
      OfflineOperationKind.REGIONS_LIST,
      OfflineOperationResultKind.REGION_LIST,
    )

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OfflineOperationHandle<List<OfflineRegionInfo>> =
    offlineOperation(
      NativeAccess.startMergeOfflineRegionsDatabase(requireLiveHandle(), path),
      OfflineOperationKind.REGIONS_MERGE_DATABASE,
      OfflineOperationResultKind.REGION_LIST,
    )

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> =
    offlineOperation(
      NativeAccess.startUpdateOfflineRegionMetadata(requireLiveHandle(), id, metadata),
      OfflineOperationKind.REGION_UPDATE_METADATA,
      OfflineOperationResultKind.REGION,
    )

  public actual fun startOfflineRegionStatus(
    id: Long
  ): OfflineOperationHandle<OfflineRegionStatus> =
    offlineOperation(
      NativeAccess.startOfflineRegionStatus(requireLiveHandle(), id),
      OfflineOperationKind.REGION_GET_STATUS,
      OfflineOperationResultKind.REGION_STATUS,
    )

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OfflineOperationHandle<Unit> =
    offlineOperation(
      NativeAccess.startSetOfflineRegionObserved(requireLiveHandle(), id, observed),
      OfflineOperationKind.REGION_SET_OBSERVED,
      OfflineOperationResultKind.NONE,
    )

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OfflineOperationHandle<Unit> = run {
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    offlineOperation(
      NativeAccess.startSetOfflineRegionDownloadState(
        requireLiveHandle(),
        id,
        downloadState.nativeValue,
      ),
      OfflineOperationKind.REGION_SET_DOWNLOAD_STATE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startInvalidateOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    offlineOperation(
      NativeAccess.startInvalidateOfflineRegion(requireLiveHandle(), id),
      OfflineOperationKind.REGION_INVALIDATE,
      OfflineOperationResultKind.NONE,
    )

  public actual fun startDeleteOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    offlineOperation(
      NativeAccess.startDeleteOfflineRegion(requireLiveHandle(), id),
      OfflineOperationKind.REGION_DELETE,
      OfflineOperationResultKind.NONE,
    )

  public actual fun takeCreateOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo {
    val operationId =
      operation.requireLive(
        this,
        OfflineOperationKind.REGION_CREATE,
        OfflineOperationResultKind.REGION,
      )
    return NativeAccess.takeCreateOfflineRegionResult(
      requireLiveHandle(),
      operationId,
      operation::markConsumed,
    )
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
    return NativeAccess.takeOfflineRegionResult(
      requireLiveHandle(),
      operationId,
      operation::markConsumed,
    )
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
    return NativeAccess.takeOfflineRegionsResult(
      requireLiveHandle(),
      operationId,
      operation::markConsumed,
    )
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
    return NativeAccess.takeMergeOfflineRegionsDatabaseResult(
      requireLiveHandle(),
      operationId,
      operation::markConsumed,
    )
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
    return NativeAccess.takeUpdateOfflineRegionMetadataResult(
      requireLiveHandle(),
      operationId,
      operation::markConsumed,
    )
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
    return NativeAccess.takeOfflineRegionStatusResult(
      requireLiveHandle(),
      operationId,
      operation::markConsumed,
    )
  }

  public actual fun setResourceProvider(callback: ResourceProviderCallback) {
    NativeAccess.ensureLoaded()
    val replacement = ResourceProviderState(callback)
    val previous: ResourceProviderState?
    try {
      resourceProviderState?.checkCanClose()
      Status.check(NativeAccess.setResourceProvider(requireLiveHandle(), replacement.descriptor()))
      previous = resourceProviderState
      resourceProviderState = replacement
    } catch (error: Throwable) {
      replacement.close()
      throw error
    }
    closeQuietly(previous)
  }

  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    NativeAccess.ensureLoaded()
    val replacement = ResourceTransformState(callback)
    val previous: ResourceTransformState?
    try {
      resourceTransformState?.checkCanClose()
      Status.check(NativeAccess.setResourceTransform(requireLiveHandle(), replacement.descriptor()))
      previous = resourceTransformState
      resourceTransformState = replacement
    } catch (error: Throwable) {
      replacement.close()
      throw error
    }
    closeQuietly(previous)
  }

  public actual fun clearResourceTransform() {
    NativeAccess.ensureLoaded()
    resourceTransformState?.checkCanClose()
    Status.check(NativeAccess.clearResourceTransform(requireLiveHandle()))
    val previous = resourceTransformState
    resourceTransformState = null
    closeQuietly(previous)
  }

  public actual fun pollEvent(): RuntimeEvent? {
    NativeAccess.ensureLoaded()
    return NativeAccess.pollRuntimeEvent(requireLiveHandle())?.toRuntimeEvent()
  }

  public actual override fun close() {
    resourceProviderState?.checkCanClose()
    resourceTransformState?.checkCanClose()
    core.closeOnce(
      destroy = { NativeAccess.destroyRuntime(handle) },
      afterSuccess = {
        resourceProviderState?.close()
        resourceProviderState = null
        resourceTransformState?.close()
        resourceTransformState = null
        liveMaps.clear()
      },
    )
  }

  public actual companion object {
    public actual fun create(options: RuntimeOptions): RuntimeHandle {
      NativeAccess.ensureLoaded()
      return RuntimeHandle(NativeAccess.createRuntime(options))
    }
  }

  private fun <T> offlineOperation(
    operationId: Long,
    kind: OfflineOperationKind,
    resultKind: OfflineOperationResultKind,
  ): OfflineOperationHandle<T> = OfflineOperationHandle(this, operationId, kind, resultKind)

  internal fun discardOfflineOperation(operation: OfflineOperationHandle<*>) {
    if (operation.isClosed) return
    val operationId = operation.requireLive(this)
    val runtime =
      try {
        requireLiveHandle()
      } catch (error: InvalidStateException) {
        operation.markConsumed()
        throw error
      }
    Status.check(NativeAccess.discardOfflineOperation(runtime, operationId))
    operation.markConsumed()
  }

  internal fun retainChild(): HandleStateCore.ChildRetention = core.retainChild()

  internal fun nativeHandle(): MemorySegment = requireLiveHandle()

  internal fun registerMap(map: MapHandle) {
    liveMaps[map.nativeAddress()] = WeakReference(map)
  }

  internal fun unregisterMap(map: MapHandle) {
    val address = map.nativeAddress()
    if (liveMaps[address]?.get() === map) {
      liveMaps.remove(address)
    }
  }

  private fun requireLiveHandle(): MemorySegment {
    core.requireLive()
    return handle
  }

  private fun NativeRuntimeEvent.toRuntimeEvent(): RuntimeEvent {
    val sourceType = RuntimeEventSourceType.fromNative(sourceType)
    val mapSource = if (sourceType == RuntimeEventSourceType.MAP) mapFor(sourceAddress) else null
    val eventType = RuntimeEventType.fromNative(type)
    if (eventType == RuntimeEventType.MAP_STYLE_LOADED) {
      mapSource?.releaseDetachedCustomGeometrySources()
    }
    return RuntimeEvent(
      eventType,
      sourceType,
      if (sourceType == RuntimeEventSourceType.RUNTIME) this@RuntimeHandle else null,
      mapSource,
      code,
      payload,
      message,
    )
  }

  private fun mapFor(address: Long): MapHandle? {
    if (address == 0L) return null
    val reference = liveMaps[address] ?: return null
    val map = reference.get()
    if (map == null) {
      liveMaps.remove(address)
    }
    return map
  }
}

private fun closeQuietly(closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (_: RuntimeException) {}
}
