package org.maplibre.nativeffi.runtime

import java.lang.ref.WeakReference
import java.nio.charset.StandardCharsets
import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.PointerPointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.TileId
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.GeometryScope
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.RenderingStats
import org.maplibre.nativeffi.map.TileOperation
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/** Owned runtime handle backed by the Android JNI bridge. */
public actual class RuntimeHandle private constructor(private val handleAddress: Long) :
  AutoCloseable {
  private val core = HandleStateCore("RuntimeHandle", handleAddress)
  private var resourceProviderState: ResourceProviderState? = null
  private var resourceTransformState: ResourceTransformState? = null
  private val liveMaps = mutableMapOf<Long, WeakReference<MapHandle>>()

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runOnce() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_runtime_run_once(runtime(requireLiveAddress())))
  }

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> {
    NativeAccess.ensureLoaded()
    val outOperationId = longArrayOf(0L)
    Status.check(
      MaplibreNativeC.mln_runtime_run_ambient_cache_operation_start(
        runtime(requireLiveAddress()),
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

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> {
    val outOperationId = longArrayOf(0L)
    OfflineRegionDefinitionScope(definition).use { nativeDefinition ->
      Status.check(
        MaplibreNativeC.mln_runtime_offline_region_create_start(
          runtime(requireLiveAddress()),
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
        runtime(requireLiveAddress()),
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
        runtime(requireLiveAddress()),
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
        runtime(requireLiveAddress()),
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
        runtime(requireLiveAddress()),
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
    PointerPointer<MaplibreNativeC.mln_offline_region_snapshot>(1).use { outSnapshot ->
      outSnapshot.put(0, null as Pointer?)
      BoolPointer(1).use { outFound ->
        Status.check(
          MaplibreNativeC.mln_runtime_offline_region_get_take_result(
            runtime(requireLiveAddress()),
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
          runtime(requireLiveAddress()),
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
          runtime(requireLiveAddress()),
          replacement.descriptor(),
        )
      )
      previous = resourceProviderState
      resourceProviderState = replacement
    } catch (error: Throwable) {
      closeAndSuppress(error, replacement)
      throw error
    }
    closeQuietly(previous)
  }

  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    val replacement = ResourceTransformState(callback)
    val previous: ResourceTransformState?
    try {
      resourceTransformState?.checkCanClose()
      Status.check(
        MaplibreNativeC.mln_runtime_set_resource_transform(
          runtime(requireLiveAddress()),
          replacement.descriptor(),
        )
      )
      previous = resourceTransformState
      resourceTransformState = replacement
    } catch (error: Throwable) {
      closeAndSuppress(error, replacement)
      throw error
    }
    closeQuietly(previous)
  }

  public actual fun clearResourceTransform() {
    resourceTransformState?.checkCanClose()
    Status.check(
      MaplibreNativeC.mln_runtime_clear_resource_transform(runtime(requireLiveAddress()))
    )
    val previous = resourceTransformState
    resourceTransformState = null
    closeQuietly(previous)
  }

  public actual fun pollEvent(): RuntimeEvent? {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_runtime_event().use { event ->
      event.size(event.sizeof())
      val hasEvent = booleanArrayOf(false)
      Status.check(
        MaplibreNativeC.mln_runtime_poll_event(runtime(requireLiveAddress()), event, hasEvent)
      )
      return if (hasEvent[0]) runtimeEvent(event) else null
    }
  }

  public actual override fun close() {
    resourceProviderState?.checkCanClose()
    resourceTransformState?.checkCanClose()
    core.closeOnce(
      destroy = { MaplibreNativeC.mln_runtime_destroy(runtime(handleAddress)) },
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
      RuntimeOptionsScope(options).use { nativeOptions ->
        val outRuntime = PointerPointer<MaplibreNativeC.mln_runtime>(1)
        outRuntime.put(0, null as Pointer?)
        Status.check(MaplibreNativeC.mln_runtime_create(nativeOptions.options, outRuntime))
        val runtime = outRuntime.get(MaplibreNativeC.mln_runtime::class.java, 0)
        val address = if (runtime == null || runtime.isNull) 0L else runtime.address()
        require(address != 0L) { "mln_runtime_create returned a null runtime" }
        return RuntimeHandle(address)
      }
    }
  }

  private fun <T> offlineOperation(
    operationId: Long,
    kind: OfflineOperationKind,
    resultKind: OfflineOperationResultKind,
  ): OfflineOperationHandle<T> = OfflineOperationHandle(this, operationId, kind, resultKind)

  private fun startOfflineOperation(start: (MaplibreNativeC.mln_runtime, LongArray) -> Int): Long {
    val outOperationId = longArrayOf(0L)
    Status.check(start(runtime(requireLiveAddress()), outOperationId))
    return outOperationId[0]
  }

  private fun startOfflineLongOperation(
    value: Long,
    start: (MaplibreNativeC.mln_runtime, Long, LongArray) -> Int,
  ): Long {
    val outOperationId = longArrayOf(0L)
    Status.check(start(runtime(requireLiveAddress()), value, outOperationId))
    return outOperationId[0]
  }

  internal fun discardOfflineOperation(operation: OfflineOperationHandle<*>) {
    if (operation.isClosed) return
    val operationId = operation.requireLive(this)
    val runtimeAddress =
      try {
        requireLiveAddress()
      } catch (error: InvalidStateException) {
        operation.markConsumed()
        throw error
      }
    Status.check(
      MaplibreNativeC.mln_runtime_offline_operation_discard(runtime(runtimeAddress), operationId)
    )
    operation.markConsumed()
  }

  internal fun retainChild(): HandleStateCore.ChildRetention = core.retainChild()

  internal fun nativeAddress(): Long = requireLiveAddress()

  internal fun registerMap(map: MapHandle) {
    liveMaps[map.nativeAddress()] = WeakReference(map)
  }

  internal fun unregisterMap(map: MapHandle) {
    val address = map.nativeAddress()
    if (liveMaps[address]?.get() === map) {
      liveMaps.remove(address)
    }
  }

  private fun requireLiveAddress(): Long {
    core.requireLive()
    return handleAddress
  }

  private fun runtimeEvent(event: MaplibreNativeC.mln_runtime_event): RuntimeEvent {
    val sourceType = RuntimeEventSourceType.fromNative(event.source_type())
    val mapSource = if (sourceType == RuntimeEventSourceType.MAP) mapFor(event.source()) else null
    val eventType = RuntimeEventType.fromNative(event.type())
    if (eventType == RuntimeEventType.MAP_STYLE_LOADED) {
      mapSource?.releaseDetachedCustomGeometrySources()
    }
    return RuntimeEvent(
      eventType,
      sourceType,
      if (sourceType == RuntimeEventSourceType.RUNTIME) this else null,
      mapSource,
      event.code(),
      runtimeEventPayload(event),
      byteString(event.message(), event.message_size()),
    )
  }

  private fun mapFor(source: Pointer?): MapHandle? {
    val address = source?.address() ?: return null
    if (address == 0L) return null
    val reference = liveMaps[address] ?: return null
    val map = reference.get()
    if (map == null) {
      liveMaps.remove(address)
    }
    return map
  }

  private fun runtimeEventPayload(event: MaplibreNativeC.mln_runtime_event): RuntimeEventPayload {
    val payloadType = event.payload_type()
    val payloadBytes = payloadBytes(event)
    return when (payloadType) {
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_NONE -> RuntimeEventPayload.None
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME ->
        if (hasPayloadSize(event, PayloadSizes.RENDER_FRAME)) {
          renderFramePayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP ->
        if (hasPayloadSize(event, PayloadSizes.RENDER_MAP)) {
          renderMapPayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING ->
        if (hasPayloadSize(event, PayloadSizes.STYLE_IMAGE_MISSING)) {
          styleImageMissingPayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION ->
        if (hasPayloadSize(event, PayloadSizes.TILE_ACTION)) {
          tileActionPayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS ->
        if (hasPayloadSize(event, PayloadSizes.OFFLINE_REGION_STATUS)) {
          offlineRegionStatusPayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR ->
        if (hasPayloadSize(event, PayloadSizes.OFFLINE_REGION_RESPONSE_ERROR)) {
          offlineRegionResponseErrorPayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
        if (hasPayloadSize(event, PayloadSizes.OFFLINE_REGION_TILE_COUNT_LIMIT)) {
          offlineRegionTileCountLimitPayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      MaplibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED ->
        if (hasPayloadSize(event, PayloadSizes.OFFLINE_OPERATION_COMPLETED)) {
          offlineOperationCompletedPayload(event.payload())
        } else {
          RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
        }
      else -> RuntimeEventPayload.Unknown(payloadType, event.payload_size(), payloadBytes)
    }
  }

  private fun takeOfflineRegionSnapshot(
    operationId: Long,
    take:
      (
        MaplibreNativeC.mln_runtime,
        Long,
        PointerPointer<MaplibreNativeC.mln_offline_region_snapshot>,
      ) -> Int,
  ): OfflineRegionInfo =
    PointerPointer<MaplibreNativeC.mln_offline_region_snapshot>(1).use { outSnapshot ->
      outSnapshot.put(0, null as Pointer?)
      Status.check(take(runtime(requireLiveAddress()), operationId, outSnapshot))
      offlineRegionSnapshot(outSnapshot)
    }

  private fun takeOfflineRegionList(
    operationId: Long,
    take:
      (
        MaplibreNativeC.mln_runtime, Long, PointerPointer<MaplibreNativeC.mln_offline_region_list>,
      ) -> Int,
  ): List<OfflineRegionInfo> =
    PointerPointer<MaplibreNativeC.mln_offline_region_list>(1).use { outList ->
      outList.put(0, null as Pointer?)
      Status.check(take(runtime(requireLiveAddress()), operationId, outList))
      offlineRegionList(outList)
    }
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

private fun hasPayloadSize(event: MaplibreNativeC.mln_runtime_event, requiredSize: Long): Boolean =
  event.payload() != null && !event.payload().isNull && event.payload_size() >= requiredSize

private fun payloadBytes(event: MaplibreNativeC.mln_runtime_event): ByteArray =
  byteArray(event.payload(), event.payload_size())

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

private fun renderFramePayload(payload: Pointer): RuntimeEventPayload.RenderFrame {
  val frame = MaplibreNativeC.mln_runtime_event_render_frame(payload)
  return RuntimeEventPayload.RenderFrame(
    RenderMode.fromNative(frame.mode()),
    frame.needs_repaint(),
    frame.placement_changed(),
    RenderingStats(
      frame.stats().encoding_time(),
      frame.stats().rendering_time(),
      frame.stats().frame_count(),
      frame.stats().draw_call_count(),
      frame.stats().total_draw_call_count(),
    ),
  )
}

private fun renderMapPayload(payload: Pointer): RuntimeEventPayload.RenderMap =
  RuntimeEventPayload.RenderMap(
    RenderMode.fromNative(MaplibreNativeC.mln_runtime_event_render_map(payload).mode())
  )

private fun styleImageMissingPayload(payload: Pointer): RuntimeEventPayload.StyleImageMissing {
  val missing = MaplibreNativeC.mln_runtime_event_style_image_missing(payload)
  return RuntimeEventPayload.StyleImageMissing(
    byteString(missing.image_id(), missing.image_id_size())
  )
}

private fun tileActionPayload(payload: Pointer): RuntimeEventPayload.TileAction {
  val action = MaplibreNativeC.mln_runtime_event_tile_action(payload)
  return RuntimeEventPayload.TileAction(
    TileOperation.fromNative(action.operation()),
    TileId(
      Integer.toUnsignedLong(action.tile_id().overscaled_z()),
      action.tile_id().wrap(),
      Integer.toUnsignedLong(action.tile_id().canonical_z()),
      Integer.toUnsignedLong(action.tile_id().canonical_x()),
      Integer.toUnsignedLong(action.tile_id().canonical_y()),
    ),
    byteString(action.source_id(), action.source_id_size()),
  )
}

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

private fun offlineRegionSnapshot(
  outSnapshot: PointerPointer<MaplibreNativeC.mln_offline_region_snapshot>
): OfflineRegionInfo {
  val snapshot = outSnapshot.get(MaplibreNativeC.mln_offline_region_snapshot::class.java, 0)
  require(snapshot != null && !snapshot.isNull) { "offline operation returned a null snapshot" }
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

private fun offlineRegionList(
  outList: PointerPointer<MaplibreNativeC.mln_offline_region_list>
): List<OfflineRegionInfo> {
  val list = outList.get(MaplibreNativeC.mln_offline_region_list::class.java, 0)
  require(list != null && !list.isNull) { "offline operation returned a null region list" }
  return try {
    SizeTPointer(1).use { outCount ->
      Status.check(MaplibreNativeC.mln_offline_region_list_count(list, outCount))
      val count = Math.toIntExact(outCount.get())
      List(count) { index ->
        MaplibreNativeC.mln_offline_region_info().use { info ->
          info.size(info.sizeof())
          Status.check(MaplibreNativeC.mln_offline_region_list_get(list, index.toLong(), info))
          offlineRegionInfo(info)
        }
      }
    }
  } finally {
    MaplibreNativeC.mln_offline_region_list_destroy(list)
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
    geometry(definition.geometry()),
    definition.min_zoom(),
    definition.max_zoom(),
    definition.pixel_ratio(),
    definition.include_ideographs(),
  )

private fun geometry(value: MaplibreNativeC.mln_geometry): Geometry =
  when (value.type()) {
    MaplibreNativeC.MLN_GEOMETRY_TYPE_EMPTY -> Geometry.Empty
    MaplibreNativeC.MLN_GEOMETRY_TYPE_POINT -> Geometry.Point(latLng(value.data_point()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING ->
      Geometry.LineString(coordinateSpan(value.data_line_string()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_POLYGON -> Geometry.Polygon(polygon(value.data_polygon()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT ->
      Geometry.MultiPoint(coordinateSpan(value.data_multi_point()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING -> {
      val native = value.data_multi_line_string()
      val lines = native.lines()
      Geometry.MultiLineString(
        List(Math.toIntExact(native.line_count())) { index ->
          coordinateSpan(lines.getPointer(index.toLong()))
        }
      )
    }
    MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON -> {
      val native = value.data_multi_polygon()
      val polygons = native.polygons()
      Geometry.MultiPolygon(
        List(Math.toIntExact(native.polygon_count())) { index ->
          polygon(polygons.getPointer(index.toLong()))
        }
      )
    }
    MaplibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION -> {
      val native = value.data_geometry_collection()
      val geometries = native.geometries()
      Geometry.Collection(
        List(Math.toIntExact(native.geometry_count())) { index ->
          geometry(geometries.getPointer(index.toLong()))
        }
      )
    }
    else -> Geometry.Unknown(value.type(), value.size())
  }

private fun coordinateSpan(value: MaplibreNativeC.mln_coordinate_span): List<LatLng> {
  val coordinates = value.coordinates()
  return List(Math.toIntExact(value.coordinate_count())) { index ->
    latLng(coordinates.getPointer(index.toLong()))
  }
}

private fun polygon(value: MaplibreNativeC.mln_polygon_geometry): List<List<LatLng>> {
  val rings = value.rings()
  return List(Math.toIntExact(value.ring_count())) { index ->
    coordinateSpan(rings.getPointer(index.toLong()))
  }
}

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

private fun offlineRegionStatusPayload(
  payload: Pointer
): RuntimeEventPayload.OfflineRegionStatusChanged {
  val status = MaplibreNativeC.mln_runtime_event_offline_region_status(payload)
  return RuntimeEventPayload.OfflineRegionStatusChanged(
    status.region_id(),
    offlineRegionStatus(status.status()),
  )
}

private fun offlineRegionResponseErrorPayload(
  payload: Pointer
): RuntimeEventPayload.OfflineRegionResponseError {
  val error = MaplibreNativeC.mln_runtime_event_offline_region_response_error(payload)
  return RuntimeEventPayload.OfflineRegionResponseError(
    error.region_id(),
    ResourceErrorReason.fromNative(error.reason()),
  )
}

private fun offlineRegionTileCountLimitPayload(
  payload: Pointer
): RuntimeEventPayload.OfflineRegionTileCountLimit {
  val limit = MaplibreNativeC.mln_runtime_event_offline_region_tile_count_limit(payload)
  return RuntimeEventPayload.OfflineRegionTileCountLimit(limit.region_id(), limit.limit())
}

private fun offlineOperationCompletedPayload(
  payload: Pointer
): RuntimeEventPayload.OfflineOperationCompleted {
  val operation = MaplibreNativeC.mln_runtime_event_offline_operation_completed(payload)
  return RuntimeEventPayload.OfflineOperationCompleted(
    operation.operation_id(),
    OfflineOperationKind.fromNative(operation.operation_kind()),
    OfflineOperationResultKind.fromNative(operation.result_kind()),
    operation.result_status(),
    operation.found(),
  )
}

private object PayloadSizes {
  val RENDER_FRAME: Long =
    MaplibreNativeC.mln_runtime_event_render_frame().use { it.sizeof().toLong() }
  val RENDER_MAP: Long = MaplibreNativeC.mln_runtime_event_render_map().use { it.sizeof().toLong() }
  val STYLE_IMAGE_MISSING: Long =
    MaplibreNativeC.mln_runtime_event_style_image_missing().use { it.sizeof().toLong() }
  val TILE_ACTION: Long =
    MaplibreNativeC.mln_runtime_event_tile_action().use { it.sizeof().toLong() }
  val OFFLINE_REGION_STATUS: Long =
    MaplibreNativeC.mln_runtime_event_offline_region_status().use { it.sizeof().toLong() }
  val OFFLINE_REGION_RESPONSE_ERROR: Long =
    MaplibreNativeC.mln_runtime_event_offline_region_response_error().use { it.sizeof().toLong() }
  val OFFLINE_REGION_TILE_COUNT_LIMIT: Long =
    MaplibreNativeC.mln_runtime_event_offline_region_tile_count_limit().use { it.sizeof().toLong() }
  val OFFLINE_OPERATION_COMPLETED: Long =
    MaplibreNativeC.mln_runtime_event_offline_operation_completed().use { it.sizeof().toLong() }
}

private fun runtime(address: Long): MaplibreNativeC.mln_runtime =
  MaplibreNativeC.mln_runtime(AddressPointer(address))

private class AddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}

private class RuntimeOptionsScope(options: RuntimeOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_runtime_options = MaplibreNativeC.mln_runtime_options_default()

  private val assetPath = optionalCString(options.assetPath)
  private val cachePath = optionalCString(options.cachePath)

  init {
    this.options.asset_path(assetPath)
    this.options.cache_path(cachePath)
    options.maximumCacheSize?.let { maximumCacheSize ->
      this.options.flags(
        this.options.flags() or MaplibreNativeC.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE
      )
      this.options.maximum_cache_size(maximumCacheSize)
    }
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
    val geometry = GeometryScope(value.geometry)
    closeables += geometry
    out.size(out.sizeof())
    out.style_url(utf8(value.styleUrl))
    out.geometry(geometry.value)
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
