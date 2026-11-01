// Snippets for the docs site's Camera page. They compile with the tests, which
// never run them.

import MaplibreNativeFFI

private func useCamera(_: Camera) {}

private func flyEnded(_: Bool) {}

// #region gesture

private func beginGesture(_ map: MapHandle) throws {
  try map.discarding.cancelTransitions()
  try map.discarding.applyCameraDelta(CameraDelta(gesturePhase: .begin))
}

private func applyDrag(_ map: MapHandle, movement: ScreenPoint) throws {
  try map.discarding.applyCameraDelta(
    CameraDelta(offset: movement, gesturePhase: .update)
  )
}

private func applyPinch(
  _ map: MapHandle,
  scale: Double,
  center: ScreenPoint
) throws {
  try map.discarding.applyCameraDelta(
    CameraDelta(scale: scale, anchor: center, gesturePhase: .update)
  )
}

private func endGesture(_ map: MapHandle) throws {
  try map.discarding.applyCameraDelta(CameraDelta(gesturePhase: .end))
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
  let fitted = try await map.cameraForLatLngBounds(bounds)

  try map.discarding.updateCamera(CameraUpdate(
    mode: .ease,
    camera: fitted,
    animation: AnimationOptions(durationMs: 500)
  ))
  // #endregion fit

  // #region limit
  try map.discarding.setBounds(
    options: BoundOptions(bounds: bounds, minZoom: 5, maxZoom: 16)
  )
  // #endregion limit

  try beginGesture(map)
  try applyDrag(map, movement: ScreenPoint(x: 12, y: -4))
  try applyPinch(map, scale: 1.1, center: ScreenPoint(x: 200, y: 150))
  try endGesture(map)

  // #region convert
  let projection = try MapProjectionHandle.create(
    view: map.getSnapshot().view
  )

  let coordinate = try projection.latLngForPixel(
    point: ScreenPoint(x: 100, y: 200)
  )
  let point = try projection.pixelForLatLng(coordinate: coordinate)

  projection.close()
  // #endregion convert
  _ = point
}
