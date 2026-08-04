package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.TileId
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLngBounds
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineGeometryRegionDefinition
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionDefinition
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionDefinitionType
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineRegionStatus
import org.maplibre.nativeffi.internal.wasm.generated.MlnOfflineTilePyramidRegionDefinition
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderingStats
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEvent
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventCameraTransitionFinished
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventOfflineOperationCompleted
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventOfflineRegionResponseError
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventOfflineRegionStatus
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventOfflineRegionTileCountLimit
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventPayloadType
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventRenderFrame
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventRenderMap
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventStyleImageMissing
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventTileAction
import org.maplibre.nativeffi.internal.wasm.generated.MlnTileId
import org.maplibre.nativeffi.map.RenderingStats
import org.maplibre.nativeffi.map.TileOperation
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.runtime.OfflineOperationKind
import org.maplibre.nativeffi.runtime.OfflineOperationResultKind
import org.maplibre.nativeffi.runtime.RuntimeEvent
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventSourceType
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle

/** Alignment the offline descriptors need, which is their widest member: a double. */
private const val DESCRIPTOR_ALIGN = 8

/** A C string carries no alignment requirement of its own. */
private const val TEXT_ALIGN = 1

/**
 * Places an offline region descriptor into the Emscripten heap, and reads one back.
 *
 * A region definition is a tagged union whose arms point at a style URL and, for the geometry arm,
 * at a whole geometry tree. So it is measured first and written into one arena, the same way
 * [GeometryMarshal] handles the tree it embeds: one acquisition and one release however large the
 * definition turns out to be.
 *
 * Reading is the mirror image, and it copies. Native hands back pointers into storage owned by the
 * snapshot or list the info came from, which the caller destroys as soon as it has read it.
 */
internal object OfflineMarshal {
  /** Bytes [definition] needs, including its style URL and any embedded geometry. */
  fun measureDefinition(definition: OfflineRegionDefinition): Int {
    val payload =
      when (definition) {
        is OfflineRegionDefinition.TilePyramid -> measureText(definition.styleUrl)
        is OfflineRegionDefinition.GeometryRegion ->
          measureText(definition.styleUrl) +
            HeapArena.aligned(
              GeometryMarshal.measure(definition.geometry).toLong(),
              DESCRIPTOR_ALIGN,
            )
        is OfflineRegionDefinition.Unknown -> throw unknownDefinition(definition)
      }
    return bounded(
      HeapArena.aligned(MlnOfflineRegionDefinition.SIZEOF.toLong(), DESCRIPTOR_ALIGN) + payload
    )
  }

