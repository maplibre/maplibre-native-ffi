import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func tileSourceOptionsMaterializeFieldMaskAndStringViews() throws {
  let options = StyleTileSourceOptions(
    minZoom: 1,
    maxZoom: 12,
    attribution: "© data",
    scheme: .tms,
    bounds: LatLngBounds(
      southwest: LatLng(latitude: -1, longitude: -2),
      northeast: LatLng(latitude: 3, longitude: 4)
    ),
    tileSize: 256,
    vectorEncoding: .mlt,
    rasterEncoding: .terrarium
  )

  let arena = NativeInputArena()
  let converted = try options.nativeValue(arena: arena)
  try withUnsafePointer(to: converted) { pointer in
    let native: UnsafePointer<mln_style_tile_source_options>? = pointer
    #expect(native != nil)
    #expect((native!.pointee.fields & MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
        .rawValue) != 0)
    #expect((native!.pointee.fields & MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
        .rawValue) != 0)
    #expect(native!.pointee.min_zoom == 1)
    #expect(native!.pointee.max_zoom == 12)
    #expect(native!.pointee.scheme == MLN_STYLE_TILE_SCHEME_TMS.rawValue)
    #expect(native!.pointee.bounds.southwest.latitude == -1)
    #expect(native!.pointee.tile_size == 256)
    #expect(native!.pointee
      .vector_encoding == MLN_STYLE_VECTOR_TILE_ENCODING_MLT.rawValue)
    #expect(native!.pointee
      .raster_encoding == MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM.rawValue)
    let attribution = try NativeString.copyUTF8(
      data: native!.pointee.attribution.data,
      size: native!.pointee.attribution.size
    )
    #expect(attribution == "© data")
  }
}

@Test func styleSourceInfoPreservesAbsentFieldsAndUnknownEnums() throws {
  var raw = mln_style_source_result()
  raw.info.type = 700
  raw.info.is_volatile = true
  raw.info.fields = MLN_STYLE_SOURCE_INFO_TILEJSON
    .rawValue | MLN_STYLE_SOURCE_INFO_TILE_SIZE
    .rawValue | MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING
    .rawValue | MLN_STYLE_SOURCE_INFO_RASTER_ENCODING.rawValue
  raw.info.scheme = 701
  raw.info.vector_encoding = 702
  raw.info.raster_encoding = 703
  let copied = try StyleSourceResult(raw: raw)
  #expect(copied.info.type.rawValue == 700)
  #expect(copied.info.isVolatile)
  #expect(copied.attribution == nil)
  #expect(copied.url == nil)
  #expect(copied.tileUrls == [])
  #expect(copied.info.tilejson?.minZoom == 0)
  #expect(copied.info.tilejson?.scheme.rawValue == 701)
  #expect(copied.info.bounds == nil)
  #expect(copied.info.tileSize == 0)
  #expect(copied.info.vectorEncoding?.rawValue == 702)
  #expect(copied.info.rasterEncoding?.rawValue == 703)
}

@Test func sourceInspectionCopiesReconstructibleMetadata() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map
    .setStyleJson(json: jsonData(#"{"version":8,"sources":{},"layers":[]}"#))
  let bounds = LatLngBounds(
    southwest: LatLng(latitude: -1, longitude: -2),
    northeast: LatLng(latitude: 3, longitude: 4)
  )
  try await map.addVectorSourceTiles(
    sourceId: "inline",
    tiles: [
      "https://a.example/{z}/{x}/{y}.mvt",
      "https://b.example/{z}/{x}/{y}.mvt",
    ],
    options: StyleTileSourceOptions(
      minZoom: 0,
      maxZoom: 9,
      attribution: "© inline",
      scheme: .tms,
      bounds: bounds,
      tileSize: 256,
      vectorEncoding: .mlt
    )
  )
  try await map.addVectorSourceUrl(
    sourceId: "remote",
    url: "https://example.com/source.json"
  )
  let emptyCollection = try Maplibre.geojsonSourceDataCreate(
    data: jsonData(#"{"type":"FeatureCollection","features":[]}"#)
  )
  defer { try? emptyCollection.close() }
  try await map.addGeojsonSourceData(sourceId: "data", data: emptyCollection)

  let inline = try #require(try await map
    .getStyleSourceInfo(sourceId: "inline"))
  #expect(inline.info.type == .vector)
  #expect(inline.url == nil)
  #expect(inline.attribution == "© inline")
  #expect(
    inline.tileUrls == [
      "https://a.example/{z}/{x}/{y}.mvt",
      "https://b.example/{z}/{x}/{y}.mvt",
    ]
  )
  #expect(inline.info.tilejson?.minZoom == 0)
  #expect(inline.info.tilejson?.maxZoom == 9)
  #expect(inline.info.tilejson?.scheme == .tms)
  #expect(inline.info.bounds == bounds)
  #expect(inline.info.tileSize == 512)
  #expect(inline.info.vectorEncoding == .mlt)
  #expect(inline.info.rasterEncoding == nil)

  let remote = try #require(try await map
    .getStyleSourceInfo(sourceId: "remote"))
  #expect(remote.url == "https://example.com/source.json")
  #expect(remote.info.tilejson == nil)
  #expect(remote.attribution == nil)

  let data = try #require(try await map.getStyleSourceInfo(sourceId: "data"))
  #expect(data.url == nil)
  #expect(data.info.tilejson == nil)
  #expect(data.info.tileSize == nil)
  #expect(data.info.vectorEncoding == nil)
  #expect(data.info.rasterEncoding == nil)
  #expect(try await map.getStyleSourceInfo(sourceId: "missing") == nil)

  let removeCommand = try await map.removeStyleSource(sourceId: "inline")
  #expect(removeCommand.disposition == .committed)
  #expect(try await map.getStyleSourceInfo(sourceId: "inline") == nil)
  try await map.close()

  // Every nested string and value remains valid after its native source and
  // owning map are gone.
  #expect(inline.attribution == "© inline")
  #expect(inline.tileUrls?.count == 2)
  #expect(inline.info.bounds == bounds)
  #expect(remote.url == "https://example.com/source.json")
}

