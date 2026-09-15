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
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 256, height: 256)
  )
  defer { try? map.close() }
  try map.jump(to: CameraOptions(
    center: LatLng(latitude: 0, longitude: 0),
    zoom: 1
  ))

  let projection = try MapProjectionHandle(map: map)
  try projection.setCamera(CameraOptions(
    center: LatLng(latitude: 1, longitude: 2),
    zoom: 2
  ))
  let camera = try projection.camera()
  #expect(abs((camera.center?.latitude ?? 0) - 1) < 0.000001)
  #expect(abs((camera.center?.longitude ?? 0) - 2) < 0.000001)

  let point = try projection.pixel(for: LatLng(latitude: 1, longitude: 2))
  let coordinate = try projection.latLng(for: point)
  #expect(abs(coordinate.latitude - 1) < 0.000001)
  #expect(abs(coordinate.longitude - 2) < 0.000001)

  try map.close()
  try runtime.close()
  try await Task.detached {
    _ = try projection.camera()
    try projection.close()
  }.value
  #expect(projection.isClosed)
}

@Test func unwrappedCoordinateConversionsPreserveVisibleWorldCopies() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1024, height: 512)
  )
  defer { try? map.close() }
  try map.jump(to: CameraOptions(
    center: LatLng(latitude: 0, longitude: 180),
    zoom: 0
  ))
  let points = [ScreenPoint(x: 0, y: 256), ScreenPoint(x: 1024, y: 256)]

  let wrapped = try map.latLngs(for: points)
  let unwrapped = try map.latLngsUnwrapped(for: points)
  #expect(wrapped.allSatisfy { (-180 ... 180).contains($0.longitude) })
  #expect(unwrapped[1].longitude - unwrapped[0].longitude > 360)
  #expect(try (-180 ... 180).contains(map.latLng(for: points[1]).longitude))
  let right = try map.latLngUnwrapped(for: points[1])
  #expect(abs(right.longitude - unwrapped[1].longitude) < 0.0000000001)

  let projection = try MapProjectionHandle(map: map)
  defer { try? projection.close() }
  #expect(try (-180 ... 180)
    .contains(projection.latLng(for: points[1]).longitude))
  let projectedRight = try projection.latLngUnwrapped(for: points[1])
  #expect(abs(projectedRight.longitude - right.longitude) < 0.0000000001)
}

@Test func mapProjectionSetVisibleCoordinatesRejectsEmptyInputBeforeCallingC(
) throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 256, height: 256)
  )
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

#if canImport(Metal)
  import Foundation
  import Metal

  @Test func sessionProjectionKeepsTheRenderedCameraAfterMapChanges() throws {
    guard Maplibre.supportedRenderBackends().contains(.metal) else { return }
    let device = try #require(MTLCreateSystemDefaultDevice())
    let runtime =
      try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
    defer { try? runtime.close() }
    let map = try MapHandle(
      runtime: runtime,
      options: MapOptions(width: 128, height: 64)
    )
    defer { try? map.close() }
    let session = try map.attachRef()
      .attachMetalOwnedTexture(MetalOwnedTextureDescriptor(
        extent: RenderTargetExtent(width: 128, height: 64, scaleFactor: 1),
        context: MetalContextDescriptor(device: NativePointer(
          bitPattern: UInt(bitPattern: Unmanaged
            .passUnretained(device as AnyObject).toOpaque())
        ))
      ))
    defer { try? session.close() }
    #expect(throws: MaplibreError.self) {
      try MapProjectionHandle(session: session)
    }
    try map
      .setStyleJSON(
        Data(#"{"version":8,"sources":{},"layers":[{"id":"bg","type":"background"}]}"#
          .utf8)
      )
    let coordinate = LatLng(latitude: 37.78, longitude: -122.41)
    try map.jump(to: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 12
    ))
    var rendered = false
    for _ in 0 ..< 500 {
      try runtime.pump()
      if try session.renderUpdate().result == .rendered {
        rendered = true
        break
      }
      Thread.sleep(forTimeInterval: 0.001)
    }
    #expect(rendered)
    let liveProjection = try MapProjectionHandle(map: map)
    defer { try? liveProjection.close() }
    let expected = try liveProjection.pixel(for: coordinate)
    try map.jump(to: CameraOptions(center: LatLng(
      latitude: 37.80,
      longitude: -122.45
    )))
    try runtime.pump()
    let projection = try MapProjectionHandle(session: session)
    defer { try? projection.close() }
    #expect(try projection.pixel(for: coordinate) == expected)
    let newer = try MapProjectionHandle(map: map)
    defer { try? newer.close() }
    #expect(try newer.pixel(for: coordinate) != expected)
    let frame = try session.acquireMetalOwnedTextureFrame()
    let acquiredProjection = try MapProjectionHandle(session: session)
    defer { try? acquiredProjection.close() }
    #expect(try acquiredProjection.pixel(for: coordinate) == expected)
    try frame.close()
    #expect(try session.renderUpdate().result == .rendered)
    let next = try MapProjectionHandle(session: session)
    defer { try? next.close() }
    #expect(try next.pixel(for: coordinate) == newer.pixel(for: coordinate))
    try session.resize(width: 96, height: 48, scaleFactor: 1)
    #expect(throws: MaplibreError.self) {
      try MapProjectionHandle(session: session)
    }
    try session.close()
    try map.close()
    try runtime.close()
    #expect(try projection.pixel(for: coordinate) == expected)
    withExtendedLifetime(device) {}
  }
#endif
