package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

/**
 * The stack that owner-affine calls need, and what crosses it.
 *
 * A scope establishes a `WebAssembly.promising` frame, which is the only stack a suspending import
 * may park on. Nothing below it is reachable without one, so these are the first tests to fail when
 * the browser or the toolchain stops providing the mechanism.
 */
class MaplibreScopeBrowserTest {
  @Test
  fun aScopeReturnsWhatItsBlockProduced(): Promise<JsAny?> = browserTest {
    // The block runs on a stack entered through the promising trampoline rather than on the
    // caller's, so its result travels back through the scope rather than out of the call.
    assertEquals("produced inside the scope", maplibreScope { "produced inside the scope" })
  }

  @Test
  fun aScopeCarriesAFailureBackWithItsOwnType(): Promise<JsAny?> = browserTest {
    val failure =
      assertFailsWith<IllegalStateException> { maplibreScope { error("raised inside the scope") } }

    // Thrown across the promising boundary it would arrive as an opaque rejection, so this is what
    // says the failure was carried rather than thrown.
    assertEquals("raised inside the scope", failure.message)
  }

  @Test
  fun aFailedScopeLeavesTheGateOpenForTheNextOne(): Promise<JsAny?> = browserTest {
    assertFailsWith<IllegalStateException> { maplibreScope { error("raised inside the scope") } }

    // Scopes are serialized by one module-wide gate. A scope that failed while holding it would
    // park every later one forever, so this is what says the gate is released on both paths.
    assertEquals(2, maplibreScope { 2 })
  }
}