/// An inline tile source reads back the tile URLs it was added with, a
/// URL-backed source reads as an empty list, and a missing source reads as nil.
@Test func styleSourceTileURLsReadBackInlineTilesAndMissingSources()
  async throws
{
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map
    .setStyleJson(json: jsonData(#"{"version":8,"sources":{},"layers":[]}"#))
  let tiles = [
    "https://a.example/{z}/{x}/{y}.mvt",
    "https://b.example/{z}/{x}/{y}.mvt",
  ]
  try await map.addVectorSourceTiles(sourceId: "inline", tiles: tiles)

  try await map.addVectorSourceUrl(
    sourceId: "remote", url: "https://example.com/source.json"
  )

  #expect(try await map.getStyleSourceTileUrls(sourceId: "inline")?
    .tileUrls == tiles)
  #expect(try await map.getStyleSourceTileUrls(sourceId: "remote")?
    .tileUrls == [])
  #expect(try await map.getStyleSourceTileUrls(sourceId: "missing") == nil)
}

/// Volatility toggles commit and are visible through source info, and a
/// missing source fails the command with `MLN_STATUS_NOT_FOUND`.
@Test func styleSourceVolatilityTogglesAndMissingSourceFails() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map
    .setStyleJson(json: jsonData(#"{"version":8,"sources":{},"layers":[]}"#))
  try await map.addVectorSourceTiles(
    sourceId: "tiles",
    tiles: ["https://example.com/{z}/{x}/{y}.mvt"]
  )

  #expect(try await map.getStyleSourceInfo(sourceId: "tiles")?.info
    .isVolatile == false)
  let enabled = try await map.setStyleSourceVolatile(
    sourceId: "tiles",
    isVolatile: true
  )
  #expect(enabled.disposition == .committed)
  #expect(try await map.getStyleSourceInfo(sourceId: "tiles")?.info
    .isVolatile == true)
  let disabled = try await map.setStyleSourceVolatile(
    sourceId: "tiles",
    isVolatile: false
  )
  #expect(disabled.disposition == .committed)
  #expect(try await map.getStyleSourceInfo(sourceId: "tiles")?.info
    .isVolatile == false)

  try expectCommandFailure(
    await map.setStyleSourceVolatile(sourceId: "missing", isVolatile: true),
    status: MLN_STATUS_NOT_FOUND
  )
}

@Test func styleImageDescriptorsMaterializeScopedPixelsAndOptions() throws {
  let image = PremultipliedRgba8Image(
    width: 1,
    height: 1,
    stride: 4,
    pixels: Data([1, 2, 3, 4])
  )

  let imageArena = NativeInputArena()
  defer { withExtendedLifetime(imageArena) {} }
  try withUnsafePointer(to: image.nativeValue(arena: imageArena)) { native in
    #expect(native.pointee.width == 1)
    #expect(native.pointee.height == 1)
    #expect(native.pointee.stride == 4)
    #expect(native.pointee.byte_length == 4)
    #expect(native.pointee.pixels![2] == 3)
  }

  try withUnsafePointer(to: StyleImageOptions(pixelRatio: 2, sdf: true)
    .nativeValue(arena: imageArena))
  { options in
    #expect((options.pointee.fields & MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
        .rawValue) != 0)
    #expect((options.pointee.fields & MLN_STYLE_IMAGE_OPTION_SDF.rawValue) !=
      0)
    #expect(options.pointee.pixel_ratio == 2)
    #expect(options.pointee.sdf)
  }
}

@Test func customGeometryOptionsRetainAndInvokeTileCallbacks() throws {
  final class TileBox: @unchecked Sendable {
    var fetched: [CanonicalTileId] = []
    var cancelled: [CanonicalTileId] = []
  }

  let box = TileBox()
  let options = CustomGeometrySourceOptions(
    fetchTile: { box.fetched.append($0) },
    cancelTile: { box.cancelled.append($0) },
    minZoom: 1,
    maxZoom: 10,
    tolerance: 0.5,
    tileSize: 256,
    buffer: 8,
    clip: true,
    wrap: false
  )

  let callbackArena = NativeInputArena()
  try withUnsafePointer(to: options
    .nativeValue(arena: callbackArena))
  { native in
    #expect((native.pointee
        .fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
        .rawValue) != 0)
    #expect((native.pointee.fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
        .rawValue) != 0)
    #expect(native.pointee.min_zoom == 1)
    #expect(native.pointee.max_zoom == 10)
    #expect(native.pointee.tolerance == 0.5)
    #expect(native.pointee.tile_size == 256)
    #expect(native.pointee.buffer == 8)
    #expect(native.pointee.clip)
    #expect(!native.pointee.wrap)
    native.pointee.fetch_tile!(
      native.pointee.user_data,
      mln_canonical_tile_id(z: 1, x: 2, y: 3)
    )
    native.pointee.cancel_tile!(
      native.pointee.user_data,
      mln_canonical_tile_id(z: 4, x: 5, y: 6)
    )
  }

  #expect(box.fetched == [CanonicalTileId(z: 1, x: 2, y: 3)])
  #expect(box.cancelled == [CanonicalTileId(z: 4, x: 5, y: 6)])
}

