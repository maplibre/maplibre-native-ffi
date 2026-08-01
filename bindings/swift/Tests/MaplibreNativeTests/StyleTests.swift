import CMaplibreNativeC
import Foundation
@testable import MaplibreNative
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

@Test func styleSourceInfoPreservesAbsentAttribution() {
  var raw = mln_style_source_info()
  raw.type = MLN_STYLE_SOURCE_TYPE_VECTOR.rawValue
  raw.id_size = 6
  raw.is_volatile = true
  raw.has_attribution = false
  raw.attribution_size = 12

  let native = NativeStyleSourceInfo(raw)
  let publicInfo = StyleSourceInfo(native: native)

  #expect(native.hasAttribution == false)
  #expect(native.attributionSize == 12)
  #expect(publicInfo.type == .vector)
  #expect(publicInfo.isVolatile)
  #expect(publicInfo.hasAttribution == false)
  #expect(publicInfo.attributionSize == 12)
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
  let sourceId = mln_string_view()
  let coordinate = NativeLatLng(latitude: 1, longitude: 2)

  do {
    try NativeStyle.addImageSourceURL(
      map,
      sourceId: sourceId,
      coordinates: [coordinate],
      url: mln_string_view()
    )
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

@Test func geoJSONFeatureMaterializesNativeDescriptorTree() throws {
  let geoJSON = GeoJSON.feature(Feature(
    geometry: .polygon([[
      LatLng(latitude: 1, longitude: 2),
      LatLng(latitude: 3, longitude: 4),
    ]]),
    properties: [JSONMember(key: "name", value: .string("shape"))],
    identifier: .uint(7)
  ))

  let arena = NativeInputArena()
  try arena.withNativeGeoJSON(geoJSON.nativeGeoJSON) { native in
    #expect(native.pointee.type == MLN_GEOJSON_TYPE_FEATURE.rawValue)
    #expect(native.pointee.data.feature!.pointee
      .identifier_type == MLN_FEATURE_IDENTIFIER_TYPE_UINT.rawValue)
    #expect(native.pointee.data.feature!.pointee.identifier.uint_value == 7)
    #expect(native.pointee.data.feature!.pointee.geometry.pointee
      .type == MLN_GEOMETRY_TYPE_POLYGON.rawValue)
    #expect(native.pointee.data.feature!.pointee.geometry.pointee.data.polygon
      .ring_count == 1)
    #expect(native.pointee.data.feature!.pointee.property_count == 1)
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

@Test func customGeometryCallbacksWaitForInFlightInvocationBeforeRelease(
) throws {
  final class CallbackHolder: @unchecked Sendable {
    private let lock = NSLock()
    private var callbacks: NativeCustomGeometrySourceCallbacks?

    init(_ callbacks: NativeCustomGeometrySourceCallbacks) {
      self.callbacks = callbacks
    }

    func withCallbacks<Result>(_ body: (
      NativeCustomGeometrySourceCallbacks
    ) throws
      -> Result) throws -> Result
    {
      lock.lock()
      let callbacks = try #require(callbacks)
      lock.unlock()
      return try body(callbacks)
    }

    func clear() {
      lock.lock()
      callbacks = nil
      lock.unlock()
    }
  }

  final class CallbackInvocation: @unchecked Sendable {
    private let fetchTile: mln_custom_geometry_source_tile_callback
    private let userDataAddress: UInt

    init(
      fetchTile: @escaping mln_custom_geometry_source_tile_callback,
      userData: UnsafeMutableRawPointer
    ) {
      self.fetchTile = fetchTile
      userDataAddress = UInt(bitPattern: userData)
    }

    func call() {
      fetchTile(
        UnsafeMutableRawPointer(bitPattern: userDataAddress),
        mln_canonical_tile_id(z: 1, x: 2, y: 3)
      )
    }
  }

  let entered = DispatchSemaphore(value: 0)
  let allowReturn = DispatchSemaphore(value: 0)
  let invocationFinished = DispatchSemaphore(value: 0)
  let releaseStarted = DispatchSemaphore(value: 0)
  let releaseFinished = DispatchSemaphore(value: 0)

  let holder =
    CallbackHolder(NativeCustomGeometrySourceCallbacks(fetchTile: { _ in
      entered.signal()
      allowReturn.wait()
    }))
  let invocation = try holder.withCallbacks { callbacks in
    try NativeCustomGeometrySourceOptions(callbacks: callbacks)
      .withNativeOptions { native in
        CallbackInvocation(
          fetchTile: native.pointee.fetch_tile!,
          userData: native.pointee.user_data!
        )
      }
  }

  Thread {
    invocation.call()
    invocationFinished.signal()
  }.start()
  #expect(entered.wait(timeout: .now() + .seconds(5)) == .success)

  Thread {
    releaseStarted.signal()
    holder.clear()
    releaseFinished.signal()
  }.start()
  #expect(releaseStarted.wait(timeout: .now() + .seconds(5)) == .success)
  #expect(releaseFinished
    .wait(timeout: .now() + .milliseconds(100)) == .timedOut)

  allowReturn.signal()
  #expect(invocationFinished.wait(timeout: .now() + .seconds(5)) == .success)
  #expect(releaseFinished.wait(timeout: .now() + .seconds(5)) == .success)
}

