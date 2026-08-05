package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.wasm.AsyncDelivery
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/**
 * Parks a delivered callback body makes while it holds its gate, so a close of that gate must wait.
 *
 * Enough that the wait spans several turns of the page's event loop, which is what leaves the task
 * below a turn to run in; few enough that the test costs a handful of round trips.
 */
private const val PARKS_INSIDE_THE_CALLBACK = 8

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

  /**
   * The same refusal again, from a page task that ran while a *close* was parked.
   *
   * A draining close is the binding's other suspension, and it is the one that hides. Nothing about
   * closing a callback registration looks like a call that parks, and the stack it happens on is
   * the host's own scope rather than a fresh one — but park it does, because what it waits for is a
   * callback body suspended on another stack, and on a page only the event loop can resume that. So
   * for the length of the wait the page is running tasks with no promising stack anywhere, and a
   * task that dispatched then would reach a suspending import with no suspender to unwind into.
   *
   * The window is built here rather than raced for. The shape that opens it in earnest — a custom
   * geometry callback answering its tile inline while the host removes the source — turns on
   * catching a body between two of its own parks, which is a race a test can only lose quietly. The
   * pieces below are the real ones either way: the gate a retiring registration closes, the queue a
   * proxied notification is delivered on, and the dispatcher's own park.
   */
  @Test
  fun aCallFromAPageTaskDuringAParkedCloseNamesTheScopeToo(): Promise<JsAny?> = browserTest {
    var failure: Throwable? = null
    var taskRanWhileTheCloseWaited = false
    var bodyFinished = false
    maplibreScope {
      withRuntime { runtime ->
        val gate = CallbackGate("a callback this test retires")
        var bodyInside = false
        // What a proxied notification does: delivered on a promising stack of its own, holding the
        // gate for as long as the host body runs, and parking that stack on every owner-affine call
        // the body makes.
        AsyncDelivery.post {
          val lease = gate.enter() ?: return@post
          bodyInside = true
          try {
            repeat(PARKS_INSIDE_THE_CALLBACK) { runtime.pollEvent() }
          } finally {
            bodyInside = false
            bodyFinished = true
            lease.close()
          }
        }
        // Hands the page over, so the delivery above runs and reaches a park of its own. From here
        // the body is a suspended stack holding the gate, which is exactly what a close waits for.
        runtime.pollEvent()
        assertTrue(bodyInside, "the delivered body never reached a park inside the gate")

        var closeWaiting = false
        // Queued before the close begins, and so ahead of the timer the close's own first turn is
        // posted on: the page reaches this task while the close is away rather than after it.
        runOnNextPageTask {
          taskRanWhileTheCloseWaited = closeWaiting
          failure = runCatching { RuntimeHandle.create(RuntimeOptions()) }.exceptionOrNull()
        }
        closeWaiting = true
        gate.close()
        closeWaiting = false
      }
    }

    assertTrue(bodyFinished, "the close returned before the body it was retiring had finished")
    assertTrue(
      taskRanWhileTheCloseWaited,
      "the page task ran outside the window the close was parked in, so it proves nothing",
    )
    val raised = assertNotNull(failure, "the page task raised nothing at all")
    assertTrue(raised is InvalidStateException, "the page task raised $raised")
    assertTrue(
      raised.message.orEmpty().contains("maplibreScope"),
      "the failure says \"${raised.message}\", which does not name maplibreScope",
    )
  }
}
