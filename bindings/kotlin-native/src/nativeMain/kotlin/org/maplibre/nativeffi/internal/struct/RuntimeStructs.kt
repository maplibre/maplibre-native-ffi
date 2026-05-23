package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toCValues
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.geo.CanonicalTileId
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
    val payload = event.payload ?: return RuntimeEventPayload.None
    return when (event.payload_type) {
      MLN_RUNTIME_EVENT_PAYLOAD_NONE -> RuntimeEventPayload.None
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME ->
        renderFrame(payload.reinterpret<mln_runtime_event_render_frame>())
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP ->
        renderMap(payload.reinterpret<mln_runtime_event_render_map>())
      MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING ->
        styleImageMissing(payload.reinterpret<mln_runtime_event_style_image_missing>())
      MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION ->
        tileAction(payload.reinterpret<mln_runtime_event_tile_action>())
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS ->
        offlineRegionStatus(payload.reinterpret<mln_runtime_event_offline_region_status>())
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR ->
        offlineRegionResponseError(
          payload.reinterpret<mln_runtime_event_offline_region_response_error>()
        )
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
        offlineRegionTileCountLimit(
          payload.reinterpret<mln_runtime_event_offline_region_tile_count_limit>()
        )
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED ->
        offlineOperationCompleted(
          payload.reinterpret<mln_runtime_event_offline_operation_completed>()
        )
      else -> RuntimeEventPayload.Unknown(event.payload_type, event.payload_size)
    }
  }

  private fun renderFrame(
    payload: CPointer<mln_runtime_event_render_frame>
  ): RuntimeEventPayload.RenderFrame {
    val value = payload.pointed
    return RuntimeEventPayload.RenderFrame(
      RenderMode.fromNative(value.mode),
      value.mode,
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
    return RuntimeEventPayload.RenderMap(RenderMode.fromNative(value.mode), value.mode)
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
        value.tile_id.overscaled_z,
        value.tile_id.wrap,
        CanonicalTileId(
          value.tile_id.canonical_z,
          value.tile_id.canonical_x,
          value.tile_id.canonical_y,
        ),
      )
    return RuntimeEventPayload.TileAction(
      TileOperation.fromNative(value.operation),
      value.operation,
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
      value.reason,
    )
  }

  private fun offlineRegionTileCountLimit(
    payload: CPointer<mln_runtime_event_offline_region_tile_count_limit>
  ): RuntimeEventPayload.OfflineRegionTileCountLimit {
    val value = payload.pointed
    return RuntimeEventPayload.OfflineRegionTileCountLimit(value.region_id, value.limit)
  }

  private fun offlineOperationCompleted(
    payload: CPointer<mln_runtime_event_offline_operation_completed>
  ): RuntimeEventPayload.OfflineOperationCompleted {
    val value = payload.pointed
    return RuntimeEventPayload.OfflineOperationCompleted(
      value.operation_id,
      OfflineOperationKind.fromNative(value.operation_kind),
      value.operation_kind,
      OfflineOperationResultKind.fromNative(value.result_kind),
      value.result_kind,
      value.result_status,
      value.found,
    )
  }

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
    }
    return native.ptr
  }

  fun offlineRegionInfo(value: mln_offline_region_info): OfflineRegionInfo =
    OfflineRegionInfo(
      value.id,
      offlineRegionDefinition(value.definition),
      value.metadata?.readBytes(value.metadata_size.toInt()) ?: ByteArray(0),
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
      else -> error("Unknown offline region definition type ${value.type}")
    }

  fun offlineRegionStatus(value: mln_offline_region_status): OfflineRegionStatus =
    OfflineRegionStatus(
      OfflineRegionDownloadState.fromNative(value.download_state),
      value.completed_resource_count,
      value.completed_resource_size,
      value.completed_tile_count,
      value.required_tile_count,
      value.completed_tile_size,
      value.required_resource_count,
      value.required_resource_count_is_precise,
      value.complete,
    )
}
