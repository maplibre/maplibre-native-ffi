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

  try options.nativeOptions.withNativeOptions { native in
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

@Test func styleSourceInfoPreservesAbsentFieldsAndUnknownEnums() {
  let native = NativeStyleSourceInfo(
    type: 700,
    isVolatile: true,
    attribution: nil,
    url: nil,
    tileJSON: NativeStyleSourceTileJSON(
      tileURLs: [],
      minZoom: 0,
      maxZoom: 0,
      scheme: 701,
      bounds: nil
    ),
    tileSize: 0,
    vectorEncoding: 702,
    rasterEncoding: 703
  )
  let publicInfo = StyleSourceInfo(native: native)

  #expect(publicInfo.type.rawValue == 700)
  #expect(publicInfo.isVolatile)
  #expect(publicInfo.attribution == nil)
  #expect(publicInfo.url == nil)
  #expect(publicInfo.tileJSON?.tileURLs == [])
  #expect(publicInfo.tileJSON?.minZoom == 0)
  #expect(publicInfo.tileJSON?.scheme.rawValue == 701)
  #expect(publicInfo.tileJSON?.bounds == nil)
  #expect(publicInfo.tileSize == 0)
  #expect(publicInfo.vectorEncoding?.rawValue == 702)
  #expect(publicInfo.rasterEncoding?.rawValue == 703)
}

@Test func sourceInspectionCopiesReconstructibleMetadata() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1, height: 1))
  defer { try? map.closeBlockingForTests() }

  try map.setStyleJSON(jsonData(#"{"version":8,"sources":{},"layers":[]}"#))
  let bounds = LatLngBounds(
    southwest: LatLng(latitude: -1, longitude: -2),
    northeast: LatLng(latitude: 3, longitude: 4)
  )
  try map.addVectorSourceTiles(
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
  try map.addVectorSourceURL(
    sourceId: "remote",
    url: "https://example.com/source.json"
  )
  let emptyCollection = try GeoJSONSourceDataHandle(
    data: jsonData(#"{"type":"FeatureCollection","features":[]}"#)
  )
  defer { try? emptyCollection.close() }
  try map.addGeoJSONSourceData(sourceId: "data", data: emptyCollection)

  let inline = try #require(try await map.styleSourceInfo("inline"))
  #expect(inline.type == .vector)
  #expect(inline.url == nil)
  #expect(inline.attribution == "© inline")
  #expect(
    inline.tileJSON?.tileURLs == [
      "https://a.example/{z}/{x}/{y}.mvt",
      "https://b.example/{z}/{x}/{y}.mvt",
    ]
  )
  #expect(inline.tileJSON?.minZoom == 0)
  #expect(inline.tileJSON?.maxZoom == 9)
  #expect(inline.tileJSON?.scheme == .tms)
  #expect(inline.tileJSON?.bounds == bounds)
  #expect(inline.tileSize == 512)
  #expect(inline.vectorEncoding == .mlt)
  #expect(inline.rasterEncoding == nil)

  let remote = try #require(try await map.styleSourceInfo("remote"))
  #expect(remote.url == "https://example.com/source.json")
  #expect(remote.tileJSON == nil)
  #expect(remote.attribution == nil)

  let data = try #require(try await map.styleSourceInfo("data"))
  #expect(data.url == nil)
  #expect(data.tileJSON == nil)
  #expect(data.tileSize == nil)
  #expect(data.vectorEncoding == nil)
  #expect(data.rasterEncoding == nil)
  #expect(try await map.styleSourceInfo("missing") == nil)

  let removeCommand = try map.removeStyleSource("inline")
  #expect(try await commandDisposition(
    removeCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  #expect(try await map.styleSourceInfo("inline") == nil)
  try map.close()

  // Every nested string and value remains valid after its native source and
  // owning map are gone.
  #expect(inline.attribution == "© inline")
  #expect(inline.tileJSON?.tileURLs.count == 2)
  #expect(inline.tileJSON?.bounds == bounds)
  #expect(remote.url == "https://example.com/source.json")
}

@Test func styleImageDescriptorsMaterializeScopedPixelsAndOptions() throws {
  let image = StyleRGBA8Image(
    width: 1,
    height: 1,
    stride: 4,
    pixels: [1, 2, 3, 4]
  )

  try image.nativeImage.withNativeImage { native in
    #expect(native.pointee.width == 1)
    #expect(native.pointee.height == 1)
    #expect(native.pointee.stride == 4)
    #expect(native.pointee.byte_length == 4)
    #expect(native.pointee.pixels![2] == 3)
  }

  try StyleImageOptions(pixelRatio: 2, sdf: true).nativeOptions
    .withNativeOptions { options in
      #expect((options.pointee.fields & MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
          .rawValue) != 0)
      #expect((options.pointee.fields & MLN_STYLE_IMAGE_OPTION_SDF.rawValue) !=
        0)
      #expect(options.pointee.pixel_ratio == 2)
      #expect(options.pointee.sdf)
    }
}

