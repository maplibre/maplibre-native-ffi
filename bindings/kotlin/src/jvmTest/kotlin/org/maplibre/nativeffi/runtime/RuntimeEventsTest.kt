package org.maplibre.nativeffi.runtime

import java.lang.foreign.Arena
import java.lang.foreign.ValueLayout
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_payload
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions

class RuntimeEventsTest {
  @Test
  fun oneDrainAfterStyleLoadReturnsEveryQueuedEvent(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      withMap { runtime, map ->
        map.setStyleJson(STYLE_JSON.encodeToByteArray())
        // Nothing drains while the style parses, so one drain reports the whole run.
        repeat(20) { runtime.barrier() }

        val batch = runtime.drainEvents()
        assertEquals(0L, batch.remainingCount)
        assertTrue(batch.events.size > 1, "expected several events, got ${batch.events}")
        assertTrue(batch.events.any { it.type == RuntimeEventType.MAP_STYLE_LOADED })
        assertTrue(batch.events.all { it.mapSource == map })
      }
    }

  @Test
  fun narrowedMapMaskDropsOneTypeAndKeepsAnother(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      withMap { runtime, map ->
        // One style load produces both types, so both are driven after this write.
        map.eventMask = RuntimeEventMask.ALL - RuntimeEventMask.MAP_LOADING_STARTED
        map.setStyleJson(STYLE_JSON.encodeToByteArray())

        val types = drainUntil(runtime) { RuntimeEventType.MAP_STYLE_LOADED in it }
        assertTrue(RuntimeEventType.MAP_LOADING_STARTED !in types, "cleared type was delivered")
      }
    }

