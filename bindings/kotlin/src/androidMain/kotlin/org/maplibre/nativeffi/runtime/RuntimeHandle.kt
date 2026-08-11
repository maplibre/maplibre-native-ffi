package org.maplibre.nativeffi.runtime

import java.lang.ref.WeakReference
import java.nio.Buffer
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets
import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.LongPointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.TileId
import org.maplibre.nativeffi.internal.callback.HttpHeaderTransformState
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.RenderingStats
import org.maplibre.nativeffi.map.TileOperation
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned runtime handle backed by the Android JNI bridge. */
public actual class RuntimeHandle private constructor(private val handleId: Long) : AutoCloseable {
  private val core = HandleStateCore("RuntimeHandle", handleId)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private var resourceProviderState: ResourceProviderState? = null
  private var resourceTransformState: ResourceTransformState? = null
  private var httpHeaderTransformState: HttpHeaderTransformState? = null
  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()
  // One batch struct per handle, reused by every drain and freed by close().
  private val batch =
    MaplibreNativeC.mln_runtime_event_batch().also {
      Pointer.memset(it, 0, EventLayout.BATCH_SIZE.toLong())
    }
  private var eventBuffer: ByteBuffer? = null
  private var eventBufferBase = 0L
  private var messageBuffer: ByteBuffer? = null
  private var messageBufferBase = 0L

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun pump(timeoutMillis: Long) {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_runtime_pump(requireLiveHandle(), timeoutMillis))
  }

  public actual fun acquireWakeSource(): WakeSource {
    NativeAccess.ensureLoaded()
    val runtime = requireLiveHandle()
    LongPointer(1).use { outSource ->
      outSource.put(0, 0L)
      Status.check(MaplibreNativeC.mln_runtime_wake_source_acquire(runtime, outSource))
      val sourceId = outSource.get()
      require(sourceId != 0L) { "mln_runtime_wake_source_acquire returned a null wake source" }
      return WakeSource(sourceId)
    }
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> {
    NativeAccess.ensureLoaded()
    val outOperationId = longArrayOf(0L)
    Status.check(
      MaplibreNativeC.mln_runtime_run_ambient_cache_operation_start(
        requireLiveHandle(),
        operation.nativeValue,
        outOperationId,
      )
    )
    return offlineOperation(
      outOperationId[0],
      OfflineOperationKind.AMBIENT_CACHE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startSetMaximumAmbientCacheSize(size: Long): OfflineOperationHandle<Unit> {
    NativeAccess.ensureLoaded()
    Status.requireArgument(size >= 0) { "size must be non-negative" }
    val outOperationId = longArrayOf(0L)
    Status.check(
      MaplibreNativeC.mln_runtime_set_maximum_ambient_cache_size_start(
        requireLiveHandle(),
        size,
        outOperationId,
      )
    )
    return offlineOperation(
      outOperationId[0],
      OfflineOperationKind.SET_MAXIMUM_AMBIENT_CACHE_SIZE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> {
    val outOperationId = longArrayOf(0L)
    OfflineRegionDefinitionScope(definition).use { nativeDefinition ->
      Status.check(
        MaplibreNativeC.mln_runtime_offline_region_create_start(
          requireLiveHandle(),
          nativeDefinition.definition,
          metadata,
          metadata.size.toLong(),
          outOperationId,
        )
      )
    }
    return offlineOperation(
      outOperationId[0],
      OfflineOperationKind.REGION_CREATE,
      OfflineOperationResultKind.REGION,
    )
  }

  public actual fun startOfflineRegion(id: Long): OfflineOperationHandle<OfflineRegionInfo?> =
    offlineOperation(
      startOfflineLongOperation(id, MaplibreNativeC::mln_runtime_offline_region_get_start),
      OfflineOperationKind.REGION_GET,
      OfflineOperationResultKind.OPTIONAL_REGION,
    )

  public actual fun startOfflineRegions(): OfflineOperationHandle<List<OfflineRegionInfo>> =
    offlineOperation(
      startOfflineOperation(MaplibreNativeC::mln_runtime_offline_regions_list_start),
      OfflineOperationKind.REGIONS_LIST,
      OfflineOperationResultKind.REGION_LIST,
    )

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OfflineOperationHandle<List<OfflineRegionInfo>> {
    JavaCppSupport.requireValidCString(path)
    val outOperationId = longArrayOf(0L)
    Status.check(
      MaplibreNativeC.mln_runtime_offline_regions_merge_database_start(
        requireLiveHandle(),
        path,
        outOperationId,
      )
    )
    return offlineOperation(
      outOperationId[0],
      OfflineOperationKind.REGIONS_MERGE_DATABASE,
      OfflineOperationResultKind.REGION_LIST,
    )
  }

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> {
    val outOperationId = longArrayOf(0L)
    Status.check(
      MaplibreNativeC.mln_runtime_offline_region_update_metadata_start(
        requireLiveHandle(),
        id,
        metadata,
        metadata.size.toLong(),
        outOperationId,
      )
    )
    return offlineOperation(
      outOperationId[0],
      OfflineOperationKind.REGION_UPDATE_METADATA,
      OfflineOperationResultKind.REGION,
    )
  }

  public actual fun startOfflineRegionStatus(
    id: Long
  ): OfflineOperationHandle<OfflineRegionStatus> =
    offlineOperation(
      startOfflineLongOperation(id, MaplibreNativeC::mln_runtime_offline_region_get_status_start),
      OfflineOperationKind.REGION_GET_STATUS,
      OfflineOperationResultKind.REGION_STATUS,
    )

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OfflineOperationHandle<Unit> {
    val outOperationId = longArrayOf(0L)
    Status.check(
      MaplibreNativeC.mln_runtime_offline_region_set_observed_start(
        requireLiveHandle(),
        id,
        observed,
        outOperationId,
      )
    )
    return offlineOperation(
      outOperationId[0],
      OfflineOperationKind.REGION_SET_OBSERVED,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OfflineOperationHandle<Unit> {
    Status.requireArgument(downloadState.isKnown) {
      "Unknown offline region download state cannot be used as input: ${downloadState.nativeValue}"
    }
    val outOperationId = longArrayOf(0L)
    Status.check(
      MaplibreNativeC.mln_runtime_offline_region_set_download_state_start(
        requireLiveHandle(),
        id,
        downloadState.nativeValue,
        outOperationId,
      )
    )
    return offlineOperation(
      outOperationId[0],
      OfflineOperationKind.REGION_SET_DOWNLOAD_STATE,
      OfflineOperationResultKind.NONE,
    )
  }

  public actual fun startInvalidateOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    offlineOperation(
      startOfflineLongOperation(id, MaplibreNativeC::mln_runtime_offline_region_invalidate_start),
      OfflineOperationKind.REGION_INVALIDATE,
      OfflineOperationResultKind.NONE,
    )

  public actual fun startDeleteOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    offlineOperation(
      startOfflineLongOperation(id, MaplibreNativeC::mln_runtime_offline_region_delete_start),
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
    val region =
      takeOfflineRegionSnapshot(
        operationId,
        MaplibreNativeC::mln_runtime_offline_region_create_take_result,
      )
    operation.markConsumed()
    return region
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
    LongPointer(1).use { outSnapshot ->
      outSnapshot.put(0, 0L)
      BoolPointer(1).use { outFound ->
        Status.check(
          MaplibreNativeC.mln_runtime_offline_region_get_take_result(
            requireLiveHandle(),
            operationId,
            outSnapshot,
            outFound,
          )
        )
        operation.markConsumed()
        return if (outFound.get()) offlineRegionSnapshot(outSnapshot) else null
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
    val regions =
      takeOfflineRegionList(
        operationId,
        MaplibreNativeC::mln_runtime_offline_regions_list_take_result,
      )
    operation.markConsumed()
    return regions
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
    val regions =
      takeOfflineRegionList(
        operationId,
        MaplibreNativeC::mln_runtime_offline_regions_merge_database_take_result,
      )
    operation.markConsumed()
    return regions
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
    val region =
      takeOfflineRegionSnapshot(
        operationId,
        MaplibreNativeC::mln_runtime_offline_region_update_metadata_take_result,
      )
    operation.markConsumed()
    return region
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
    MaplibreNativeC.mln_offline_region_status().use { status ->
      status.size(status.sizeof())
      Status.check(
        MaplibreNativeC.mln_runtime_offline_region_get_status_take_result(
          requireLiveHandle(),
          operationId,
          status,
        )
      )
      operation.markConsumed()
      return offlineRegionStatus(status)
    }
  }

  public actual fun setResourceProvider(callback: ResourceProviderCallback) {
    val replacement = ResourceProviderState(callback)
    val previous: ResourceProviderState?
    try {
      resourceProviderState?.checkCanClose()
      Status.check(
        MaplibreNativeC.mln_runtime_set_resource_provider(
          requireLiveHandle(),
          replacement.descriptor(),
        )
      )
      previous = resourceProviderState
      resourceProviderState = replacement
      HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    } catch (error: Throwable) {
      closeAndSuppress(error, replacement)
      throw error
    }
    releaseCallbackRoot(previous)
  }

  public actual fun clearResourceProvider() {
    resourceProviderState?.checkCanClose()
    Status.check(MaplibreNativeC.mln_runtime_clear_resource_provider(requireLiveHandle()))
    val previous = resourceProviderState
    resourceProviderState = null
    // The install path retained this as a strong leak-cleaner root, so closing
    // alone would keep it and everything its callback captured reachable.
    releaseCallbackRoot(previous)
  }

  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    val replacement = ResourceTransformState(callback)
    val previous: ResourceTransformState?
    try {
      resourceTransformState?.checkCanClose()
      Status.check(
        MaplibreNativeC.mln_runtime_set_resource_transform(
          requireLiveHandle(),
          replacement.descriptor(),
        )
      )
      previous = resourceTransformState
      resourceTransformState = replacement
      HandleLeakCleaner.retainNativeCallbackRoot(replacement)
    } catch (error: Throwable) {
      closeAndSuppress(error, replacement)
      throw error
    }
    releaseCallbackRoot(previous)
  }

  public actual fun clearResourceTransform() {
    resourceTransformState?.checkCanClose()
    Status.check(MaplibreNativeC.mln_runtime_clear_resource_transform(requireLiveHandle()))
    val previous = resourceTransformState
    resourceTransformState = null
    releaseCallbackRoot(previous)
  }

  public actual fun setHttpHeaderTransform(callback: HttpHeaderTransformCallback) {
    val replacement = HttpHeaderTransformState(callback)
    try {
      httpHeaderTransformState?.checkCanClose()
      Status.check(
        MaplibreNativeC.mln_runtime_set_http_header_transform(
          requireLiveHandle(),
          replacement.descriptor(),
        )
      )
      val previous = httpHeaderTransformState
      httpHeaderTransformState = replacement
      HandleLeakCleaner.retainNativeCallbackRoot(replacement)
      releaseCallbackRoot(previous)
    } catch (error: Throwable) {
      closeAndSuppress(error, replacement)
      throw error
    }
  }

  public actual fun clearHttpHeaderTransform() {
    httpHeaderTransformState?.checkCanClose()
    Status.check(MaplibreNativeC.mln_runtime_clear_http_header_transform(requireLiveHandle()))
    val previous = httpHeaderTransformState
    httpHeaderTransformState = null
    releaseCallbackRoot(previous)
  }

  public actual fun drainEvents(maxEvents: Int): RuntimeEventBatch {
    NativeAccess.ensureLoaded()
    Status.requireArgument(maxEvents >= 0) { "maxEvents must be non-negative" }
    val runtime = requireLiveHandle()
    batch.size(EventLayout.BATCH_SIZE)
    Status.check(MaplibreNativeC.mln_runtime_drain_events(runtime, maxEvents.toLong(), batch))
    val remainingCount = batch.remaining_count()
    val eventCount = Math.toIntExact(batch.event_count())
    if (eventCount == 0) {
      return RuntimeEventBatch(emptyList(), remainingCount)
    }
    // The stride the batch reports can exceed the probed record, so index by it.
    val eventSize = batch.event_size()
    require(eventSize >= EventLayout.EVENT_SIZE) {
      "Loaded native library reports a ${eventSize}-byte runtime event, " +
        "smaller than this binding's ${EventLayout.EVENT_SIZE}-byte record"
    }
    val events = eventBytes(batch.events(), eventCount.toLong() * eventSize)
    val messages = messageBytes(batch.messages(), batch.messages_size())
    val copied =
      List(eventCount) { index ->
        copiedEvent(events, index * eventSize, eventSize, messages).toRuntimeEvent()
      }
    return RuntimeEventBatch(copied, remainingCount)
  }

  public actual var eventMask: RuntimeEventMask
    get() {
      NativeAccess.ensureLoaded()
      val outMask = LongArray(1)
      Status.check(MaplibreNativeC.mln_runtime_get_event_mask(requireLiveHandle(), outMask))
      return RuntimeEventMask(outMask[0])
    }
    set(value) {
      NativeAccess.ensureLoaded()
      Status.check(
        MaplibreNativeC.mln_runtime_set_event_mask(requireLiveHandle(), value.nativeValue)
      )
    }

  public actual override fun close() {
    resourceProviderState?.checkCanClose()
    resourceTransformState?.checkCanClose()
    httpHeaderTransformState?.checkCanClose()
    core.closeOnce(
      destroy = { MaplibreNativeC.mln_runtime_destroy(handleId) },
      afterSuccess = {
        batch.close()
        eventBuffer = null
        messageBuffer = null
        releaseCallbackRoot(resourceProviderState)
        resourceProviderState = null
        releaseCallbackRoot(resourceTransformState)
        resourceTransformState = null
        releaseCallbackRoot(httpHeaderTransformState)
        httpHeaderTransformState = null
        liveMaps.clear()
      },
    )
  }

  public actual companion object {
    public actual fun create(options: RuntimeOptions): RuntimeHandle {
      NativeAccess.ensureLoaded()
      RuntimeOptionsScope(options).use { nativeOptions ->
        val outRuntime = LongPointer(1)
        outRuntime.put(0, 0L)
        Status.check(MaplibreNativeC.mln_runtime_create(nativeOptions.options, outRuntime))
        val runtime = outRuntime.get()
        require(runtime != 0L) { "mln_runtime_create returned a null runtime" }
        return RuntimeHandle(runtime)
      }
    }
  }

  private fun <T> offlineOperation(
    operationId: Long,
    kind: OfflineOperationKind,
    resultKind: OfflineOperationResultKind,
  ): OfflineOperationHandle<T> = OfflineOperationHandle(this, operationId, kind, resultKind)

  private fun startOfflineOperation(start: (Long, LongArray) -> Int): Long {
    val outOperationId = longArrayOf(0L)
    Status.check(start(requireLiveHandle(), outOperationId))
    return outOperationId[0]
  }

  private fun startOfflineLongOperation(value: Long, start: (Long, Long, LongArray) -> Int): Long {
    val outOperationId = longArrayOf(0L)
    Status.check(start(requireLiveHandle(), value, outOperationId))
    return outOperationId[0]
  }

  internal fun discardOfflineOperation(operation: OfflineOperationHandle<*>) {
    if (operation.isClosed) return
    val operationId = operation.requireLive(this)
    val runtimeId =
      try {
        requireLiveHandle()
      } catch (error: InvalidStateException) {
        operation.markConsumed()
        throw error
      }
    Status.check(MaplibreNativeC.mln_runtime_offline_operation_discard(runtimeId, operationId))
    operation.markConsumed()
  }

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  internal fun nativeHandleId(): Long = requireLiveHandle()

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
  ): RuntimeEvent = CopiedEvent(type, sourceType, sourceId, code, payload, message).toRuntimeEvent()

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }

  /**
   * Maps a batch pointer to a reused direct buffer, re-basing only when the runtime's storage moved
   * or grew past the mapped window.
   */
  private fun eventBytes(events: Pointer, byteCount: Long): ByteBuffer {
    val cached = eventBuffer
    if (cached != null && eventBufferBase == events.address() && cached.capacity() >= byteCount) {
      return cached
    }
    val mapped = directBuffer(events, byteCount)
    eventBuffer = mapped
    eventBufferBase = events.address()
    return mapped
  }

  private fun messageBytes(messages: Pointer?, byteCount: Long): ByteBuffer? {
    if (messages == null || messages.isNull || byteCount == 0L) {
      return null
    }
    val cached = messageBuffer
    if (
      cached != null && messageBufferBase == messages.address() && cached.capacity() >= byteCount
    ) {
      return cached
    }
    val mapped = directBuffer(messages, byteCount)
    messageBuffer = mapped
    messageBufferBase = messages.address()
    return mapped
  }

  /** Reads one event record at [base] into binding-owned values. */
  private fun copiedEvent(
    events: ByteBuffer,
    base: Int,
    eventSize: Int,
    messages: ByteBuffer?,
  ): CopiedEvent {
    val payloadType = events.getInt(base + EventLayout.PAYLOAD_TYPE)
    return CopiedEvent(
      type = events.getInt(base + EventLayout.TYPE),
      sourceType = events.getInt(base + EventLayout.SOURCE_TYPE),
      sourceId = events.getLong(base + EventLayout.SOURCE),
      code = events.getInt(base + EventLayout.CODE),
      payload = runtimeEventPayload(payloadType, events, base, eventSize),
      message =
        message(
          messages,
          events.getInt(base + EventLayout.MESSAGE_OFFSET),
          events.getInt(base + EventLayout.MESSAGE_SIZE),
        ),
    )
  }

  /** Converts one copied event, resolving the map that queued it when it is still live. */
  private fun CopiedEvent.toRuntimeEvent(): RuntimeEvent {
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

  private fun mapFor(sourceId: Long): MapHandle? {
    if (sourceId == 0L) return null
    val reference = liveMaps[sourceId] ?: return null
    val map = reference.get()
    if (map == null) {
      liveMaps.remove(sourceId)
    }
    return map
  }

  private fun takeOfflineRegionSnapshot(
    operationId: Long,
    take: (Long, Long, LongPointer) -> Int,
  ): OfflineRegionInfo =
    LongPointer(1).use { outSnapshot ->
      outSnapshot.put(0, 0L)
      Status.check(take(requireLiveHandle(), operationId, outSnapshot))
      offlineRegionSnapshot(outSnapshot)
    }

  private fun takeOfflineRegionList(
    operationId: Long,
    take: (Long, Long, LongPointer) -> Int,
  ): List<OfflineRegionInfo> =
    LongPointer(1).use { outList ->
      outList.put(0, 0L)
      Status.check(take(requireLiveHandle(), operationId, outList))
      offlineRegionList(outList)
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

private fun byteString(pointer: BytePointer?, byteCount: Long): String =
  String(byteArray(pointer, byteCount), StandardCharsets.UTF_8)

private fun byteArray(pointer: Pointer?, byteCount: Long): ByteArray {
  if (pointer == null || pointer.isNull || byteCount == 0L) {
    return ByteArray(0)
  }
  val bytes = ByteArray(Math.toIntExact(byteCount))
  BytePointer(pointer).get(bytes, 0, bytes.size)
  return bytes
}

/** One event's fields, copied out of the drained batch before the drain returns. */
private class CopiedEvent(
  val type: Int,
  val sourceType: Int,
  val sourceId: Long,
  val code: Int,
  val payload: RuntimeEventPayload,
  val message: String,
)

private fun directBuffer(pointer: Pointer, byteCount: Long): ByteBuffer =
  BytePointer(pointer).capacity(byteCount).asByteBuffer().order(ByteOrder.nativeOrder())

private fun message(messages: ByteBuffer?, offset: Int, size: Int): String {
  if (messages == null || size == 0) {
    return ""
  }
  val bytes = ByteArray(size)
  readAt(messages, offset, bytes)
  return String(bytes, StandardCharsets.UTF_8)
}

/**
 * Copies [bytes].size bytes at [offset] out of [buffer].
 *
 * A duplicate keeps the reused buffer's own position untouched, and the Buffer-typed local keeps
 * this off `ByteBuffer.position(int)`, whose covariant return the Android API floor lacks.
 */
private fun readAt(buffer: ByteBuffer, offset: Int, bytes: ByteArray) {
  val view: Buffer = buffer.duplicate()
  view.position(offset)
  (view as ByteBuffer).get(bytes, 0, bytes.size)
}

private fun runtimeEventPayload(
  payloadType: Int,
  events: ByteBuffer,
  base: Int,
  eventSize: Int,
): RuntimeEventPayload =
  when (payloadType) {
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_NONE -> RuntimeEventPayload.None
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME -> renderFramePayload(events, base)
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP ->
      RuntimeEventPayload.RenderMap(
        RenderMode.fromNative(events.getInt(base + EventLayout.RENDER_MAP_MODE))
      )
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION -> tileActionPayload(events, base)
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS ->
      offlineRegionStatusPayload(events, base)
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR ->
      RuntimeEventPayload.OfflineRegionResponseError(
        events.getLong(base + EventLayout.RESPONSE_ERROR_REGION_ID),
        ResourceErrorReason.fromNative(events.getInt(base + EventLayout.RESPONSE_ERROR_REASON)),
      )
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
      RuntimeEventPayload.OfflineRegionTileCountLimit(
        events.getLong(base + EventLayout.TILE_COUNT_LIMIT_REGION_ID),
        events.getLong(base + EventLayout.TILE_COUNT_LIMIT_LIMIT),
      )
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED ->
      offlineOperationCompletedPayload(events, base)
    MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED ->
      RuntimeEventPayload.CameraTransitionFinished(events.getLong(base + EventLayout.TRANSITION_ID))
    else -> unknownPayload(payloadType, events, base, eventSize)
  }

/**
 * Copies the payload window of an event whose payload kind this version does not name, which is the
 * batch stride minus the offset of the payload inside one event record.
 */
private fun unknownPayload(
  payloadType: Int,
  events: ByteBuffer,
  base: Int,
  eventSize: Int,
): RuntimeEventPayload.Unknown {
  val bytes = ByteArray(eventSize - EventLayout.PAYLOAD)
  readAt(events, base + EventLayout.PAYLOAD, bytes)
  return RuntimeEventPayload.Unknown(payloadType, bytes)
}

private fun renderFramePayload(events: ByteBuffer, base: Int): RuntimeEventPayload.RenderFrame =
  RuntimeEventPayload.RenderFrame(
    RenderMode.fromNative(events.getInt(base + EventLayout.RENDER_FRAME_MODE)),
    events.get(base + EventLayout.RENDER_FRAME_NEEDS_REPAINT) != 0.toByte(),
    events.get(base + EventLayout.RENDER_FRAME_PLACEMENT_CHANGED) != 0.toByte(),
    RenderingStats(
      events.getDouble(base + EventLayout.STATS_ENCODING_TIME),
      events.getDouble(base + EventLayout.STATS_RENDERING_TIME),
      events.getLong(base + EventLayout.STATS_FRAME_COUNT),
      events.getLong(base + EventLayout.STATS_DRAW_CALL_COUNT),
      events.getLong(base + EventLayout.STATS_TOTAL_DRAW_CALL_COUNT),
    ),
  )

private fun tileActionPayload(events: ByteBuffer, base: Int): RuntimeEventPayload.TileAction =
  RuntimeEventPayload.TileAction(
    TileOperation.fromNative(events.getInt(base + EventLayout.TILE_ACTION_OPERATION)),
    TileId(
      Integer.toUnsignedLong(events.getInt(base + EventLayout.TILE_ID_OVERSCALED_Z)),
      events.getInt(base + EventLayout.TILE_ID_WRAP),
      Integer.toUnsignedLong(events.getInt(base + EventLayout.TILE_ID_CANONICAL_Z)),
      Integer.toUnsignedLong(events.getInt(base + EventLayout.TILE_ID_CANONICAL_X)),
      Integer.toUnsignedLong(events.getInt(base + EventLayout.TILE_ID_CANONICAL_Y)),
    ),
  )

private fun offlineRegionStatusPayload(
  events: ByteBuffer,
  base: Int,
): RuntimeEventPayload.OfflineRegionStatusChanged =
  RuntimeEventPayload.OfflineRegionStatusChanged(
    events.getLong(base + EventLayout.REGION_STATUS_REGION_ID),
    OfflineRegionStatus(
      OfflineRegionDownloadState.fromNative(
        events.getInt(base + EventLayout.REGION_STATUS_DOWNLOAD_STATE)
      ),
      events.getLong(base + EventLayout.REGION_STATUS_COMPLETED_RESOURCE_COUNT),
      events.getLong(base + EventLayout.REGION_STATUS_COMPLETED_RESOURCE_SIZE),
      events.getLong(base + EventLayout.REGION_STATUS_COMPLETED_TILE_COUNT),
      events.getLong(base + EventLayout.REGION_STATUS_REQUIRED_TILE_COUNT),
      events.getLong(base + EventLayout.REGION_STATUS_COMPLETED_TILE_SIZE),
      events.getLong(base + EventLayout.REGION_STATUS_REQUIRED_RESOURCE_COUNT),
      events.get(base + EventLayout.REGION_STATUS_COUNT_IS_PRECISE) != 0.toByte(),
      events.get(base + EventLayout.REGION_STATUS_COMPLETE) != 0.toByte(),
    ),
  )

private fun offlineOperationCompletedPayload(
  events: ByteBuffer,
  base: Int,
): RuntimeEventPayload.OfflineOperationCompleted =
  RuntimeEventPayload.OfflineOperationCompleted(
    events.getLong(base + EventLayout.OPERATION_ID),
    OfflineOperationKind.fromNative(events.getInt(base + EventLayout.OPERATION_KIND)),
    OfflineOperationResultKind.fromNative(events.getInt(base + EventLayout.OPERATION_RESULT_KIND)),
    events.getInt(base + EventLayout.OPERATION_RESULT_STATUS),
    events.get(base + EventLayout.OPERATION_FOUND) != 0.toByte(),
  )

private fun offlineRegionStatus(
  status: MaplibreNativeC.mln_offline_region_status
): OfflineRegionStatus =
  OfflineRegionStatus(
    OfflineRegionDownloadState.fromNative(status.download_state()),
    status.completed_resource_count(),
    status.completed_resource_size(),
    status.completed_tile_count(),
    status.required_tile_count(),
    status.completed_tile_size(),
    status.required_resource_count(),
    status.required_resource_count_is_precise(),
    status.complete(),
  )

private fun offlineRegionSnapshot(outSnapshot: LongPointer): OfflineRegionInfo {
  val snapshot = outSnapshot.get()
  require(snapshot != 0L) { "offline operation returned a null snapshot" }
  return try {
    MaplibreNativeC.mln_offline_region_info().use { info ->
      info.size(info.sizeof())
      Status.check(MaplibreNativeC.mln_offline_region_snapshot_get(snapshot, info))
      offlineRegionInfo(info)
    }
  } finally {
    MaplibreNativeC.mln_offline_region_snapshot_destroy(snapshot)
  }
}

private fun offlineRegionList(outList: LongPointer): List<OfflineRegionInfo> {
  val list = outList.get()
  require(list != 0L) { "offline operation returned a null region list" }
  return offlineRegionList(
    list,
    counter = MaplibreNativeC::mln_offline_region_list_count,
    getter = MaplibreNativeC::mln_offline_region_list_get,
    destroyer = MaplibreNativeC::mln_offline_region_list_destroy,
  )
}

private fun offlineRegionList(
  list: Long,
  counter: (Long, SizeTPointer) -> Int,
  getter: (Long, Long, MaplibreNativeC.mln_offline_region_info) -> Int,
  destroyer: (Long) -> Unit,
): List<OfflineRegionInfo> {
  return try {
    SizeTPointer(1).use { outCount ->
      Status.check(counter(list, outCount))
      val count = Math.toIntExact(outCount.get())
      List(count) { index ->
        MaplibreNativeC.mln_offline_region_info().use { info ->
          info.size(info.sizeof())
          Status.check(getter(list, index.toLong(), info))
          offlineRegionInfo(info)
        }
      }
    }
  } finally {
    destroyer(list)
  }
}

private fun offlineRegionInfo(info: MaplibreNativeC.mln_offline_region_info): OfflineRegionInfo =
  OfflineRegionInfo(
    info.id(),
    offlineRegionDefinition(info.definition()),
    byteArray(info.metadata(), info.metadata_size()),
  )

private fun offlineRegionDefinition(
  definition: MaplibreNativeC.mln_offline_region_definition
): OfflineRegionDefinition =
  when (definition.type()) {
    MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID ->
      offlineTilePyramidDefinition(definition.data_tile_pyramid())
    MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY ->
      offlineGeometryDefinition(definition.data_geometry())
    else -> OfflineRegionDefinition.Unknown(definition.type(), definition.size())
  }

private fun offlineTilePyramidDefinition(
  definition: MaplibreNativeC.mln_offline_tile_pyramid_region_definition
): OfflineRegionDefinition.TilePyramid =
  OfflineRegionDefinition.TilePyramid(
    byteString(definition.style_url(), cStringLength(definition.style_url())),
    latLngBounds(definition.bounds()),
    definition.min_zoom(),
    definition.max_zoom(),
    definition.pixel_ratio(),
    definition.include_ideographs(),
  )

private fun offlineGeometryDefinition(
  definition: MaplibreNativeC.mln_offline_geometry_region_definition
): OfflineRegionDefinition.GeometryRegion =
  OfflineRegionDefinition.GeometryRegion(
    byteString(definition.style_url(), cStringLength(definition.style_url())),
    byteArray(definition.geometry().data(), definition.geometry().size()),
    definition.min_zoom(),
    definition.max_zoom(),
    definition.pixel_ratio(),
    definition.include_ideographs(),
  )

private fun latLngBounds(bounds: MaplibreNativeC.mln_lat_lng_bounds): LatLngBounds =
  LatLngBounds(
    LatLng(bounds.southwest().latitude(), bounds.southwest().longitude()),
    LatLng(bounds.northeast().latitude(), bounds.northeast().longitude()),
  )

private fun latLng(value: MaplibreNativeC.mln_lat_lng): LatLng =
  LatLng(value.latitude(), value.longitude())

private fun cStringLength(pointer: BytePointer?): Long {
  if (pointer == null || pointer.isNull) {
    return 0
  }
  var length = 0L
  while (pointer.get(length) != 0.toByte()) {
    length++
  }
  return length
}

/**
 * Byte offsets inside one `mln_runtime_event` record, derived once from the loaded library.
 *
 * Every offset comes from writing a sentinel through a generated typed setter and finding where it
 * landed in the record's raw bytes, so a reordered or repadded field moves this table with it
 * rather than misdecoding. A hand-written constant could not do that.
 */
private object EventLayout {
  private const val INT_SENTINEL = 0x5A4B3C2D
  private const val LONG_SENTINEL = 0x5A4B3C2D1E0F7788L

  // Two records, so the stride comes from the distance between them.
  private val records = MaplibreNativeC.mln_runtime_event(2)
  private val record: MaplibreNativeC.mln_runtime_event = records.getPointer(0)
  private val stride = records.getPointer(1).address() - records.address()
  private val bytes =
    BytePointer(records).capacity(2L * stride).asByteBuffer().order(ByteOrder.nativeOrder())

  val EVENT_SIZE: Int = Math.toIntExact(stride)
  val BATCH_SIZE: Int = MaplibreNativeC.mln_runtime_event_batch().use { it.sizeof() }

  val TYPE: Int = probeInt { it.type(INT_SENTINEL) }
  val SOURCE_TYPE: Int = probeInt { it.source_type(INT_SENTINEL) }
  val SOURCE: Int = probeLong { it.source(LONG_SENTINEL) }
  val CODE: Int = probeInt { it.code(INT_SENTINEL) }
  val PAYLOAD_TYPE: Int = probeInt { it.payload_type(INT_SENTINEL) }
  val MESSAGE_OFFSET: Int = probeInt { it.message_offset(INT_SENTINEL) }
  val MESSAGE_SIZE: Int = probeInt { it.message_size(INT_SENTINEL) }

  val RENDER_FRAME_MODE: Int = probeInt { it.payload().render_frame().mode(INT_SENTINEL) }
  val RENDER_FRAME_NEEDS_REPAINT: Int = probeFlag {
    it.payload().render_frame().needs_repaint(true)
  }
  val RENDER_FRAME_PLACEMENT_CHANGED: Int = probeFlag {
    it.payload().render_frame().placement_changed(true)
  }
  val STATS_ENCODING_TIME: Int = probeDouble {
    it.payload().render_frame().stats().encoding_time(SENTINEL_DOUBLE)
  }
  val STATS_RENDERING_TIME: Int = probeDouble {
    it.payload().render_frame().stats().rendering_time(SENTINEL_DOUBLE)
  }
  val STATS_FRAME_COUNT: Int = probeLong {
    it.payload().render_frame().stats().frame_count(LONG_SENTINEL)
  }
  val STATS_DRAW_CALL_COUNT: Int = probeLong {
    it.payload().render_frame().stats().draw_call_count(LONG_SENTINEL)
  }
  val STATS_TOTAL_DRAW_CALL_COUNT: Int = probeLong {
    it.payload().render_frame().stats().total_draw_call_count(LONG_SENTINEL)
  }

  val RENDER_MAP_MODE: Int = probeInt { it.payload().render_map().mode(INT_SENTINEL) }

  val TILE_ACTION_OPERATION: Int = probeInt { it.payload().tile_action().operation(INT_SENTINEL) }
  val TILE_ID_OVERSCALED_Z: Int = probeInt {
    it.payload().tile_action().tile_id().overscaled_z(INT_SENTINEL)
  }
  val TILE_ID_WRAP: Int = probeInt { it.payload().tile_action().tile_id().wrap(INT_SENTINEL) }
  val TILE_ID_CANONICAL_Z: Int = probeInt {
    it.payload().tile_action().tile_id().canonical_z(INT_SENTINEL)
  }
  val TILE_ID_CANONICAL_X: Int = probeInt {
    it.payload().tile_action().tile_id().canonical_x(INT_SENTINEL)
  }
  val TILE_ID_CANONICAL_Y: Int = probeInt {
    it.payload().tile_action().tile_id().canonical_y(INT_SENTINEL)
  }

  val REGION_STATUS_REGION_ID: Int = probeLong {
    it.payload().offline_region_status().region_id(LONG_SENTINEL)
  }
  val REGION_STATUS_DOWNLOAD_STATE: Int = probeInt {
    it.payload().offline_region_status().status().download_state(INT_SENTINEL)
  }
  val REGION_STATUS_COMPLETED_RESOURCE_COUNT: Int = probeLong {
    it.payload().offline_region_status().status().completed_resource_count(LONG_SENTINEL)
  }
  val REGION_STATUS_COMPLETED_RESOURCE_SIZE: Int = probeLong {
    it.payload().offline_region_status().status().completed_resource_size(LONG_SENTINEL)
  }
  val REGION_STATUS_COMPLETED_TILE_COUNT: Int = probeLong {
    it.payload().offline_region_status().status().completed_tile_count(LONG_SENTINEL)
  }
  val REGION_STATUS_REQUIRED_TILE_COUNT: Int = probeLong {
    it.payload().offline_region_status().status().required_tile_count(LONG_SENTINEL)
  }
  val REGION_STATUS_COMPLETED_TILE_SIZE: Int = probeLong {
    it.payload().offline_region_status().status().completed_tile_size(LONG_SENTINEL)
  }
  val REGION_STATUS_REQUIRED_RESOURCE_COUNT: Int = probeLong {
    it.payload().offline_region_status().status().required_resource_count(LONG_SENTINEL)
  }
  val REGION_STATUS_COUNT_IS_PRECISE: Int = probeFlag {
    it.payload().offline_region_status().status().required_resource_count_is_precise(true)
  }
  val REGION_STATUS_COMPLETE: Int = probeFlag {
    it.payload().offline_region_status().status().complete(true)
  }

  val RESPONSE_ERROR_REGION_ID: Int = probeLong {
    it.payload().offline_region_response_error().region_id(LONG_SENTINEL)
  }
  val RESPONSE_ERROR_REASON: Int = probeInt {
    it.payload().offline_region_response_error().reason(INT_SENTINEL)
  }

  val TILE_COUNT_LIMIT_REGION_ID: Int = probeLong {
    it.payload().offline_region_tile_count_limit().region_id(LONG_SENTINEL)
  }
  // JavaCPP renames this field, because `limit` would hide Pointer.limit.
  val TILE_COUNT_LIMIT_LIMIT: Int = probeLong {
    it.payload().offline_region_tile_count_limit()._limit(LONG_SENTINEL)
  }

  val OPERATION_ID: Int = probeLong {
    it.payload().offline_operation_completed().operation_id(LONG_SENTINEL)
  }
  val OPERATION_KIND: Int = probeInt {
    it.payload().offline_operation_completed().operation_kind(INT_SENTINEL)
  }
  val OPERATION_RESULT_KIND: Int = probeInt {
    it.payload().offline_operation_completed().result_kind(INT_SENTINEL)
  }
  val OPERATION_RESULT_STATUS: Int = probeInt {
    it.payload().offline_operation_completed().result_status(INT_SENTINEL)
  }
  val OPERATION_FOUND: Int = probeFlag { it.payload().offline_operation_completed().found(true) }

  val TRANSITION_ID: Int = probeLong {
    it.payload().camera_transition_finished().transition_id(LONG_SENTINEL)
  }

  /**
   * Offset of the inline payload union, taken as the lowest offset any payload member landed on.
   * The payload is the last member of the record, so this bounds the window of a payload kind this
   * version does not name.
   */
  val PAYLOAD: Int =
    minOf(
      RENDER_FRAME_MODE,
      RENDER_MAP_MODE,
      TILE_ACTION_OPERATION,
      REGION_STATUS_REGION_ID,
      RESPONSE_ERROR_REGION_ID,
      TILE_COUNT_LIMIT_REGION_ID,
      OPERATION_ID,
      TRANSITION_ID,
    )

  private val SENTINEL_DOUBLE: Double
    get() = Double.fromBits(LONG_SENTINEL)

  private fun probeInt(write: (MaplibreNativeC.mln_runtime_event) -> Unit): Int =
    probe(Int.SIZE_BYTES, write) { bytes.getInt(it) == INT_SENTINEL }

  private fun probeLong(write: (MaplibreNativeC.mln_runtime_event) -> Unit): Int =
    probe(Long.SIZE_BYTES, write) { bytes.getLong(it) == LONG_SENTINEL }

  private fun probeDouble(write: (MaplibreNativeC.mln_runtime_event) -> Unit): Int =
    probe(Long.SIZE_BYTES, write) { bytes.getDouble(it) == SENTINEL_DOUBLE }

  private fun probeFlag(write: (MaplibreNativeC.mln_runtime_event) -> Unit): Int =
    probe(Byte.SIZE_BYTES, write) { bytes.get(it) != 0.toByte() }

  /**
   * Writes one sentinel into a zeroed record and returns the aligned offset it landed on.
   *
   * The second record stays zeroed, so a write past the first record's end is caught here rather
   * than by a misdecoded event.
   */
  private fun probe(
    alignment: Int,
    write: (MaplibreNativeC.mln_runtime_event) -> Unit,
    matches: (Int) -> Boolean,
  ): Int {
    Pointer.memset(records, 0, 2L * stride)
    write(record)
    var offset = 0
    var found = -1
    while (offset + alignment <= EVENT_SIZE) {
      if (matches(offset)) {
        check(found < 0) { "Sentinel found at both offset $found and offset $offset" }
        found = offset
      }
      offset += alignment
    }
    check(found >= 0) { "Sentinel landed outside the first $EVENT_SIZE-byte event record" }
    return found
  }
}

private class RuntimeOptionsScope(options: RuntimeOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_runtime_options = MaplibreNativeC.mln_runtime_options_default()

  private val assetPath = optionalCString(options.assetPath)
  private val cachePath = optionalCString(options.cachePath)

  init {
    this.options.asset_path(assetPath)
    this.options.cache_path(cachePath)
    this.options.event_mask(options.eventMask.nativeValue)
  }

  override fun close() {
    assetPath?.close()
    cachePath?.close()
    options.close()
  }
}

private fun optionalCString(value: String?): BytePointer? = value?.let {
  JavaCppSupport.cString(it)
}

private class OfflineRegionDefinitionScope(value: OfflineRegionDefinition) : AutoCloseable {
  private val owned = mutableListOf<Pointer>()
  private val closeables = mutableListOf<AutoCloseable>()

  val definition: MaplibreNativeC.mln_offline_region_definition =
    own(MaplibreNativeC.mln_offline_region_definition())

  init {
    definition.size(definition.sizeof())
    when (value) {
      is OfflineRegionDefinition.TilePyramid -> {
        definition.type(MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID)
        definition.data_tile_pyramid(tilePyramid(value))
      }
      is OfflineRegionDefinition.GeometryRegion -> {
        definition.type(MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY)
        definition.data_geometry(geometry(value))
      }
      is OfflineRegionDefinition.Unknown ->
        throw Status.invalidArgument("unknown offline region definitions cannot be used as input")
    }
  }

  override fun close() {
    for (index in closeables.lastIndex downTo 0) {
      closeables[index].close()
    }
    for (index in owned.lastIndex downTo 0) {
      owned[index].close()
    }
  }

  private fun tilePyramid(
    value: OfflineRegionDefinition.TilePyramid
  ): MaplibreNativeC.mln_offline_tile_pyramid_region_definition {
    val out = own(MaplibreNativeC.mln_offline_tile_pyramid_region_definition())
    out.size(out.sizeof())
    out.style_url(utf8(value.styleUrl))
    out.bounds(bounds(value.bounds))
    out.min_zoom(value.minZoom)
    out.max_zoom(value.maxZoom)
    out.pixel_ratio(value.pixelRatio)
    out.include_ideographs(value.includeIdeographs)
    return out
  }

  private fun geometry(
    value: OfflineRegionDefinition.GeometryRegion
  ): MaplibreNativeC.mln_offline_geometry_region_definition {
    val out = own(MaplibreNativeC.mln_offline_geometry_region_definition())
    val geometry = ByteArrayViewScope(value.geometryTransit)
    closeables += geometry
    out.size(out.sizeof())
    out.style_url(utf8(value.styleUrl))
    out.geometry(geometry.view)
    out.min_zoom(value.minZoom)
    out.max_zoom(value.maxZoom)
    out.pixel_ratio(value.pixelRatio)
    out.include_ideographs(value.includeIdeographs)
    return out
  }

  private fun bounds(value: LatLngBounds): MaplibreNativeC.mln_lat_lng_bounds {
    val out = own(MaplibreNativeC.mln_lat_lng_bounds())
    out.southwest().latitude(value.southwest.latitude)
    out.southwest().longitude(value.southwest.longitude)
    out.northeast().latitude(value.northeast.latitude)
    out.northeast().longitude(value.northeast.longitude)
    return out
  }

  private fun utf8(value: String): BytePointer {
    return own(JavaCppSupport.cString(value))
  }

  private fun <T : Pointer> own(pointer: T): T {
    owned.add(pointer)
    return pointer
  }
}

/** Direct test seam for the JavaCPP offline and runtime-event adapter. */
internal object JavaCppRuntimeStructs {
  fun offlineRegionDefinitionRoundTrip(value: OfflineRegionDefinition): OfflineRegionDefinition =
    OfflineRegionDefinitionScope(value).use { offlineRegionDefinition(it.definition) }

  fun offlineRegionInfoSnapshot(
    id: Long,
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineRegionInfo =
    OfflineRegionDefinitionScope(definition).use { nativeDefinition ->
      BytePointer(Math.max(metadata.size, 1).toLong()).use { nativeMetadata ->
        if (metadata.isNotEmpty()) nativeMetadata.put(metadata, 0, metadata.size)
        MaplibreNativeC.mln_offline_region_info().use { info ->
          info.id(id)
          info.definition(nativeDefinition.definition)
          info.metadata(if (metadata.isEmpty()) null else nativeMetadata)
          info.metadata_size(metadata.size.toLong())
          offlineRegionInfo(info)
        }
      }
    }

  /**
   * Decodes a payload window of [bytes], for tests that synthesize a payload kind this version
   * cannot queue. The synthetic record ends at the payload, so the window is [bytes] alone.
   */
  fun unknownRuntimePayload(type: Int, bytes: ByteArray): RuntimeEventPayload {
    val record = ByteArray(EventLayout.PAYLOAD + bytes.size)
    bytes.copyInto(record, EventLayout.PAYLOAD)
    val events = ByteBuffer.wrap(record).order(ByteOrder.nativeOrder())
    return runtimeEventPayload(type, events, 0, record.size)
  }

  fun offlineRegionListCleanupAfterCopyFailure(): Int {
    var destroys = 0
    try {
      offlineRegionList(
        1L,
        counter = { _, outCount ->
          outCount.put(Long.MAX_VALUE)
          org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode
        },
        getter = { _, _, _ -> org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode },
        destroyer = { destroys++ },
      )
    } catch (_: ArithmeticException) {
      return destroys
    }
    error("offline list conversion unexpectedly succeeded")
  }
}