@Test func imageSourceCoordinatesRejectInvalidCountBeforeCallingC() throws {
  let map = SyntheticHandles.map()
  let sourceId = mln_buffer_view()
  let coordinate = NativeLatLng(latitude: 1, longitude: 2)

  do {
    try NativeStyle.imageSourceCommand([coordinate]) { _, _, _ in
      Issue.record("native command should not run")
    }
    Issue.record("invalid coordinate count should throw")
  } catch let failure as NativeStatusFailure {
    #expect(!failure.isNativeStatus)
    #expect(failure.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(failure
      .diagnostic ==
      "image source coordinates must contain exactly 4 coordinates")
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func customGeometryOptionsRetainAndInvokeTileCallbacks() throws {
  final class TileBox: @unchecked Sendable {
    var fetched: [NativeCanonicalTileID] = []
    var cancelled: [NativeCanonicalTileID] = []
  }

  let box = TileBox()
  let callbacks = NativeCustomGeometrySourceCallbacks(
    fetchTile: { box.fetched.append($0) },
    cancelTile: { box.cancelled.append($0) }
  )
  defer { callbacks.release() }
  let options = NativeCustomGeometrySourceOptions(
    callbacks: callbacks,
    minZoom: 1,
    maxZoom: 10,
    tolerance: 0.5,
    tileSize: 256,
    buffer: 8,
    clip: true,
    wrap: false
  )

  try options.withNativeOptions { native in
    #expect((native.pointee.fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
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

  #expect(box.fetched == [NativeCanonicalTileID(z: 1, x: 2, y: 3)])
  #expect(box.cancelled == [NativeCanonicalTileID(z: 4, x: 5, y: 6)])
}

/// The C API can call the release callback while a tile callback is still
/// running on another thread, so the release waits for that call rather than
/// freeing the state under it.
private func waitForSemaphore(
  _ semaphore: DispatchSemaphore,
  timeout: DispatchTime
) -> DispatchTimeoutResult {
  semaphore.wait(timeout: timeout)
}

@Test func customGeometryCallbacksWaitForInFlightInvocationBeforeRelease(
) async throws {
  // The C ABI callbacks an options struct carries, held past the struct's
  // lifetime so a test calls them the way the C API does.
  final class NativeCallbacks: @unchecked Sendable {
    private let fetchTile: mln_custom_geometry_source_tile_callback
    private let releaseUserData: mln_custom_geometry_source_release_callback
    private let userDataAddress: UInt

    init(
      _ options: UnsafePointer<mln_custom_geometry_source_options>
    ) throws {
      fetchTile = try #require(options.pointee.fetch_tile)
      releaseUserData = try #require(options.pointee.release_user_data)
      userDataAddress =
        try UInt(bitPattern: #require(options.pointee.user_data))
    }

    private var userData: UnsafeMutableRawPointer? {
      UnsafeMutableRawPointer(bitPattern: userDataAddress)
    }

    func fetch() {
      fetchTile(userData, mln_canonical_tile_id(z: 1, x: 2, y: 3))
    }

    func release() {
      releaseUserData(userData)
    }
  }

  let entered = DispatchSemaphore(value: 0)
  let allowReturn = DispatchSemaphore(value: 0)
  let invocationFinished = DispatchSemaphore(value: 0)
  let releaseStarted = DispatchSemaphore(value: 0)
  let releaseFinished = DispatchSemaphore(value: 0)

  let native = try NativeCustomGeometrySourceOptions(
    callbacks: NativeCustomGeometrySourceCallbacks(fetchTile: { _ in
      entered.signal()
      allowReturn.wait()
    })
  ).withNativeOptions(NativeCallbacks.init)

  Thread {
    native.fetch()
    invocationFinished.signal()
  }.start()
  #expect(await Task.detached {
    waitForSemaphore(entered, timeout: .now() + .seconds(5))
  }.value == .success)

  Thread {
    releaseStarted.signal()
    native.release()
    releaseFinished.signal()
  }.start()
  #expect(await Task.detached {
    waitForSemaphore(releaseStarted, timeout: .now() + .seconds(5))
  }.value == .success)
  #expect(await Task.detached {
    waitForSemaphore(releaseFinished, timeout: .now() + .milliseconds(100))
  }.value == .timedOut)

  allowReturn.signal()
  #expect(await Task.detached {
    waitForSemaphore(invocationFinished, timeout: .now() + .seconds(5))
  }.value == .success)
  #expect(await Task.detached {
    waitForSemaphore(releaseFinished, timeout: .now() + .seconds(5))
  }.value == .success)
}

