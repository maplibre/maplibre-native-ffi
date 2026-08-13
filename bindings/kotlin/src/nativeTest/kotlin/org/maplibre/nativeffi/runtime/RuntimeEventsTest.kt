package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class RuntimeEventsTest : org.maplibre.nativeffi.NativeTestBase() {
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
  fun styleReplacementReleasesADroppedSourceWithNoStyleLoadedEvent(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
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
        )
        val map =
          MapHandle.create(
            runtime,
            mapOptions().apply {
              eventMask = RuntimeEventMask.ALL - RuntimeEventMask.MAP_STYLE_LOADED
            },
          )
        try {
          map.setStyleJson(STYLE_JSON.encodeToByteArray())
          map.addCustomGeometrySource("custom", customGeometrySourceOptions())
          assertEquals(1, map.customGeometrySourceCountForTesting())
          // The binding adds no subscription of its own, so the mask reads back as written.
          assertEquals(
            RuntimeEventMask.ALL_MAP_EVENTS - RuntimeEventMask.MAP_STYLE_LOADED,
            map.eventMask,
            "the host's own mask changed",
          )

          // A URL load drops the source in the pump that completes it, and native
          // reports that through the release callback rather than through an event.
          map.setStyleUrl(SERVED_STYLE_URL)
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
  fun orderedStyleReplacementReleasesDroppedSourcesBeforeCompletion(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      withMap { runtime, map ->
        map.setStyleJson(STYLE_JSON.encodeToByteArray())
        runtime.barrier()
        map.addCustomGeometrySource("removed", customGeometrySourceOptions())
        map.addCustomGeometrySource("dropped", customGeometrySourceOptions())
        runtime.barrier()
        assertEquals(2, map.customGeometrySourceCountForTesting())

        assertTrue(map.removeStyleSource("removed"))
        assertEquals(1, map.customGeometrySourceCountForTesting())

        map.setStyleJson(STYLE_JSON.encodeToByteArray())
        runtime.barrier()
        assertEquals(0, map.customGeometrySourceCountForTesting())

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
        val failure = AtomicReference<Throwable?>(null)
        runOnNativeThread(BackgroundEventCalls(runtime, map, failure))
        failure.load()?.let { throw it }
        assertEquals(
          RuntimeEventMask.ALL - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED,
          runtime.eventMask,
        )
        assertEquals(
          RuntimeEventMask.ALL_MAP_EVENTS - RuntimeEventMask.MAP_TILE_ACTION,
          map.eventMask,
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
      val map = MapHandle.create(runtime, mapOptions())
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
      runtime.barrier()
      types += runtime.drainEvents().events.map { it.type }
      if (done(types)) return types
      usleep(1_000U)
    }
    error("the runtime did not report the events this test drove: $types")
  }

  private fun runOnNativeThread(calls: BackgroundEventCalls) {
    memScoped {
      val selfRef = StableRef.create(calls)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::runEventCallsOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundEventCalls(
  private val runtime: RuntimeHandle,
  private val map: MapHandle,
  private val failure: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      runtime.drainEvents()
      runtime.eventMask = RuntimeEventMask.ALL - RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
      map.eventMask = RuntimeEventMask.ALL - RuntimeEventMask.MAP_TILE_ACTION
    } catch (throwable: Throwable) {
      failure.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun runEventCallsOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundEventCalls>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

private const val STYLE_JSON = "{\"version\":8,\"sources\":{},\"layers\":[]}"
private const val SERVED_STYLE_URL = "custom://events-style.json"