  @Test
  fun creationMaskNarrowsTheMapBeforeItsFirstStyleLoad(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val map =
          MapHandle.create(
            runtime,
            mapOptions().apply {
              eventMask = RuntimeEventMask.ALL - RuntimeEventMask.MAP_LOADING_STARTED
            },
          )
        try {
          assertEquals(
            RuntimeEventMask.ALL_MAP_EVENTS - RuntimeEventMask.MAP_LOADING_STARTED,
            map.eventMask,
          )
          map.setStyleJson(STYLE_JSON.encodeToByteArray())

          val types = drainUntil(runtime) { RuntimeEventType.MAP_STYLE_LOADED in it }
          assertTrue(RuntimeEventType.MAP_LOADING_STARTED !in types, "cleared type was delivered")
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun bothHandlesReportEveryTypeUntilNarrowedAndKeepUnrelatedBitsOnAWrite(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      withMap { runtime, map ->
        // The runtime reports its global queue mask; the map reports its map-originated subset.
        assertEquals(RuntimeEventMask.ALL, runtime.eventMask)
        assertEquals(RuntimeEventMask.ALL_MAP_EVENTS, map.eventMask)

        runtime.eventMask = RuntimeEventMask.ALL
        map.eventMask = RuntimeEventMask.ALL
        runtime.barrier()
        assertEquals(RuntimeEventMask.ALL, runtime.eventMask)
        assertEquals(RuntimeEventMask.ALL_MAP_EVENTS, map.eventMask)

        // Read, clear one bit, write back: every other bit survives.
        map.eventMask = map.eventMask - RuntimeEventMask.MAP_TILE_ACTION
        assertEquals(
          RuntimeEventMask.ALL_MAP_EVENTS - RuntimeEventMask.MAP_TILE_ACTION,
          map.eventMask,
        )
        assertTrue(RuntimeEventType.MAP_STYLE_LOADED in map.eventMask)

        runtime.eventMask = runtime.eventMask - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
        assertEquals(
          RuntimeEventMask.ALL - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED,
          runtime.eventMask,
        )
      }
    }

  @Test
  fun maskBitOutsideEveryKnownTypeFailsEverySetterAndBothCreations(): Unit =
    org.maplibre.nativeffi.runtime
      .runSuspendTest { // The bit sits above the low 32, so a mask this binding narrowed on the way
        // out would reach native as a value it accepts.
        val unknownBit = RuntimeEventMask.ALL + RuntimeEventMask(1L shl 63)
        assertFailsWith<InvalidArgumentException> {
          runSuspendTest { RuntimeHandle.create(RuntimeOptions().apply { eventMask = unknownBit }) }
        }
        withMap { runtime, map ->
          assertFailsWith<InvalidArgumentException> { runtime.eventMask = unknownBit }
          assertFailsWith<InvalidArgumentException> { map.eventMask = unknownBit }
          assertFailsWith<InvalidArgumentException> {
            runSuspendTest {
              MapHandle.create(runtime, mapOptions().apply { eventMask = unknownBit })
            }
          }
        }
      }

  @Test
  fun boundedDrainReportsWhatStaysQueuedAndTheNextDrainReachesZero(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      withMap { runtime, map ->
        map.setStyleJson(STYLE_JSON.encodeToByteArray())
        repeat(20) { runtime.barrier() }

        val bounded = runtime.drainEvents(maxEvents = 1)
        assertEquals(1, bounded.events.size)
        assertTrue(bounded.remainingCount > 0, "a bounded drain reported nothing left")

        val rest = runtime.drainEvents()
        assertEquals(0L, rest.remainingCount)
        assertEquals(bounded.remainingCount, rest.events.size.toLong())
        assertFailsWith<InvalidArgumentException> { runtime.drainEvents(maxEvents = -1) }
      }
    }

  @Test
  fun unknownPayloadTypeKeepsItsRawValueAndCopiesTheWholeWindow(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      Arena.ofConfined().use { arena ->
        val window = mln_runtime_event.sizeof() - mln_runtime_event.`payload$offset`()
        val payload = arena.allocate(window)
        payload.set(ValueLayout.JAVA_BYTE, 0, 1)
        payload.set(ValueLayout.JAVA_BYTE, 1, 2)
        payload.set(ValueLayout.JAVA_BYTE, 2, 3)

        val decoded = NativeAccess.runtimeEventPayloadForTesting(999, payload)

        val unknown = assertIs<RuntimeEventPayload.Unknown>(decoded)
        assertEquals(999, unknown.rawPayloadType)
        assertEquals(mln_runtime_event_payload.sizeof().toInt(), unknown.payloadBytes.size)
        assertContentEquals(byteArrayOf(1, 2, 3), unknown.payloadBytes.take(3).toByteArray())

        // The copy survives a write through the source and a write into a copy.
        payload.set(ValueLayout.JAVA_BYTE, 0, 9)
        unknown.payloadBytes[1] = 9
        assertContentEquals(byteArrayOf(1, 2, 3), unknown.payloadBytes.take(3).toByteArray())
      }
    }

  @Test
  fun styleReplacementReleasesADroppedSourceWithNoStyleLoadedEvent(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        assertCommandCommitted(
          runtime,
          runtime.setResourceProvider(
            ResourceProviderCallback { request, handle ->
              if (request.requestedUrl != SERVED_STYLE_URL) {
                return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
              }
              handle.complete(
                ResourceResponse(ResourceResponseStatus.OK).apply {
                  bytes = STYLE_JSON.encodeToByteArray()
                }
              )
              ResourceProviderDecision.HANDLE
            }
          ),
        )
        val map =
          MapHandle.create(
            runtime,
            mapOptions().apply {
              eventMask = RuntimeEventMask.ALL - RuntimeEventMask.MAP_STYLE_LOADED
            },
          )
        try {
          assertCommandCommitted(runtime, map.setStyleJson(STYLE_JSON.encodeToByteArray()))
          assertCommandCommitted(
            runtime,
            map.addCustomGeometrySource("custom", customGeometrySourceOptions()),
          )
          assertEquals(1, map.customGeometrySourceCountForTesting())
          // The binding adds no subscription of its own, so the mask reads back as written.
          assertEquals(
            RuntimeEventMask.ALL_MAP_EVENTS - RuntimeEventMask.MAP_STYLE_LOADED,
            map.eventMask,
            "the host's own mask changed",
          )

          // A URL load drops the source in the pump that completes it, and native
          // reports that through the release callback rather than through an event.
          assertCommandCommitted(runtime, map.setStyleUrl(SERVED_STYLE_URL))
          val types = drainUntil(runtime) { map.customGeometrySourceCountForTesting() == 0 }
          assertTrue(
            RuntimeEventType.MAP_STYLE_LOADED !in types,
            "a cleared style-loaded event reached the host",
          )
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun inlineStyleCommandsReleaseTheirSourcesAfterTheBarrier(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      withMap { runtime, map ->
        map.setStyleJson(STYLE_JSON.encodeToByteArray())
        runtime.barrier()
        map.addCustomGeometrySource("removed", customGeometrySourceOptions())
        map.addCustomGeometrySource("dropped", customGeometrySourceOptions())
        runtime.barrier()
        assertEquals(2, map.customGeometrySourceCountForTesting())

        assertTrue(map.removeStyleSource("removed"))
        runtime.barrier()
        assertEquals(1, map.customGeometrySourceCountForTesting())

        map.setStyleJson(STYLE_JSON.encodeToByteArray())
        runtime.barrier()
        assertEquals(0, map.customGeometrySourceCountForTesting())

        // Closing the map quiesces and releases the source it still owns.
        map.addCustomGeometrySource("surviving", customGeometrySourceOptions())
        runtime.barrier()
        map.close()
        assertEquals(0, map.customGeometrySourceCountForTesting())
      }
    }

  @Test
  fun drainAndBothMaskSettersAreAnyThread(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      withMap { runtime, map ->
        val failures = mutableListOf<Throwable>()
        val thread = Thread {
          try {
            runtime.drainEvents()
            runtime.eventMask = RuntimeEventMask.ALL
            map.eventMask = RuntimeEventMask.ALL
          } catch (error: Throwable) {
            failures += error
          }
        }
        thread.start()
        thread.join()
        assertTrue(failures.isEmpty(), "any-thread APIs failed: $failures")
      }
    }

  private fun customGeometrySourceOptions(): CustomGeometrySourceOptions =
    CustomGeometrySourceOptions(
      object : CustomGeometrySourceCallback {
        override fun fetchTile(tileId: CanonicalTileId) = Unit
      }
    )

  private fun mapOptions(): MapOptions =
    MapOptions().apply {
      width = 64
      height = 64
    }

  private suspend fun withMap(body: suspend (RuntimeHandle, MapHandle) -> Unit) {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val map = MapHandle.create(runtime, mapOptions())
      try {
        body(runtime, map)
      } finally {
        map.close()
      }
    }
  }

  private suspend fun assertCommandCommitted(runtime: RuntimeHandle, commandId: ULong) {
    runtime.barrier()
    val matches =
      runtime
        .drainEvents()
        .events
        .mapNotNull { it.payload as? RuntimeEventPayload.CommandFinished }
        .filter { it.commandId == commandId }
    assertEquals(1, matches.size, "terminal outcome count for command $commandId")
    assertEquals(CommandDisposition.COMMITTED, matches.single().disposition)
  }

  /** Drains until [done] holds for the event types seen so far. */
  private suspend fun drainUntil(
    runtime: RuntimeHandle,
    done: (Set<RuntimeEventType>) -> Boolean,
  ): Set<RuntimeEventType> {
    val types = mutableSetOf<RuntimeEventType>()
    repeat(10_000) {
      runtime.barrier()
      types += runtime.drainEvents().events.map { it.type }
      if (done(types)) return types
      Thread.sleep(1)
    }
    error("the runtime did not report the events this test drove: $types")
  }
}

private const val STYLE_JSON = """{"version":8,"sources":{},"layers":[]}"""
private const val SERVED_STYLE_URL = "custom://events-style.json"
