package org.maplibre.nativeffi.internal.wasm

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertFalse
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.nextPageTask
import org.maplibre.nativeffi.withRuntime

/**
 * What the page pays for having called this binding.
 *
 * The owner thread answers through a ring that nothing signals, so the page polls it on a
 * zero-delay task. A poll that rescheduled itself unconditionally would keep that task running for
 * as long as the document lived, which on a phone is battery spent on a map that is doing nothing.
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
}
