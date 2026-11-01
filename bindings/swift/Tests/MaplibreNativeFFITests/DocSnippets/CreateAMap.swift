// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.

import MaplibreNativeFFI

/// Attaches a render target, as the Rendering guides show.
private func attachTarget(_: MapHandle) async throws -> RenderSessionHandle {
  fatalError("not run")
}

private func createAMap() async throws {
  // #region runtime
  let runtime = try RuntimeHandle.create(options: RuntimeOptions())
  // #endregion runtime

  // #region map
  let created = try runtime.createMap(options: MapOptions(
    initialExtent: LogicalExtent(width: 800, height: 600, scaleFactor: 2),
    mapMode: .continuous
  ))
  let map = created.map
  try await created.ready.value
  // #endregion map

  let session = try await attachTarget(map)

  // #region close
  try session.dispose()
  try await map.close()
  try await runtime.close()
  // #endregion close
}