// The C API can call the release callback while a tile callback is still
// running on another thread, so the release waits for that call rather than
// freeing the state under it.

/// Captured by a custom source's tile closure, so its deallocation reports
/// that the C API released that source's callback state.
private final class ReleaseSentinel: @unchecked Sendable {
  private let counter: LockedBox<Int>

  init(_ counter: LockedBox<Int>) {
    self.counter = counter
  }

  deinit {
    counter.update { $0 += 1 }
  }
}

/// Adds a custom geometry source whose callback state reports its own release
/// through `counter`.
private func addSourceReportingItsRelease(
  to map: MapHandle,
  sourceId: String = "custom",
  counter: LockedBox<Int>
) async throws -> CommandCompletion {
  let sentinel = ReleaseSentinel(counter)
  return try await map.addCustomGeometrySource(sourceId: sourceId,
                                               options: CustomGeometrySourceOptions(
                                                 fetchTile: { _ in
                                                   withExtendedLifetime(
                                                     sentinel
                                                   ) {}
                                                 }
                                               ))
}

/// A style load drops the sources the previous style held, and the C API
/// releases their callback state without a map-style-loaded event, so a host
/// that never selected that event type still gets its state freed.
@Test func aStyleLoadReleasesADroppedCustomGeometrySource() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  try await runtime
    .setResourceProvider(
      provider: ResourceProvider(callback: { request, handle in
        guard request.requestedUrl == "maplibre://maps/replacement" else {
          return .passThrough
        }
        try? handle.complete(response: ResourceResponse(
          status: .ok,
          bytes: emptyStyleJSON
        ))
        return .handle
      })
    )
  let narrowed = RuntimeEventMask.all.subtracting(.mapStyleLoaded)
  let map = try await runtime.mapCreate(options: MapOptions(
    initialExtent: LogicalExtent(width: 64, height: 64, scaleFactor: 1),
    eventMask: narrowed
  ))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleJson(json: emptyStyleJSON)
  let counter = LockedBox(0)
  _ = try await addSourceReportingItsRelease(to: map, counter: counter)
  #expect(counter.value == 0)

  try await map.setStyleUrl(url: "maplibre://maps/replacement")
  var styleLoadedReported = false
  let deadline = Date().addingTimeInterval(10)
  while Date() < deadline, counter.value == 0 {
    try await runtime.barrier()
    styleLoadedReported = try styleLoadedReported || runtime.drainEventCopies()
      .contains { $0.type == .mapStyleLoaded }
    try await Task<Never, Never>.sleep(nanoseconds: 1_000_000)
  }

  #expect(counter.value == 1)
  #expect(!styleLoadedReported)
  #expect(try map.snapshotGet().eventMask == narrowed)
  #expect(try await map.getStyleSourceInfo(sourceId: "custom") == nil)
}

/// Removing a custom geometry source releases its callback state, and does so
/// once.
@Test func removingACustomGeometrySourceReleasesItsCallbacks() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleJson(json: emptyStyleJSON)
  let counter = LockedBox(0)
  _ = try await addSourceReportingItsRelease(to: map, counter: counter)

  let removeCommand = try await map.removeStyleSource(sourceId: "custom")
  #expect(removeCommand.disposition == .committed)
  #expect(counter.value == 1)
  try await map.close()
  #expect(counter.value == 1)
}

/// Destroying a map releases the callback state of the sources it still holds.
@Test func closingAMapReleasesItsCustomGeometrySources() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleJson(json: emptyStyleJSON)
  let counter = LockedBox(0)
  for sourceId in ["first", "second"] {
    _ = try await addSourceReportingItsRelease(
      to: map,
      sourceId: sourceId,
      counter: counter
    )
  }
  #expect(counter.value == 0)

  try await map.close()

  #expect(counter.value == 2)
}

/// Accepted commands own callback state. A command rejected by the map worker
/// releases that state before a subsequent runtime barrier completes.
@Test func aRejectedCustomGeometrySourceAddReleasesItsCallbacks() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try await map.setStyleJson(json: emptyStyleJSON)
  #expect(styleCommand.disposition == .committed)
  let accepted = LockedBox(0)
  let acceptedCommand = try await addSourceReportingItsRelease(
    to: map, counter: accepted
  )
  #expect(acceptedCommand.disposition == .committed)

  // A second source with the ID the accepted one already took is rejected.
  let rejected = LockedBox(0)
  try expectCommandFailure(
    await addSourceReportingItsRelease(
      to: map, sourceId: "custom", counter: rejected
    ),
    status: MLN_STATUS_INVALID_ARGUMENT
  )
  try await runtime.barrier()

  #expect(rejected.value == 1)
  #expect(accepted.value == 0)
}