/// Counts the custom geometry source callback states the C API has released.
private final class ReleaseCounter: @unchecked Sendable {
  private let lock = NSLock()
  private var count = 0

  var value: Int {
    lock.withLock { count }
  }

  func increment() {
    lock.withLock { count += 1 }
  }
}

/// Captured by a custom geometry source's tile closure, so its deallocation
/// reports that the C API released that source's callback state.
private final class ReleaseSentinel: @unchecked Sendable {
  private let counter: ReleaseCounter

  init(_ counter: ReleaseCounter) {
    self.counter = counter
  }

  deinit {
    counter.increment()
  }
}

/// Adds a custom geometry source whose callback state reports its own release
/// through `counter`.
private func addSourceReportingItsRelease(
  to map: MapHandle,
  sourceId: String = "custom",
  counter: ReleaseCounter
) throws -> UInt64 {
  let sentinel = ReleaseSentinel(counter)
  return try map.addCustomGeometrySource(
    sourceId: sourceId,
    options: CustomGeometrySourceOptions(fetchTile: { _ in
      withExtendedLifetime(sentinel) {}
    })
  )
}

/// A style load drops the sources the previous style held, and the C API
/// releases their callback state without a map-style-loaded event, so a host
/// that never selected that event type still gets its state freed.
@Test func aStyleLoadReleasesADroppedCustomGeometrySource() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  try runtime.setResourceProvider { request, handle in
    guard request.requestedUrl == "maplibre://maps/replacement" else {
      return .passThrough
    }
    try? handle.complete(ResourceResponse(status: .ok, bytes: emptyStyleJSON))
    return .handle
  }
  let narrowed = RuntimeEventMask.all.subtracting(.mapStyleLoaded)
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(
                                  width: 64,
                                  height: 64,
                                  eventMask: narrowed
                                ))
  defer { try? map.closeBlockingForTests() }

  try map.setStyleJSON(emptyStyleJSON)
  let counter = ReleaseCounter()
  try addSourceReportingItsRelease(to: map, counter: counter)
  #expect(counter.value == 0)

  try map.setStyleURL("maplibre://maps/replacement")
  var styleLoadedReported = false
  let deadline = Date().addingTimeInterval(10)
  while Date() < deadline, counter.value == 0 {
    try await runtime.barrier()
    styleLoadedReported = try styleLoadedReported || runtime.drainEvents()
      .events.contains { $0.type == .mapStyleLoaded }
    try await Task<Never, Never>.sleep(nanoseconds: 1_000_000)
  }

  #expect(counter.value == 1)
  #expect(!styleLoadedReported)
  #expect(try map.eventMask == narrowed)
  #expect(try await map.styleSourceInfo("custom") == nil)
}

