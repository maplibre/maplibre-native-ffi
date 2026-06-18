package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.cstr
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
import org.maplibre.nativeffi.internal.c.mln_offline_region_definition
import org.maplibre.nativeffi.internal.c.mln_offline_region_info
import org.maplibre.nativeffi.internal.c.mln_offline_tile_pyramid_region_definition
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_operation_completed
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_map
import org.maplibre.nativeffi.internal.c.mln_runtime_event_tile_action
import org.maplibre.nativeffi.map.TileOperation
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.runtime.OfflineOperationKind
import org.maplibre.nativeffi.runtime.OfflineOperationResultKind
import org.maplibre.nativeffi.runtime.RuntimeEventPayload

@OptIn(ExperimentalForeignApi::class)
class RuntimeOfflineStructsTest {
  @Test
  fun offlineRegionDefinitionMaterializesTilePyramidAndGeometryVariants() {
    memScoped {
      val tilePyramid =
        RuntimeStructs.offlineRegionDefinition(
            OfflineRegionDefinition.TilePyramid(
              "asset://style.json",
              LatLngBounds(LatLng(1.0, 2.0), LatLng(3.0, 4.0)),
              1.0,
              5.0,
              2.0f,
              true,
            ),
            this,
          )
          .pointed
      assertEquals(MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID, tilePyramid.type)
      assertEquals(1.0, tilePyramid.data.tile_pyramid.bounds.southwest.latitude)
      assertEquals(4.0, tilePyramid.data.tile_pyramid.bounds.northeast.longitude)
      assertEquals(2.0f, tilePyramid.data.tile_pyramid.pixel_ratio)

      val geometry =
        RuntimeStructs.offlineRegionDefinition(
            OfflineRegionDefinition.GeometryRegion(
              "asset://style.json",
              Geometry.Point(LatLng(5.0, 6.0)),
              2.0,
              6.0,
              1.0f,
              false,
            ),
            this,
          )
          .pointed
      assertEquals(MLN_OFFLINE_REGION_DEFINITION_GEOMETRY, geometry.type)
      assertNotNull(geometry.data.geometry.geometry)
    }
  }

  @Test
  fun offlineMetadataUsesNullPointerOnlyForEmptyMetadata() {
    memScoped {
      assertEquals(null, RuntimeStructs.metadata(ByteArray(0), this))
      assertNotNull(RuntimeStructs.metadata(byteArrayOf(1, 2, 3), this))
    }
  }

  @Test
  fun offlineRegionInfoPreservesUnknownDefinitionDiscriminator() {
    memScoped {
      val info = alloc<mln_offline_region_info>()
      info.size = sizeOf<mln_offline_region_info>().toUInt()
      info.id = 7
      info.definition.size = sizeOf<mln_offline_region_definition>().toUInt()
      info.definition.type = 999U
      info.metadata = null
      info.metadata_size = 0UL

      val definition = RuntimeStructs.offlineRegionInfo(info).definition

      assertEquals(
        OfflineRegionDefinition.Unknown(999, sizeOf<mln_offline_region_definition>().toInt()),
        definition,
      )
    }
  }

  @Test
  fun unknownOfflineRegionDefinitionIsOutputOnly() {
    memScoped {
      assertFailsWith<IllegalArgumentException> {
        RuntimeStructs.offlineRegionDefinition(OfflineRegionDefinition.Unknown(999, 8), this)
      }
    }
  }