/// Adds a custom MVT vector source whose callback state reports its own
/// release through `counter`.
private func addMvtSourceReportingItsRelease(
  to map: MapHandle,
  sourceId: String = "custom-mvt",
  counter: LockedBox<Int>
) async throws -> CommandCompletion {
  let sentinel = ReleaseSentinel(counter)
  return try await map.addCustomMvtVectorSource(sourceId: sourceId,
                                                options: CustomMvtVectorSourceOptions(
                                                  fetchTile: { _ in
                                                    withExtendedLifetime(
                                                      sentinel
                                                    ) {}
                                                  }
                                                ))
}

@Test func customMvtVectorOptionsRetainAndInvokeTileCallbacks() throws {
  final class TileBox: @unchecked Sendable {
    var fetched: [CanonicalTileId] = []
    var cancelled: [CanonicalTileId] = []
  }

  let box = TileBox()
  let options = CustomMvtVectorSourceOptions(
    fetchTile: { box.fetched.append($0) },
    cancelTile: { box.cancelled.append($0) },
    minZoom: 1,
    maxZoom: 10
  )

  let callbackArena = NativeInputArena()
  try withUnsafePointer(to: options
    .nativeValue(arena: callbackArena))
  { native in
    #expect((native.pointee
        .fields & MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
        .rawValue) != 0)
    #expect(native.pointee.min_zoom == 1)
    #expect(native.pointee.max_zoom == 10)
    native.pointee.fetch_tile!(
      native.pointee.user_data,
      mln_canonical_tile_id(z: 1, x: 2, y: 3)
    )
    native.pointee.cancel_tile!(
      native.pointee.user_data,
      mln_canonical_tile_id(z: 4, x: 5, y: 6)
    )
  }

  #expect(box.fetched == [CanonicalTileId(z: 1, x: 2, y: 3)])
  #expect(box.cancelled == [CanonicalTileId(z: 4, x: 5, y: 6)])
}

@Test func customMvtVectorSourcesCanBeAddedInspectedAndReleased(
) async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleJson(json: emptyStyleJSON)
  let counter = LockedBox(0)
  let addCommand = try await addMvtSourceReportingItsRelease(
    to: map, counter: counter
  )
  #expect(addCommand.disposition == .committed)
  #expect(try await map.getStyleSourceInfo(sourceId: "custom-mvt")?
    .info.type == .customMvtVector)

  let tileId = CanonicalTileId(z: 0, x: 0, y: 0)
  try await map.setCustomMvtVectorSourceTileData(
    sourceId: "custom-mvt",
    tileId: tileId,
    data: Data()
  )
  try await map.setCustomMvtVectorSourceTileError(
    sourceId: "custom-mvt",
    tileId: tileId,
    message: "tile missing"
  )
  try await map.invalidateCustomMvtVectorSourceTile(
    sourceId: "custom-mvt",
    tileId: tileId
  )

  let removeCommand = try await map.removeStyleSource(sourceId: "custom-mvt")
  #expect(removeCommand.disposition == .committed)
  #expect(counter.value == 1)
}

@Test func aRejectedCustomMvtVectorSourceAddReleasesItsCallbacks(
) async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleJson(json: emptyStyleJSON)
  let accepted = LockedBox(0)
  let acceptedCommand = try await addMvtSourceReportingItsRelease(
    to: map, counter: accepted
  )
  #expect(acceptedCommand.disposition == .committed)

  // A second source with the ID the accepted one already took is rejected.
  let rejected = LockedBox(0)
  try expectCommandFailure(
    await addMvtSourceReportingItsRelease(
      to: map, sourceId: "custom-mvt", counter: rejected
    ),
    status: MLN_STATUS_INVALID_ARGUMENT
  )
  try await runtime.barrier()

  #expect(rejected.value == 1)
  #expect(accepted.value == 0)
}

@Test func loadedStyleDocumentAndURLReadBackWhatWasLoaded() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  // Nothing parsed and nothing requested yet.
  #expect(try await map.loadedStyleJson() == Data())
  #expect(try await map.styleUrl() == "")

  // The document reads back byte-for-byte, so it can be reloaded unchanged.
  let styleJSON = jsonData(#"{"version":8,"sources":{},"layers":[]}"#)
  try await map.setStyleJson(json: styleJSON)
  #expect(try await map.loadedStyleJson() == styleJSON)
  // Inline JSON clears the URL.
  #expect(try await map.styleUrl() == "")

  // The URL is request state, recorded before the load can succeed, while the
  // document still reports the style that last parsed.
  try await map.setStyleUrl(url: "https://example.com/style.json")
  #expect(try await map.styleUrl() == "https://example.com/style.json")
  #expect(try await map.loadedStyleJson() == styleJSON)
}

