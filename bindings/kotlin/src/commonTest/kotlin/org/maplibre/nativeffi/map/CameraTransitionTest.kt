package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CameraUpdateMode
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.GesturePhase
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.MapOptions
import org.maplibre.nativeffi.generated.RuntimeEventPayload
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use

class CameraTransitionTest {
  @Test
  fun aTransitionReportsOneTerminalOutcome(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime.mapCreate(mapOptions()).await().use { map ->
        runtime.drainFinishedTransitions()

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
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime.mapCreate(mapOptions()).await().use { map ->
        runtime.drainFinishedTransitions()

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
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime.mapCreate(mapOptions()).await().use { map ->
        runtime.drainFinishedTransitions()
        assertFalse(map.snapshotGet().gestureInProgress)

        map
          .submitCameraUpdate(
            CameraUpdateMode.EASE,
            zoom = 5.0,
            transitionId = 31,
            gesturePhase = GesturePhase.BEGIN,
          )
          .awaitCommitted()
        assertTrue(map.snapshotGet().gestureInProgress)
        // BEGIN only sets the flag; the transition it rode in with is still running.
        assertEquals(emptyList(), runtime.drainFinishedTransitions())

        map
          .submitCameraUpdate(CameraUpdateMode.JUMP, zoom = 5.0, gesturePhase = GesturePhase.END)
          .awaitCommitted()
        assertFalse(map.snapshotGet().gestureInProgress)
        assertEquals(listOf(31L), runtime.drainFinishedTransitions())
      }
    }
  }

  @Test
  fun closingTheMapCancelsACommandItNeverFinished(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val map = runtime.mapCreate(mapOptions().copy(mapMode = MapMode.STATIC)).await()
      // A static map with no render session never renders the image, so the request is still
      // outstanding when the map retires.
      val pending = map.requestStillImage()
      map.release().await()

      val failure =
        kotlin.test.assertFailsWith<org.maplibre.nativeffi.error.MaplibreException> {
          pending.await()
        }
      assertEquals(MaplibreStatus.CANCELLED, failure.status)
    }
  }

  private fun mapOptions(): MapOptions =
    GeneratedApi.mapOptionsDefault()
      .copy(
        initialExtent =
          GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
      )

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
        camera = CameraOptions(zoom = zoom),
        animation = AnimationOptions(durationMs = 60_000.0, transitionId = transitionId?.toULong()),
        gesturePhase = gesturePhase,
      )
    )

  private fun RuntimeHandle.drainFinishedTransitions(): List<Long> {
    val batch =
      try {
        drainEvents()
      } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
        if (error.status == MaplibreStatus.NOT_READY) return emptyList()
        throw error
      }
    return batch.use { owner ->
      owner
        .get()
        .events
        .filter { it.type == RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED }
        .map {
          (it.payload as RuntimeEventPayload.CameraTransitionFinished).value.transitionId.toLong()
        }
    }
  }
}