  @Test
  fun offlineRegionSnapshotCopiesInfoAndDestroysNativeHandle() {
    memScoped {
      var destroys = 0
      val metadata = allocArray<UByteVar>(2)
      metadata[0] = 4U
      metadata[1] = 5U
      val snapshot = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_offline_region_snapshot>()
      val styleUrl = "asset://offline-style.json".cstr.getPointer(this)

      val info =
        RuntimeStructs.offlineRegionSnapshot(
          snapshot,
          getter = { _, outInfo ->
            fillValidOfflineRegionInfo(outInfo.pointed, styleUrl, metadata, 2UL)
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )

      assertEquals(7, info.id)
      assertEquals(validTilePyramidDefinition(), info.definition)
      assertContentEquals(byteArrayOf(4, 5), info.metadata)
      assertEquals(1, destroys)
    }
  }

  @Test
  fun offlineRegionSnapshotDestroysNativeHandleWhenCopyFails() {
    memScoped {
      var destroys = 0
      val metadata = alloc<UByteVar>()
      val snapshot = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_offline_region_snapshot>()

      assertFailsWith<IllegalArgumentException> {
        RuntimeStructs.offlineRegionSnapshot(
          snapshot,
          getter = { _, outInfo ->
            fillOfflineRegionInfoWithOversizedMetadata(outInfo.pointed, metadata.ptr)
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )
      }

      assertEquals(1, destroys)
    }
  }

  @Test
  fun offlineRegionListCopiesInfoAndDestroysNativeHandle() {
    memScoped {
      var destroys = 0
      val metadata = allocArray<UByteVar>(1)
      metadata[0] = 6U
      val list = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_offline_region_list>()
      val styleUrl = "asset://offline-style.json".cstr.getPointer(this)

      val regions =
        RuntimeStructs.offlineRegionList(
          list,
          counter = { _, outCount ->
            outCount[0] = 1UL
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, outInfo ->
            fillValidOfflineRegionInfo(outInfo.pointed, styleUrl, metadata, 1UL)
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )

      assertEquals(1, regions.size)
      assertEquals(7, regions.single().id)
      assertEquals(validTilePyramidDefinition(), regions.single().definition)
      assertContentEquals(byteArrayOf(6), regions.single().metadata)
      assertEquals(1, destroys)
    }
  }

  @Test
  fun offlineRegionListDestroysNativeHandleWhenCopyFails() {
    memScoped {
      var destroys = 0
      val metadata = alloc<UByteVar>()
      val list = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_offline_region_list>()

      assertFailsWith<IllegalArgumentException> {
        RuntimeStructs.offlineRegionList(
          list,
          counter = { _, outCount ->
            outCount[0] = 1UL
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, outInfo ->
            fillOfflineRegionInfoWithOversizedMetadata(outInfo.pointed, metadata.ptr)
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )
      }

      assertEquals(1, destroys)
    }
  }

  @Test
  fun runtimePayloadPreservesKnownTypeWhenPayloadPointerIsNull() {
    memScoped {
      val event = alloc<mln_runtime_event>()
      event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
      event.payload = null
      event.payload_size = 0UL

      assertEquals(
        RuntimeEventPayload.Unknown(MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP.toInt(), 0, ByteArray(0)),
        RuntimeStructs.payload(event),
      )
    }
  }

  @Test
  fun unknownRuntimePayloadCopiesRawBytes() {
    memScoped {
      val payload = allocArray<ByteVar>(3)
      payload[0] = 1
      payload[1] = 2
      payload[2] = 3
      val event = alloc<mln_runtime_event>()
      event.payload_type = 999U
      event.payload = payload
      event.payload_size = 3UL

      val result = RuntimeStructs.payload(event) as RuntimeEventPayload.Unknown

      assertEquals(999, result.rawPayloadType)
      assertEquals(3L, result.payloadSize)
      assertContentEquals(byteArrayOf(1, 2, 3), result.payloadBytes)

      payload[0] = 9
      val firstCopy = result.payloadBytes
      firstCopy[1] = 9
      assertContentEquals(byteArrayOf(1, 2, 3), result.payloadBytes)
    }
  }

  @Test
  fun unknownRuntimePayloadRejectsOversizedRawBytes() {
    memScoped {
      val payload = alloc<ByteVar>()
      val event = alloc<mln_runtime_event>()
      event.payload_type = 999U
      event.payload = payload.ptr
      event.payload_size = Int.MAX_VALUE.toULong() + 1UL

      assertFailsWith<IllegalArgumentException> { RuntimeStructs.payload(event) }
    }
  }

  @Test
  fun typedRuntimePayloadsPreserveUnknownRawEnumsAndCopiedStrings() {
    var tilePayload: RuntimeEventPayload.TileAction? = null
    memScoped {
      val renderMap = alloc<mln_runtime_event_render_map>()
      renderMap.mode = 900U
      val renderMapEvent = alloc<mln_runtime_event>()
      renderMapEvent.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
      renderMapEvent.payload = renderMap.ptr
      renderMapEvent.payload_size = sizeOf<mln_runtime_event_render_map>().toULong()

      val renderMapResult = RuntimeStructs.payload(renderMapEvent) as RuntimeEventPayload.RenderMap
      assertEquals(RenderMode(900), renderMapResult.mode)
      assertEquals(900, renderMapResult.mode.nativeValue)

      val sourceId = "source"
      val tileAction = alloc<mln_runtime_event_tile_action>()
      tileAction.operation = 901U
      tileAction.tile_id.overscaled_z = 1U
      tileAction.tile_id.wrap = -1
      tileAction.tile_id.canonical_z = 2U
      tileAction.tile_id.canonical_x = 3U
      tileAction.tile_id.canonical_y = 4U
      tileAction.source_id = sourceId.cstr.getPointer(this)
      tileAction.source_id_size = sourceId.encodeToByteArray().size.toULong()
      val tileActionEvent = alloc<mln_runtime_event>()
      tileActionEvent.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
      tileActionEvent.payload = tileAction.ptr
      tileActionEvent.payload_size = sizeOf<mln_runtime_event_tile_action>().toULong()

      tilePayload = RuntimeStructs.payload(tileActionEvent) as RuntimeEventPayload.TileAction
    }

    assertEquals(TileOperation(901), tilePayload?.operation)
    assertEquals(901, tilePayload?.operation?.nativeValue)
    assertEquals("source", tilePayload?.sourceId)
  }

  @Test
  fun offlineOperationCompletedPreservesOperationIdBitPattern() {
    memScoped {
      val payload = alloc<mln_runtime_event_offline_operation_completed>()
      payload.operation_id = Long.MAX_VALUE.toULong() + 1UL
      payload.operation_kind = 902U
      payload.result_kind = 903U
      val event = alloc<mln_runtime_event>()
      event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
      event.payload = payload.ptr
      event.payload_size = sizeOf<mln_runtime_event_offline_operation_completed>().toULong()

      val result = RuntimeStructs.payload(event) as RuntimeEventPayload.OfflineOperationCompleted

      assertEquals((Long.MAX_VALUE.toULong() + 1UL).toLong(), result.operationId)
      assertEquals(OfflineOperationKind(902), result.operationKind)
      assertEquals(902, result.operationKind.nativeValue)
      assertEquals(OfflineOperationResultKind(903), result.resultKind)
      assertEquals(903, result.resultKind.nativeValue)
    }
  }

  @Test
  fun runtimePayloadRejectsUndersizedKnownPayloads() {
    memScoped {
      val payload = allocArray<ByteVar>(2)
      payload[0] = 4
      payload[1] = 5

      listOf(
          MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP,
          MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
          MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS,
        )
        .forEach { payloadType ->
          val event = alloc<mln_runtime_event>()
          event.payload_type = payloadType
          event.payload = payload
          event.payload_size = 2UL

          val result = RuntimeStructs.payload(event) as RuntimeEventPayload.Unknown

          assertEquals(payloadType.toInt(), result.rawPayloadType)
          assertEquals(2L, result.payloadSize)
          assertContentEquals(byteArrayOf(4, 5), result.payloadBytes)
        }
    }
  }

  private fun fillOfflineRegionInfoWithOversizedMetadata(
    info: mln_offline_region_info,
    metadata: kotlinx.cinterop.CPointer<UByteVar>,
  ) {
    info.size = sizeOf<mln_offline_region_info>().toUInt()
    info.id = 7
    info.definition.size = sizeOf<mln_offline_region_definition>().toUInt()
    info.definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
    info.definition.data.tile_pyramid.size =
      sizeOf<mln_offline_tile_pyramid_region_definition>().toUInt()
    info.metadata = metadata
    info.metadata_size = Int.MAX_VALUE.toULong() + 1UL
  }

  private fun fillValidOfflineRegionInfo(
    info: mln_offline_region_info,
    styleUrl: CPointer<ByteVar>,
    metadata: CPointer<UByteVar>,
    metadataSize: ULong,
  ) {
    info.size = sizeOf<mln_offline_region_info>().toUInt()
    info.id = 7
    info.definition.size = sizeOf<mln_offline_region_definition>().toUInt()
    info.definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
    info.definition.data.tile_pyramid.size =
      sizeOf<mln_offline_tile_pyramid_region_definition>().toUInt()
    info.definition.data.tile_pyramid.style_url = styleUrl
    info.definition.data.tile_pyramid.bounds.southwest.latitude = 1.0
    info.definition.data.tile_pyramid.bounds.southwest.longitude = 2.0
    info.definition.data.tile_pyramid.bounds.northeast.latitude = 3.0
    info.definition.data.tile_pyramid.bounds.northeast.longitude = 4.0
    info.definition.data.tile_pyramid.min_zoom = 1.0
    info.definition.data.tile_pyramid.max_zoom = 5.0
    info.definition.data.tile_pyramid.pixel_ratio = 2.0f
    info.definition.data.tile_pyramid.include_ideographs = true
    info.metadata = metadata
    info.metadata_size = metadataSize
  }

  private fun validTilePyramidDefinition(): OfflineRegionDefinition.TilePyramid =
    OfflineRegionDefinition.TilePyramid(
      "asset://offline-style.json",
      LatLngBounds(LatLng(1.0, 2.0), LatLng(3.0, 4.0)),
      1.0,
      5.0,
      2.0f,
      true,
    )
}