/// Removing a custom geometry source releases its callback state, and does so
/// once.
@Test func removingACustomGeometrySourceReleasesItsCallbacks() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 64, height: 64))
  defer { try? map.closeBlockingForTests() }

  try map.setStyleJSON(emptyStyleJSON)
  let counter = ReleaseCounter()
  try addSourceReportingItsRelease(to: map, counter: counter)

  let removeCommand = try map.removeStyleSource("custom")
  #expect(try await commandDisposition(
    removeCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  #expect(counter.value == 1)
  try map.close()
  #expect(counter.value == 1)
}

/// Destroying a map releases the callback state of the sources it still holds.
@Test func closingAMapReleasesItsCustomGeometrySources() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 64, height: 64))
  defer { try? map.closeBlockingForTests() }

  try map.setStyleJSON(emptyStyleJSON)
  let counter = ReleaseCounter()
  for sourceId in ["first", "second"] {
    try addSourceReportingItsRelease(
      to: map,
      sourceId: sourceId,
      counter: counter
    )
  }
  #expect(counter.value == 0)

  try map.close()
  try await runtime.barrier()

  #expect(counter.value == 2)
}

/// Accepted commands own callback state. A command rejected by the map worker
/// releases that state before a subsequent runtime barrier completes.
@Test func aRejectedCustomGeometrySourceAddReleasesItsCallbacks() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 64, height: 64))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try map.setStyleJSON(emptyStyleJSON)
  #expect(try await commandDisposition(
    styleCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  let accepted = ReleaseCounter()
  let acceptedCommand = try addSourceReportingItsRelease(
    to: map, counter: accepted
  )
  #expect(try await commandDisposition(
    acceptedCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  let rejected = ReleaseCounter()
  let rejectedCommand = try addSourceReportingItsRelease(
    to: map, counter: rejected
  )
  #expect(try await commandDisposition(
    rejectedCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_FAILED.rawValue)

  #expect(rejected.value == 1)
  #expect(accepted.value == 0)
}

@Test func loadedStyleDocumentAndURLReadBackWhatWasLoaded() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1, height: 1))
  defer { try? map.closeBlockingForTests() }

  // Nothing parsed and nothing requested yet.
  #expect(try await map.loadedStyleJSON() == Data())
  #expect(try await map.styleURL() == "")

  // The document reads back byte-for-byte, so it can be reloaded unchanged.
  let styleJSON = jsonData(#"{"version":8,"sources":{},"layers":[]}"#)
  try map.setStyleJSON(styleJSON)
  #expect(try await map.loadedStyleJSON() == styleJSON)
  // Inline JSON clears the URL.
  #expect(try await map.styleURL() == "")

  // The URL is request state, recorded before the load can succeed, while the
  // document still reports the style that last parsed.
  try map.setStyleURL("https://example.com/style.json")
  #expect(try await map.styleURL() == "https://example.com/style.json")
  #expect(try await map.loadedStyleJSON() == styleJSON)
}

