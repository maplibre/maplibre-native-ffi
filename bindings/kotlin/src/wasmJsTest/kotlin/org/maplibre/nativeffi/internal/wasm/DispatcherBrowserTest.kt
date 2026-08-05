package org.maplibre.nativeffi.internal.wasm

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.beginCapturingUncaughtErrors
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.endCapturingUncaughtErrors
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.nextPageTask
import org.maplibre.nativeffi.withRuntime

/**
 * The task the page runs on this binding's behalf, and what it costs to get wrong.
 *
 * The owner thread answers through a ring that nothing signals, so the page polls it on a
 * zero-delay task. A poll that rescheduled itself unconditionally would keep that task running for
 * as long as the document lived, which on a phone is battery spent on a map that is doing nothing;
 * a poll that stopped rescheduling by accident is worse, because it is the only thing that ever
 * wakes a parked caller.
 */
class DispatcherBrowserTest {
  @Test
  fun theCompletionDrainStopsOnceNoCallIsOutstanding(): Promise<JsAny?> = browserTest {
    // A runtime created and closed, which is two dispatched calls and so a drain that certainly
    // started.
    maplibreScope { withRuntime {} }

    // The turn that takes the last completion is scheduled before the caller it wakes has resumed
    // to count itself out, so one further turn always runs. A few page turns is more than that and
    // far less than forever.
    repeat(4) { nextPageTask() }

    assertFalse(
      Dispatcher.isDraining,
      "the drain kept rescheduling itself with nothing left to drain",
    )

    // Restarting is the half that matters as much: a caller that parks after the drain stopped has
    // nothing to wake it, so this call would never return rather than fail.
    maplibreScope { withRuntime {} }
    repeat(4) { nextPageTask() }

    assertFalse(Dispatcher.isDraining, "the drain did not stop again after it was restarted")
  }

  /**
   * A turn that threw leaves the drain able to run again, rather than believing one is scheduled.
   *
   * The flag that says a turn is scheduled is set by the caller that starts the drain and cleared
   * only by a turn, and a call that finds it set starts nothing. So a turn that ended without
   * either clearing it or scheduling its successor takes the page with it: every caller already
   * parked waits for an answer nothing will collect, and every later call parks behind the same
   * flag. That is a whole map lost to one failed task, which is why the recovery is a finally
   * rather than the turn's last statement.
   *
   * The failure is injected onto the turn that runs with nothing outstanding, so what is asserted
   * is the flag itself rather than a call that would have hung. It reaches the page as an uncaught
   * error, because a task the dispatcher scheduled for itself has no caller to report to, and this
   * stands in front of the harness's error handler for exactly as long as it expects one.
   */
  @Test
  fun aDrainTurnThatThrewLeavesTheDrainAbleToStartAgain(): Promise<JsAny?> = browserTest {
    // Two dispatched calls, so a drain certainly started. The turn that took the last completion
    // scheduled another before the caller it woke had resumed to count itself out, so exactly one
    // turn is pending here -- and a page task cannot have run in between, because resuming from a
    // scope is microtask work.
    maplibreScope { withRuntime {} }
    assertTrue(Dispatcher.isDraining, "no turn was pending, so the fault below would take none")

    var reported = ""
    beginCapturingUncaughtErrors()
    try {
      InjectedFaults.failNextDrainTurn()
      // The pending turn was scheduled before this one, so it runs, and throws, first.
      nextPageTask()
    } finally {
      InjectedFaults.reset()
      reported = endCapturingUncaughtErrors()
    }

    assertTrue(reported.isNotEmpty(), "the page was never told the drain turn had failed")
    assertFalse(
      Dispatcher.isDraining,
      "a turn that threw left the drain believing a turn was still scheduled",
    )

    // And the dispatcher really is usable rather than merely reporting that it is: this call has to
    // start the drain the failed turn left stopped, and it parks until that drain answers it.
    maplibreScope { withRuntime {} }
    repeat(4) { nextPageTask() }
    assertFalse(Dispatcher.isDraining, "the drain did not stop after the call that restarted it")
  }
}
