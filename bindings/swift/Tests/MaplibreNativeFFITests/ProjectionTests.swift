import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func projectedMetersRoundTrip() throws {
  let coordinate = LatLng(latitude: 45, longitude: -122)
  let meters = try Maplibre.projectedMetersForLatLng(coordinate: coordinate)
  let roundTripped = try Maplibre.latLngForProjectedMeters(meters: meters)

  #expect(abs(roundTripped.latitude - coordinate.latitude) < 0.000001)
  #expect(abs(roundTripped.longitude - coordinate.longitude) < 0.000001)
}

/// A projection created after a camera command observes that command, every
/// later call is synchronous, a setter changes later conversions, and close is
/// synchronous.
@Test func mapProjectionIsSynchronousAfterCreation() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 256,
      height: 256,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  _ = try await map.updateCamera(update: CameraUpdate(camera: CameraOptions(
    center: LatLng(latitude: 10, longitude: 20),
    zoom: 3
  )))

  // Creation is ordered after the accepted camera command, so the copied
  // transform observes it without a barrier.
  let projection = try await map.projectionCreate()
  let created = try projection.getCamera()
  #expect(abs((created.center?.latitude ?? 0) - 10) < 0.000001)
  #expect(abs((created.center?.longitude ?? 0) - 20) < 0.000001)
  #expect(abs((created.zoom ?? 0) - 3) < 0.000001)

  let meters = try await map.metersPerPixelAtLatitude(latitude: 45)
  #expect(try projection.metersPerPixelAtLatitude(latitude: 45) == meters)

  // A synchronous conversion round-trips within tolerance.
  let point = try projection.pixelForLatLng(coordinate: LatLng(
    latitude: 10,
    longitude: 20
  ))
  let coordinate = try projection.latLngForPixel(point: point)
  #expect(abs(coordinate.latitude - 10) < 0.000001)
  #expect(abs(coordinate.longitude - 20) < 0.000001)

  // A setter applies before returning and changes later conversions.
  try projection.setCamera(camera: CameraOptions(
    center: LatLng(latitude: 1, longitude: 2),
    zoom: 5
  ))
  #expect(try projection.metersPerPixelAtLatitude(latitude: 45) == meters / 4)
  #expect(try await map.metersPerPixelAtLatitude(latitude: 45) == meters)

  let updated = try projection.getCamera()
  #expect(abs((updated.center?.latitude ?? 0) - 1) < 0.000001)
  #expect(abs((updated.center?.longitude ?? 0) - 2) < 0.000001)
  let moved = try projection.pixelForLatLng(coordinate: LatLng(
    latitude: 10,
    longitude: 20
  ))
  #expect(abs(moved.x - point.x) > 1 || abs(moved.y - point.y) > 1)

  try await map.close()
  try await runtime.close()
  let detached = try projection.latLngForPixel(point: moved)
  #expect(abs(detached.latitude - 10) < 0.000001)
  #expect(abs(detached.longitude - 20) < 0.000001)

  // Close is synchronous and works from any thread.
  try await Task.detached {
    _ = try projection.getCamera()
    try projection.close()
  }.value
  #expect(projection.isClosed)
}

/// Unwrapped conversions keep the visible world copy that wrapped
/// conversions fold back into -180 to 180.
@Test func unwrappedCoordinateConversionsPreserveVisibleWorldCopies(
) async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1024,
      height: 512,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  _ = try await map.updateCamera(update: CameraUpdate(camera: CameraOptions(
    center: LatLng(latitude: 0, longitude: 180),
    zoom: 0
  )))
  let points = [ScreenPoint(x: 0, y: 256), ScreenPoint(x: 1024, y: 256)]

  let wrapped = try await map.latLngsForPixels(points: points)
  let unwrapped = try await map.latLngsForPixelsUnwrapped(points: points)
  #expect(wrapped.allSatisfy { (-180 ... 180).contains($0.longitude) })
  #expect(unwrapped[1].longitude - unwrapped[0].longitude > 360)
  let wrappedRight = try await map.latLngForPixel(point: points[1])
  #expect((-180 ... 180).contains(wrappedRight.longitude))
  let right = try await map.latLngForPixelUnwrapped(point: points[1])
  #expect(abs(right.longitude - unwrapped[1].longitude) < 0.0000000001)

  let projection = try await map.projectionCreate()
  defer { try? projection.close() }
  #expect(try (-180 ... 180)
    .contains(projection.latLngForPixel(point: points[1]).longitude))
  let projectedRight = try projection.latLngForPixelUnwrapped(point: points[1])
  #expect(abs(projectedRight.longitude - right.longitude) < 0.0000000001)
}