@Test func closedMapRejectsStyleCallsThroughSwiftHandleState() async throws {
  let runtime = try RuntimeHandle()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1, height: 1))
  try map.close()

  do {
    _ = try await map.styleLayerIds()
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
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1, height: 1))
  defer { try? map.closeBlockingForTests() }

  try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let image = StyleRGBA8Image(
    width: 2,
    height: 2,
    stride: 8,
    pixels: [UInt8](repeating: 0, count: 16)
  )
  let options = StyleImageOptions(
    stretchX: [ImageStretch(from: 0, to: 1)],
    stretchY: [ImageStretch(from: 0, to: 1), ImageStretch(from: 1, to: 2)],
    content: ImageContent(left: 0.5, top: 0.5, right: 1.5, bottom: 1.5),
    textFitHeight: .proportional
  )
  try map.setStyleImage(imageId: "patch", image: image, options: options)

  let info = try #require(try await map.styleImageInfo("patch"))
  #expect(info.stretchXCount == 1)
  #expect(info.stretchYCount == 2)
  #expect(info.content?.right == 1.5)
  // An absent text fit stays distinguishable from a present default.
  #expect(info.textFitWidth == nil)
  #expect(info.textFitHeight == .proportional)

  let stretches = try #require(try await map.styleImageStretches("patch"))
  #expect(stretches.stretchX == [ImageStretch(from: 0, to: 1)])
  #expect(
    stretches.stretchY == [
      ImageStretch(from: 0, to: 1), ImageStretch(from: 1, to: 2),
    ]
  )
  #expect(try await map.styleImageStretches("missing") == nil)

  // A backwards interval is rejected by C.
  #expect(throws: MaplibreError.self) {
    try map.setStyleImage(
      imageId: "bad",
      image: image,
      options: StyleImageOptions(stretchX: [ImageStretch(from: 2, to: 1)])
    )
  }
}

@Test func layerBaseAccessorsRoundTripThroughNativeMap() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1, height: 1))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"bg","type":"background"},{"id":"fill","type":"fill","source":"geo"}]}
  """))
  #expect(try await commandDisposition(
    styleCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  #expect(try await map.layerSourceLayer("fill") == "")
  let sourceLayerCommand = try map.setLayerSourceLayer(
    layerId: "fill", sourceLayer: "roads"
  )
  #expect(try await commandDisposition(
    sourceLayerCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  #expect(try await map.layerSourceLayer("fill") == "roads")
  #expect(try await map.layerSourceId("fill") == "geo")

  let rejectedSourceLayer = try map.setLayerSourceLayer(
    layerId: "bg", sourceLayer: "roads"
  )
  #expect(try await commandDisposition(
    rejectedSourceLayer, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_FAILED.rawValue)
  #expect(try await map.layerSourceLayer("bg") == "")
  #expect(try await map.layerSourceId("bg") == "")

  // The layer-info aggregate reports the unbounded zoom range, the layer
  // type, the visibility, and the source strings its sizes describe.
  let unbounded = try #require(try await map.styleLayerInfo("fill"))
  #expect(unbounded.type == "fill")
  #expect(unbounded.minZoom == -Double.infinity)
  #expect(unbounded.maxZoom == Double.infinity)
  #expect(unbounded.visibility == .visible)
  #expect(unbounded.sourceId == "geo")
  #expect(unbounded.sourceLayer == "roads")

  let background = try #require(try await map.styleLayerInfo("bg"))
  #expect(background.type == "background")
  #expect(background.sourceId == nil)
  #expect(background.sourceLayer == nil)

  let minZoomCommand = try map.setLayerMinZoom(layerId: "fill", minZoom: 4)
  #expect(try await commandDisposition(
    minZoomCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  let maxZoomCommand = try map.setLayerMaxZoom(
    layerId: "fill", maxZoom: 12.5
  )
  #expect(try await commandDisposition(
    maxZoomCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  let visibilityCommand = try map.setLayerVisibility(
    layerId: "fill", visibility: .none
  )
  #expect(try await commandDisposition(
    visibilityCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  let bounded = try #require(try await map.styleLayerInfo("fill"))
  #expect(bounded.minZoom == 4)
  #expect(bounded.maxZoom == 12.5)
  #expect(bounded.visibility == .none)

  let rejectedVisibility = try map.setLayerVisibility(
    layerId: "fill",
    visibility: StyleLayerVisibility(rawValue: 900)
  )
  #expect(try await commandDisposition(
    rejectedVisibility, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_FAILED.rawValue)
  #expect(try await map.styleLayerInfo("fill")?
    .visibility == StyleLayerVisibility.none)

  #expect(try await map.styleLayerInfo("missing") == nil)
}

/// A removal command commits when the object existed, and fails with
/// `MLN_STATUS_NOT_FOUND` when nothing has the ID. The info getters' found
/// flag re-checks existence.
@Test func styleRemovalsCommitOrFailWithNotFound() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1, height: 1))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"fill","type":"fill","source":"geo"}]}
  """))
  #expect(try await commandDisposition(
    styleCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  try map.setStyleImage(
    imageId: "marker",
    image: StyleRGBA8Image(width: 1, height: 1, stride: 4,
                           pixels: [0, 0, 0, 0])
  )

  // A source still used by a layer fails with invalid-state.
  let usedSource = try map.removeStyleSource("geo")
  let usedResult = try #require(try await commandFinished(
    usedSource, runtime: runtime
  ))
  #expect(usedResult.finished
    .disposition == MLN_COMMAND_DISPOSITION_FAILED.rawValue)
  #expect(usedResult.code == MLN_STATUS_INVALID_STATE.rawValue)
  #expect(try await map.styleSourceInfo("geo") != nil)

  // Existing objects commit their removal, re-checked through info getters.
  // Each wait drains and discards unrelated events, so submit one at a time.
  let removals: [(String, (MapHandle) throws -> UInt64)] = [
    ("layer", { try $0.removeStyleLayer("fill") }),
    ("source", { try $0.removeStyleSource("geo") }),
    ("image", { try $0.removeStyleImage("marker") }),
  ]
  for (subject, remove) in removals {
    let command = try remove(map)
    #expect(try await commandDisposition(
      command, runtime: runtime
    ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue, "removing a \(subject)")
  }
  #expect(try await map.styleLayerInfo("fill") == nil)
  #expect(try await map.styleSourceInfo("geo") == nil)
  #expect(try await map.styleImageInfo("marker") == nil)

  // Removing a missing object fails with NOT_FOUND, surfaced through the
  // binding's error domain.
  for (subject, remove) in removals {
    let command = try remove(map)
    let result = try #require(try await commandFinished(
      command, runtime: runtime
    ))
    #expect(result.finished
      .disposition == MLN_COMMAND_DISPOSITION_FAILED.rawValue,
      "removing a missing \(subject)")
    #expect(result.code == MLN_STATUS_NOT_FOUND.rawValue)
    #expect(MaplibreError.fromNativeFailure(NativeStatusFailure(
      rawStatus: result.code, diagnostic: ""
    )).kind == .notFound)
  }
}

