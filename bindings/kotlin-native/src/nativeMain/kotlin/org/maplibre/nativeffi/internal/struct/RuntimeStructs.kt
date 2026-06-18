package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toCValues
import kotlinx.cinterop.toKString
import kotlinx.cinterop.value
import org.maplibre.nativeffi.geo.TileId
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_NONE
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
import org.maplibre.nativeffi.internal.c.mln_offline_region_definition
import org.maplibre.nativeffi.internal.c.mln_offline_region_info
import org.maplibre.nativeffi.internal.c.mln_offline_region_list_count
import org.maplibre.nativeffi.internal.c.mln_offline_region_list_destroy
import org.maplibre.nativeffi.internal.c.mln_offline_region_list_get
import org.maplibre.nativeffi.internal.c.mln_offline_region_snapshot_destroy
import org.maplibre.nativeffi.internal.c.mln_offline_region_snapshot_get
import org.maplibre.nativeffi.internal.c.mln_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_operation_completed
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_response_error
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_tile_count_limit
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_frame
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_map
import org.maplibre.nativeffi.internal.c.mln_runtime_event_style_image_missing
import org.maplibre.nativeffi.internal.c.mln_runtime_event_tile_action
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
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
import org.maplibre.nativeffi.runtime.RuntimeEventPayload

/** Copies runtime event payloads out of native event storage. */
@OptIn(ExperimentalForeignApi::class, ExperimentalUnsignedTypes::class)
internal object RuntimeStructs {
  fun message(event: mln_runtime_event): String =
    MemoryUtil.copyStringView(event.message, event.message_size)

  fun payload(event: mln_runtime_event): RuntimeEventPayload {
    val payload = event.payload
    if (payload == null) {
      return if (event.payload_type == MLN_RUNTIME_EVENT_PAYLOAD_NONE) RuntimeEventPayload.None
      else unknownPayload(event)
    }
    return when (event.payload_type) {
      MLN_RUNTIME_EVENT_PAYLOAD_NONE -> RuntimeEventPayload.None
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME ->
        if (hasPayloadSize<mln_runtime_event_render_frame>(event)) {
          renderFrame(payload.reinterpret<mln_runtime_event_render_frame>())
        } else unknownPayload(event)
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP ->
        if (hasPayloadSize<mln_runtime_event_render_map>(event)) {
          renderMap(payload.reinterpret<mln_runtime_event_render_map>())
        } else unknownPayload(event)
      MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING ->
        if (hasPayloadSize<mln_runtime_event_style_image_missing>(event)) {
          styleImageMissing(payload.reinterpret<mln_runtime_event_style_image_missing>())
        } else unknownPayload(event)
      MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION ->
        if (hasPayloadSize<mln_runtime_event_tile_action>(event)) {
          tileAction(payload.reinterpret<mln_runtime_event_tile_action>())
        } else unknownPayload(event)
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS ->
        if (hasPayloadSize<mln_runtime_event_offline_region_status>(event)) {
          offlineRegionStatus(payload.reinterpret<mln_runtime_event_offline_region_status>())
        } else unknownPayload(event)
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR ->
        if (hasPayloadSize<mln_runtime_event_offline_region_response_error>(event)) {
          offlineRegionResponseError(
            payload.reinterpret<mln_runtime_event_offline_region_response_error>()
          )
        } else unknownPayload(event)
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
        if (hasPayloadSize<mln_runtime_event_offline_region_tile_count_limit>(event)) {
          offlineRegionTileCountLimit(
            payload.reinterpret<mln_runtime_event_offline_region_tile_count_limit>()
          )
        } else unknownPayload(event)
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED ->
        if (hasPayloadSize<mln_runtime_event_offline_operation_completed>(event)) {
          offlineOperationCompleted(
            payload.reinterpret<mln_runtime_event_offline_operation_completed>()
          )
        } else unknownPayload(event)
      else -> unknownPayload(event)
    }
  }

  private inline fun <reified T : kotlinx.cinterop.CVariable> hasPayloadSize(
    event: mln_runtime_event
  ): Boolean = event.payload_size >= sizeOf<T>().toULong()

  private fun unknownPayload(event: mln_runtime_event): RuntimeEventPayload.Unknown =
    RuntimeEventPayload.Unknown(
      event.payload_type.toInt(),
      checkedLong(event.payload_size, "payload size"),
      event.payload
        ?.reinterpret<kotlinx.cinterop.ByteVar>()
        ?.readBytes(checkedInt(event.payload_size, "payload size")) ?: ByteArray(0),
    )

  private fun renderFrame(
    payload: CPointer<mln_runtime_event_render_frame>
  ): RuntimeEventPayload.RenderFrame {
    val value = payload.pointed
    return RuntimeEventPayload.RenderFrame(
      RenderMode.fromNative(value.mode),
      value.needs_repaint,
      value.placement_changed,
      RenderingStats(
        value.stats.encoding_time,
        value.stats.rendering_time,
        value.stats.frame_count,
        value.stats.draw_call_count,
        value.stats.total_draw_call_count,
      ),
    )
  }