/// Projection calls are internally serialized, so a second thread converts
/// through the same live handle.
@Test func mapProjectionIsUsableFromASecondThread() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 256,
      height: 256,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  let projection = try await map.projectionCreate()
  defer { try? projection.close() }

  let expected = try projection.pixelForLatLng(coordinate: LatLng(
    latitude: 5,
    longitude: 6
  ))
  let result = try await Task.detached {
    try projection.pixelForLatLng(coordinate: LatLng(latitude: 5, longitude: 6))
  }.value
  #expect(abs(result.x - expected.x) < 0.000001)
  #expect(abs(result.y - expected.y) < 0.000001)
}

@Test func mapProjectionSetVisibleCoordinatesRejectsEmptyInputBeforeCallingC(
) async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 256,
      height: 256,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  let projection = try await map.projectionCreate()
  defer { try? projection.close() }

  do {
    try projection.setVisibleCoordinates(coordinates: [], padding: EdgeInsets())
    Issue.record("empty coordinates should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

extension OwnedTextureTests {
  @Test func sessionProjectionKeepsTheRenderedCameraAfterMapChanges(
  ) async throws {
    let fixture = try OwnedTextureFixture()
    defer { withExtendedLifetime(fixture) {} }
    let runtime =
      try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
    defer { try? runtime.closeBlockingForTests() }
    let map = try await runtime
      .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
        width: 32,
        height: 32,
        scaleFactor: 1
      )))
    defer { try? map.closeBlockingForTests() }
    let attachment = try fixture.attach(map: map)
    let session = attachment.session
    defer {
      if !session.isClosed { _ = try? session.abandon(); try? session.close() }
    }
    try await attachment.completion.value
    #expect(throws: MaplibreError.self) {
      try session.projectionCreate()
    }
    _ = try await map
      .setStyleJson(
        json: Data(##"{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ff0000"}}]}"##
          .utf8)
      )
    _ = try await map
      .updateCamera(update: CameraUpdate(camera: CameraOptions(zoom: 3)))
    try session.requestFrame(demand: FrameDemand(flags: [], token: 1))
    try await session.barrier()
    #expect(try session.drainFrameCopies().first?.disposition == .rendered)
    _ = try await map
      .updateCamera(update: CameraUpdate(camera: CameraOptions(zoom: 6)))
    let projection = try session.projectionCreate()
    defer { try? projection.close() }
    #expect(try projection.getCamera().zoom == 3)
    let image = try await session.textureReadPremultipliedRgba8()
    #expect(image.info.width == 32)
    #expect(image.info.height == 32)
    #expect(image.data.count == 32 * 32 * 4)
    #expect(stride(from: 0, to: image.data.count, by: 4).allSatisfy {
      Array(image.data[$0 ..< $0 + 4]) == [255, 0, 0, 255]
    })
    if try session.getCapabilities().flags.contains(.frameAcquisition) {
      let frame = try session.acquireFrame()
      let captured = try session.projectionCreate()
      #expect(try captured.getCamera().zoom == 3)
      try captured.close()
      try frame.release(consumerCompletion: .default)
    }
    try await session.resize(extent: RenderTargetExtent(
      width: 16,
      height: 16,
      scaleFactor: 1
    ))
    #expect(throws: MaplibreError.self) {
      try session.projectionCreate()
    }
    try await session.detach()
    try session.close()
    try await map.close()
    try await runtime.close()
    let retainedZoom = try await Task
      .detached { try projection.getCamera().zoom }
      .value
    #expect(retainedZoom == 3)
  }
}
