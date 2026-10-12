package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraTransitionEnd
import org.maplibre.nativeffi.generated.CameraTransitionHandler
import org.maplibre.nativeffi.generated.CameraTransitionOutcome
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CameraUpdateMode
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.withMap

/** Camera end handlers, which a command carries two records deep in its input. */
class CameraEndHandlerTest {
  @Test
  fun anEndHandlerIsRootedUntilItRuns(): Unit = runSuspendTest {
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
      map.updateCamera(update).awaitCommitted()
      assertEquals(base + 1, roots.rootCountForTesting())

      // Native runs the handler once, and releases it, when the identity is cancelled.
      map.cancelCameraTransition(ULong.MAX_VALUE).awaitCommitted()
      val end = ended.awaitWithin("the end handler")
      assertEquals(CameraTransitionOutcome.CANCELLED, end.outcome)
      assertNotEquals(0uL, end.generation)
      runtime.barrier().awaitWithin("the barrier after the end")
      assertEquals(base, roots.rootCountForTesting())

      // A command that native rejects on the calling thread roots nothing.
      val rejected =
        CameraDelta(
          scale = -1.0,
          animation = AnimationOptions(endHandler = CameraTransitionHandler {}),
        )
      assertFailsWith<InvalidArgumentException> { map.applyCameraDelta(rejected) }
      assertEquals(base, roots.rootCountForTesting())
    }
  }
}
