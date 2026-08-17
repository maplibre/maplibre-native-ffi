package org.maplibre.nativeffi.runtime

import java.lang.ref.WeakReference
import org.maplibre.nativeffi.internal.callback.HttpHeaderTransformState
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.loader.NativeAccess.NativeRuntimeEvent
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned runtime handle backed by the JVM FFM bridge. */
public actual class RuntimeHandle
private constructor(
  private val handle: NativeRuntime,
  private var notificationSource: Long,
  private val notifications: NotificationDispatcher,
) {
  private val core = HandleStateCore("RuntimeHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun setNotificationCallback(callback: () -> Unit) {
    NativeAccess.ensureLoaded()
    core.requireLive()
    notifications.setCallback(callback)
  }

  public actual fun clearNotificationCallback() {
    NativeAccess.ensureLoaded()
    core.requireLive()
    notifications.clearCallback()
  }

  public actual fun drainReady(): List<ReadyEndpoint> {
    NativeAccess.ensureLoaded()
    core.requireLive()
    return notifications.drainReady()
  }

  public actual suspend fun barrier() {
    NativeAccess.ensureLoaded()
    val operation = NativeAccess.startRuntimeBarrier(requireLiveHandle())
    try {
      notifications.await(operation)
    } finally {
      NativeAccess.releaseOperation(operation)
    }
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OperationHandle<Unit> {
    NativeAccess.ensureLoaded()
    val operationId =
      NativeAccess.startAmbientCacheOperation(requireLiveHandle(), operation.nativeValue)
    return offlineOperation(operationId, OperationKind.AMBIENT_CACHE, OperationResultKind.NONE)
  }

  public actual fun startSetMaximumAmbientCacheSize(size: Long): OperationHandle<Unit> {
    NativeAccess.ensureLoaded()
    Status.requireArgument(size >= 0) { "size must be non-negative" }
    val operationId = NativeAccess.startSetMaximumAmbientCacheSize(requireLiveHandle(), size)
    return offlineOperation(
      operationId,
      OperationKind.SET_MAXIMUM_AMBIENT_CACHE_SIZE,
      OperationResultKind.NONE,
    )
  }

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OperationHandle<OfflineRegionInfo> =
    offlineOperation(
      NativeAccess.startCreateOfflineRegion(requireLiveHandle(), definition, metadata),
      OperationKind.REGION_CREATE,
      OperationResultKind.REGION,
    )

  public actual fun startOfflineRegion(id: Long): OperationHandle<OfflineRegionInfo?> =
    offlineOperation(
      NativeAccess.startOfflineRegion(requireLiveHandle(), id),
      OperationKind.REGION_GET,
      OperationResultKind.OPTIONAL_REGION,
    )

  public actual fun startOfflineRegions(): OperationHandle<List<OfflineRegionInfo>> =
    offlineOperation(
      NativeAccess.startOfflineRegions(requireLiveHandle()),
      OperationKind.REGIONS_LIST,
      OperationResultKind.REGION_LIST,
    )

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OperationHandle<List<OfflineRegionInfo>> =
    offlineOperation(
      NativeAccess.startMergeOfflineRegionsDatabase(requireLiveHandle(), path),
      OperationKind.REGIONS_MERGE_DATABASE,
      OperationResultKind.REGION_LIST,
    )

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OperationHandle<OfflineRegionInfo> =
    offlineOperation(
      NativeAccess.startUpdateOfflineRegionMetadata(requireLiveHandle(), id, metadata),
      OperationKind.REGION_UPDATE_METADATA,
      OperationResultKind.REGION,
    )

  public actual fun startOfflineRegionStatus(id: Long): OperationHandle<OfflineRegionStatus> =
    offlineOperation(
      NativeAccess.startOfflineRegionStatus(requireLiveHandle(), id),
      OperationKind.REGION_GET_STATUS,
      OperationResultKind.REGION_STATUS,
    )

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OperationHandle<Unit> =
    offlineOperation(
      NativeAccess.startSetOfflineRegionObserved(requireLiveHandle(), id, observed),
      OperationKind.REGION_SET_OBSERVED,
      OperationResultKind.NONE,
    )

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OperationHandle<Unit> = run {
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    offlineOperation(
      NativeAccess.startSetOfflineRegionDownloadState(
        requireLiveHandle(),
        id,
        downloadState.nativeValue,
      ),
      OperationKind.REGION_SET_DOWNLOAD_STATE,
      OperationResultKind.NONE,
    )
  }

  public actual fun startInvalidateOfflineRegion(id: Long): OperationHandle<Unit> =
    offlineOperation(
      NativeAccess.startInvalidateOfflineRegion(requireLiveHandle(), id),
      OperationKind.REGION_INVALIDATE,
      OperationResultKind.NONE,
    )

  public actual fun startDeleteOfflineRegion(id: Long): OperationHandle<Unit> =
    offlineOperation(
      NativeAccess.startDeleteOfflineRegion(requireLiveHandle(), id),
      OperationKind.REGION_DELETE,
      OperationResultKind.NONE,
    )

  public actual fun takeCreateOfflineRegionResult(
    operation: OperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    operation.withResultUse(OperationKind.REGION_CREATE, OperationResultKind.REGION) { operationId
      ->
      NativeAccess.takeCreateOfflineRegionResult(operationId, operation::markResultConsumed)
    }

  public actual fun takeOfflineRegionResult(
    operation: OperationHandle<OfflineRegionInfo?>
  ): OfflineRegionInfo? =
    operation.withResultUse(OperationKind.REGION_GET, OperationResultKind.OPTIONAL_REGION) {
      operationId ->
      NativeAccess.takeOfflineRegionResult(operationId, operation::markResultConsumed)
    }

  public actual fun takeOfflineRegionsResult(
    operation: OperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> =
    operation.withResultUse(OperationKind.REGIONS_LIST, OperationResultKind.REGION_LIST) {
      operationId ->
      NativeAccess.takeOfflineRegionsResult(operationId, operation::markResultConsumed)
    }

  public actual fun takeMergeOfflineRegionsDatabaseResult(
    operation: OperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> =
    operation.withResultUse(
      OperationKind.REGIONS_MERGE_DATABASE,
      OperationResultKind.REGION_LIST,
    ) { operationId ->
      NativeAccess.takeMergeOfflineRegionsDatabaseResult(operationId, operation::markResultConsumed)
    }

  public actual fun takeUpdateOfflineRegionMetadataResult(
    operation: OperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    operation.withResultUse(OperationKind.REGION_UPDATE_METADATA, OperationResultKind.REGION) {
      operationId ->
      NativeAccess.takeUpdateOfflineRegionMetadataResult(operationId, operation::markResultConsumed)
    }

  public actual fun takeOfflineRegionStatusResult(
    operation: OperationHandle<OfflineRegionStatus>
  ): OfflineRegionStatus =
    operation.withResultUse(OperationKind.REGION_GET_STATUS, OperationResultKind.REGION_STATUS) {
      operationId ->
      NativeAccess.takeOfflineRegionStatusResult(operationId, operation::markResultConsumed)
    }

  public actual fun setResourceProvider(callback: ResourceProviderCallback): ULong {
    NativeAccess.ensureLoaded()
    val replacement = ResourceProviderState(callback)
    HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    return try {
      NativeAccess.setResourceProvider(requireLiveHandle(), replacement.descriptor()).toULong()
    } catch (error: Throwable) {
      releaseCallbackRoot(replacement)
      throw error
    }
  }

  public actual fun clearResourceProvider(): ULong {
    NativeAccess.ensureLoaded()
    return NativeAccess.clearResourceProvider(requireLiveHandle()).toULong()
  }

  public actual fun setResourceTransform(callback: ResourceTransformCallback): ULong {
    NativeAccess.ensureLoaded()
    val replacement = ResourceTransformState(callback)
    HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    return try {
      NativeAccess.setResourceTransform(requireLiveHandle(), replacement.descriptor()).toULong()
    } catch (error: Throwable) {
      releaseCallbackRoot(replacement)
      throw error
    }
  }

  public actual fun clearResourceTransform(): ULong {
    NativeAccess.ensureLoaded()
    return NativeAccess.clearResourceTransform(requireLiveHandle()).toULong()
  }

  public actual fun setHttpHeaderTransform(callback: HttpHeaderTransformCallback): ULong {
    NativeAccess.ensureLoaded()
    val replacement = HttpHeaderTransformState(callback)
    HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    return try {
      NativeAccess.setHttpHeaderTransform(requireLiveHandle(), replacement.descriptor()).toULong()
    } catch (error: Throwable) {
      releaseCallbackRoot(replacement)
      throw error
    }
  }

  public actual fun clearHttpHeaderTransform(): ULong {
    NativeAccess.ensureLoaded()
    return NativeAccess.clearHttpHeaderTransform(requireLiveHandle()).toULong()
  }

  public actual fun drainEvents(): RuntimeEventBatch {
    NativeAccess.ensureLoaded()
    val batch = NativeAccess.drainRuntimeEvents(requireLiveHandle())
    val events = batch.events.map { it.toRuntimeEvent() }
    return RuntimeEventBatch(events)
  }

  public actual var eventMask: RuntimeEventMask
    get() {
      NativeAccess.ensureLoaded()
      return RuntimeEventMask(NativeAccess.runtimeEventMask(requireLiveHandle()))
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setRuntimeEventMask(requireLiveHandle(), value.nativeValue)
    }

  public actual suspend fun close() {
    if (!core.beginClose()) return
    val operation =
      try {
        NativeAccess.startRuntimeClose(handle)
      } catch (error: Throwable) {
        core.abortClose()
        throw error
      }
    try {
      notifications.await(operation)
    } catch (error: Throwable) {
      core.abortClose()
      throw error
    } finally {
      NativeAccess.releaseOperation(operation)
    }
    core.completeClose { liveMaps.clear() }
    notifications.close()
    val source = notificationSource
    if (source != 0L) {
      NativeAccess.closeNotificationSource(source)
      notificationSource = 0L
    }
  }

  public actual companion object {
    public actual suspend fun create(options: RuntimeOptions): RuntimeHandle {
      NativeAccess.ensureLoaded()
      val source = NativeAccess.createNotificationSource()
      val notifications = NotificationDispatcher(source)
      try {
        val operation = NativeAccess.startCreateRuntime(options, source)
        try {
          notifications.await(operation)
          return RuntimeHandle(NativeAccess.takeCreatedRuntime(operation), source, notifications)
        } finally {
          NativeAccess.releaseOperation(operation)
        }
      } catch (error: Throwable) {
        try {
          notifications.close()
          NativeAccess.closeNotificationSource(source)
        } catch (cleanup: Throwable) {
          error.addSuppressed(cleanup)
        }
        throw error
      }
    }
  }

  private fun <T> offlineOperation(
    operationId: Long,
    kind: OperationKind,
    resultKind: OperationResultKind,
  ): OperationHandle<T> = OperationHandle(this, operationId, kind, resultKind)

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  internal fun nativeHandle(): NativeRuntime = requireLiveHandle()

  internal suspend fun awaitOperation(operation: Long) {
    notifications.await(operation)
  }

  internal fun forgetOperation(operation: Long) {
    notifications.forget(operation)
  }

  internal fun registerMap(map: MapHandle) {
    liveMaps[map.nativeHandleId()] = WeakReference(map)
  }

  internal fun unregisterMap(map: MapHandle) {
    // An id names one map for the life of the process, so this key can only be
    // this map's.
    liveMaps.remove(map.nativeHandleId())
  }

  internal fun copyEventForTesting(
    type: Int,
    sourceType: Int,
    sourceId: Long,
    code: Int,
    payload: RuntimeEventPayload,
    message: String,
  ): RuntimeEvent =
    NativeRuntimeEvent(type, sourceType, sourceId, code, payload, message).toRuntimeEvent()

  private fun requireLiveHandle(): NativeRuntime {
    core.requireLive()
    return handle
  }

  /** Converts one copied event, resolving the map that queued it when it is still live. */
  private fun NativeRuntimeEvent.toRuntimeEvent(): RuntimeEvent {
    val sourceType = RuntimeEventSourceType.fromNative(sourceType)
    return RuntimeEvent(
      RuntimeEventType.fromNative(type),
      sourceType,
      sourceId,
      if (sourceType == RuntimeEventSourceType.RUNTIME) this@RuntimeHandle else null,
      if (sourceType == RuntimeEventSourceType.MAP) mapFor(sourceId) else null,
      code,
      payload,
      message,
    )
  }

  private fun mapFor(id: Long): MapHandle? {
    if (id == 0L) return null
    val reference = liveMaps[id] ?: return null
    val map = reference.get()
    if (map == null) {
      liveMaps.remove(id)
    }
    return map
  }
}

private fun releaseCallbackRoot(root: AutoCloseable?) {
  HandleLeakCleaner.releaseNativeCallbackRoot(root)
  closeQuietly(root)
}

private fun closeQuietly(closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (_: RuntimeException) {}
}

private fun closeAndSuppress(error: Throwable, closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (cleanup: Throwable) {
    error.addSuppressed(cleanup)
  }
}
