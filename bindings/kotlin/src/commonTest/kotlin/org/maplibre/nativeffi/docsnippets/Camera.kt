// Snippets for the docs site's Camera page. They compile with the
// tests, which never run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.AnimationOptions
import org.maplibre.nativeffi.BoundOptions
import org.maplibre.nativeffi.Camera
import org.maplibre.nativeffi.CameraFitUpdate
import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraTransitionHandler
import org.maplibre.nativeffi.CameraTransitionOutcome
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.CameraUpdateMode
import org.maplibre.nativeffi.GestureInput
import org.maplibre.nativeffi.GesturePointer
import org.maplibre.nativeffi.GesturePointerDown
import org.maplibre.nativeffi.GestureRecognizerHandle
import org.maplibre.nativeffi.GestureResult
import org.maplibre.nativeffi.LatLng
import org.maplibre.nativeffi.LatLngBounds
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MapProjectionHandle
import org.maplibre.nativeffi.PointerKind
import org.maplibre.nativeffi.ScreenPoint

private fun useCamera(camera: Camera) {}

private fun usePoint(point: ScreenPoint) {}

private fun flyEnded(completed: Boolean) {}

// #region gesture
private fun createRecognizer(
  map: MapHandle,
): GestureRecognizerHandle = map.gestureRecognizerCreate()

private fun scheduleTick(result: GestureResult) {
  // On the next frame, or at result.nextTickMs.
}

private fun pointerDown(
  recognizer: GestureRecognizerHandle,
  id: UInt,
  position: ScreenPoint,
  timeMs: Double,
) {
  val input =
    GestureInput.PointerDown(
      GesturePointerDown(
        id = id,
        kind = PointerKind.TOUCH,
        position = position,
      ),
    )
  scheduleTick(recognizer.handle(input, timeMs))
}

private fun pointerMove(
  recognizer: GestureRecognizerHandle,
  id: UInt,
  position: ScreenPoint,
  timeMs: Double,
) {
  val input =
    GestureInput.PointerMove(
      GesturePointer(id = id, position = position),
    )
  scheduleTick(recognizer.handle(input, timeMs))
}

private fun tick(
  recognizer: GestureRecognizerHandle,
  timeMs: Double,
) {
  scheduleTick(recognizer.tick(timeMs))
}

// #endregion gesture

private suspend fun camera(map: MapHandle) {
  // #region read
  val snapshot = map.getSnapshot()
  useCamera(snapshot.view.camera)

  val result = map.getCamera().await()
  useCamera(result.camera)
  // #endregion read

  // #region jump
  map.discarding.updateCamera(
    CameraUpdate(
      mode = CameraUpdateMode.JUMP,
      camera =
        CameraOptions(
          center = LatLng(latitude = 48.8566, longitude = 2.3522),
          zoom = 12.0,
        ),
    ),
  )
  // #endregion jump

  // #region animate
  map.discarding.updateCamera(
    CameraUpdate(
      mode = CameraUpdateMode.FLY,
      camera =
        CameraOptions(
          center = LatLng(latitude = 40.7128, longitude = -74.006),
          zoom = 10.0,
        ),
      animation =
        AnimationOptions(
          durationMs = 3000.0,
          // Runs after the animation ends, on a library thread, or on
          // the page in Kotlin/Wasm.
          endHandler =
            CameraTransitionHandler { end ->
              flyEnded(
                end.outcome == CameraTransitionOutcome.COMPLETED,
              )
            },
        ),
    ),
  )
  // #endregion animate

  // #region fit
  val bounds =
    LatLngBounds(
      southwest = LatLng(latitude = 45.8, longitude = 5.9),
      northeast = LatLng(latitude = 47.8, longitude = 10.5),
    )
  map.discarding.fitCameraToLatLngBounds(
    bounds,
    CameraFitUpdate(
      mode = CameraUpdateMode.EASE,
      animation = AnimationOptions(durationMs = 500.0),
    ),
  )
  // #endregion fit

  // #region limit
  map.discarding.setBounds(
    BoundOptions(bounds = bounds, minZoom = 5.0, maxZoom = 16.0),
  )
  // #endregion limit

  val recognizer = createRecognizer(map)
  pointerDown(recognizer, 1u, ScreenPoint(x = 100.0, y = 100.0), 0.0)
  pointerMove(recognizer, 1u, ScreenPoint(x = 112.0, y = 96.0), 16.0)
  tick(recognizer, 32.0)
  recognizer.release()

  // #region convert
  val projection = MapProjectionHandle.create(map.getSnapshot().view)

  val coordinate =
    projection.latLngForPixel(ScreenPoint(x = 100.0, y = 200.0))
  val point = projection.pixelForLatLng(coordinate)

  projection.release()
  // #endregion convert
  usePoint(point)
}