@Test func customGeometryCallbacksCanAbandonRetainedBoxForNativeLeakPath(
) throws {
  final class Counter: @unchecked Sendable {
    private let lock = NSLock()
    private var count = 0

    func increment() {
      lock.withLock {
        count += 1
      }
    }

    func value() -> Int {
      lock.withLock { count }
    }
  }

  let counter = Counter()
  var fetchTile: mln_custom_geometry_source_tile_callback?
  var userData: UnsafeMutableRawPointer?

  do {
    let callbacks = NativeCustomGeometrySourceCallbacks(fetchTile: { _ in
      counter.increment()
    })
    try NativeCustomGeometrySourceOptions(callbacks: callbacks)
      .withNativeOptions { native in
        fetchTile = native.pointee.fetch_tile
        userData = native.pointee.user_data
      }
    callbacks.abandonRetainedBox()
  }

  let abandonedUserData = try #require(userData)
  let abandonedFetchTile = try #require(fetchTile)
  defer {
    NativeCustomGeometrySourceCallbacks
      .releaseAbandonedRetainedBoxForTesting(abandonedUserData)
  }
  abandonedFetchTile(abandonedUserData, mln_canonical_tile_id(z: 1, x: 2, y: 3))

  #expect(counter.value() == 1)
}

@Test func staleStyleLoadedEventDoesNotReleaseCallbacksWhileOldSourceExists(
) throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  defer { try? map.close() }

  try map.setStyleJSON("""
  {"version":8,"sources":{},"layers":[]}
  """)
  try map.addCustomGeometrySource(
    sourceId: "custom",
    options: CustomGeometrySourceOptions(fetchTile: { _ in })
  )

  try map.setStyleURL("https://tiles.openfreemap.org/styles/bright")
  map.releaseCallbacksForLoadedStyleURLIfNeeded()

  #expect(map.retainsCustomGeometrySourceCallbacks(sourceId: "custom"))
}

@Test func loadedStyleDocumentAndURLReadBackWhatWasLoaded() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  defer { try? map.close() }

  // Nothing parsed and nothing requested yet.
  #expect(try map.loadedStyleJSON() == "")
  #expect(try map.styleURL() == "")

  // The document reads back byte-for-byte, so it can be reloaded unchanged.
  let styleJSON = #"{"version":8,"sources":{},"layers":[]}"#
  try map.setStyleJSON(styleJSON)
  #expect(try map.loadedStyleJSON() == styleJSON)
  // Inline JSON clears the URL.
  #expect(try map.styleURL() == "")

  // The URL is request state, recorded before the load can succeed, while the
  // document still reports the style that last parsed.
  try map.setStyleURL("https://example.com/style.json")
  #expect(try map.styleURL() == "https://example.com/style.json")
  #expect(try map.loadedStyleJSON() == styleJSON)
}

@Test func staleStyleLoadedEventReleasesOnlyCallbacksForMissingSources() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  defer { try? map.close() }

  try map.setStyleJSON("""
  {"version":8,"sources":{},"layers":[]}
  """)
  map.storeCustomGeometrySourceCallbacks(
    NativeCustomGeometrySourceCallbacks(fetchTile: { _ in }),
    sourceId: "missing"
  )

  try map.setStyleURL("https://tiles.openfreemap.org/styles/bright")
  map.releaseCallbacksForLoadedStyleURLIfNeeded()

  #expect(!map.retainsCustomGeometrySourceCallbacks(sourceId: "missing"))
}