  /** Writes [definition] into [arena] and returns the tagged descriptor's address. */
  fun writeDefinition(arena: HeapArena, definition: OfflineRegionDefinition): HeapPointer {
    val base = arena.allocate(MlnOfflineRegionDefinition.SIZEOF, DESCRIPTOR_ALIGN)
    // The leading size field is how the C API versions a descriptor: it carries the size this
    // binding was generated against so native can tell which fields it may read. The arm inside
    // the union carries its own, for the same reason.
    MlnOfflineRegionDefinition.setSize(base, MlnOfflineRegionDefinition.SIZEOF)
    val data = base + MlnOfflineRegionDefinition.OFFSET_DATA
    when (definition) {
      is OfflineRegionDefinition.TilePyramid -> {
        MlnOfflineRegionDefinition.setType(
          base,
          MlnOfflineRegionDefinitionType.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
        )
        MlnOfflineTilePyramidRegionDefinition.setSize(
          data,
          MlnOfflineTilePyramidRegionDefinition.SIZEOF,
        )
        MlnOfflineTilePyramidRegionDefinition.setStyleUrl(
          data,
          writeText(arena, definition.styleUrl),
        )
        writeBounds(data + MlnOfflineTilePyramidRegionDefinition.OFFSET_BOUNDS, definition.bounds)
        MlnOfflineTilePyramidRegionDefinition.setMinZoom(data, definition.minZoom)
        MlnOfflineTilePyramidRegionDefinition.setMaxZoom(data, definition.maxZoom)
        MlnOfflineTilePyramidRegionDefinition.setPixelRatio(data, definition.pixelRatio)
        MlnOfflineTilePyramidRegionDefinition.setIncludeIdeographs(
          data,
          definition.includeIdeographs,
        )
      }
      is OfflineRegionDefinition.GeometryRegion -> {
        MlnOfflineRegionDefinition.setType(
          base,
          MlnOfflineRegionDefinitionType.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
        )
        MlnOfflineGeometryRegionDefinition.setSize(data, MlnOfflineGeometryRegionDefinition.SIZEOF)
        MlnOfflineGeometryRegionDefinition.setStyleUrl(data, writeText(arena, definition.styleUrl))
        MlnOfflineGeometryRegionDefinition.setGeometry(
          data,
          GeometryMarshal.write(arena, definition.geometry),
        )
        MlnOfflineGeometryRegionDefinition.setMinZoom(data, definition.minZoom)
        MlnOfflineGeometryRegionDefinition.setMaxZoom(data, definition.maxZoom)
        MlnOfflineGeometryRegionDefinition.setPixelRatio(data, definition.pixelRatio)
        MlnOfflineGeometryRegionDefinition.setIncludeIdeographs(data, definition.includeIdeographs)
      }
      is OfflineRegionDefinition.Unknown -> throw unknownDefinition(definition)
    }
    return base
  }

  /**
   * Writes the region-info header alone, for a buffer native fills.
   *
   * An output descriptor still states its size: native reads it to decide whether it may write the
   * fields this binding expects, and refuses a zeroed block outright.
   */
  fun writeRegionInfoHeader(base: HeapPointer) {
    MlnOfflineRegionInfo.setSize(base, MlnOfflineRegionInfo.SIZEOF)
  }

  /** Reads the region info at [base], copying its metadata out of snapshot-owned storage. */
  fun readRegionInfo(base: HeapPointer): OfflineRegionInfo =
    OfflineRegionInfo(
      MlnOfflineRegionInfo.id(base),
      readDefinition(base + MlnOfflineRegionInfo.OFFSET_DEFINITION),
      readBytes(MlnOfflineRegionInfo.metadata(base), MlnOfflineRegionInfo.metadataSize(base)),
    )

  /** Writes the region-status header alone, for a buffer native fills. */
  fun writeStatusHeader(base: HeapPointer) {
    MlnOfflineRegionStatus.setSize(base, MlnOfflineRegionStatus.SIZEOF)
  }

  /** Reads the region status at [base]. Every field is a value, so nothing here borrows. */
  fun readStatus(base: HeapPointer): OfflineRegionStatus =
    OfflineRegionStatus(
      OfflineRegionDownloadState.fromNative(MlnOfflineRegionStatus.downloadState(base)),
      MlnOfflineRegionStatus.completedResourceCount(base),
      MlnOfflineRegionStatus.completedResourceSize(base),
      MlnOfflineRegionStatus.completedTileCount(base),
      MlnOfflineRegionStatus.requiredTileCount(base),
      MlnOfflineRegionStatus.completedTileSize(base),
      MlnOfflineRegionStatus.requiredResourceCount(base),
      MlnOfflineRegionStatus.requiredResourceCountIsPrecise(base),
      MlnOfflineRegionStatus.complete(base),
    )

