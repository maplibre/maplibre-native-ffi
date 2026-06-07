import Testing

@testable import MaplibreNative

@Test func projectedMetersRoundTrip() throws {
  let coordinate = LatLng(latitude: 45, longitude: -122)
  let meters = try Maplibre.projectedMeters(for: coordinate)
  let roundTripped = try Maplibre.latLng(forProjectedMeters: meters)

  #expect(abs(roundTripped.latitude - coordinate.latitude) < 0.000001)
  #expect(abs(roundTripped.longitude - coordinate.longitude) < 0.000001)
}

@Test func mapProjectionCameraAndCoordinateConversion() throws {
  let runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(runtime: runtime, options: MapOptions(width: 256, height: 256))
  defer { try? map.close() }
  try map.jump(to: CameraOptions(center: LatLng(latitude: 0, longitude: 0), zoom: 1))

  let projection = try MapProjectionHandle(map: map)
  try projection.setCamera(CameraOptions(center: LatLng(latitude: 1, longitude: 2), zoom: 2))
  let camera = try projection.camera()
  #expect(abs((camera.center?.latitude ?? 0) - 1) < 0.000001)
  #expect(abs((camera.center?.longitude ?? 0) - 2) < 0.000001)

  let point = try projection.pixel(for: LatLng(latitude: 1, longitude: 2))
  let coordinate = try projection.latLng(for: point)
  #expect(abs(coordinate.latitude - 1) < 0.000001)
  #expect(abs(coordinate.longitude - 2) < 0.000001)

  try projection.close()
  #expect(projection.isClosed)
}

@Test func mapProjectionSetVisibleCoordinatesRejectsEmptyInputBeforeCallingC() throws {
  let runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(runtime: runtime, options: MapOptions(width: 256, height: 256))
  defer { try? map.close() }
  let projection = try MapProjectionHandle(map: map)
  defer { try? projection.close() }

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