@Test func closedMapRejectsStyleCallsThroughSwiftHandleState() async throws {
  let runtime = try Maplibre.runtimeCreate(options: .default)
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  try await map.close()

  do {
    _ = try await map.listStyleLayerIds()
    Issue.record("closed map should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func ninePatchStyleImageRoundTripsStretchContentAndTextFit() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let image = PremultipliedRgba8Image(
    width: 2,
    height: 2,
    stride: 8,
    pixels: Data(repeating: 0, count: 16)
  )
  let options = StyleImageOptions(
    stretchX: [ImageStretch(from: 0, to: 1)],
    stretchY: [ImageStretch(from: 0, to: 1), ImageStretch(from: 1, to: 2)],
    content: ImageContent(left: 0.5, top: 0.5, right: 1.5, bottom: 1.5),
    textFitHeight: .proportional
  )
  try await map.setStyleImage(imageId: "patch", image: image, options: options)

  let info = try #require(try await map.getStyleImageInfo(imageId: "patch"))
    .info
  #expect(info.stretchXCount == 1)
  #expect(info.stretchYCount == 2)
  #expect(info.content?.right == 1.5)
  // An absent text fit stays distinguishable from a present default.
  #expect(info.textFitWidth == nil)
  #expect(info.textFitHeight == .proportional)

  let stretches = try #require(try await map
    .copyStyleImageStretches(imageId: "patch"))
  #expect(stretches.stretchX == [ImageStretch(from: 0, to: 1)])
  #expect(
    stretches.stretchY == [
      ImageStretch(from: 0, to: 1), ImageStretch(from: 1, to: 2),
    ]
  )
  #expect(try await map.copyStyleImageStretches(imageId: "missing") == nil)

  // A backwards interval is rejected by C.
  do {
    _ = try await map.setStyleImage(
      imageId: "bad",
      image: image,
      options: StyleImageOptions(stretchX: [ImageStretch(from: 2, to: 1)])
    )
    Issue.record("a backwards stretch interval should fail")
  } catch is MaplibreError {}
}

@Test func layerBaseAccessorsRoundTripThroughNativeMap() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"bg","type":"background"},{"id":"fill","type":"fill","source":"geo"}]}
  """))
  #expect(styleCommand.disposition == .committed)

  #expect(try await map.copyLayerSourceLayer(layerId: "fill") == nil)
  let sourceLayerCommand = try await map.setLayerSourceLayer(
    layerId: "fill",
    sourceLayer: "roads"
  )
  #expect(sourceLayerCommand.disposition == .committed)
  #expect(try await map.copyLayerSourceLayer(layerId: "fill") == "roads")
  #expect(try await map.copyLayerSourceId(layerId: "fill") == "geo")

  try expectCommandFailure(
    await map.setLayerSourceLayer(layerId: "bg", sourceLayer: "roads"),
    status: MLN_STATUS_INVALID_ARGUMENT
  )
  #expect(try await map.copyLayerSourceLayer(layerId: "bg") == nil)
  #expect(try await map.copyLayerSourceId(layerId: "bg") == nil)

  // The layer-info aggregate reports the unbounded zoom range, the layer
  // type, the visibility, and the source strings its sizes describe.
  let unbounded = try #require(try await map.getStyleLayerInfo(layerId: "fill"))
  #expect(unbounded.info.type == "fill")
  #expect(unbounded.info.minZoom == -Double.infinity)
  #expect(unbounded.info.maxZoom == Double.infinity)
  #expect(unbounded.info.visibility == .visible)
  #expect(unbounded.sourceId == "geo")
  #expect(unbounded.sourceLayer == "roads")

  let background = try #require(try await map.getStyleLayerInfo(layerId: "bg"))
  #expect(background.info.type == "background")
  #expect(background.sourceId == nil)
  #expect(background.sourceLayer == nil)

  let minZoomCommand = try await map.setLayerMinZoom(
    layerId: "fill",
    minZoom: 4
  )
  #expect(minZoomCommand.disposition == .committed)
  let maxZoomCommand = try await map.setLayerMaxZoom(
    layerId: "fill",
    maxZoom: 12.5
  )
  #expect(maxZoomCommand.disposition == .committed)
  let visibilityCommand = try await map.setLayerVisibility(
    layerId: "fill", visibility: .none
  )
  #expect(visibilityCommand.disposition == .committed)
  let bounded = try #require(try await map.getStyleLayerInfo(layerId: "fill"))
  #expect(bounded.info.minZoom == 4)
  #expect(bounded.info.maxZoom == 12.5)
  #expect(bounded.info.visibility == .none)

  try expectCommandFailure(
    await map.setLayerVisibility(
      layerId: "fill",
      visibility: StyleLayerVisibility(rawValue: 900)
    ),
    status: MLN_STATUS_INVALID_ARGUMENT
  )
  #expect(try await map.getStyleLayerInfo(layerId: "fill")?
    .info.visibility == StyleLayerVisibility.none)

  #expect(try await map.getStyleLayerInfo(layerId: "missing") == nil)
}

/// A removal command commits when the object existed, and fails with
/// `MLN_STATUS_NOT_FOUND` when nothing has the ID. The info getters' found
/// flag re-checks existence.
@Test func styleRemovalsCommitOrFailWithNotFound() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"fill","type":"fill","source":"geo"}]}
  """))
  #expect(styleCommand.disposition == .committed)
  try await map.setStyleImage(
    imageId: "marker",
    image: PremultipliedRgba8Image(width: 1, height: 1, stride: 4,
                                   pixels: Data([0, 0, 0, 0]))
  )

  // A source still used by a layer fails with invalid-state.
  try expectCommandFailure(
    await map.removeStyleSource(sourceId: "geo"),
    status: MLN_STATUS_INVALID_STATE
  )
  #expect(try await map.getStyleSourceInfo(sourceId: "geo") != nil)

  // Existing objects commit their removal, re-checked through info getters.
  // Each wait drains and discards unrelated events, so submit one at a time.
  let removals: [(String, (MapHandle) async throws -> CommandCompletion)] = [
    ("layer", { try await $0.removeStyleLayer(layerId: "fill") }),
    ("source", { try await $0.removeStyleSource(sourceId: "geo") }),
    ("image", { try await $0.removeStyleImage(imageId: "marker") }),
  ]
  for (subject, remove) in removals {
    let command = try await remove(map)
    #expect(command.disposition == .committed, "removing a \(subject)")
  }
  #expect(try await map.getStyleLayerInfo(layerId: "fill") == nil)
  #expect(try await map.getStyleSourceInfo(sourceId: "geo") == nil)
  #expect(try await map.getStyleImageInfo(imageId: "marker") == nil)

  // Removing a missing object resolves with a failed NOT_FOUND completion.
  for (_, remove) in removals {
    try expectCommandFailure(
      await remove(map),
      status: MLN_STATUS_NOT_FOUND
    )
  }
}

