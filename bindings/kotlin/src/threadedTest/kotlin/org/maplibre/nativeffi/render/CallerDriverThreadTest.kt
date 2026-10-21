package org.maplibre.nativeffi.render

import kotlin.test.Test
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.runSuspendTest

/** A caller-driven session that a second Kotlin thread tries to service. */
class CallerDriverThreadTest {
  @Test
  fun aCallerDrivenSessionStaysOnTheGraphicsThreadItWasServicedFrom() = runSuspendTest {
    withOwnedTexture {
      setStyle(RED_BACKGROUND_STYLE)
      // The fixture services the session from a coroutine on its own graphics thread, woken by the
      // session's driver-work and frame wakes, while this test runs on another thread.
      renderStill()

      assertFailsWithStatus(Status.WRONG_THREAD) { session.serviceDriverWork() }
    }
  }
}
