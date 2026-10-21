@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.browser

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.TestSupport
import org.maplibre.nativeffi.awaitDeliveries
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.internal.callback.NativeRoots
import org.maplibre.nativeffi.requestCollection
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withConsoleLog
import org.maplibre.nativeffi.withMap

/** Kotlin on the browser page, which never blocks, drives runtimes and maps. */
class PageThreadTest {
  @Test
  fun aRuntimeCreatedOnThePageCreatesAMap() = runSuspendTest {
    assertTrue(crossOriginIsolated(), "the test page is not cross-origin isolated")
    assertTrue(onThePage(), "the test runs off the page thread")
    val roots = NativeRoots.countForTesting()

    // The page refuses mln_runtime_create, so a runtime that arrives came from a thread that the
    // binding started, and the page yields while that thread starts.
    val creation =
      async(start = CoroutineStart.UNDISPATCHED) {
        GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
      }
    assertFalse(creation.isCompleted, "runtime creation returned before its thread could start")
    val runtime = creation.awaitWithin("the runtime")

    // Native queues the map for the page, which drains it only after it yields.
    val creatingMap = runtime.createMap(smallMapOptions())
    assertFalse(creatingMap.isCompleted, "the map arrived inside the call that created it")
    val map = creatingMap.awaitWithin("the map")

    map.release().awaitWithin("the map release")
    runtime.release().awaitWithin("the runtime release")
    assertTrue(map.isClosed)
    awaitDeliveries()
    assertEquals(roots, NativeRoots.countForTesting(), "a completion root outlived its release")
  }

  @Test
  fun runtimeCreationCompletesWhileThePoolIsEmpty() = runSuspendTest {
    val runtime = TestSupport.withEmptyPool {
      assertEquals(0, TestSupport.idleWorkers(), "the pool kept a Worker")
      val workers = TestSupport.allWorkers()
      // The creation's thread needs a Worker that the page creates, and that Worker starts only
      // after the page yields, so a call that waited for the start would never return.
      val creation =
        async(start = CoroutineStart.UNDISPATCHED) {
          GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
        }
      assertFalse(creation.isCompleted, "runtime creation returned before the page yielded")
      val runtime = creation.awaitWithin("the runtime")
      assertTrue(TestSupport.allWorkers() > workers, "the creation started no new Worker")
      runtime
    }
    runtime.release().awaitWithin("the runtime release")
  }

  @Test
  fun aCancelledCreationReleasesTheRuntimeItReceives() = runSuspendTest {
    val leaks = mutableListOf<String>()
    withConsoleLog({ line -> if (line.startsWith("Leaked RuntimeHandle ")) leaks += line }) {
      val roots = NativeRoots.countForTesting()
      val creation =
        async(start = CoroutineStart.UNDISPATCHED) {
          GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
        }
      creation.cancel()
      // The creation's thread still runs, so the caller resumes once its runtime arrives.
      creation.join()
      assertTrue(creation.isCancelled)
      // The binding disposes the runtime that arrived, so its owner reports no leak once the
      // collector reclaims it, and the creation's root is gone.
      requestCollection()
      awaitDeliveries()
      assertEquals(roots, NativeRoots.countForTesting(), "the creation's root outlived it")
      assertEquals(emptyList(), leaks, "the cancelled creation's runtime leaked")
    }
  }

  @Test
  fun twoRuntimesCreatedOnThePageLoadStylesWithoutAPump() = runSuspendTest {
    withMap {
      val first = this
      withMap {
        assertNotEquals(first.runtime.id, runtime.id)
        // Each runtime runs its maps on an executor thread of its own, so the page only waits for
        // the posted results, and nothing services either runtime from the page.
        val styles =
          listOf(first, this).map { it.map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()) }
        styles.forEach { it.awaitWithin("a style command") }
        first.awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
        awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
      }
    }
  }
}

private fun crossOriginIsolated(): Boolean = js("globalThis.crossOriginIsolated === true")

// A dedicated or pthread worker has no document.
private fun onThePage(): Boolean = js("typeof document === 'object'")
