package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

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

  /**
   * The failure a host gets for the mistake this API invites.
   *
   * Every other target's actuals are ordinary synchronous functions, so host code shared with one
   * of them arrives here with no scope around it. The suspension underneath is legal only on a
   * promising stack, and reaching it without one traps in the virtual machine or surfaces as an
   * opaque JavaScript error — neither of which says what was left out.
   */
  @Test
  fun aCallOutsideAScopeNamesTheScopeItNeeded(): Promise<JsAny?> = browserTest {
    val failure = assertFailsWith<InvalidStateException> { RuntimeHandle.create(RuntimeOptions()) }

    assertTrue(
      failure.message.orEmpty().contains("maplibreScope"),
      "the failure says \"${failure.message}\", which does not name maplibreScope",
    )
  }

  /**
   * The same refusal from a page task that ran while a scope was parked.
   *
   * A scope that is parked is not a stack that is running: the page reached its event loop, and
   * anything it runs there is on a stack of its own that may not park either. A guard that only
   * asked whether a scope had been entered would report the parked scope's answer to this frame and
   * let it trap.
   */
  @Test
  fun aCallFromAPageTaskDuringAParkedScopeNamesTheScopeToo(): Promise<JsAny?> = browserTest {
    var failure: Throwable? = null
    maplibreScope {
      runOnNextPageTask {
        failure = runCatching { RuntimeHandle.create(RuntimeOptions()) }.exceptionOrNull()
      }
      // A runtime created and closed the ordinary way, which parks this stack and so lets the task
      // above run before the scope ends.
      RuntimeHandle.create(RuntimeOptions()).close()
    }

    val raised = assertNotNull(failure, "the page task raised nothing at all")
    assertTrue(raised is InvalidStateException, "the page task raised $raised")
    assertTrue(
      raised.message.orEmpty().contains("maplibreScope"),
      "the failure says \"${raised.message}\", which does not name maplibreScope",
    )
  }
}
