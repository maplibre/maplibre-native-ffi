import Foundation
import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// Host strings cross as counted views, and native validates URLs before a
/// C-string consumer reads them. Native-lent strings decode as C strings.
@Test func hostStringsUseViewsAndNativeLentStringsDecode() async throws {
  let arena = NativeInputArena()
  let view = arena.view("é\0")
  #expect(try NativeString.copyUTF8(data: view.data, size: view.size) == "é\0")
  #expect(try NativeString
    .copyCString(arena.cString("native-lent")) == "native-lent")

  try await withMapFixture { fixture in
    let error = await expectMaplibreError(.invalidArgument) {
      try await fixture.map.setStyleUrl("custom://a\0b.json")
    }
    #expect(error?.status == .invalidArgument)
  }

  var invalid: [UInt8] = [0xFF]
  #expect(throws: MaplibreError.self) {
    try invalid.withUnsafeMutableBufferPointer { buffer in
      try NativeString.copyUTF8(
        data: UnsafeRawPointer(buffer.baseAddress!)
          .assumingMemoryBound(to: CChar.self),
        size: buffer.count
      )
    }
  }
}

/// A presence field tells an absent value from a zero one in both directions,
/// and a record read from native keeps the fields native left absent.
@Test func presenceFieldsTellAbsentFromZero() throws {
  #expect(CameraFitOptions(bearing: 0).nativeValue().fields ==
    MLN_CAMERA_FIT_OPTION_BEARING.rawValue)
  #expect(CameraFitOptions().nativeValue().fields == 0)

  var raw = mln_camera_options_default()
  raw.fields = MLN_CAMERA_OPTION_ZOOM.rawValue
  raw.zoom = 5
  raw.bearing = 90
  let camera = CameraOptions(raw: raw)
  #expect(camera.zoom == 5)
  #expect(camera.bearing == nil)
  #expect(camera.center == nil)
  #expect(camera.nativeValue().fields == MLN_CAMERA_OPTION_ZOOM.rawValue)

  var source = mln_style_source_info()
  source.fields = MLN_STYLE_SOURCE_INFO_TILE_SIZE.rawValue
  let copied = try StyleSourceInfo(raw: source)
  #expect(copied.tileSize == 0)
  #expect(copied.bounds == nil)
  #expect(copied.tilejson == nil)
  #expect(copied.url == nil)
}

/// A count that does not fit the native type is rejected rather than
/// truncated, an enum value this version does not name keeps its raw value,
/// and a 64-bit Vulkan handle keeps its high bits on the way into a record.
@Test func valuesCheckNarrowingKeepUnknownEnumsAndCarryFullWidthHandles(
) throws {
  #expect(throws: MaplibreError.self) {
    let _: UInt32 = try NativeInputArena.count(Int(UInt32.max) + 1)
  }

  var source = mln_style_source_info()
  source.type = 700
  source.fields = MLN_STYLE_SOURCE_INFO_TILEJSON.rawValue
  source.tilejson.scheme = 701
  let copied = try StyleSourceInfo(raw: source)
  #expect(copied.type.rawValue == 700)
  #expect(copied.tilejson?.scheme.rawValue == 701)

  let texture = VulkanBorrowedTexture(
    image: 0x8000_0000_0000_0001,
    imageView: 0x0000_0001_0000_0000
  ).nativeValue()
  #expect(texture.image == 0x8000_0000_0000_0001)
  #expect(texture.image_view == 0x0000_0001_0000_0000)
}

/// A copy out of a native record rejects a null pointer with a nonzero size
/// instead of reading through it.
@Test func aNativeCopyRejectsANullPointerWithANonzeroSize() {
  var definition = mln_offline_region_definition()
  definition.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY.rawValue
  definition.data.geometry = mln_offline_geometry_region_definition()
  definition.data.geometry.geometry = mln_buffer_view(data: nil, size: 1)
  #expect(throws: MaplibreError.self) {
    try OfflineRegionDefinition(raw: definition)
  }
}

/// The input arena copies what a call submits, so the caller's later changes
/// to its own storage do not reach native, and an empty input still has
/// storage behind it.
@Test func inputsAreCopiedAtSubmission() throws {
  let arena = NativeInputArena()
  var data = Data(#"{"type":"Point","coordinates":[2,1]}"#.utf8)
  let view = arena.view(data)
  data[0] = 0
  var values: [UInt32] = [1, 2, 3]
  let array = try #require(arena.array(values))
  values[0] = 9
  for byte in UInt8.min ... 64 {
    _ = arena.view(Data(repeating: byte, count: 32))
  }

  let copied = try Data(bytes: #require(view.data), count: view.size)
  #expect(copied == Data(#"{"type":"Point","coordinates":[2,1]}"#.utf8))
  #expect(Array(UnsafeBufferPointer(start: array, count: 3)) == [1, 2, 3])
  let empty = arena.view(Data())
  #expect(empty.data != nil)
  #expect(empty.size == 0)
  withExtendedLifetime(arena) {}
}

/// Each initializer parameter defaults to the native default's field, so the
/// record it builds matches what the native default function returns.
@Test func aRecordBuiltFromItsInitializerDefaultsEqualsTheNativeDefault() {
  #expect(MapOptions() == MapOptions.default)
}

@Test func synchronousCallsFillCallerSizedOutputsInInputOrder() async throws {
  try await withMapFixture { fixture in
    let projection = try Maplibre.mapProjectionCreate(
      view: fixture.map.getSnapshot().view
    )
    defer { projection.close() }
    let coordinates = [
      LatLng(latitude: 0, longitude: 0),
      LatLng(latitude: 10, longitude: 20),
      LatLng(latitude: -12, longitude: -30),
    ]
    let points = try projection.pixelsForLatLngs(coordinates: coordinates)
    #expect(try points ==
      (coordinates.map { try projection.pixelForLatLng(coordinate: $0) }))
    #expect(try projection.latLngsForPixels(points: points) == points
      .map { try projection.latLngForPixel(point: $0) })
    #expect(try projection.latLngsForPixelsUnwrapped(points: points) == points
      .map { try projection.latLngForPixelUnwrapped(point: $0) })
    #expect(try projection.pixelsForLatLngs(coordinates: []).isEmpty)
    #expect(try projection.latLngsForPixels(points: []).isEmpty)
    #expect(try projection.latLngsForPixelsUnwrapped(points: []).isEmpty)
  }
}

@Test func callerSizedTextReturnsResultsLongerThanItsFirstStorage() throws {
  let zoom = try Maplibre.styleExpressionCreate(
    Data(#"["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]"#
      .utf8)
  )
  #expect(try zoom.evaluate(inputs: StyleExpressionInputs(zoom: 5)) ==
    Data("50.0".utf8))
  zoom.close()

  // The result outgrows the runtime's first storage, so the call runs again.
  let text = Data(("\"" + String(repeating: "a", count: 1000) + "\"").utf8)
  let long = try Maplibre.styleExpressionCreate(text)
  #expect(try long.evaluate() == text)
  long.close()
}
