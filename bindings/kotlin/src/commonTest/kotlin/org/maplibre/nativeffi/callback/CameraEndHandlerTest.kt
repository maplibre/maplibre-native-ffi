package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.AnimationOptions
import org.maplibre.nativeffi.CameraDelta
import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraTransitionEnd
import org.maplibre.nativeffi.CameraTransitionHandler
import org.maplibre.nativeffi.CameraTransitionOutcome
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.CameraUpdateMode
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** Camera end handlers, which a command carries two records deep in its input. */
class CameraEndHandlerTest {
  @Test
  fun anEndHandlerIsRootedUntilItRuns() = runSuspendTest {
    withMap {
      val roots = map.bindingCallbacks
      val base = roots.rootCountForTesting()
      val ended = CompletableDeferred<CameraTransitionEnd>()
      val update =
        CameraUpdate(
          mode = CameraUpdateMode.EASE,
          camera = CameraOptions(zoom = 4.0),
          animation =
            AnimationOptions(
              durationMs = 60000.0,
              transitionId = ULong.MAX_VALUE,
              endHandler = CameraTransitionHandler { ended.complete(it) },
            ),
        )
      map.updateCamera(update).await()
      assertEquals(base + 1, roots.rootCountForTesting())

      // Native runs the handler once, and releases it, when the identity is cancelled.
      map.cancelCameraTransition(ULong.MAX_VALUE).await()
      val end = ended.awaitWithin("the end handler")
      assertEquals(CameraTransitionOutcome.CANCELLED, end.outcome)
      assertNotEquals(0uL, end.generation)
      // The handler's release runs on the worker before a later command on the map.
      map.requestRepaint().awaitWithin("a command after the end")
      assertEquals(base, roots.rootCountForTesting())

      // A command that native rejects on the calling thread roots nothing.
      val rejected =
        CameraDelta(
          scale = -1.0,
          animation = AnimationOptions(endHandler = CameraTransitionHandler {}),
        )
      assertFailsWithStatus(Status.INVALID_ARGUMENT) { map.applyCameraDelta(rejected) }
      assertEquals(base, roots.rootCountForTesting())
    }
  }
}
