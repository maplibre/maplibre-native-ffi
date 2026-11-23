// Snippets for the docs site's Camera page. They compile with the
// tests, which
// never run them.

import MaplibreNativeFFI

private func useCamera(_: Camera) {}

private func flyEnded(_: Bool) {}

// #region gesture

private func createRecognizer(
  _ map: MapHandle
) throws -> GestureRecognizerHandle {
  try map.gestureRecognizerCreate()
}

private func scheduleTick(_: GestureResult) {
  // On the next frame, or at the result's nextTickMs.
}

private func pointerDown(
  _ recognizer: GestureRecognizerHandle,
  id: UInt32,
  position: ScreenPoint,
  timeMs: Double
) throws {
  let result = try recognizer.handle(
    input: .pointerDown(
      GesturePointerDown(id: id, kind: .touch, position: position)
    ),
    timeMs: timeMs
  )
  scheduleTick(result)
}

private func pointerMove(
  _ recognizer: GestureRecognizerHandle,
  id: UInt32,
  position: ScreenPoint,
  timeMs: Double
) throws {
  let result = try recognizer.handle(
    input: .pointerMove(GesturePointer(id: id, position: position)),
    timeMs: timeMs
  )
  scheduleTick(result)
}

private func tick(
  _ recognizer: GestureRecognizerHandle,
  timeMs: Double
) throws {
  try scheduleTick(recognizer.tick(timeMs: timeMs))
}

// #endregion gesture

private func camera(_ map: MapHandle) async throws {
  // #region read
  let snapshot = try map.getSnapshot()
  useCamera(snapshot.view.camera)

  let result = try await map.getCamera()
  useCamera(result.camera)
  // #endregion read

  // #region jump
  try map.discarding.updateCamera(CameraUpdate(
    mode: .jump,
    camera: CameraOptions(
      center: LatLng(latitude: 48.8566, longitude: 2.3522),
      zoom: 12
    )
  ))
  // #endregion jump

  // #region animate
  try map.discarding.updateCamera(CameraUpdate(
    mode: .fly,
    camera: CameraOptions(
      center: LatLng(latitude: 40.7128, longitude: -74.006),
      zoom: 10
    ),
    animation: AnimationOptions(
      durationMs: 3000,
      // Runs on a library thread, after the animation ends.
      endHandler: CameraTransitionHandler { end in
        flyEnded(end.outcome == .completed)
      }
    )
  ))
  // #endregion animate

  // #region fit
  let bounds = LatLngBounds(
    southwest: LatLng(latitude: 45.8, longitude: 5.9),
    northeast: LatLng(latitude: 47.8, longitude: 10.5)
  )
  try map.discarding.fitCameraToLatLngBounds(
    bounds,
    update: CameraFitUpdate(
      mode: .ease,
      animation: AnimationOptions(durationMs: 500)
    )
  )
  // #endregion fit

  // #region limit
  try map.discarding.setBounds(
    options: BoundOptions(bounds: bounds, minZoom: 5, maxZoom: 16)
  )
  // #endregion limit

  let recognizer = try createRecognizer(map)
  try pointerDown(
    recognizer,
    id: 1,
    position: ScreenPoint(x: 100, y: 100),
    timeMs: 0
  )
  try pointerMove(
    recognizer,
    id: 1,
    position: ScreenPoint(x: 112, y: 96),
    timeMs: 16
  )
  try tick(recognizer, timeMs: 32)
  recognizer.release()

  // #region convert
  let projection = try MapProjectionHandle.create(
    view: map.getSnapshot().view
  )

  let coordinate = try projection.latLngForPixel(
    point: ScreenPoint(x: 100, y: 200)
  )
  let point = try projection.pixelForLatLng(coordinate: coordinate)

  projection.release()
  // #endregion convert
  _ = point
}