/// the layer stack is one copied list with absent fields as nil.
@Test func styleLayersListTheWholeStackInStyleOrder() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{"tiles":{"type":"vector",  "tiles":["https://example.com/{z}/{x}/{y}.pbf"]}},  "layers":[{"id":"bg","type":"background"},  {"id":"roads","type":"line","source":"tiles","source-layer":"road"}]}
  """))

  #expect(try await map.listStyleLayers() == [
    StyleLayerEntry(id: "bg", type: "background", sourceId: nil,
                    sourceLayer: nil),
    StyleLayerEntry(id: "roads", type: "line", sourceId: "tiles",
                    sourceLayer: "road"),
  ])
}

@Test func geoJSONSourceOptionsMaterializeFieldMaskAndClusterProperties(
) throws {
  let options = GeojsonSourceOptions(
    minZoom: 1,
    maxZoom: 12,
    tolerance: 0.5,
    clusterMaxZoom: 15,
    clusterProperties: clusterPropertiesData,
    tileSize: 256,
    buffer: 64,
    clusterRadius: 60,
    clusterMinPoints: 3,
    lineMetrics: true,
    cluster: true,
    synchronousTiling: true
  )

  let optionsArena = NativeInputArena()
  try withUnsafePointer(to: options
    .nativeValue(arena: optionsArena))
  { native in
    let fields = native.pointee.fields
    #expect((fields & MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM.rawValue) != 0)
    #expect((fields & MLN_GEOJSON_SOURCE_OPTION_CLUSTER.rawValue) != 0)
    #expect(
      (fields & MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES.rawValue) != 0
    )
    #expect(native.pointee.min_zoom == 1)
    #expect(native.pointee.max_zoom == 12)
    #expect(native.pointee.tolerance == 0.5)
    #expect(native.pointee.cluster_max_zoom == 15)
    #expect(native.pointee.tile_size == 256)
    #expect(native.pointee.buffer == 64)
    #expect(native.pointee.cluster_radius == 60)
    #expect(native.pointee.cluster_min_points == 3)
    #expect(native.pointee.line_metrics)
    #expect(native.pointee.cluster)
    #expect(
      (fields & MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING.rawValue) != 0
    )
    #expect(native.pointee.synchronous_tiling)

    let clusterProperties = native.pointee.cluster_properties
    let data = try #require(clusterProperties.data)
    #expect(
      Data(bytes: data, count: clusterProperties.size) ==
        clusterPropertiesData
    )
  }

  // Absent options keep the descriptor out of the call.
  let empty = try GeojsonSourceOptions().nativeValue(arena: NativeInputArena())
  #expect(empty.fields == 0)
}

/// Preparation parses, tiles, and validates without a runtime or map, so bad
/// documents and bad cluster options fail at the prepare step.
@Test func geoJSONSourceDataPreparationValidatesWithoutARuntime() throws {
  let prepared = try Maplibre.geojsonSourceDataCreate(
    data: nearbyPoints(),
    options: clusterOptions()
  )
  try prepared.close()

  // The cluster aggregation graph is parsed by MapLibre Native at
  // preparation, so an unparseable expression fails the prepare.
  var invalid = clusterOptions()
  invalid.clusterProperties = jsonData(
    #"{"weight_sum":"not-an-expression"}"#
  )
  #expect(throws: MaplibreError.self) {
    try Maplibre.geojsonSourceDataCreate(data: nearbyPoints(), options: invalid)
  }

  // Clustering rejects a single feature at preparation.
  #expect(throws: MaplibreError.self) {
    try Maplibre.geojsonSourceDataCreate(
      data: jsonData(
        #"{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}"#
      ),
      options: clusterOptions()
    )
  }

  // An unparseable document fails the prepare.
  #expect(throws: MaplibreError.self) {
    try Maplibre.geojsonSourceDataCreate(data: jsonData("not geojson"))
  }
}

/// One prepared handle installs on any number of sources, and the source
/// adopts the options the data was prepared with.
@Test func preparedGeoJSONSourceDataAddsAndUpdatesAcrossSources() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 512,
      height: 512,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))
  #expect(styleCommand.disposition == .committed)

  let clustered = try Maplibre.geojsonSourceDataCreate(
    data: nearbyPoints(),
    options: clusterOptions()
  )
  defer { try? clustered.close() }

  let firstAdd = try await map.addGeojsonSourceData(
    sourceId: "first",
    data: clustered
  )
  #expect(firstAdd.disposition == .committed)
  let secondAdd = try await map.addGeojsonSourceData(
    sourceId: "second",
    data: clustered
  )
  #expect(secondAdd.disposition == .committed)
  #expect(try #require(try await map.getStyleSourceInfo(sourceId: "first"))
    .info.type == .geojson)
  #expect(try #require(try await map.getStyleSourceInfo(sourceId: "second"))
    .info.type == .geojson)

  // A cheap install of already-prepared data updates both sources, because
  // the sources adopted the options the data was prepared with.
  let replacement = try Maplibre.geojsonSourceDataCreate(
    data: nearbyPoints(),
    options: clusterOptions()
  )
  defer { try? replacement.close() }
  let firstSet = try await map.setGeojsonSourceData(
    sourceId: "first",
    data: replacement
  )
  #expect(firstSet.disposition == .committed)
  let secondSet = try await map.setGeojsonSourceData(
    sourceId: "second",
    data: replacement
  )
  #expect(secondSet.disposition == .committed)

  // Cluster aggregations are part of the options-equality requirement, so
  // data prepared with different cluster_properties fails on the map thread.
  var reaggregated = clusterOptions()
  reaggregated.clusterProperties = jsonData(
    #"{"weight_max":["max",["get","weight"]]}"#
  )
  let differentAggregation = try Maplibre.geojsonSourceDataCreate(
    data: nearbyPoints(),
    options: reaggregated
  )
  defer { try? differentAggregation.close() }
  try expectCommandFailure(
    await map.setGeojsonSourceData(
      sourceId: "first",
      data: differentAggregation
    ),
    status: MLN_STATUS_INVALID_ARGUMENT
  )
}

/// A set rejects data whose baked-in options differ from the options the
/// source was added with, because they would tile inconsistently.
@Test func setGeoJSONSourceDataRejectsMismatchedOptions() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 512,
      height: 512,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let clustered = try Maplibre.geojsonSourceDataCreate(
    data: nearbyPoints(),
    options: clusterOptions()
  )
  defer { try? clustered.close() }
  let addCommand = try await map.addGeojsonSourceData(
    sourceId: "clustered",
    data: clustered
  )
  #expect(addCommand.disposition == .committed)

  let unclustered = try Maplibre.geojsonSourceDataCreate(data: nearbyPoints())
  defer { try? unclustered.close() }

  // The options mismatch is validated on the map thread, so the install is
  // accepted here and fails asynchronously through command completion.
  try expectCommandFailure(
    await map.setGeojsonSourceData(
      sourceId: "clustered",
      data: unclustered
    ),
    status: MLN_STATUS_INVALID_ARGUMENT
  )
}

/// Sources keep their own reference, so closing the handle never invalidates
/// a source, while the closed handle itself stops installing.
@Test func closedGeoJSONSourceDataStopsInstallingButKeepsSources() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 512,
      height: 512,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let prepared = try Maplibre.geojsonSourceDataCreate(data: nearbyPoints())
  let addCommand = try await map.addGeojsonSourceData(
    sourceId: "kept",
    data: prepared
  )
  #expect(addCommand.disposition == .committed)

  #expect(!prepared.isClosed)
  try prepared.close()
  #expect(prepared.isClosed)
  // A second close is a no-op success.
  try prepared.close()

  // The source outlives the handle that seeded it.
  #expect(try #require(try await map.getStyleSourceInfo(sourceId: "kept"))
    .info.type == .geojson)

  // The closed handle fails in Swift handle state before reaching native.
  do {
    try await map.addGeojsonSourceData(sourceId: "late", data: prepared)
    Issue.record("closed prepared data should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
  #expect(try await map.getStyleSourceInfo(sourceId: "late") == nil)
}

/// The runtime override slices tiles inline while enabled and restores the
/// source's own option when disabled.
@Test func synchronousTilingOverrideTogglesPerSource() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 512,
      height: 512,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let prepared = try Maplibre.geojsonSourceDataCreate(data: nearbyPoints())
  defer { try? prepared.close() }
  let addCommand = try await map.addGeojsonSourceData(
    sourceId: "tracked",
    data: prepared
  )
  #expect(addCommand.disposition == .committed)

  let enable = try await map.setGeojsonSourceSynchronousTiling(
    sourceId: "tracked",
    enabled: true
  )
  #expect(enable.disposition == .committed)

  // Installs under the override still take prepared data.
  let install = try await map.setGeojsonSourceData(
    sourceId: "tracked",
    data: prepared
  )
  #expect(install.disposition == .committed)

  let disable = try await map.setGeojsonSourceSynchronousTiling(
    sourceId: "tracked",
    enabled: false
  )
  #expect(disable.disposition == .committed)

  // A source that does not exist fails on the map thread, asynchronously
  // through command completion.
  try expectCommandFailure(
    await map.setGeojsonSourceSynchronousTiling(
      sourceId: "missing",
      enabled: true
    ),
    status: MLN_STATUS_NOT_FOUND
  )
}

@Test func styleTransitionOptionsRoundTripThroughTheCAPI() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  // A map with no style yet reports no duration or delay. The placement flag
  // always reports, because MapLibre Native always holds a value for it.
  let empty = try await map.getStyleTransitionOptions()
  #expect(empty.durationMs == nil)
  #expect(empty.delayMs == nil)
  #expect(empty.enablePlacementTransitions == true)

  // The style parser fills in its own 300ms duration for a style that declares
  // no transition.
  try await map.setStyleJson(json: jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))
  let parsed = try await map.getStyleTransitionOptions()
  #expect(parsed.durationMs == 300)
  #expect(parsed.delayMs == nil)

  try await map.setStyleJson(json: jsonData(transitionStyleJSON))
  let declared = try await map.getStyleTransitionOptions()
  #expect(declared.durationMs == 750)
  #expect(declared.delayMs == 100)
  #expect(declared.enablePlacementTransitions == true)

  // A present zero stays distinguishable from an absent field, and an absent
  // field clears what the style declared rather than merging into it.
  let options = StyleTransitionOptions(
    durationMs: 0,
    enablePlacementTransitions: false
  )
  try await map.setStyleTransitionOptions(options: options)
  #expect(try await map.getStyleTransitionOptions() == options)

  // Omitting the flag leaves the cross-fade on rather than clearing it.
  try await map.setStyleTransitionOptions(options:
    StyleTransitionOptions(durationMs: 250))
  #expect(try await map.getStyleTransitionOptions()
    .enablePlacementTransitions == true)

  // Loading a style replaces the override with what that style declares.
  try await map.setStyleJson(json: jsonData(transitionStyleJSON))
  #expect(try await map.getStyleTransitionOptions() == declared)

  try expectCommandFailure(
    await map.setStyleTransitionOptions(options:
      StyleTransitionOptions(delayMs: -1)),
    status: MLN_STATUS_INVALID_ARGUMENT
  )
  #expect(try await map.getStyleTransitionOptions() == declared)
}

private let transitionStyleJSON = """
{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}
"""

private let clusterPropertiesData = jsonData(
  #"{"weight_sum":["+",["get","weight"]]}"#
)

private func clusterOptions() -> GeojsonSourceOptions {
  GeojsonSourceOptions(
    clusterMaxZoom: 17,
    clusterProperties: clusterPropertiesData,
    clusterRadius: 60,
    clusterMinPoints: 2,
    cluster: true
  )
}

private func nearbyPoints() -> Data {
  jsonData(
    #"{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"weight":1}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"weight":2}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"weight":3}}]}"#
  )
}

private func jsonData(_ value: String) -> Data {
  Data(value.utf8)
}

/// A style mutation that names an ID no object carries fails with
/// `MLN_STATUS_NOT_FOUND` through its completion, whichever kind of object the
/// ID was meant to name.
@Test func styleMutationsReportNotFoundForAMissingId() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  #expect(try await map.setStyleJson(json: emptyStyleJSON)
    .disposition == .committed)

  let mutations: [(String, (MapHandle) async throws -> CommandCompletion)] = [
    ("a GeoJSON source URL", {
      try await $0.setGeojsonSourceUrl(
        sourceId: "missing",
        url: "https://example.test/data.json"
      )
    }),
    ("synchronous tiling", {
      try await $0.setGeojsonSourceSynchronousTiling(
        sourceId: "missing",
        enabled: true
      )
    }),
    ("an image source URL", {
      try await $0.setImageSourceUrl(
        sourceId: "missing",
        url: "https://example.test/image.png"
      )
    }),
    ("a layer's visibility", {
      try await $0.setLayerVisibility(
        layerId: "missing", visibility: StyleLayerVisibility.none
      )
    }),
    ("a layer's minimum zoom", {
      try await $0.setLayerMinZoom(layerId: "missing", minZoom: 2)
    }),
    ("a layer move", {
      try await $0.moveStyleLayer(layerId: "missing")
    }),
  ]
  for (subject, mutate) in mutations {
    let command = try await mutate(map)
    #expect(command.disposition == .failed, "setting \(subject)")
    #expect(
      command.rawStatus == MLN_STATUS_NOT_FOUND.rawValue,
      "setting \(subject)"
    )
  }
}

/// An image source takes exactly four corner coordinates, which the binding
/// checks before it reaches the C API.
@Test func imageSourceCoordinatesRejectAnyCountButFour() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 1,
      height: 1,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  do {
    _ = try await map.addImageSourceUrl(
      sourceId: "image",
      coordinates: [LatLng(latitude: 0, longitude: 0)],
      url: "https://example.test/image.png"
    )
    Issue.record("three missing corners should be rejected")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(error.diagnostic.contains("must be 4"))
  }
}

@Test func globalStateDefaultsUpdatesAndStyleReplacement() async throws {
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
  let rejected = try await map.setGlobalStateProperty(
    propertyName: "theme",
    value: Data("true".utf8)
  )
  #expect(rejected.disposition == .failed)
  #expect(rejected.diagnostic.contains("style JSON has not loaded"))
  try await map
    .setStyleJson(
      json: Data(#"{"version":8,"sources":{},"layers":[],"state":{"theme":{"default":"light"}}}"#
        .utf8)
    )
  #expect(try await map.getGlobalState() == Data(#"{"theme":"light"}"#.utf8))
  try await map.setGlobalStateProperty(
    propertyName: "theme",
    value: Data(#"["dark",{"enabled":true}]"#.utf8)
  )
  let snapshot = try await map.getGlobalState()
  try await map.setGlobalStateProperty(
    propertyName: "theme",
    value: Data("null".utf8)
  )
  #expect(try await map.getGlobalState() == Data(#"{"theme":"light"}"#.utf8))
  #expect(snapshot == Data(#"{"theme":["dark",{"enabled":true}]}"#.utf8))
  try await map
    .setStyleJson(json: Data(#"{"version":8,"sources":{},"layers":[]}"#.utf8))
  #expect(try await map.getGlobalState() == Data("{}".utf8))
}
