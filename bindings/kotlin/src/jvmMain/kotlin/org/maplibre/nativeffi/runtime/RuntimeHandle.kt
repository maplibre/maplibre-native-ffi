package org.maplibre.nativeffi.runtime

import java.lang.ref.WeakReference
import kotlinx.coroutines.Deferred
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
public actual class RuntimeHandle private constructor(private val handle: NativeRuntime) {
  private val core = HandleStateCore("RuntimeHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun barrier(): Deferred<Unit> {
    NativeAccess.ensureLoaded()
    return NativeAccess.runtimeBarrier(requireLiveHandle())
  }

  public actual fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit> {
    NativeAccess.ensureLoaded()
    return NativeAccess.runAmbientCacheOperation(requireLiveHandle(), operation.nativeValue)
  }

  public actual fun setMaximumAmbientCacheSize(size: Long): Deferred<Unit> {
    NativeAccess.ensureLoaded()
    Status.requireArgument(size >= 0) { "size must be non-negative" }
    return NativeAccess.setMaximumAmbientCacheSize(requireLiveHandle(), size)
  }

  public actual fun createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> =
    NativeAccess.createOfflineRegion(requireLiveHandle(), definition, metadata)

  public actual fun offlineRegion(id: Long): Deferred<OfflineRegionInfo?> =
    NativeAccess.offlineRegion(requireLiveHandle(), id)

  public actual fun offlineRegions(): Deferred<List<OfflineRegionInfo>> =
    NativeAccess.offlineRegions(requireLiveHandle())

  public actual fun mergeOfflineRegionsDatabase(path: String): Deferred<List<OfflineRegionInfo>> =
    NativeAccess.mergeOfflineRegionsDatabase(requireLiveHandle(), path)

  public actual fun updateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> =
    NativeAccess.updateOfflineRegionMetadata(requireLiveHandle(), id, metadata)

  public actual fun offlineRegionStatus(id: Long): Deferred<OfflineRegionStatus> =
    NativeAccess.offlineRegionStatus(requireLiveHandle(), id)

  public actual fun setOfflineRegionObserved(id: Long, observed: Boolean): Deferred<Unit> =
    NativeAccess.setOfflineRegionObserved(requireLiveHandle(), id, observed)

  public actual fun setOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): Deferred<Unit> {
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    return NativeAccess.setOfflineRegionDownloadState(
      requireLiveHandle(),
      id,
      downloadState.nativeValue,
    )
  }

  public actual fun invalidateOfflineRegion(id: Long): Deferred<Unit> =
    NativeAccess.invalidateOfflineRegion(requireLiveHandle(), id)

  public actual fun deleteOfflineRegion(id: Long): Deferred<Unit> =
    NativeAccess.deleteOfflineRegion(requireLiveHandle(), id)

  public actual fun setResourceProvider(
    callback: ResourceProviderCallback
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    val replacement = ResourceProviderState(callback)
    HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    return try {
      NativeAccess.setResourceProvider(requireLiveHandle(), replacement.descriptor())
    } catch (error: Throwable) {
      releaseCallbackRoot(replacement)
      throw error
    }
  }

  public actual fun clearResourceProvider(): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.clearResourceProvider(requireLiveHandle())
  }

  public actual fun setResourceTransform(
    callback: ResourceTransformCallback
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    val replacement = ResourceTransformState(callback)
    HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    return try {
      NativeAccess.setResourceTransform(requireLiveHandle(), replacement.descriptor())
    } catch (error: Throwable) {
      releaseCallbackRoot(replacement)
      throw error
    }
  }

  public actual fun clearResourceTransform(): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.clearResourceTransform(requireLiveHandle())
  }

  public actual fun setHttpHeaderTransform(
    callback: HttpHeaderTransformCallback
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    val replacement = HttpHeaderTransformState(callback)
    HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    return try {
      NativeAccess.setHttpHeaderTransform(requireLiveHandle(), replacement.descriptor())
    } catch (error: Throwable) {
      releaseCallbackRoot(replacement)
      throw error
    }
  }

  public actual fun clearHttpHeaderTransform(): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.clearHttpHeaderTransform(requireLiveHandle())
  }

  public actual fun drainEvents(): RuntimeEventBatch {
    NativeAccess.ensureLoaded()
    val batch = NativeAccess.drainRuntimeEvents(requireLiveHandle())
    return RuntimeEventBatch(batch.events.map { it.toRuntimeEvent() })
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

  public actual fun close() {
    if (!core.beginClose()) return
    try {
      NativeAccess.releaseRuntime(handle)
    } catch (error: Throwable) {
      core.abortClose()
      throw error
    }
    core.completeClose { liveMaps.clear() }
  }

  public actual companion object {
    public actual fun create(options: RuntimeOptions): RuntimeHandle {
      NativeAccess.ensureLoaded()
      return RuntimeHandle(NativeAccess.createRuntime(options))
    }
  }

  internal fun nativeHandle(): NativeRuntime = requireLiveHandle()

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