  /**
   * Reads the tagged definition at [base].
   *
   * A tag this binding does not name is preserved rather than guessed at: the arm behind it has an
   * unknown shape, so reading any field of it would be reading at an offset that means nothing.
   */
  private fun readDefinition(base: HeapPointer): OfflineRegionDefinition {
    val data = base + MlnOfflineRegionDefinition.OFFSET_DATA
    return when (val type = MlnOfflineRegionDefinition.type(base)) {
      MlnOfflineRegionDefinitionType.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID ->
        OfflineRegionDefinition.TilePyramid(
          Heap.loadUtf8(MlnOfflineTilePyramidRegionDefinition.styleUrl(data)),
          readBounds(data + MlnOfflineTilePyramidRegionDefinition.OFFSET_BOUNDS),
          MlnOfflineTilePyramidRegionDefinition.minZoom(data),
          MlnOfflineTilePyramidRegionDefinition.maxZoom(data),
          MlnOfflineTilePyramidRegionDefinition.pixelRatio(data),
          MlnOfflineTilePyramidRegionDefinition.includeIdeographs(data),
        )
      MlnOfflineRegionDefinitionType.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY ->
        OfflineRegionDefinition.GeometryRegion(
          Heap.loadUtf8(MlnOfflineGeometryRegionDefinition.styleUrl(data)),
          GeometryMarshal.read(MlnOfflineGeometryRegionDefinition.geometry(data), 0),
          MlnOfflineGeometryRegionDefinition.minZoom(data),
          MlnOfflineGeometryRegionDefinition.maxZoom(data),
          MlnOfflineGeometryRegionDefinition.pixelRatio(data),
          MlnOfflineGeometryRegionDefinition.includeIdeographs(data),
        )
      else -> OfflineRegionDefinition.Unknown(type, MlnOfflineRegionDefinition.size(base))
    }
  }

  private fun writeBounds(base: HeapPointer, bounds: LatLngBounds) {
    CameraMarshal.writeLatLng(base + MlnLatLngBounds.OFFSET_SOUTHWEST, bounds.southwest)
    CameraMarshal.writeLatLng(base + MlnLatLngBounds.OFFSET_NORTHEAST, bounds.northeast)
  }

  private fun readBounds(base: HeapPointer): LatLngBounds =
    LatLngBounds(
      CameraMarshal.readLatLng(base + MlnLatLngBounds.OFFSET_SOUTHWEST),
      CameraMarshal.readLatLng(base + MlnLatLngBounds.OFFSET_NORTHEAST),
    )

  private fun measureText(text: String): Long =
    HeapArena.aligned(Heap.utf8Size(text).toLong(), DESCRIPTOR_ALIGN)

  private fun writeText(arena: HeapArena, text: String): HeapPointer {
    val bytes = Heap.utf8Size(text)
    val pointer = arena.allocate(bytes, TEXT_ALIGN)
    Heap.storeUtf8(pointer, text)
    return pointer
  }

  private fun unknownDefinition(definition: OfflineRegionDefinition.Unknown) =
    Status.invalidArgument(
      "An offline region definition of unknown native type ${definition.rawType} cannot be sent " +
        "to native; it was read from a tag this binding does not recognise."
    )

  /** Converts a measured size to the count a 32-bit pointer can address, or refuses it. */
  private fun bounded(bytes: Long): Int {
    Status.requireArgument(bytes in 1..Int.MAX_VALUE.toLong()) {
      "an offline region definition of $bytes bytes cannot be addressed on this target"
    }
    return bytes.toInt()
  }
}

/**
 * Copies one runtime event out of the queue.
 *
 * Every string and payload the C API reports here points into runtime-owned storage that the next
 * poll for the same runtime overwrites, so nothing may be left borrowed: the public event has to be
 * whole by the time this returns. That is the whole reason this reads eagerly rather than exposing
 * lazy accessors over the descriptor.
 *
 * It lives beside the offline marshalling because two of its payloads are offline descriptors and a
 * third reports an offline operation's outcome.
 */
internal object RuntimeEventMarshal {
  /** Writes the event header alone, for the buffer native fills on each poll. */
  fun writeHeader(base: HeapPointer) {
    MlnRuntimeEvent.setSize(base, MlnRuntimeEvent.SIZEOF)
  }

