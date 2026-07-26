@testable import MaplibreNative
import Testing

@Test func mapCreateCameraStyleAndClose() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(
      width: 256,
      height: 256,
      scaleFactor: 1.0,
      mode: .continuous
    )
  )

  try map.setStyleURL("https://tiles.openfreemap.org/styles/bright")
  try map.jump(
    to: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 12,
      bearing: 5,
      pitch: 10
    )
  )
  let camera = try map.camera()
  #expect(abs((camera.center?.latitude ?? 0) - 37.7749) < 0.000001)
  #expect(abs((camera.center?.longitude ?? 0) - -122.4194) < 0.000001)
  #expect(abs((camera.zoom ?? 0) - 12) < 0.000001)

  try map.cancelTransitions()
  try map.moveBy(deltaX: 1, deltaY: 2)
  try map.moveBy(
    deltaX: -1,
    deltaY: -2,
    animation: AnimationOptions(durationMilliseconds: 1)
  )
  try map.scaleBy(1.01, anchor: ScreenPoint(x: 128, y: 128))
  try map.scaleBy(
    1.0 / 1.01,
    anchor: nil,
    animation: AnimationOptions(durationMilliseconds: 1)
  )
  try map.ease(
    to: CameraOptions(bearing: 0, pitch: 0),
    animation: AnimationOptions(durationMilliseconds: 1)
  )

  try map.close()
  #expect(map.isClosed)
}

@Test func mapSizeReportsCreationExtentAndPixelRatio() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 512, height: 256, scaleFactor: 2.0)
  )
  defer { try? map.close() }

  let size = try map.size()
  #expect(size.width == 512)
  #expect(size.height == 256)
  #expect(size.scaleFactor == 2.0)
}

@Test func styleURLRejectsEmbeddedNULAsPublicInvalidArgument() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  defer { try? map.close() }

  do {
    try map.setStyleURL("https://example.test/style\0oops")
    Issue.record("embedded NUL should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus == nil)
    #expect(error.diagnostic.contains("embedded NUL"))
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

/// This verifies that the geographic constraint reports and applies the
/// unbounded state distinctly from world bounds, which the southwest/northeast
/// pair alone cannot express.
@Test func cameraBoundsDistinguishUnboundedFromWorldBounds() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 256, height: 256)
  )
  defer { try? map.close() }

  func longitudeAfterJump(to longitude: Double) throws -> Double {
    try map.jump(to: CameraOptions(
      center: LatLng(latitude: 0, longitude: longitude),
      zoom: 2
    ))
    return try map.camera().center?.longitude ?? .nan
  }

  #expect(try map.bounds().bounds == .unbounded)
  // An unbounded map wraps across the antimeridian.
  #expect(try abs(longitudeAfterJump(to: 200) - -160) < 0.000001)

  try map.setBounds(BoundOptions(bounds: .bounded(LatLngBounds(
    southwest: LatLng(latitude: -90, longitude: -180),
    northeast: LatLng(latitude: 90, longitude: 180)
  ))))

  if case let .bounded(reported) = try map.bounds().bounds {
    #expect(abs(reported.northeast.longitude - 180) < 0.000001)
  } else {
    Issue.record("world bounds should report a bounded constraint")
  }
  // World bounds clamp at the antimeridian instead of wrapping.
  #expect(try abs(longitudeAfterJump(to: 200) - 180) < 0.000001)

  try map.setBounds(BoundOptions(bounds: .unbounded))
  #expect(try map.bounds().bounds == .unbounded)
  // Releasing the constraint restores antimeridian wrapping.
  #expect(try abs(longitudeAfterJump(to: 200) - -160) < 0.000001)
}

@Test func closedMapReportsSwiftOwnedStateError() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  try map.close()

  do {
    try map.requestRepaint()
    Issue.record("closed map should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}