  private fun renderMap(
    payload: CPointer<mln_runtime_event_render_map>
  ): RuntimeEventPayload.RenderMap {
    val value = payload.pointed
    return RuntimeEventPayload.RenderMap(RenderMode.fromNative(value.mode))
  }

  private fun styleImageMissing(
    payload: CPointer<mln_runtime_event_style_image_missing>
  ): RuntimeEventPayload.StyleImageMissing {
    val value = payload.pointed
    return RuntimeEventPayload.StyleImageMissing(
      MemoryUtil.copyStringView(value.image_id, value.image_id_size)
    )
  }

  private fun tileAction(
    payload: CPointer<mln_runtime_event_tile_action>
  ): RuntimeEventPayload.TileAction {
    val value = payload.pointed
    val tileId =
      TileId(
        checkedLong(value.tile_id.overscaled_z.toULong(), "tile overscaled z"),
        value.tile_id.wrap,
        checkedLong(value.tile_id.canonical_z.toULong(), "tile canonical z"),
        checkedLong(value.tile_id.canonical_x.toULong(), "tile canonical x"),
        checkedLong(value.tile_id.canonical_y.toULong(), "tile canonical y"),
      )
    return RuntimeEventPayload.TileAction(
      TileOperation.fromNative(value.operation),
      tileId,
      MemoryUtil.copyStringView(value.source_id, value.source_id_size),
    )
  }

  private fun offlineRegionStatus(
    payload: CPointer<mln_runtime_event_offline_region_status>
  ): RuntimeEventPayload.OfflineRegionStatusChanged {
    val value = payload.pointed
    return RuntimeEventPayload.OfflineRegionStatusChanged(
      value.region_id,
      offlineRegionStatus(value.status),
    )
  }

  private fun offlineRegionResponseError(
    payload: CPointer<mln_runtime_event_offline_region_response_error>
  ): RuntimeEventPayload.OfflineRegionResponseError {
    val value = payload.pointed
    return RuntimeEventPayload.OfflineRegionResponseError(
      value.region_id,
      ResourceErrorReason.fromNative(value.reason),
    )
  }

  private fun offlineRegionTileCountLimit(
    payload: CPointer<mln_runtime_event_offline_region_tile_count_limit>
  ): RuntimeEventPayload.OfflineRegionTileCountLimit {
    val value = payload.pointed
    return RuntimeEventPayload.OfflineRegionTileCountLimit(
      value.region_id,
      checkedLong(value.limit, "offline tile count limit"),
    )
  }

  private fun offlineOperationCompleted(
    payload: CPointer<mln_runtime_event_offline_operation_completed>
  ): RuntimeEventPayload.OfflineOperationCompleted {
    val value = payload.pointed
    return RuntimeEventPayload.OfflineOperationCompleted(
      uint64BitsToLong(value.operation_id),
      OfflineOperationKind.fromNative(value.operation_kind),
      OfflineOperationResultKind.fromNative(value.result_kind),
      value.result_status,
      value.found,
    )
  }

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private fun checkedLong(value: ULong, name: String): Long {
    require(value <= Long.MAX_VALUE.toULong()) { "$name exceeds Long.MAX_VALUE" }
    return value.toLong()
  }

  private fun uint64BitsToLong(value: ULong): Long = value.toLong()

  fun metadata(
    value: ByteArray,
    scope: MemScope,
  ): kotlinx.cinterop.CPointer<kotlinx.cinterop.UByteVar>? =
    if (value.isEmpty()) null else value.toUByteArray().toCValues().getPointer(scope)

  fun offlineRegionDefinition(
    value: OfflineRegionDefinition,
    scope: MemScope,
  ): CPointer<mln_offline_region_definition> {
    val native = scope.alloc<mln_offline_region_definition>()
    native.size = sizeOf<mln_offline_region_definition>().toUInt()
    when (value) {
      is OfflineRegionDefinition.TilePyramid -> {
        native.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
        native.data.tile_pyramid.size =
          sizeOf<org.maplibre.nativeffi.internal.c.mln_offline_tile_pyramid_region_definition>()
            .toUInt()
        native.data.tile_pyramid.style_url = MemoryUtil.cString(scope, value.styleUrl)
        native.data.tile_pyramid.bounds.southwest.latitude = value.bounds.southwest.latitude
        native.data.tile_pyramid.bounds.southwest.longitude = value.bounds.southwest.longitude
        native.data.tile_pyramid.bounds.northeast.latitude = value.bounds.northeast.latitude
        native.data.tile_pyramid.bounds.northeast.longitude = value.bounds.northeast.longitude
        native.data.tile_pyramid.min_zoom = value.minZoom
        native.data.tile_pyramid.max_zoom = value.maxZoom
        native.data.tile_pyramid.pixel_ratio = value.pixelRatio
        native.data.tile_pyramid.include_ideographs = value.includeIdeographs
      }
      is OfflineRegionDefinition.GeometryRegion -> {
        native.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
        native.data.geometry.size =
          sizeOf<org.maplibre.nativeffi.internal.c.mln_offline_geometry_region_definition>()
            .toUInt()
        native.data.geometry.style_url = MemoryUtil.cString(scope, value.styleUrl)
        native.data.geometry.geometry = ValueStructs.geometry(value.geometry, scope)
        native.data.geometry.min_zoom = value.minZoom
        native.data.geometry.max_zoom = value.maxZoom
        native.data.geometry.pixel_ratio = value.pixelRatio
        native.data.geometry.include_ideographs = value.includeIdeographs
      }
      is OfflineRegionDefinition.Unknown ->
        throw IllegalArgumentException("unknown offline region definitions cannot be used as input")
    }
    return native.ptr
  }