@Test func geoJSONSourceOptionsMaterializeFieldMaskAndClusterProperties(
) throws {
  let options = StyleGeoJSONSourceOptions(
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

  try options.nativeOptions.withNativeOptions { native in
    let native = try #require(native)
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
      Data(bytes: data, count: clusterProperties.size) == clusterPropertiesData
    )
  }

  // Absent options keep the descriptor out of the call.
  try StyleGeoJSONSourceOptions().nativeOptions.withNativeOptions { native in
    #expect(native == nil)
  }
}

/// Preparation parses, tiles, and validates without a runtime or map, so bad
/// documents and bad cluster options fail at the prepare step.
@Test func geoJSONSourceDataPreparationValidatesWithoutARuntime() throws {
  let prepared = try GeoJSONSourceDataHandle(
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
    try GeoJSONSourceDataHandle(data: nearbyPoints(), options: invalid)
  }

  // Clustering rejects a single feature at preparation.
  #expect(throws: MaplibreError.self) {
    try GeoJSONSourceDataHandle(
      data: jsonData(
        #"{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}"#
      ),
      options: clusterOptions()
    )
  }

  // An unparseable document fails the prepare.
  #expect(throws: MaplibreError.self) {
    try GeoJSONSourceDataHandle(data: jsonData("not geojson"))
  }
}

