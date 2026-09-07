package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.camera.GesturePhase
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use

class CameraTransitionTest {
  @Test
  fun aTransitionReportsOneTerminalOutcome(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(runtime, mapOptions()).await().use { map ->
        runtime.drainEvents()

        // A running transition reports nothing until something ends it.
        map
          .submitCameraUpdate(CameraUpdateMode.EASE, zoom = 4.0, transitionId = 11)
          .awaitCommitted()
        assertEquals(emptyList(), runtime.drainFinishedTransitions())

        // A second ease replaces the first, which ends the first and reports the
        // replaced ID rather than the superseding one.
        map
          .submitCameraUpdate(CameraUpdateMode.EASE, zoom = 6.0, transitionId = 12)
          .awaitCommitted()
        assertEquals(listOf(11L), runtime.drainFinishedTransitions())

        // A jump cancels the running transition, which reports the cancelled ID.
        map.submitCameraUpdate(CameraUpdateMode.JUMP, zoom = 8.0).awaitCommitted()
        assertEquals(listOf(12L), runtime.drainFinishedTransitions())

        // An ease with no transition ID is silent, and so is the jump that ends it.
        map.submitCameraUpdate(CameraUpdateMode.EASE, zoom = 10.0).awaitCommitted()
        map.submitCameraUpdate(CameraUpdateMode.JUMP, zoom = 12.0).awaitCommitted()
        assertEquals(emptyList(), runtime.drainFinishedTransitions())
      }
    }
  }

  @Test
  fun cancelTransitionsEndsARunningTransitionAndCommitsWithNoneRunning(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(runtime, mapOptions()).await().use { map ->
        runtime.drainEvents()

        map
          .submitCameraUpdate(CameraUpdateMode.EASE, zoom = 5.0, transitionId = 21)
          .awaitCommitted()
        map.cancelTransitions().awaitCommitted()
        assertEquals(listOf(21L), runtime.drainFinishedTransitions())

        // Cancelling with nothing running still commits and reports nothing.
        map.cancelTransitions().awaitCommitted()
        assertEquals(emptyList(), runtime.drainFinishedTransitions())
      }
    }
  }

  @Test
  fun aGestureMarksTheMapWithoutEndingARunningTransition(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(runtime, mapOptions()).await().use { map ->
        runtime.drainEvents()
        assertFalse(map.snapshot().gestureInProgress)

        map
          .submitCameraUpdate(
            CameraUpdateMode.EASE,
            zoom = 5.0,
            transitionId = 31,
            gesturePhase = GesturePhase.BEGIN,
          )
          .awaitCommitted()
        assertTrue(map.snapshot().gestureInProgress)
        // BEGIN only sets the flag; the transition it rode in with is still running.
        assertEquals(emptyList(), runtime.drainFinishedTransitions())

        map
          .submitCameraUpdate(CameraUpdateMode.JUMP, zoom = 5.0, gesturePhase = GesturePhase.END)
          .awaitCommitted()
        assertFalse(map.snapshot().gestureInProgress)
        assertEquals(listOf(31L), runtime.drainFinishedTransitions())
      }
    }
  }

  @Test
  fun closingTheMapCancelsACommandItNeverFinished(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val map = MapHandle.create(runtime, mapOptions().apply { mapMode = MapMode.STATIC }).await()
      // A static map with no render session never renders the image, so the request is still
      // outstanding when the map retires.
      val pending = map.requestStillImage()
      map.close().await()

      val completion = pending.await()
      assertEquals(MaplibreStatus.CANCELLED, completion.status)
    }
  }

  private fun mapOptions(): MapOptions =
    MapOptions().apply {
      width = 64
      height = 64
    }

  /**
   * Submits one camera update whose transition outlives the next command, so every outcome the
   * caller asserts is the one it named rather than a transition that ended on its own.
   */
  private fun MapHandle.submitCameraUpdate(
    mode: CameraUpdateMode,
    zoom: Double,
    transitionId: Long? = null,
    gesturePhase: GesturePhase = GesturePhase.NONE,
  ) =
    updateCamera(
      CameraUpdate(
        mode = mode,
        camera = CameraOptions().apply { this.zoom = zoom },
        animation =
          AnimationOptions().apply {
            durationMs = 60_000.0
            this.transitionId = transitionId
          },
        gesturePhase = gesturePhase,
      )
    )

  private fun RuntimeHandle.drainFinishedTransitions(): List<Long> =
    drainEvents()
      .filter { it.type == RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED }
      .map { (it.payload as RuntimeEventPayload.CameraTransitionFinished).transitionId }
}
