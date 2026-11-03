package org.maplibre.nativeffi.lifecycle

// `useAndClose` is AutoCloseable's `use`, which the suspending `use` imported below would shadow.
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlin.use as useAndClose
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.RuntimeEventType
import org.maplibre.nativeffi.RuntimeFixture
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.RuntimeOptions
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.TestWeakReference
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.requestCollection
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.use

/** How the generated owners close and keep their parents. */
class HandleLifecycleTest {
  @Test
  fun closingAHandleTwiceIsSafeAndLaterUseIsRefused() = runSuspendTest {
    val data = GeoJsonSourceDataHandle.create(POINT_JSON)
    data.close()
    data.close()

    val runtime = RuntimeHandle.create(RuntimeOptions())
    runtime.release().awaitWithin("the runtime teardown")
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.getEventMask() }
    // Native refuses a released handle, whether to release it again or to use it.
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.release() }
    assertFailsWithStatus(Status.INVALID_STATE) { runtime.clearResourceTransform() }
  }

  @Test
  fun aHandleNamesItsIdAndWhetherItIsClosed() {
    val data = GeoJsonSourceDataHandle.create(POINT_JSON)
    val id = "0x" + data.id.toString(16)
    assertFalse(data.isClosed)
    assertEquals("GeoJsonSourceDataHandle(id=$id, closed=false)", data.toString())
    data.close()
    assertTrue(data.isClosed)
    assertEquals("GeoJsonSourceDataHandle(id=$id, closed=true)", data.toString())
  }

  @Test
  fun aDisposeEndsTheHandleAndRepeatedCleanupDoesNothing() = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    assertFalse(runtime.isClosed)
    runtime.dispose()
    assertTrue(runtime.isClosed)
    // A second dispose, a close, and either `use` leave the disposed handle as it is.
    runtime.dispose()
    runtime.close()
    runtime.use {}
    runtime.useAndClose {}
  }

  @Test
  fun closingAHandleWithADisposeDisposesIt() = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    runtime.useAndClose {}
    assertTrue(runtime.isClosed)
    runtime.close()
  }

  @Test
  fun useLeavesAnOwnerThatItsBlockReleased() = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      runtime.release().awaitWithin("the runtime teardown")
    }
  }

  @Test
  fun aRefusedReleaseLeavesTheRuntimeUsableAndALaterReleaseSucceeds() = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
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
    val runtime = RuntimeHandle.create(RuntimeOptions())
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
    val runtime = RuntimeHandle.create(RuntimeOptions())
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
    val runtime = RuntimeHandle.create(RuntimeOptions())
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
    val (map, weakRuntime) = mapWithUnreferencedRuntime()
    var runtime: RuntimeHandle? = null
    try {
      // Only the map references its runtime, so the runtime survives a collection and still works.
      requestCollection()
      runtime = assertNotNull(weakRuntime.value as RuntimeHandle?, "a live map let its runtime go")
      runtime.clearResourceTransform().awaitWithin("a command on the runtime")
      map.setStyleJson(EMPTY_STYLE_JSON).awaitWithin("a command on the map")
    } finally {
      map.release().awaitWithin("the map release")
      runtime?.release()?.awaitWithin("the runtime release")
    }
  }

  private suspend fun returnFromUse(runtime: RuntimeHandle) {
    runtime.use {
      return
    }
  }

  /** Creates a map whose runtime nothing but the map references, and a weak reference to it. */
  private suspend fun mapWithUnreferencedRuntime(): Pair<MapHandle, TestWeakReference> {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val (map, ready) = runtime.createMap(smallMapOptions())
    ready.awaitWithin("the map")
    return map to TestWeakReference(runtime)
  }

  private companion object {
    const val POINT_JSON = """{"type":"Point","coordinates":[0,0]}"""
  }
}