  /**
   * Reads the event at [base], attributing it to whichever handle raised it.
   *
   * A map-originated event names its map by native id, which the runtime resolves against the maps
   * it still holds. An id names one map for the life of the process, so a lookup that misses means
   * the map has been closed rather than that the wrong one might be found; the public contract
   * already allows a null map for exactly that case.
   */
  fun readEvent(base: HeapPointer, runtime: RuntimeHandle): RuntimeEvent {
    val sourceType = RuntimeEventSourceType.fromNative(MlnRuntimeEvent.sourceType(base))
    val source = MlnRuntimeEvent.source(base)
    return RuntimeEvent(
      RuntimeEventType.fromNative(MlnRuntimeEvent.type(base)),
      sourceType,
      runtime.takeIf { sourceType == RuntimeEventSourceType.RUNTIME },
      if (sourceType == RuntimeEventSourceType.MAP && source != 0L) runtime.liveMap(source)
      else null,
      MlnRuntimeEvent.code(base),
      readPayload(
        MlnRuntimeEvent.payloadType(base),
        MlnRuntimeEvent.payload(base),
        MlnRuntimeEvent.payloadSize(base),
      ),
      readText(MlnRuntimeEvent.message(base), MlnRuntimeEvent.messageSize(base)),
    )
  }

  /**
   * Reads the payload [payloadType] selects.
   *
   * A payload shorter than the struct this binding was generated against is read as unknown rather
   * than field by field: the fields past the reported size belong to a module built from other
   * headers, and reading them would report whatever the runtime's storage held there.
   */
  private fun readPayload(
    payloadType: Int,
    payload: HeapPointer,
    payloadSize: Int,
  ): RuntimeEventPayload {
    fun fits(required: Int) = payload.address != 0 && payloadSize >= required
    return when (payloadType) {
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_NONE -> RuntimeEventPayload.None
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME ->
        if (fits(MlnRuntimeEventRenderFrame.SIZEOF)) readRenderFrame(payload)
        else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP ->
        if (fits(MlnRuntimeEventRenderMap.SIZEOF)) {
          RuntimeEventPayload.RenderMap(
            RenderMode.fromNative(MlnRuntimeEventRenderMap.mode(payload))
          )
        } else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING ->
        if (fits(MlnRuntimeEventStyleImageMissing.SIZEOF)) {
          RuntimeEventPayload.StyleImageMissing(
            readText(
              MlnRuntimeEventStyleImageMissing.imageId(payload),
              MlnRuntimeEventStyleImageMissing.imageIdSize(payload),
            )
          )
        } else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION ->
        if (fits(MlnRuntimeEventTileAction.SIZEOF)) readTileAction(payload)
        else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS ->
        if (fits(MlnRuntimeEventOfflineRegionStatus.SIZEOF)) {
          RuntimeEventPayload.OfflineRegionStatusChanged(
            MlnRuntimeEventOfflineRegionStatus.regionId(payload),
            OfflineMarshal.readStatus(payload + MlnRuntimeEventOfflineRegionStatus.OFFSET_STATUS),
          )
        } else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR ->
        if (fits(MlnRuntimeEventOfflineRegionResponseError.SIZEOF)) {
          RuntimeEventPayload.OfflineRegionResponseError(
            MlnRuntimeEventOfflineRegionResponseError.regionId(payload),
            ResourceErrorReason.fromNative(
              MlnRuntimeEventOfflineRegionResponseError.reason(payload)
            ),
          )
        } else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
        if (fits(MlnRuntimeEventOfflineRegionTileCountLimit.SIZEOF)) {
          RuntimeEventPayload.OfflineRegionTileCountLimit(
            MlnRuntimeEventOfflineRegionTileCountLimit.regionId(payload),
            MlnRuntimeEventOfflineRegionTileCountLimit.limit(payload),
          )
        } else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED ->
        if (fits(MlnRuntimeEventOfflineOperationCompleted.SIZEOF)) {
          readOperationCompleted(payload)
        } else readUnknown(payloadType, payload, payloadSize)
      MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED ->
        if (fits(MlnRuntimeEventCameraTransitionFinished.SIZEOF)) {
          RuntimeEventPayload.CameraTransitionFinished(
            MlnRuntimeEventCameraTransitionFinished.transitionId(payload)
          )
        } else readUnknown(payloadType, payload, payloadSize)
      else -> readUnknown(payloadType, payload, payloadSize)
    }
  }

