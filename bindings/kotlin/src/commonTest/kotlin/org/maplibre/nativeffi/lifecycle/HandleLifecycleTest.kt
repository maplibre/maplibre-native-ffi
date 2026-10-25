package org.maplibre.nativeffi.lifecycle

// AutoCloseable's `use`, which the suspending `use` imported below would shadow.
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertTrue
import kotlin.use as useAndClose
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.RuntimeFixture
import org.maplibre.nativeffi.TestWeakReference
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitCollected
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.RuntimeOptions
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.use
import org.maplibre.nativeffi.smallMapOptions

/** How the generated owners close and keep their parents. */
class HandleLifecycleTest {
  @Test
  fun closingAHandleTwiceIsSafeAndLaterUseIsRefused() = runSuspendTest {
    val data = GeneratedApi.geoJsonSourceDataCreate(POINT_JSON)
    data.close()
    data.close()

    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    runtime.release().awaitWithin("the runtime teardown")
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.getEventMask() }
    // Native refuses a released handle, whether to release it again or to use it.
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.release() }
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.clearResourceTransform() }
  }

  @Test
  fun aHandleNamesItsIdAndWhetherItIsClosed() {
    val data = GeneratedApi.geoJsonSourceDataCreate(POINT_JSON)
    val id = "0x" + data.id.toString(16)
    assertFalse(data.isClosed)
    assertEquals("GeoJsonSourceDataHandle(id=$id, closed=false)", data.toString())
    data.close()
    assertTrue(data.isClosed)
    assertEquals("GeoJsonSourceDataHandle(id=$id, closed=true)", data.toString())
  }

  @Test
  fun aDisposeEndsTheHandleAndRepeatedCleanupDoesNothing() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    val id = "0x" + runtime.id.toString(16)
    assertFalse(runtime.isClosed)
    assertEquals("RuntimeHandle(id=$id, closed=false)", runtime.toString())
    runtime.dispose()
    assertTrue(runtime.isClosed)
    assertEquals("RuntimeHandle(id=$id, closed=true)", runtime.toString())
    // A second dispose, a close, and either `use` leave the disposed handle as it is.
    runtime.dispose()
    runtime.close()
    runtime.use {}
    runtime.useAndClose {}
  }

  @Test
  fun closingAHandleWithADisposeDisposesIt() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    runtime.useAndClose {}
    assertTrue(runtime.isClosed)
    runtime.close()
  }

  @Test
  fun useLeavesAnOwnerThatItsBlockReleased() = runSuspendTest {
    GeneratedApi.runtimeCreate(RuntimeOptions()).use { runtime ->
      runtime.release().awaitWithin("the runtime teardown")
    }
  }

  @Test
  fun aRefusedReleaseLeavesTheRuntimeUsableAndALaterReleaseSucceeds() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    val (map, ready) = runtime.createMap(smallMapOptions())
    ready.awaitWithin("the map")

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
  fun useKeepsTheBlockFailureAndSuppressesTheRefusedRelease() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    lateinit var map: MapHandle

    // The live map makes native refuse the runtime release after the block throws.
    val failure =
      assertFailsWith<IllegalStateException> {
        runtime.use {
          val (created, ready) = it.createMap(smallMapOptions())
          map = created
          ready.awaitWithin("the map")
          error("the block failure")
        }
      }
    assertEquals("the block failure", failure.message)
    val suppressed = failure.suppressedExceptions.single()
    assertEquals(Status.INVALID_STATE, assertIs<MaplibreException>(suppressed).status)

    map.release().awaitWithin("the map release")
    runtime.release().awaitWithin("the retried runtime release")
  }

  @Test
  fun useReportsARefusedReleaseAfterTheBlockReturns() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    lateinit var map: MapHandle

    assertFailsWithStatus(Status.INVALID_STATE) {
      runtime.use {
        val (created, ready) = it.createMap(smallMapOptions())
        map = created
        ready.awaitWithin("the map")
      }
    }

    map.release().awaitWithin("the map release")
    runtime.release().awaitWithin("the retried runtime release")
  }

  @Test
  fun useReleasesTheOwnerWhenItsBlockReturnsFromTheCaller() = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    returnFromUse(runtime)
    assertTrue(runtime.isClosed)
  }

  @Test
  fun aHandleIdNamesItsEventsAndOutlivesRelease() = runSuspendTest {
    RuntimeFixture.use {
      val (map, ready) = runtime.createMap(smallMapOptions())
      ready.awaitWithin("the map")
      map.setStyleJson(EMPTY_STYLE_JSON).await()
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
      map.setStyleJson(EMPTY_STYLE_JSON).await()
    } finally {
      map.release().awaitWithin("the map release")
      (runtime.value as RuntimeHandle?)?.release()?.awaitWithin("the runtime release")
    }
  }

  /** A weak reference to an object that nothing else references. */
  private suspend fun returnFromUse(runtime: RuntimeHandle) {
    runtime.use {
      return
    }
  }

  private fun unreferencedObject(): TestWeakReference = TestWeakReference(Any())

  /** Creates a map whose runtime nothing but the map references, and a weak reference to it. */
  private suspend fun mapWithUnreferencedRuntime(): Pair<MapHandle, TestWeakReference> {
    val runtime = GeneratedApi.runtimeCreate(RuntimeOptions())
    val (map, ready) = runtime.createMap(smallMapOptions())
    ready.awaitWithin("the map")
    return map to TestWeakReference(runtime)
  }

  private companion object {
    const val POINT_JSON = """{"type":"Point","coordinates":[0,0]}"""
  }
}