@Test func staleStyleLoadedEventReleasesCallbacksForSourceTypeCollisions(
) throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  defer { try? map.close() }

  try map.setStyleJSON("""
  {"version":8,"sources":{"custom":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[]}
  """)
  map.storeCustomGeometrySourceCallbacks(
    NativeCustomGeometrySourceCallbacks(fetchTile: { _ in }),
    sourceId: "custom"
  )

  try map.setStyleURL("https://tiles.openfreemap.org/styles/bright")
  map.releaseCallbacksForLoadedStyleURLIfNeeded()

  #expect(!map.retainsCustomGeometrySourceCallbacks(sourceId: "custom"))
}

@Test func closedMapRejectsStyleCallsThroughSwiftHandleState() throws {
  let runtime = try RuntimeHandle()
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  try map.close()

  do {
    _ = try map.styleLayerIds()
    Issue.record("closed map should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func ninePatchStyleImageRoundTripsStretchContentAndTextFit() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  defer { try? map.close() }

  try map.setStyleJSON("""
  {"version":8,"sources":{},"layers":[]}
  """)

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

  let info = try #require(try map.styleImageInfo("patch"))
  #expect(info.stretchXCount == 1)
  #expect(info.stretchYCount == 2)
  #expect(info.content?.right == 1.5)
  // An absent text fit stays distinguishable from a present default.
  #expect(info.textFitWidth == nil)
  #expect(info.textFitHeight == .proportional)

  let stretches = try #require(try map.styleImageStretches("patch"))
  #expect(stretches.stretchX == [ImageStretch(from: 0, to: 1)])
  #expect(
    stretches.stretchY == [
      ImageStretch(from: 0, to: 1), ImageStretch(from: 1, to: 2),
    ]
  )
  #expect(try map.styleImageStretches("missing") == nil)

  // A backwards interval is rejected by C.
  #expect(throws: MaplibreError.self) {
    try map.setStyleImage(
      imageId: "bad",
      image: image,
      options: StyleImageOptions(stretchX: [ImageStretch(from: 2, to: 1)])
    )
  }
}

@Test func layerBaseAccessorsRoundTripThroughNativeMap() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  defer { try? map.close() }

  try map.setStyleJSON("""
  {"version":8,"sources":{"geo":{"type":"geojson","data":  {"type":"FeatureCollection","features":[]}}},  "layers":[{"id":"bg","type":"background"},  {"id":"fill","type":"fill","source":"geo"}]}
  """)

  #expect(try map.layerSourceLayer("fill") == "")
  try map.setLayerSourceLayer(layerId: "fill", sourceLayer: "roads")
  #expect(try map.layerSourceLayer("fill") == "roads")
  #expect(try map.layerSourceId("fill") == "geo")

  // A layer type that takes no source is rejected rather than silently ignored.
  #expect(throws: MaplibreError.self) {
    try map.setLayerSourceLayer(layerId: "bg", sourceLayer: "roads")
  }
  #expect(try map.layerSourceId("bg") == "")

  // An unset zoom range crosses the boundary as infinities.
  #expect(try map.layerMinZoom("fill") == -Double.infinity)
  #expect(try map.layerMaxZoom("fill") == Double.infinity)
  try map.setLayerMinZoom(layerId: "fill", minZoom: 4)
  try map.setLayerMaxZoom(layerId: "fill", maxZoom: 12.5)
  #expect(try map.layerMinZoom("fill") == 4)
  #expect(try map.layerMaxZoom("fill") == 12.5)

  #expect(try map.layerVisibility("fill") == .visible)
  try map.setLayerVisibility(layerId: "fill", visibility: .none)
  #expect(try map.layerVisibility("fill") == StyleLayerVisibility.none)

  // An unknown raw visibility passes through to C, which rejects it.
  #expect(throws: MaplibreError.self) {
    try map.setLayerVisibility(
      layerId: "fill",
      visibility: StyleLayerVisibility(rawValue: 900)
    )
  }
  #expect(throws: MaplibreError.self) {
    _ = try map.layerMinZoom("missing")
  }
}

