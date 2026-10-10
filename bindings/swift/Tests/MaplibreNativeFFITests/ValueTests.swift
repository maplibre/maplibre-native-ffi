import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

/// A string crosses as a counted view, which keeps an embedded NUL, or as a
/// C string, which cannot, so the binding rejects one before a native call
/// that needs termination sees it.
@Test func stringsCrossAsCountedViewsOrTerminatedCStrings() async throws {
  let arena = NativeInputArena()
  let view = arena.view("é\0")
  #expect(try NativeString.copyUTF8(data: view.data, size: view.size) == "é\0")
  #expect(throws: NativeStringError.self) { try arena.cString("a\0b") }

  try await withMapFixture { fixture in
    let error = await expectMaplibreError(.invalidArgument) {
      try await fixture.map.setStyleUrl(url: "custom://a\0b.json")
    }
    #expect(error?.rawStatus == nil)
  }

  var invalid: [UInt8] = [0xFF]
  #expect(throws: NativeStringError.self) {
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
  #expect(AnimationOptions(transitionId: 0).nativeValue().fields ==
    MLN_ANIMATION_OPTION_TRANSITION_ID.rawValue)
  #expect(AnimationOptions().nativeValue().fields == 0)

  var raw = mln_camera_options_default()
  raw.fields = MLN_CAMERA_OPTION_ZOOM.rawValue
  raw.zoom = 5
  raw.bearing = 90
  let camera = CameraOptions(raw: raw)
  #expect(camera.zoom == 5)
  #expect(camera.bearing == nil)
  #expect(camera.center == nil)
  #expect(camera.nativeValue().fields == MLN_CAMERA_OPTION_ZOOM.rawValue)

  var source = mln_style_source_result()
  source.info.fields = MLN_STYLE_SOURCE_INFO_TILE_SIZE.rawValue
  let copied = try StyleSourceResult(raw: source)
  #expect(copied.info.tileSize == 0)
  #expect(copied.info.bounds == nil)
  #expect(copied.info.tilejson == nil)
  #expect(copied.url == nil)
}

/// A count that does not fit the native type is rejected rather than
/// truncated, an enum value this version does not name keeps its raw value,
/// and a 64-bit Vulkan handle keeps its high bits on the way into a
/// descriptor.
@Test func valuesCheckNarrowingKeepUnknownEnumsAndCarryFullWidthHandles(
) throws {
  #expect(throws: NativeStringError.self) {
    let _: UInt32 = try NativeInputArena.count(Int(UInt32.max) + 1)
  }

  var source = mln_style_source_result()
  source.info.type = 700
  source.info.fields = MLN_STYLE_SOURCE_INFO_TILEJSON.rawValue
  source.info.scheme = 701
  let copied = try StyleSourceResult(raw: source)
  #expect(copied.info.type.rawValue == 700)
  #expect(copied.info.tilejson?.scheme.rawValue == 701)
  #expect(RenderResult(rawValue: 99).rawValue == 99)

  let context = VulkanContextDescriptor(
    instance: NativePointer(bitPattern: 0x30),
    physicalDevice: NativePointer(bitPattern: 0x40),
    device: NativePointer(bitPattern: 0x50),
    graphicsQueue: NativePointer(bitPattern: 0x60),
    graphicsQueueFamilyIndex: 7,
    getInstanceProcAddr: NativePointer(bitPattern: 0x90),
    getDeviceProcAddr: NativePointer(bitPattern: 0xA0)
  )
  let extent = RenderTargetExtent(width: 64, height: 32, scaleFactor: 2)
  let texture = VulkanBorrowedTextureDescriptor(
    extent: extent,
    physicalWidth: 128,
    physicalHeight: 64,
    context: context,
    image: 0x8000_0000_0000_0001,
    imageView: 0x0000_0001_0000_0000,
    format: 44,
    initialLayout: 1,
    finalLayout: 2
  ).nativeValue()
  #expect(texture.image == 0x8000_0000_0000_0001)
  #expect(texture.image_view == 0x0000_0001_0000_0000)
  let surface = VulkanSurfaceDescriptor(
    extent: extent,
    context: context,
    surface: 0xFEDC_BA98_7654_3210
  ).nativeValue()
  #expect(surface.surface == 0xFEDC_BA98_7654_3210)
  #expect(UInt(bitPattern: surface.context.device) == 0x50)
}

/// A copy out of a native record rejects a null pointer with a nonzero size
/// instead of reading through it.
@Test func aNativeCopyRejectsANullPointerWithANonzeroSize() {
  var definition = mln_offline_region_definition()
  definition.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY.rawValue
  definition.data.geometry = mln_offline_geometry_region_definition()
  definition.data.geometry.geometry = mln_buffer_view(data: nil, size: 1)
  #expect(throws: NativeStringError.self) {
    try OfflineRegionDefinition(raw: definition)
  }

  var info = mln_offline_region_info()
  info.definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID.rawValue
  info.metadata = nil
  info.metadata_size = 1
  #expect(throws: NativeStringError.self) { try OfflineRegionInfo(raw: info) }
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