  fun offlineRegionSnapshot(
    snapshot: CPointer<cnames.structs.mln_offline_region_snapshot>,
    getter:
      (
        CPointer<cnames.structs.mln_offline_region_snapshot>, CPointer<mln_offline_region_info>,
      ) -> Int =
      ::mln_offline_region_snapshot_get,
    destroyer: (CPointer<cnames.structs.mln_offline_region_snapshot>) -> Unit =
      ::mln_offline_region_snapshot_destroy,
  ): OfflineRegionInfo =
    try {
      memScoped {
        val info = alloc<mln_offline_region_info>()
        info.size = sizeOf<mln_offline_region_info>().toUInt()
        Status.check(getter(snapshot, info.ptr))
        offlineRegionInfo(info)
      }
    } finally {
      destroyer(snapshot)
    }

  fun offlineRegionList(
    list: CPointer<cnames.structs.mln_offline_region_list>,
    counter: (CPointer<cnames.structs.mln_offline_region_list>, CPointer<ULongVar>) -> Int =
      ::mln_offline_region_list_count,
    getter:
      (
        CPointer<cnames.structs.mln_offline_region_list>, ULong, CPointer<mln_offline_region_info>,
      ) -> Int =
      ::mln_offline_region_list_get,
    destroyer: (CPointer<cnames.structs.mln_offline_region_list>) -> Unit =
      ::mln_offline_region_list_destroy,
  ): List<OfflineRegionInfo> =
    try {
      memScoped {
        val outCount = alloc<ULongVar>()
        Status.check(counter(list, outCount.ptr))
        List(checkedInt(outCount.value, "offline region count")) { index ->
          val info = alloc<mln_offline_region_info>()
          info.size = sizeOf<mln_offline_region_info>().toUInt()
          Status.check(getter(list, index.toULong(), info.ptr))
          offlineRegionInfo(info)
        }
      }
    } finally {
      destroyer(list)
    }

  fun offlineRegionInfo(value: mln_offline_region_info): OfflineRegionInfo =
    OfflineRegionInfo(
      value.id,
      offlineRegionDefinition(value.definition),
      value.metadata?.readBytes(checkedInt(value.metadata_size, "offline metadata size"))
        ?: ByteArray(0),
    )

  private fun offlineRegionDefinition(
    value: mln_offline_region_definition
  ): OfflineRegionDefinition =
    when (value.type) {
      MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID -> {
        val definition = value.data.tile_pyramid
        OfflineRegionDefinition.TilePyramid(
          definition.style_url?.toKString() ?: "",
          CoreStructs.latLngBounds(definition.bounds),
          definition.min_zoom,
          definition.max_zoom,
          definition.pixel_ratio,
          definition.include_ideographs,
        )
      }
      MLN_OFFLINE_REGION_DEFINITION_GEOMETRY -> {
        val definition = value.data.geometry
        OfflineRegionDefinition.GeometryRegion(
          definition.style_url?.toKString() ?: "",
          ValueStructs.geometrySnapshot(definition.geometry),
          definition.min_zoom,
          definition.max_zoom,
          definition.pixel_ratio,
          definition.include_ideographs,
        )
      }
      else ->
        OfflineRegionDefinition.Unknown(
          value.type.toInt(),
          checkedInt(value.size.toULong(), "offline region definition size"),
        )
    }

  fun offlineRegionStatus(value: mln_offline_region_status): OfflineRegionStatus =
    OfflineRegionStatus(
      OfflineRegionDownloadState.fromNative(value.download_state),
      checkedLong(value.completed_resource_count, "completed resource count"),
      checkedLong(value.completed_resource_size, "completed resource size"),
      checkedLong(value.completed_tile_count, "completed tile count"),
      checkedLong(value.required_tile_count, "required tile count"),
      checkedLong(value.completed_tile_size, "completed tile size"),
      checkedLong(value.required_resource_count, "required resource count"),
      value.required_resource_count_is_precise,
      value.complete,
    )
}