  private fun readRenderFrame(payload: HeapPointer): RuntimeEventPayload.RenderFrame {
    val stats = payload + MlnRuntimeEventRenderFrame.OFFSET_STATS
    return RuntimeEventPayload.RenderFrame(
      RenderMode.fromNative(MlnRuntimeEventRenderFrame.mode(payload)),
      MlnRuntimeEventRenderFrame.needsRepaint(payload),
      MlnRuntimeEventRenderFrame.placementChanged(payload),
      RenderingStats(
        MlnRenderingStats.encodingTime(stats),
        MlnRenderingStats.renderingTime(stats),
        MlnRenderingStats.frameCount(stats),
        MlnRenderingStats.drawCallCount(stats),
        MlnRenderingStats.totalDrawCallCount(stats),
      ),
    )
  }

  private fun readTileAction(payload: HeapPointer): RuntimeEventPayload.TileAction {
    val tile = payload + MlnRuntimeEventTileAction.OFFSET_TILE_ID
    return RuntimeEventPayload.TileAction(
      TileOperation.fromNative(MlnRuntimeEventTileAction.operation(payload)),
      TileId(
        // Zoom and tile coordinates are unsigned in C, so the widening cannot go through Int:
        // a high-bit value would arrive as a negative zoom.
        unsigned(MlnTileId.overscaledZ(tile)),
        MlnTileId.wrap(tile),
        unsigned(MlnTileId.canonicalZ(tile)),
        unsigned(MlnTileId.canonicalX(tile)),
        unsigned(MlnTileId.canonicalY(tile)),
      ),
      readText(
        MlnRuntimeEventTileAction.sourceId(payload),
        MlnRuntimeEventTileAction.sourceIdSize(payload),
      ),
    )
  }

  private fun readOperationCompleted(
    payload: HeapPointer
  ): RuntimeEventPayload.OfflineOperationCompleted =
    RuntimeEventPayload.OfflineOperationCompleted(
      MlnRuntimeEventOfflineOperationCompleted.operationId(payload),
      OfflineOperationKind.fromNative(
        MlnRuntimeEventOfflineOperationCompleted.operationKind(payload)
      ),
      OfflineOperationResultKind.fromNative(
        MlnRuntimeEventOfflineOperationCompleted.resultKind(payload)
      ),
      MlnRuntimeEventOfflineOperationCompleted.resultStatus(payload),
      MlnRuntimeEventOfflineOperationCompleted.found(payload),
    )

  private fun readUnknown(
    payloadType: Int,
    payload: HeapPointer,
    payloadSize: Int,
  ): RuntimeEventPayload.Unknown =
    RuntimeEventPayload.Unknown(payloadType, unsigned(payloadSize), readBytes(payload, payloadSize))

  private fun unsigned(value: Int): Long = value.toUInt().toLong()
}

/**
 * Copies [length] bytes of borrowed native storage, tolerating the null span.
 *
 * The C API spells an absent span as a null pointer with a zero length, and a present-but-empty one
 * the same way, so both arrive here and neither is an error.
 */
private fun readBytes(pointer: HeapPointer, length: Int): ByteArray =
  if (pointer.address == 0 || length <= 0) ByteArray(0) else Heap.loadBytes(pointer, length)

/**
 * Copies [length] bytes of borrowed native storage as text.
 *
 * Length-delimited rather than null-delimited because that is what the C API documents: the length
 * excludes a terminator that a payload is not obliged to carry.
 */
private fun readText(pointer: HeapPointer, length: Int): String =
  readBytes(pointer, length).decodeToString()