/// One prepared handle installs on any number of sources, and the source
/// adopts the options the data was prepared with.
@Test func preparedGeoJSONSourceDataAddsAndUpdatesAcrossSources() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 512, height: 512))
  defer { try? map.closeBlockingForTests() }

  let styleCommand = try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))
  #expect(try await commandDisposition(
    styleCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  let clustered = try GeoJSONSourceDataHandle(
    data: nearbyPoints(),
    options: clusterOptions()
  )
  defer { try? clustered.close() }

  let firstAdd = try map.addGeoJSONSourceData(
    sourceId: "first",
    data: clustered
  )
  #expect(try await commandDisposition(
    firstAdd, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  let secondAdd = try map.addGeoJSONSourceData(
    sourceId: "second",
    data: clustered
  )
  #expect(try await commandDisposition(
    secondAdd, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  #expect(try #require(try await map.styleSourceInfo("first"))
    .type == .geoJSON)
  #expect(try #require(try await map.styleSourceInfo("second"))
    .type == .geoJSON)

  // A cheap install of already-prepared data updates both sources, because
  // the sources adopted the options the data was prepared with.
  let replacement = try GeoJSONSourceDataHandle(
    data: nearbyPoints(),
    options: clusterOptions()
  )
  defer { try? replacement.close() }
  let firstSet = try map.setGeoJSONSourceData(
    sourceId: "first",
    data: replacement
  )
  #expect(try await commandDisposition(
    firstSet, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  let secondSet = try map.setGeoJSONSourceData(
    sourceId: "second",
    data: replacement
  )
  #expect(try await commandDisposition(
    secondSet, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  // Cluster aggregations are part of the options-equality requirement, so
  // data prepared with different cluster_properties fails on the map thread.
  var reaggregated = clusterOptions()
  reaggregated.clusterProperties = jsonData(
    #"{"weight_max":["max",["get","weight"]]}"#
  )
  let differentAggregation = try GeoJSONSourceDataHandle(
    data: nearbyPoints(),
    options: reaggregated
  )
  defer { try? differentAggregation.close() }
  let rejected = try map.setGeoJSONSourceData(
    sourceId: "first",
    data: differentAggregation
  )
  #expect(try await commandDisposition(
    rejected, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_FAILED.rawValue)
}

/// A set rejects data whose baked-in options differ from the options the
/// source was added with, because they would tile inconsistently.
@Test func setGeoJSONSourceDataRejectsMismatchedOptions() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 512, height: 512))
  defer { try? map.closeBlockingForTests() }
  try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let clustered = try GeoJSONSourceDataHandle(
    data: nearbyPoints(),
    options: clusterOptions()
  )
  defer { try? clustered.close() }
  let addCommand = try map.addGeoJSONSourceData(
    sourceId: "clustered",
    data: clustered
  )
  #expect(try await commandDisposition(
    addCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  let unclustered = try GeoJSONSourceDataHandle(data: nearbyPoints())
  defer { try? unclustered.close() }

  // The options mismatch is validated on the map thread, so the install is
  // accepted here and fails asynchronously through COMMAND_FINISHED.
  let rejected = try map.setGeoJSONSourceData(
    sourceId: "clustered",
    data: unclustered
  )
  #expect(try await commandDisposition(
    rejected, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_FAILED.rawValue)
}

