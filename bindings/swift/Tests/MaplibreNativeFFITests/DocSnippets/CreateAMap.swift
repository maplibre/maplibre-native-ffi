// Snippets for the docs site's Create a map page. They compile with
// the tests, which never run them.

import MaplibreNativeFFI

private func reportFailure(_: any Error) {}

/// Attaches a render target, as the render target guides show.
private func attachTarget(_: MapHandle) async throws
  -> RenderSessionHandle
{
  fatalError("not run")
}

private func createAMap() async throws {
  // #region runtime
  let runtime = try RuntimeHandle.create(options: RuntimeOptions())
  // #endregion runtime

  // #region map
  let created = try runtime.createMap(options: MapOptions(
    initialExtent: LogicalExtent(
      width: 800,
      height: 600,
      scaleFactor: 2
    ),
    mapMode: .continuous
  ))
  let map = created.map
  try await created.ready.value
  // #endregion map

  // #region style
  try map.discarding
    .setStyleUrl("https://demotiles.maplibre.org/style.json")
  // #endregion style

  let update = CameraUpdate(
    mode: .jump,
    camera: CameraOptions(zoom: 3)
  )

  // #region wait
  try await map.updateCamera(update)

  // Use the discarding variant to ignore the result.
  try map.discarding.updateCamera(update)
  // #endregion wait

  // #region errors
  do {
    try await map.updateCamera(update)
  } catch {
    // Its message describes the failure.
    reportFailure(error)
  }
  // #endregion errors

  let session = try await attachTarget(map)

  // #region close
  try session.dispose()
  try map.dispose()
  try await runtime.close()
  // #endregion close
}
