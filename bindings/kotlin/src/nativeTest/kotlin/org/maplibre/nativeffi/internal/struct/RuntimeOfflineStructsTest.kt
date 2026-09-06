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
import kotlinx.cinterop.toCValues
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
import org.maplibre.nativeffi.internal.c.mln_offline_region_definition
import org.maplibre.nativeffi.internal.c.mln_offline_region_info
import org.maplibre.nativeffi.internal.c.mln_offline_tile_pyramid_region_definition
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_payload
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.memory.CSize
import org.maplibre.nativeffi.internal.memory.toCSize
import org.maplibre.nativeffi.map.TileOperation
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.runtime.RuntimeEventPayload

@OptIn(ExperimentalForeignApi::class)
class RuntimeOfflineStructsTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun offlineRegionDefinitionMaterializesTilePyramidAndGeometryVariants(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
                "{\"type\":\"Point\",\"coordinates\":[6,5]}".encodeToByteArray(),
                2.0,
                6.0,
                1.0f,
                false,
              ),
              this,
            )
            .pointed
        assertEquals(MLN_OFFLINE_REGION_DEFINITION_GEOMETRY, geometry.type)
        assertEquals(36UL, geometry.data.geometry.geometry.size.toULong())
      }
    }

  @Test
  fun offlineMetadataUsesNullPointerOnlyForEmptyMetadata(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        assertEquals(null, RuntimeStructs.metadata(ByteArray(0), this))
        assertNotNull(RuntimeStructs.metadata(byteArrayOf(1, 2, 3), this))
      }
    }

  @Test
  fun offlineRegionInfoPreservesUnknownDefinitionDiscriminator(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val info = alloc<mln_offline_region_info>()
        info.size = sizeOf<mln_offline_region_info>().toUInt()
        info.id = 7
        info.definition.size = sizeOf<mln_offline_region_definition>().toUInt()
        info.definition.type = 999U
        info.metadata = null
        info.metadata_size = 0.toCSize()

        val definition = RuntimeStructs.offlineRegionInfo(info).definition

        assertEquals(
          OfflineRegionDefinition.Unknown(999, sizeOf<mln_offline_region_definition>().toInt()),
          definition,
        )
      }
    }

  @Test
  fun unknownOfflineRegionDefinitionIsOutputOnly(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        assertFailsWith<InvalidArgumentException> {
          RuntimeStructs.offlineRegionDefinition(OfflineRegionDefinition.Unknown(999, 8), this)
        }
      }
    }

  @Test
  fun offlineRegionSnapshotCopiesInfoAndDestroysNativeHandle(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        var destroys = 0
        val metadata = allocArray<UByteVar>(2)
        metadata[0] = 4U
        metadata[1] = 5U
        val snapshot = SyntheticHandles.offlineRegionSnapshot()
        val styleUrl = "asset://offline-style.json".cstr.getPointer(this)

        val info =
          RuntimeStructs.offlineRegionSnapshot(
            snapshot,
            getter = { _, outInfo ->
              fillValidOfflineRegionInfo(outInfo.pointed, styleUrl, metadata, 2.toCSize())
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
  fun offlineRegionSnapshotDestroysNativeHandleWhenCopyFails(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        var destroys = 0
        val metadata = alloc<UByteVar>()
        val snapshot = SyntheticHandles.offlineRegionSnapshot()

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
  fun offlineRegionListCopiesInfoAndDestroysNativeHandle(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        var destroys = 0
        val metadata = allocArray<UByteVar>(1)
        metadata[0] = 6U
        val list = SyntheticHandles.offlineRegionList()
        val styleUrl = "asset://offline-style.json".cstr.getPointer(this)

        val regions =
          RuntimeStructs.offlineRegionList(
            list,
            counter = { _, outCount ->
              outCount[0] = 1.toCSize()
              MaplibreStatus.OK.nativeCode
            },
            getter = { _, _, outInfo ->
              fillValidOfflineRegionInfo(outInfo.pointed, styleUrl, metadata, 1.toCSize())
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
  fun offlineRegionListDestroysNativeHandleWhenCopyFails(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        var destroys = 0
        val metadata = alloc<UByteVar>()
        val list = SyntheticHandles.offlineRegionList()

        assertFailsWith<IllegalArgumentException> {
          RuntimeStructs.offlineRegionList(
            list,
            counter = { _, outCount ->
              outCount[0] = 1.toCSize()
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
  fun unknownRuntimePayloadCopiesTheWholeUnionWindow(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val event = alloc<mln_runtime_event>()
        event.payload_type = 999U
        val payload = event.payload.ptr.reinterpret<ByteVar>()
        for (index in 0 until payloadWindowSize) {
          payload[index] = 0
        }
        payload[0] = 1
        payload[1] = 2
        payload[2] = 3

        val result = RuntimeStructs.payload(event, eventSize) as RuntimeEventPayload.Unknown

        assertEquals(999, result.rawPayloadType)
        assertEquals(payloadWindowSize, result.payloadBytes.size)
        assertContentEquals(byteArrayOf(1, 2, 3), result.payloadBytes.take(3).toByteArray())

        // The copy survives both a write through the source and a write into a
        // previously returned array.
        payload[0] = 9
        val firstCopy = result.payloadBytes
        firstCopy[1] = 9
        assertContentEquals(byteArrayOf(1, 2, 3), result.payloadBytes.take(3).toByteArray())
      }
    }

  @Test
  fun payloadTypeSelectsWhichUnionMemberOneWindowDecodesAs(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val event = alloc<mln_runtime_event>()
        event.payload.camera_transition_finished.transition_id = 0x0000_0007_0000_0384UL

        event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED
        val transition =
          RuntimeStructs.payload(event, eventSize) as RuntimeEventPayload.CameraTransitionFinished
        assertEquals(0x0000_0007_0000_0384UL.toLong(), transition.transitionId)

        // The same window read as another member takes that member's own field.
        event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
        val renderMap = RuntimeStructs.payload(event, eventSize) as RuntimeEventPayload.RenderMap
        assertEquals(RenderMode(900), renderMap.mode)
      }
    }

  @Test
  fun typedRuntimePayloadsPreserveUnknownRawEnums(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val renderMapEvent = alloc<mln_runtime_event>()
        renderMapEvent.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
        renderMapEvent.payload.render_map.mode = 900U

        val renderMapResult =
          RuntimeStructs.payload(renderMapEvent, eventSize) as RuntimeEventPayload.RenderMap
        assertEquals(RenderMode(900), renderMapResult.mode)
        assertEquals(900, renderMapResult.mode.nativeValue)

        val tileActionEvent = alloc<mln_runtime_event>()
        tileActionEvent.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
        tileActionEvent.payload.tile_action.operation = 901U
        tileActionEvent.payload.tile_action.tile_id.overscaled_z = 1U
        tileActionEvent.payload.tile_action.tile_id.wrap = -1
        tileActionEvent.payload.tile_action.tile_id.canonical_z = 2U
        tileActionEvent.payload.tile_action.tile_id.canonical_x = 3U
        tileActionEvent.payload.tile_action.tile_id.canonical_y = 4U

        val tilePayload =
          RuntimeStructs.payload(tileActionEvent, eventSize) as RuntimeEventPayload.TileAction
        assertEquals(TileOperation(901), tilePayload.operation)
        assertEquals(901, tilePayload.operation.nativeValue)
        assertEquals(1L, tilePayload.tileId.overscaledZ)
        assertEquals(-1, tilePayload.tileId.wrap)
        assertEquals(2L, tilePayload.tileId.canonicalZ)
        assertEquals(3L, tilePayload.tileId.canonicalX)
        assertEquals(4L, tilePayload.tileId.canonicalY)
      }
    }

  @Test
  fun runtimeEventMessageComesFromItsOffsetInTheBatchArena(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val arena = "firstsecond".encodeToByteArray().toCValues().getPointer(this)
        val event = alloc<mln_runtime_event>()
        event.message_offset = 5U
        event.message_size = 6U

        assertEquals("second", RuntimeStructs.message(event, arena))

        event.message_size = 0U
        assertEquals("", RuntimeStructs.message(event, arena))
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
    info.metadata_size = (Int.MAX_VALUE.toULong() + 1UL).toCSize()
  }

  private fun fillValidOfflineRegionInfo(
    info: mln_offline_region_info,
    styleUrl: CPointer<ByteVar>,
    metadata: CPointer<UByteVar>,
    metadataSize: CSize,
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

/** Layout of one event record, as this binding compiled it. */
@OptIn(ExperimentalForeignApi::class) private val eventSize: Long = sizeOf<mln_runtime_event>()

@OptIn(ExperimentalForeignApi::class)
private val payloadWindowSize: Int = sizeOf<mln_runtime_event_payload>().toInt()
