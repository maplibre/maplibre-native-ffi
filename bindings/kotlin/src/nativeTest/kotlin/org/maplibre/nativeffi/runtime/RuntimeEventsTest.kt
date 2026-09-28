package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapOptions
import org.maplibre.nativeffi.generated.RuntimeEventMask
import org.maplibre.nativeffi.generated.RuntimeEventSourceType
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.runOnBackgroundThread

@OptIn(
  kotlin.experimental.ExperimentalNativeApi::class,
  kotlin.native.runtime.NativeRuntimeApi::class,
)
class RuntimeEventsTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun oneDrainAfterStyleLoadReturnsEveryQueuedEvent(): Unit = runSuspendTest {
    withMap { runtime, map ->
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
      // Nothing drains while the style parses, so one drain reports the whole run.
      repeat(20) { runtime.barrier().await() }

      val events = runtime.drainEvents().use { it.get().events }
      assertTrue(events.size > 1, "expected several events, got $events")
      assertTrue(events.any { it.type == RuntimeEventType.MAP_STYLE_LOADED })
      assertTrue(events.all { it.sourceType == RuntimeEventSourceType.MAP })
    }
  }

  @Test
  fun narrowedMapMaskDropsOneTypeAndKeepsAnother(): Unit = runSuspendTest {
    withMap { runtime, map ->
      // One style load produces both types, so both are driven after this write.
      map.setEventMask(RuntimeEventMask.ALL without RuntimeEventMask.MAP_LOADING_STARTED).await()
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()

      val types = drainUntil(runtime) { RuntimeEventType.MAP_STYLE_LOADED in it }
      assertTrue(RuntimeEventType.MAP_LOADING_STARTED !in types, "cleared type was delivered")
    }
  }

  @Test
  fun creationMaskNarrowsTheMapBeforeItsFirstStyleLoad(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val map =
        runtime
          .mapCreate(
            mapOptions()
              .copy(eventMask = RuntimeEventMask.ALL without RuntimeEventMask.MAP_LOADING_STARTED)
          )
          .await()
      try {
        assertEquals(
          RuntimeEventMask.ALL without RuntimeEventMask.MAP_LOADING_STARTED,
          map.snapshotGet().eventMask,
        )
        map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()

        val types = drainUntil(runtime) { RuntimeEventType.MAP_STYLE_LOADED in it }
        assertTrue(RuntimeEventType.MAP_LOADING_STARTED !in types, "cleared type was delivered")
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun bothHandlesReportEveryTypeUntilNarrowedAndKeepUnrelatedBitsOnAWrite(): Unit = runSuspendTest {
    withMap { runtime, map ->
      // The runtime reports its global queue mask; the map reports its map-originated subset.
      assertEquals(RuntimeEventMask.ALL, runtime.getEventMask())
      assertEquals(RuntimeEventMask.ALL, map.snapshotGet().eventMask)

      runtime.setEventMask(RuntimeEventMask.ALL)
      map.setEventMask(RuntimeEventMask.ALL).await()
      runtime.barrier().await()
      assertEquals(RuntimeEventMask.ALL, runtime.getEventMask())
      assertEquals(RuntimeEventMask.ALL, map.snapshotGet().eventMask)

      // Read, clear one bit, write back: every other bit survives.
      map.setEventMask(map.snapshotGet().eventMask without RuntimeEventMask.MAP_TILE_ACTION).await()
      assertEquals(
        RuntimeEventMask.ALL without RuntimeEventMask.MAP_TILE_ACTION,
        map.snapshotGet().eventMask,
      )
      assertTrue(RuntimeEventMask.MAP_STYLE_LOADED in map.snapshotGet().eventMask)

      runtime.setEventMask(
        runtime.getEventMask() without RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
      )
      assertEquals(
        RuntimeEventMask.ALL without RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED,
        runtime.getEventMask(),
      )
    }
  }

  // The bit sits above the low 32, so a mask this binding narrowed on the way out would reach
  // native as a value it accepts.
  @Test
  fun maskBitOutsideEveryKnownTypeFailsEverySetterAndBothCreations(): Unit = runSuspendTest {
    val unknownBit = RuntimeEventMask.ALL or RuntimeEventMask(1uL shl 63)
    assertFailsWith<InvalidArgumentException> {
      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault().copy(eventMask = unknownBit))
    }
    withMap { runtime, map ->
      assertFailsWith<InvalidArgumentException> { runtime.setEventMask(unknownBit) }
      assertFailsWith<InvalidArgumentException> { map.setEventMask(unknownBit).await() }
      assertFailsWith<InvalidArgumentException> {
        runtime.mapCreate(mapOptions().copy(eventMask = unknownBit)).await()
      }
    }
  }

  @Test
  fun styleReplacementReleasesADroppedSourceWithNoStyleLoadedEvent(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ request, handle ->
                if (request.requestedUrl != SERVED_STYLE_URL) {
                  return@provider ResourceProviderDecision.PASS_THROUGH
                }
                handle.resourceRequestComplete(
                  ResourceResponse(
                    status = ResourceResponseStatus.OK,
                    bytes = EMPTY_STYLE_JSON.encodeToByteArray(),
                  )
                )
                handle.close()
                ResourceProviderDecision.HANDLE
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            mapOptions()
              .copy(eventMask = RuntimeEventMask.ALL without RuntimeEventMask.MAP_STYLE_LOADED)
          )
          .await()
      try {
        map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).awaitCommitted()
        addCustomGeometrySource(map, "custom").awaitCommitted()
        assertLiveSources(1)
        // The binding adds no subscription of its own, so the mask reads back as written.
        assertEquals(
          RuntimeEventMask.ALL without RuntimeEventMask.MAP_STYLE_LOADED,
          map.snapshotGet().eventMask,
          "the host's own mask changed",
        )

        // A URL load drops the source when the asynchronous load completes, and native
        // reports that through the release callback rather than through an event.
        map.setStyleUrl(SERVED_STYLE_URL).awaitCommitted()
        val types = drainUntil(runtime) { liveSourceCount() == 0 }
        assertTrue(
          RuntimeEventType.MAP_STYLE_LOADED !in types,
          "a cleared style-loaded event reached the host",
        )
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun inlineStyleCommandsReleaseTheirSourcesAfterTheBarrier(): Unit = runSuspendTest {
    withMap { runtime, map ->
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
      runtime.barrier().await()
      addCustomGeometrySource(map, "removed").await()
      addCustomGeometrySource(map, "dropped").await()
      runtime.barrier().await()
      assertLiveSources(2, "register")

      map.removeStyleSource("removed").await()
      runtime.barrier().await()
      assertLiveSources(1, "remove")

      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
      runtime.barrier().await()
      assertLiveSources(0, "style replacement")

      // Closing the map quiesces and releases the source it still owns.
      addCustomGeometrySource(map, "surviving").await()
      runtime.barrier().await()
      map.release().await()
      runtime.barrier().await()
      assertLiveSources(0, "map close")
    }
  }

  @Test
  fun aRejectedCustomSourceRegistrationKeepsNoPendingState(): Unit = runSuspendTest {
    withMap { runtime, map ->
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).await()
      addCustomGeometrySource(map, "live").awaitCommitted()
      runtime.barrier().await()
      assertLiveSources(1)

      // A closed map rejects the registration on the calling thread, so the registry keeps
      // neither the rejected state nor a displaced entry.
      map.release().await()
      rejectRegistration(map)
      assertLiveSources(0)
    }
  }

  @Test
  fun drainAndBothMaskSettersAreAnyThread(): Unit = runSuspendTest {
    withMap { runtime, map ->
      val failures = mutableListOf<Throwable>()
      var committed: kotlinx.coroutines.Deferred<CommandCompletion>? = null
      runOnBackgroundThread {
        try {
          runtime.drainEvents().use { it.get().events }
          runtime.setEventMask(
            RuntimeEventMask.ALL without RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
          )
          committed =
            map.setEventMask(RuntimeEventMask.ALL without RuntimeEventMask.MAP_TILE_ACTION)
        } catch (error: Throwable) {
          failures += error
        }
      }
      assertTrue(failures.isEmpty(), "any-thread APIs failed: $failures")
      committed?.await()

      // Each setter wrote a distinct mask, so the read-back proves both crossed threads.
      assertEquals(
        RuntimeEventMask.ALL without RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED,
        runtime.getEventMask(),
      )
      assertEquals(
        RuntimeEventMask.ALL without RuntimeEventMask.MAP_TILE_ACTION,
        map.snapshotGet().eventMask,
      )
    }
  }

  private suspend fun rejectRegistration(map: MapHandle) {
    assertFailsWith<InvalidStateException> { addCustomGeometrySource(map, "rejected").await() }
  }

  private val sources = mutableListOf<kotlin.native.ref.WeakReference<Any>>()

  // Keep callback arguments out of suspended test frames when checking native ownership.
  private fun addCustomGeometrySource(
    map: MapHandle,
    sourceId: String,
  ): Deferred<CommandCompletion> {
    val captured = Any()
    sources += kotlin.native.ref.WeakReference(captured)
    return map.addCustomGeometrySource(
      sourceId,
      CustomGeometrySourceOptions(fetchTile = { captured.hashCode() }),
    )
  }

  private fun liveSourceCount(): Int {
    kotlin.native.runtime.GC.collect()
    return sources.count { it.get() != null }
  }

  private fun assertLiveSources(expected: Int, phase: String = "release") {
    assertEquals(
      expected,
      liveSourceCount(),
      "native release must drop callback captures after $phase",
    )
  }

  private fun mapOptions(): MapOptions =
    GeneratedApi.mapOptionsDefault()
      .copy(
        initialExtent =
          GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
      )

  private suspend fun withMap(body: suspend (RuntimeHandle, MapHandle) -> Unit) {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val map = runtime.mapCreate(mapOptions()).await()
      try {
        body(runtime, map)
      } finally {
        map.release()
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
      types += runtime.drainEvents().use { it.get().events }.map { it.type }
      if (done(types)) return types
      org.maplibre.nativeffi.sleepMillis(1)
    }
    error("the runtime did not report the events this test drove: $types")
  }
}

private const val SERVED_STYLE_URL = "custom://events-style.json"

private infix fun RuntimeEventMask.without(other: RuntimeEventMask) =
  RuntimeEventMask(rawValue and other.rawValue.inv())
