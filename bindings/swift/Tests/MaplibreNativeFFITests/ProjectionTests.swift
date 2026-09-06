import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func projectedMetersRoundTrip() throws {
  let coordinate = LatLng(latitude: 45, longitude: -122)
  let meters = try Maplibre.projectedMeters(for: coordinate)
  let roundTripped = try Maplibre.latLng(forProjectedMeters: meters)

  #expect(abs(roundTripped.latitude - coordinate.latitude) < 0.000001)
  #expect(abs(roundTripped.longitude - coordinate.longitude) < 0.000001)
}

/// A projection created after a camera command observes that command, every
/// later call is synchronous, a setter changes later conversions, and close is
/// synchronous.
@Test func mapProjectionIsSynchronousAfterCreation() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 256, height: 256))
  defer { try? map.closeBlockingForTests() }
  _ = try await map.updateCamera(CameraUpdate(camera: CameraOptions(
    center: LatLng(latitude: 10, longitude: 20),
    zoom: 3
  )))

  // Creation is ordered after the accepted camera command, so the copied
  // transform observes it without a barrier.
  let projection = try await MapProjectionHandle(map: map)
  let created = try projection.camera()
  #expect(abs((created.center?.latitude ?? 0) - 10) < 0.000001)
  #expect(abs((created.center?.longitude ?? 0) - 20) < 0.000001)
  #expect(abs((created.zoom ?? 0) - 3) < 0.000001)

  // A synchronous conversion round-trips within tolerance.
  let point = try projection.pixel(for: LatLng(latitude: 10, longitude: 20))
  let coordinate = try projection.latLng(for: point)
  #expect(abs(coordinate.latitude - 10) < 0.000001)
  #expect(abs(coordinate.longitude - 20) < 0.000001)

  // A setter applies before returning and changes later conversions.
  try projection.setCamera(CameraOptions(
    center: LatLng(latitude: 1, longitude: 2),
    zoom: 5
  ))
  let updated = try projection.camera()
  #expect(abs((updated.center?.latitude ?? 0) - 1) < 0.000001)
  #expect(abs((updated.center?.longitude ?? 0) - 2) < 0.000001)
  let moved = try projection.pixel(for: LatLng(latitude: 10, longitude: 20))
  #expect(abs(moved.x - point.x) > 1 || abs(moved.y - point.y) > 1)

  try await map.close()
  try await runtime.close()
  let detached = try projection.latLng(for: moved)
  #expect(abs(detached.latitude - 10) < 0.000001)
  #expect(abs(detached.longitude - 20) < 0.000001)

  // Close is synchronous and works from any thread.
  try await Task.detached {
    _ = try projection.camera()
    try projection.close()
  }.value
  #expect(projection.isClosed)
}

/// Unwrapped conversions keep the visible world copy that wrapped
/// conversions fold back into -180 to 180.
@Test func unwrappedCoordinateConversionsPreserveVisibleWorldCopies(
) async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1024, height: 512))
  defer { try? map.closeBlockingForTests() }
  _ = try await map.updateCamera(CameraUpdate(camera: CameraOptions(
    center: LatLng(latitude: 0, longitude: 180),
    zoom: 0
  )))
  let points = [ScreenPoint(x: 0, y: 256), ScreenPoint(x: 1024, y: 256)]

  let wrapped = try await map.latLngs(for: points)
  let unwrapped = try await map.latLngs(for: points, unwrapped: true)
  #expect(wrapped.allSatisfy { (-180 ... 180).contains($0.longitude) })
  #expect(unwrapped[1].longitude - unwrapped[0].longitude > 360)
  let wrappedRight = try await map.latLng(for: points[1])
  #expect((-180 ... 180).contains(wrappedRight.longitude))
  let right = try await map.latLng(for: points[1], unwrapped: true)
  #expect(abs(right.longitude - unwrapped[1].longitude) < 0.0000000001)

  let projection = try await MapProjectionHandle(map: map)
  defer { try? projection.close() }
  #expect(try (-180 ... 180)
    .contains(projection.latLng(for: points[1]).longitude))
  let projectedRight = try projection.latLng(for: points[1], unwrapped: true)
  #expect(abs(projectedRight.longitude - right.longitude) < 0.0000000001)
}

/// Projection calls are internally serialized, so a second thread converts
/// through the same live handle.
@Test func mapProjectionIsUsableFromASecondThread() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 256, height: 256))
  defer { try? map.closeBlockingForTests() }
  let projection = try await MapProjectionHandle(map: map)
  defer { try? projection.close() }

  let expected = try projection.pixel(for: LatLng(latitude: 5, longitude: 6))
  let result = try await Task.detached {
    try projection.pixel(for: LatLng(latitude: 5, longitude: 6))
  }.value
  #expect(abs(result.x - expected.x) < 0.000001)
  #expect(abs(result.y - expected.y) < 0.000001)
}

@Test func mapProjectionSetVisibleCoordinatesRejectsEmptyInputBeforeCallingC(
) async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 256, height: 256))
  defer { try? map.closeBlockingForTests() }
  let projection = try await MapProjectionHandle(map: map)
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
