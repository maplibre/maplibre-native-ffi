@testable import MaplibreNativeFFI
import Testing

@Test func projectedMetersRoundTrip() throws {
  let coordinate = LatLng(latitude: 45, longitude: -122)
  let meters = try Maplibre.projectedMeters(for: coordinate)
  let roundTripped = try Maplibre.latLng(forProjectedMeters: meters)

  #expect(abs(roundTripped.latitude - coordinate.latitude) < 0.000001)
  #expect(abs(roundTripped.longitude - coordinate.longitude) < 0.000001)
}

@Test func mapProjectionCameraAndCoordinateConversion() async throws {
  let runtime =
    try await RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 256, height: 256))
  defer { try? map.closeBlockingForTests() }
  _ = try map.updateCamera(CameraUpdate(camera: CameraOptions(
    center: LatLng(latitude: 0, longitude: 0),
    zoom: 1
  )))
  try await runtime.barrier()

  let projection = try await MapProjectionHandle(map: map)
  try projection.setCamera(CameraOptions(
    center: LatLng(latitude: 1, longitude: 2),
    zoom: 2
  ))
  let camera = try await projection.camera()
  #expect(abs((camera.center?.latitude ?? 0) - 1) < 0.000001)
  #expect(abs((camera.center?.longitude ?? 0) - 2) < 0.000001)

  let point = try await projection.pixel(
    for: LatLng(latitude: 1, longitude: 2)
  )
  let coordinate = try await projection.latLng(for: point)
  #expect(abs(coordinate.latitude - 1) < 0.000001)
  #expect(abs(coordinate.longitude - 2) < 0.000001)

  try await projection.close()
  #expect(projection.isClosed)
}

@Test func mapProjectionSetVisibleCoordinatesRejectsEmptyInputBeforeCallingC(
) async throws {
  let runtime =
    try await RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 256, height: 256))
  defer { try? map.closeBlockingForTests() }
  let projection = try await MapProjectionHandle(map: map)
  defer { try? projection.closeBlockingForTests() }

  do {
    try projection.setVisibleCoordinates([])
    Issue.record("empty coordinates should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}