@Test func geoJSONSourceOptionsMaterializeFieldMaskAndClusterProperties(
) throws {
  let options = StyleGeoJSONSourceOptions(
    minZoom: 1,
    maxZoom: 12,
    tolerance: 0.5,
    clusterMaxZoom: 15,
    clusterProperties: clusterPropertiesValue(),
    tileSize: 256,
    buffer: 64,
    clusterRadius: 60,
    clusterMinPoints: 3,
    lineMetrics: true,
    cluster: true,
    synchronousUpdate: true
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
      (fields & MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE.rawValue) != 0
    )
    #expect(native.pointee.synchronous_update)

    let clusterProperties = try #require(native.pointee.cluster_properties)
    let copied = try NativeJSONValue(copying: clusterProperties.pointee)
    #expect(JSONValue(native: copied) == clusterPropertiesValue())
  }

  // Absent options keep the descriptor out of the call.
  try StyleGeoJSONSourceOptions().nativeOptions.withNativeOptions { native in
    #expect(native == nil)
  }
}

@Test func clusteredGeoJSONSourceOptionsParseThroughNativeMap() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 512, height: 512)
  )
  defer { try? map.close() }

  try map.setStyleJSON("""
  {"version":8,"sources":{},"layers":[]}
  """)

  try map.addGeoJSONSourceData(
    sourceId: "clustered",
    data: nearbyPoints(),
    options: clusterOptions()
  )

  #expect(try map.styleSourceExists("clustered"))
  #expect(try map.styleSourceType("clustered") == .geoJSON)

  // The cluster aggregation graph is borrowed for the call and parsed by
  // MapLibre Native, so an unparseable expression fails the add.
  var invalid = clusterOptions()
  invalid.clusterProperties = .object([
    JSONMember(key: "weight_sum", value: .string("not-an-expression")),
  ])

  do {
    try map.addGeoJSONSourceData(
      sourceId: "clustered-invalid",
      data: nearbyPoints(),
      options: invalid
    )
    Issue.record("unparseable cluster properties should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  } catch {
    Issue.record("unexpected error: \(error)")
  }

  #expect(try !map.styleSourceExists("clustered-invalid"))
}

@Test func styleTransitionOptionsRoundTripThroughTheCAPI() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 1, height: 1)
  )
  defer { try? map.close() }

  // A map with no style yet reports no duration or delay. The placement flag
  // always reports, because MapLibre Native always holds a value for it.
  let empty = try map.styleTransitionOptions()
  #expect(empty.durationMilliseconds == nil)
  #expect(empty.delayMilliseconds == nil)
  #expect(empty.enablePlacementTransitions == true)

  // The style parser fills in its own 300ms duration for a style that declares
  // no transition.
  try map.setStyleJSON("""
  {"version":8,"sources":{},"layers":[]}
  """)
  let parsed = try map.styleTransitionOptions()
  #expect(parsed.durationMilliseconds == 300)
  #expect(parsed.delayMilliseconds == nil)

  try map.setStyleJSON(transitionStyleJSON)
  let declared = try map.styleTransitionOptions()
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
  #expect(try map.styleTransitionOptions() == options)

  // Omitting the flag leaves the cross-fade on rather than clearing it.
  try map.setStyleTransitionOptions(
    StyleTransitionOptions(durationMilliseconds: 250)
  )
  #expect(try map.styleTransitionOptions().enablePlacementTransitions == true)

  // Loading a style replaces the override with what that style declares.
  try map.setStyleJSON(transitionStyleJSON)
  #expect(try map.styleTransitionOptions() == declared)

  do {
    try map.setStyleTransitionOptions(
      StyleTransitionOptions(delayMilliseconds: -1)
    )
    Issue.record("a negative delay should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

private let transitionStyleJSON = """
{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}
"""

private func clusterPropertiesValue() -> JSONValue {
  .object([
    JSONMember(
      key: "weight_sum",
      value: .array([.string("+"), .array([.string("get"), .string("weight")])])
    ),
  ])
}

private func clusterOptions() -> StyleGeoJSONSourceOptions {
  StyleGeoJSONSourceOptions(
    clusterMaxZoom: 17,
    clusterProperties: clusterPropertiesValue(),
    clusterRadius: 60,
    clusterMinPoints: 2,
    cluster: true
  )
}

private func nearbyPoints() -> GeoJSON {
  .featureCollection((0 ..< 3).map { index in
    let offset = Double(index) * 0.001
    return Feature(
      geometry: .point(LatLng(latitude: offset, longitude: offset)),
      properties: [JSONMember(key: "weight", value: .uint(UInt64(index + 1)))]
    )
  })
}
