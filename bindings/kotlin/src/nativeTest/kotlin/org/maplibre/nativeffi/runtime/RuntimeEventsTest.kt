package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.runOnBackgroundThread
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class RuntimeEventsTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun narrowedMapMaskDropsOneTypeAndKeepsAnother(): Unit = runSuspendTest {
    withMap { runtime, map ->
      // One style load produces both types, so both are driven after this write.
      map.setEventMask(RuntimeEventMask.ALL - RuntimeEventMask.MAP_LOADING_STARTED).await()
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()

      val types = drainUntil(runtime) { RuntimeEventType.MAP_STYLE_LOADED in it }
      assertTrue(RuntimeEventType.MAP_LOADING_STARTED !in types, "cleared type was delivered")
    }
  }

  @Test
  fun creationMaskNarrowsTheMapBeforeItsFirstStyleLoad(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val map =
        MapHandle.create(
            runtime,
            mapOptions().apply {
              eventMask = RuntimeEventMask.ALL - RuntimeEventMask.MAP_LOADING_STARTED
            },
          )
          .await()
      try {
        assertEquals(
          RuntimeEventMask.ALL - RuntimeEventMask.MAP_LOADING_STARTED,
          map.snapshot().eventMask,
        )
        map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()

        val types = drainUntil(runtime) { RuntimeEventType.MAP_STYLE_LOADED in it }
        assertTrue(RuntimeEventType.MAP_LOADING_STARTED !in types, "cleared type was delivered")
      } finally {
        map.close()
      }
    }
  }

  @Test
  fun bothHandlesReportEveryTypeUntilNarrowedAndKeepUnrelatedBitsOnAWrite(): Unit = runSuspendTest {
    withMap { runtime, map ->
      // The runtime reports its global queue mask; the map reports its map-originated subset.
      assertEquals(RuntimeEventMask.ALL, runtime.eventMask)
      assertEquals(RuntimeEventMask.ALL, map.snapshot().eventMask)

      runtime.eventMask = RuntimeEventMask.ALL
      map.setEventMask(RuntimeEventMask.ALL).await()
      assertEquals(RuntimeEventMask.ALL, runtime.eventMask)
      assertEquals(RuntimeEventMask.ALL, map.snapshot().eventMask)

      // Read, clear one bit, write back: every other bit survives.
      map.setEventMask(map.snapshot().eventMask - RuntimeEventMask.MAP_TILE_ACTION).await()
      assertEquals(
        RuntimeEventMask.ALL - RuntimeEventMask.MAP_TILE_ACTION,
        map.snapshot().eventMask,
      )
      assertTrue(RuntimeEventType.MAP_STYLE_LOADED in map.snapshot().eventMask)

      runtime.eventMask = runtime.eventMask - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
      assertEquals(
        RuntimeEventMask.ALL - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED,
        runtime.eventMask,
      )
    }
  }

  // The bit sits above the low 32, so a mask this binding narrowed on the way out would reach
  // native as a value it accepts.
  @Test
  fun maskBitOutsideEveryKnownTypeFailsEverySetterAndBothCreations(): Unit = runSuspendTest {
    val unknownBit = RuntimeEventMask.ALL + RuntimeEventMask(1L shl 63)
    assertFailsWith<InvalidArgumentException> {
      RuntimeHandle.create(RuntimeOptions().apply { eventMask = unknownBit })
    }
    withMap { runtime, map ->
      assertFailsWith<InvalidArgumentException> { runtime.eventMask = unknownBit }
      assertFailsWith<InvalidArgumentException> { map.setEventMask(unknownBit).await() }
      assertFailsWith<InvalidArgumentException> {
        MapHandle.create(runtime, mapOptions().apply { eventMask = unknownBit }).await()
      }
    }
  }

  @Test
  fun styleReplacementReleasesADroppedSourceWithNoStyleLoadedEvent(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      runtime
        .setResourceProvider(
          ResourceProviderCallback { request, handle ->
            if (request.requestedUrl != SERVED_STYLE_URL) {
              return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
            }
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = EMPTY_STYLE_JSON.encodeToByteArray()
              }
            )
            ResourceProviderDecision.HANDLE
          }
        )
        .await()
      val map =
        MapHandle.create(
            runtime,
            mapOptions().apply {
              eventMask = RuntimeEventMask.ALL - RuntimeEventMask.MAP_STYLE_LOADED
            },
          )
          .await()
      try {
        map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
        map.addCustomGeometrySource("custom", customGeometrySourceOptions()).await()
        assertEquals(1, map.customGeometrySourceCountForTesting())
        // The binding adds no subscription of its own, so the mask reads back as written.
        assertEquals(
          RuntimeEventMask.ALL - RuntimeEventMask.MAP_STYLE_LOADED,
          map.snapshot().eventMask,
          "the host's own mask changed",
        )

        // A URL load drops the source when the asynchronous load completes, and native
        // reports that through the release callback rather than through an event.
        map.setStyleUrl(SERVED_STYLE_URL).await()
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
  fun orderedStyleReplacementReleasesDroppedSourcesBeforeCompletion(): Unit = runSuspendTest {
    withMap { runtime, map ->
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
      runtime.barrier().await()
      map.addCustomGeometrySource("removed", customGeometrySourceOptions()).await()
      map.addCustomGeometrySource("dropped", customGeometrySourceOptions()).await()
      runtime.barrier().await()
      assertEquals(2, map.customGeometrySourceCountForTesting())

      map.removeStyleSource("removed").await()
      runtime.barrier().await()
      assertEquals(1, map.customGeometrySourceCountForTesting())

      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
      runtime.barrier().await()
      assertEquals(0, map.customGeometrySourceCountForTesting())

      map.addCustomGeometrySource("surviving", customGeometrySourceOptions()).await()
      runtime.barrier().await()
      map.close()
      runtime.barrier().await()
      assertEquals(0, map.customGeometrySourceCountForTesting())
    }
  }

  @Test
  fun aRejectedCustomSourceRegistrationKeepsNoPendingState(): Unit = runSuspendTest {
    withMap { runtime, map ->
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
      map.addCustomGeometrySource("live", customGeometrySourceOptions()).awaitCommitted()
      runtime.barrier().await()
      assertEquals(1, map.customGeometrySourceCountForTesting())

      // A closed map rejects the registration on the calling thread, so the registry keeps
      // neither the rejected state nor a displaced entry.
      map.close().await()
      assertFailsWith<InvalidStateException> {
        map.addCustomGeometrySource("rejected", customGeometrySourceOptions()).await()
      }
      assertEquals(0, map.customGeometrySourceCountForTesting())
    }
  }

  @Test
  fun drainAndBothMaskSettersAreAnyThread(): Unit = runSuspendTest {
    withMap { runtime, map ->
      val failure = AtomicReference<Throwable?>(null)
      val completion = AtomicReference<kotlinx.coroutines.Deferred<CommandCompletion>?>(null)
      runOnBackgroundThread {
        try {
          runtime.drainEvents()
          runtime.eventMask = RuntimeEventMask.ALL - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
          completion.store(
            map.setEventMask(RuntimeEventMask.ALL - RuntimeEventMask.MAP_TILE_ACTION)
          )
        } catch (throwable: Throwable) {
          failure.store(throwable)
        }
      }
      failure.load()?.let { throw it }
      completion.load()?.await()
      assertEquals(
        RuntimeEventMask.ALL - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED,
        runtime.eventMask,
      )
      assertEquals(
        RuntimeEventMask.ALL - RuntimeEventMask.MAP_TILE_ACTION,
        map.snapshot().eventMask,
      )
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
      width = 128
      height = 128
    }

  private suspend fun withMap(body: suspend (RuntimeHandle, MapHandle) -> Unit) {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val map = MapHandle.create(runtime, mapOptions()).await()
      try {
        body(runtime, map)
      } finally {
        map.close()
      }
    }
  }

  /** Drains until [done] holds for the event types seen so far. */
  private suspend fun drainUntil(
    runtime: RuntimeHandle,
    done: (Set<RuntimeEventType>) -> Boolean,
  ): Set<RuntimeEventType> {
    val types = mutableSetOf<RuntimeEventType>()
    repeat(10_000) {
      runtime.barrier().await()
      types += runtime.drainEvents().map { it.type }
      if (done(types)) return types
      usleep(1_000U)
    }
    error("the runtime did not report the events this test drove: $types")
  }
}

private const val SERVED_STYLE_URL = "custom://events-style.json"
