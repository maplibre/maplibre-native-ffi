package org.maplibre.nativeffi.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.RuntimeFixture
import org.maplibre.nativeffi.TestWeakReference
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitCollected
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.holdThreadUntil
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.smallMapOptions

/** How the generated owners close and keep their parents. */
@OptIn(ExperimentalAtomicApi::class)
class HandleLifecycleTest {
  @Test
  fun closingAHandleTwiceIsSafeAndLaterUseIsRefused() = runSuspendTest {
    val data = GeneratedApi.geojsonSourceDataCreate(POINT_JSON.encodeToByteArray())
    data.close()
    data.close()

    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    runtime.release().awaitWithin("the runtime teardown")
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.getEventMask() }
    // Native refuses a released handle, whether to release it again or to use it.
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.release() }
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.clearResourceTransform() }
  }

  @Test
  fun aRefusedReleaseLeavesTheRuntimeUsableAndALaterReleaseSucceeds() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map = runtime.createMap(smallMapOptions()).awaitWithin("the map")

    // Native refuses to release a runtime with a live map, so the call throws and the runtime
    // stays usable.
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.release() }
    runtime.getEventMask()
    runtime.clearResourceTransform().awaitWithin("a command on the refused runtime")

    map.release().awaitWithin("the map release")
    runtime.release().awaitWithin("the retried runtime release")
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.getEventMask() }
  }

  @Test
  fun aHandleIdNamesItsEventsAndOutlivesRelease() = runSuspendTest {
    RuntimeFixture.use {
      val map = runtime.createMap(smallMapOptions()).awaitWithin("the map")
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).awaitCommitted()
      val event = awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
      assertEquals(map.id, event.source)
      map.release().awaitWithin("the map release")
      // A host routes events that it drains after the release by the id it kept.
      assertEquals(event.source, map.id)
    }
  }

  @Test
  fun aMapKeepsItsRuntimeReachable() = runSuspendTest {
    val (map, runtime) = mapWithUnreferencedRuntime()
    try {
      // Once the collector has reclaimed an unrelated object, it has run a full cycle, and the
      // runtime is still reachable through its map.
      assertTrue(awaitCollected(unreferencedObject()), "the collector never ran")
      assertFalse(runtime.isCleared, "a live map let its runtime be collected")
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).awaitCommitted()
    } finally {
      map.release().awaitWithin("the map release")
      (runtime.value as RuntimeHandle?)?.release()?.awaitWithin("the runtime release")
    }
  }

  @Test
  fun droppingAMapCreationRetiresTheMapItCreates() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map = runtime.createMap(smallMapOptions()).awaitWithin("the first map")
    // Parking the runtime's worker holds the creation behind it, so the creation is still pending
    // when the test drops it.
    val resume = CompletableDeferred<Unit>()
    parkWorker(map, resume)
    val creation = runtime.createMap(smallMapOptions())
    try {
      creation.cancel()
    } finally {
      resume.complete(Unit)
    }
    // The creation completes on the worker before this command does, and the binding retires the
    // map it no longer has a caller for, so the runtime's only live child is the first map.
    map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).awaitCommitted()
    assertTrue(creation.isCancelled)
    map.release().awaitWithin("the first map's release")
    runtime.release().awaitWithin("the runtime release")
  }

  /**
   * Parks the runtime's worker inside the completion of one of [map]'s commands until [resume]
   * completes, and returns once it is parked. A command that completes before its handler is
   * registered runs the handler on this thread, which must not park, so the next command tries
   * again.
   */
  private suspend fun parkWorker(map: MapHandle, resume: CompletableDeferred<Unit>) {
    while (true) {
      val registered = AtomicInt(0)
      val parked = CompletableDeferred<Boolean>()
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).invokeOnCompletion {
        if (registered.load() == 0) {
          parked.complete(false)
        } else {
          parked.complete(true)
          holdThreadUntil(resume)
        }
      }
      registered.store(1)
      if (parked.awaitWithin("the worker to finish the command")) return
    }
  }

  /** A weak reference to an object that nothing else references. */
  private fun unreferencedObject(): TestWeakReference = TestWeakReference(Any())

  /** Creates a map whose runtime nothing but the map references, and a weak reference to it. */
  private suspend fun mapWithUnreferencedRuntime(): Pair<MapHandle, TestWeakReference> {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map = runtime.createMap(smallMapOptions()).awaitWithin("the map")
    return map to TestWeakReference(runtime)
  }

  private companion object {
    const val POINT_JSON = """{"type":"Point","coordinates":[0,0]}"""
  }
}
