@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.browser

import kotlin.test.Test
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.RuntimeEventType
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.RuntimeOptions
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap

/**
 * Kotlin on the browser page drives runtimes and maps. The module runs on the page too, never
 * blocks it, and reports each completion in a task of its own after the call that started it.
 */
class PageThreadTest {
  @Test
  fun aRuntimeCreatedOnThePageCreatesAMap() = runSuspendTest {
    assertTrue(onThePage(), "the test runs off the page thread")
    val runtime = RuntimeHandle.create(RuntimeOptions())

    // Native queues the map's readiness for the page, which drains it only after it yields.
    val map = runtime.createMap(smallMapOptions())
    assertFalse(
      map.ready.asDeferred().isCompleted,
      "the map was ready inside the call that created it",
    )
    map.ready.awaitWithin("the map")

    map.release().awaitWithin("the map release")
    runtime.release().awaitWithin("the runtime release")
    assertTrue(map.isClosed)
  }

  @Test
  fun twoRuntimesOnThePageLoadStyles() = runSuspendTest {
    withMap {
      val first = this
      withMap {
        assertNotEquals(first.runtime.id, runtime.id)
        val styles = listOf(first, this).map { it.map.setStyleJson(EMPTY_STYLE_JSON) }
        styles.forEach { it.awaitWithin("a style command") }
        first.awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
        awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
      }
    }
  }
}

private fun onThePage(): Boolean = js("typeof document !== 'undefined'")
