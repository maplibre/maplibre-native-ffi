@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.WebObject
import org.maplibre.nativeffi.runSuspendTest

/** A JavaScript object lent to the module is a handle into the module's table until it closes. */
class WebObjectTest {
  @Test
  fun aLentObjectIsItsHandlesUntilItCloses() = runSuspendTest {
    val canvas = offscreenCanvas()
    val first = WebObject.of(canvas)
    val second = WebObject.of(canvas)
    assertNotEquals(0u, first.handle)
    assertNotEquals(first.handle, second.handle)
    assertTrue(sameObject(canvas, WebObject.get(first.handle)), "the handle names another object")

    val handle = first.handle
    first.close()
    first.close()
    assertNull(WebObject.get(handle))
    assertFailsWith<IllegalStateException> { first.handle }
    assertTrue(
      sameObject(canvas, WebObject.get(second.handle)),
      "a close released the other handle",
    )
    second.close()
  }

  @Test
  fun aHandleThatNamesNoObjectGetsNull() = runSuspendTest {
    assertNull(WebObject.get(0u))
    assertNull(WebObject.get(UInt.MAX_VALUE))
    assertNull(WebObject.get(0x7fff_ffffu))
  }
}

private fun offscreenCanvas(): JsAny = js("new OffscreenCanvas(4, 4)")

private fun sameObject(expected: JsAny, actual: JsAny?): Boolean = js("expected === actual")