/// Sources keep their own reference, so closing the handle never invalidates
/// a source, while the closed handle itself stops installing.
@Test func closedGeoJSONSourceDataStopsInstallingButKeepsSources() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 512, height: 512))
  defer { try? map.closeBlockingForTests() }
  try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let prepared = try GeoJSONSourceDataHandle(data: nearbyPoints())
  let addCommand = try map.addGeoJSONSourceData(
    sourceId: "kept",
    data: prepared
  )
  #expect(try await commandDisposition(
    addCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  #expect(!prepared.isClosed)
  try prepared.close()
  #expect(prepared.isClosed)
  // A second close is a no-op success.
  try prepared.close()

  // The source outlives the handle that seeded it.
  #expect(try #require(try await map.styleSourceInfo("kept"))
    .type == .geoJSON)

  // The closed handle fails in Swift handle state before reaching native.
  do {
    try map.addGeoJSONSourceData(sourceId: "late", data: prepared)
    Issue.record("closed prepared data should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
  #expect(try await map.styleSourceInfo("late") == nil)
}

/// The runtime override slices tiles inline while enabled and restores the
/// source's own option when disabled.
@Test func synchronousTilingOverrideTogglesPerSource() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 512, height: 512))
  defer { try? map.closeBlockingForTests() }
  try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))

  let prepared = try GeoJSONSourceDataHandle(data: nearbyPoints())
  defer { try? prepared.close() }
  let addCommand = try map.addGeoJSONSourceData(
    sourceId: "tracked",
    data: prepared
  )
  #expect(try await commandDisposition(
    addCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  let enable = try map.setGeoJSONSourceSynchronousTiling(
    sourceId: "tracked",
    enabled: true
  )
  #expect(try await commandDisposition(
    enable, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  // Installs under the override still take prepared data.
  let install = try map.setGeoJSONSourceData(
    sourceId: "tracked",
    data: prepared
  )
  #expect(try await commandDisposition(
    install, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  let disable = try map.setGeoJSONSourceSynchronousTiling(
    sourceId: "tracked",
    enabled: false
  )
  #expect(try await commandDisposition(
    disable, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

  // A source that does not exist fails on the map thread, asynchronously
  // through COMMAND_FINISHED.
  let missing = try map.setGeoJSONSourceSynchronousTiling(
    sourceId: "missing",
    enabled: true
  )
  #expect(try await commandDisposition(
    missing, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_FAILED.rawValue)
}

@Test func styleTransitionOptionsRoundTripThroughTheCAPI() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(runtime: runtime,
                                options: MapOptions(width: 1, height: 1))
  defer { try? map.closeBlockingForTests() }

  // A map with no style yet reports no duration or delay. The placement flag
  // always reports, because MapLibre Native always holds a value for it.
  let empty = try await map.styleTransitionOptions()
  #expect(empty.durationMilliseconds == nil)
  #expect(empty.delayMilliseconds == nil)
  #expect(empty.enablePlacementTransitions == true)

  // The style parser fills in its own 300ms duration for a style that declares
  // no transition.
  try map.setStyleJSON(jsonData("""
  {"version":8,"sources":{},"layers":[]}
  """))
  let parsed = try await map.styleTransitionOptions()
  #expect(parsed.durationMilliseconds == 300)
  #expect(parsed.delayMilliseconds == nil)

  try map.setStyleJSON(jsonData(transitionStyleJSON))
  let declared = try await map.styleTransitionOptions()
  #expect(declared.durationMilliseconds == 750)
  #expect(declared.delayMilliseconds == 100)
  #expect(declared.enablePlacementTransitions == true)

  // A present zero stays distinguishable from an absent field, and an absent
  // field clears what the style declared rather than merging into it.
  let options = StyleTransitionOptions(
    durationMilliseconds: 0,
    enablePlacementTransitions: false
  )
  try map.setStyleTransitionOptions(options)
  #expect(try await map.styleTransitionOptions() == options)

  // Omitting the flag leaves the cross-fade on rather than clearing it.
  try map.setStyleTransitionOptions(
    StyleTransitionOptions(durationMilliseconds: 250)
  )
  #expect(try await map.styleTransitionOptions()
    .enablePlacementTransitions == true)

  // Loading a style replaces the override with what that style declares.
  try map.setStyleJSON(jsonData(transitionStyleJSON))
  #expect(try await map.styleTransitionOptions() == declared)

  let rejectedCommand = try map.setStyleTransitionOptions(
    StyleTransitionOptions(delayMilliseconds: -1)
  )
  #expect(try await commandDisposition(
    rejectedCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_FAILED.rawValue)
  #expect(try await map.styleTransitionOptions() == declared)
}

private let transitionStyleJSON = """
{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}
"""

private let clusterPropertiesData = jsonData(
  #"{"weight_sum":["+",["get","weight"]]}"#
)

private func clusterOptions() -> StyleGeoJSONSourceOptions {
  StyleGeoJSONSourceOptions(
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
