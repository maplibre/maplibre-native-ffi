package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.pointed
import kotlinx.cinterop.reinterpret
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.TileId
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_NONE
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
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
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.runtime.OfflineOperationKind
import org.maplibre.nativeffi.runtime.OfflineOperationResultKind
import org.maplibre.nativeffi.runtime.RuntimeEventPayload

/** Copies runtime event payloads out of native event storage. */
@OptIn(ExperimentalForeignApi::class)
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
