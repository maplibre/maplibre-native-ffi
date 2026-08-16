internal import CMaplibreNativeC
import Foundation

struct NativeLatLngBounds: Equatable {
  let southwest: NativeLatLng
  let northeast: NativeLatLng

  init(southwest: NativeLatLng, northeast: NativeLatLng) {
    self.southwest = southwest
    self.northeast = northeast
  }

  init(_ raw: mln_lat_lng_bounds) {
    southwest = NativeLatLng(raw.southwest)
    northeast = NativeLatLng(raw.northeast)
  }

  var native: mln_lat_lng_bounds {
    mln_lat_lng_bounds(southwest: southwest.native, northeast: northeast.native)
  }
}

struct NativeStyleTileSourceOptions: Equatable {
  var minZoom: Double?
  var maxZoom: Double?
  var attribution: String?
  var scheme: UInt32?
  var bounds: NativeLatLngBounds?
  var tileSize: UInt32?
  var vectorEncoding: UInt32?
  var rasterEncoding: UInt32?

  init(
    minZoom: Double? = nil,
    maxZoom: Double? = nil,
    attribution: String? = nil,
    scheme: UInt32? = nil,
    bounds: NativeLatLngBounds? = nil,
    tileSize: UInt32? = nil,
    vectorEncoding: UInt32? = nil,
    rasterEncoding: UInt32? = nil
  ) {
    self.minZoom = minZoom
    self.maxZoom = maxZoom
    self.attribution = attribution
    self.scheme = scheme
    self.bounds = bounds
    self.tileSize = tileSize
    self.vectorEncoding = vectorEncoding
    self.rasterEncoding = rasterEncoding
  }

  func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_style_tile_source_options>?) throws -> Result
  ) throws -> Result {
    if minZoom == nil, maxZoom == nil, attribution == nil, scheme == nil,
       bounds == nil, tileSize == nil,
       vectorEncoding == nil, rasterEncoding == nil
    {
      return try body(nil)
    }
    let arena = NativeInputArena()
    var options = mln_style_tile_source_options_default()
    if let minZoom {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM.rawValue
      options.min_zoom = minZoom
    }
    if let maxZoom {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM.rawValue
      options.max_zoom = maxZoom
    }
    if let attribution {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION.rawValue
      options.attribution = arena.view(attribution)
    }
    if let scheme {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_SCHEME.rawValue
      options.scheme = scheme
    }
    if let bounds {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS.rawValue
      options.bounds = bounds.native
    }
    if let tileSize {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE.rawValue
      options.tile_size = tileSize
    }
    if let vectorEncoding {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING.rawValue
      options.vector_encoding = vectorEncoding
    }
    if let rasterEncoding {
      options.fields |= MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING.rawValue
      options.raster_encoding = rasterEncoding
    }
    return try withUnsafePointer(to: &options) { options in
      try withExtendedLifetime(arena) { try body(options) }
    }
  }
}

struct NativeGeoJSONSourceOptions: Equatable {
  var minZoom: Double?
  var maxZoom: Double?
  var tolerance: Double?
  var clusterMaxZoom: Double?
  var clusterProperties: Data?
  var tileSize: UInt32?
  var buffer: UInt32?
  var clusterRadius: UInt32?
  var clusterMinPoints: UInt32?
  var lineMetrics: Bool?
  var cluster: Bool?
  var synchronousTiling: Bool?

  init(
    minZoom: Double? = nil,
    maxZoom: Double? = nil,
    tolerance: Double? = nil,
    clusterMaxZoom: Double? = nil,
    clusterProperties: Data? = nil,
    tileSize: UInt32? = nil,
    buffer: UInt32? = nil,
    clusterRadius: UInt32? = nil,
    clusterMinPoints: UInt32? = nil,
    lineMetrics: Bool? = nil,
    cluster: Bool? = nil,
    synchronousTiling: Bool? = nil
  ) {
    self.minZoom = minZoom
    self.maxZoom = maxZoom
    self.tolerance = tolerance
    self.clusterMaxZoom = clusterMaxZoom
    self.clusterProperties = clusterProperties
    self.tileSize = tileSize
    self.buffer = buffer
    self.clusterRadius = clusterRadius
    self.clusterMinPoints = clusterMinPoints
    self.lineMetrics = lineMetrics
    self.cluster = cluster
    self.synchronousTiling = synchronousTiling
  }

  func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_geojson_source_options>?) throws -> Result
  ) throws -> Result {
    if minZoom == nil, maxZoom == nil, tolerance == nil, clusterMaxZoom == nil,
       clusterProperties == nil, tileSize == nil, buffer == nil,
       clusterRadius == nil, clusterMinPoints == nil, lineMetrics == nil,
       cluster == nil, synchronousTiling == nil
    {
      return try body(nil)
    }
    let arena = NativeInputArena()
    var options = mln_geojson_source_options_default()
    if let minZoom {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM.rawValue
      options.min_zoom = minZoom
    }
    if let maxZoom {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM.rawValue
      options.max_zoom = maxZoom
    }
    if let tolerance {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_TOLERANCE.rawValue
      options.tolerance = tolerance
    }
    if let clusterMaxZoom {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM.rawValue
      options.cluster_max_zoom = clusterMaxZoom
    }
    if let clusterProperties {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES.rawValue
      options.cluster_properties = arena.view(clusterProperties)
    }
    if let tileSize {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE.rawValue
      options.tile_size = tileSize
    }
    if let buffer {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_BUFFER.rawValue
      options.buffer = buffer
    }
    if let clusterRadius {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS.rawValue
      options.cluster_radius = clusterRadius
    }
    if let clusterMinPoints {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS.rawValue
      options.cluster_min_points = clusterMinPoints
    }
    if let lineMetrics {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS.rawValue
      options.line_metrics = lineMetrics
    }
    if let cluster {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER.rawValue
      options.cluster = cluster
    }
    if let synchronousTiling {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING.rawValue
      options.synchronous_tiling = synchronousTiling
    }
    return try withUnsafePointer(to: &options) { options in
      try withExtendedLifetime(arena) { try body(options) }
    }
  }
}

struct NativeCanonicalTileID: Equatable {
  let z: UInt32
  let x: UInt32
  let y: UInt32

  init(z: UInt32, x: UInt32, y: UInt32) {
    self.z = z
    self.x = x
    self.y = y
  }

  init(_ raw: mln_canonical_tile_id) {
    z = raw.z
    x = raw.x
    y = raw.y
  }

  var native: mln_canonical_tile_id {
    mln_canonical_tile_id(z: z, x: x, y: y)
  }
}

struct NativePremultipliedRGBA8Image: Equatable {
  let width: UInt32
  let height: UInt32
  let stride: UInt32
  let pixels: [UInt8]

  func withNativeImage<Result>(
    _ body: (UnsafePointer<mln_premultiplied_rgba8_image>) throws -> Result
  ) throws -> Result {
    try pixels.withUnsafeBufferPointer { pixels in
      var image = mln_premultiplied_rgba8_image_default()
      image.width = width
      image.height = height
      image.stride = stride
      image.pixels = pixels.baseAddress
      image.byte_length = pixels.count
      return try withUnsafePointer(to: &image, body)
    }
  }
}

struct NativeStyleImageOptions: Equatable {
  let pixelRatio: Float?
  let sdf: Bool?
  let stretchX: [ImageStretch]?
  let stretchY: [ImageStretch]?
  let content: ImageContent?
  let textFitWidth: StyleImageTextFit?
  let textFitHeight: StyleImageTextFit?

  init(
    pixelRatio: Float? = nil,
    sdf: Bool? = nil,
    stretchX: [ImageStretch]? = nil,
    stretchY: [ImageStretch]? = nil,
    content: ImageContent? = nil,
    textFitWidth: StyleImageTextFit? = nil,
    textFitHeight: StyleImageTextFit? = nil
  ) {
    self.pixelRatio = pixelRatio
    self.sdf = sdf
    self.stretchX = stretchX
    self.stretchY = stretchY
    self.content = content
    self.textFitWidth = textFitWidth
    self.textFitHeight = textFitHeight
  }

  /// Materializes the options with the stretch arrays kept alive for the call,
  /// which native borrows rather than copies until it returns.
  func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_style_image_options>) throws -> Result
  ) throws -> Result {
    var nativeStretchX = (stretchX ?? []).map {
      mln_image_stretch(from: $0.from, to: $0.to)
    }
    var nativeStretchY = (stretchY ?? []).map {
      mln_image_stretch(from: $0.from, to: $0.to)
    }
    return try nativeStretchX.withUnsafeMutableBufferPointer { bufferX in
      try nativeStretchY.withUnsafeMutableBufferPointer { bufferY in
        var options = mln_style_image_options_default()
        if let pixelRatio {
          options.fields |= MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO.rawValue
          options.pixel_ratio = pixelRatio
        }
        if let sdf {
          options.fields |= MLN_STYLE_IMAGE_OPTION_SDF.rawValue
          options.sdf = sdf
        }
        if let stretchX {
          options.fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_X.rawValue
          options.stretch_x = UnsafePointer(bufferX.baseAddress)
          options.stretch_x_count = stretchX.count
        }
        if let stretchY {
          options.fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_Y.rawValue
          options.stretch_y = UnsafePointer(bufferY.baseAddress)
          options.stretch_y_count = stretchY.count
        }
        if let content {
          options.fields |= MLN_STYLE_IMAGE_OPTION_CONTENT.rawValue
          options.content = mln_image_content(
            left: content.left,
            top: content.top,
            right: content.right,
            bottom: content.bottom
          )
        }
        if let textFitWidth {
          options.fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH.rawValue
          options.text_fit_width = textFitWidth.rawValue
        }
        if let textFitHeight {
          options.fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT.rawValue
          options.text_fit_height = textFitHeight.rawValue
        }
        return try withUnsafePointer(to: &options, body)
      }
    }
  }
}

struct NativeStyleTransitionOptions: Equatable {
  let durationMilliseconds: Double?
  let delayMilliseconds: Double?
  let enablePlacementTransitions: Bool?

  init(
    durationMilliseconds: Double? = nil,
    delayMilliseconds: Double? = nil,
    enablePlacementTransitions: Bool? = nil
  ) {
    self.durationMilliseconds = durationMilliseconds
    self.delayMilliseconds = delayMilliseconds
    self.enablePlacementTransitions = enablePlacementTransitions
  }

  init(_ raw: mln_style_transition_options) {
    durationMilliseconds = raw.fields
      & MLN_STYLE_TRANSITION_OPTION_DURATION.rawValue != 0 ? raw
      .duration_ms : nil
    delayMilliseconds = raw.fields
      & MLN_STYLE_TRANSITION_OPTION_DELAY.rawValue != 0 ? raw.delay_ms : nil
    enablePlacementTransitions = raw.fields
      & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
      .rawValue != 0 ? raw.enable_placement_transitions : nil
  }

  func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_style_transition_options>) throws -> Result
  ) rethrows -> Result {
    var options = mln_style_transition_options_default()
    if let enablePlacementTransitions {
      options.fields |= MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
        .rawValue
      options.enable_placement_transitions = enablePlacementTransitions
    }
    if let durationMilliseconds {
      options.fields |= MLN_STYLE_TRANSITION_OPTION_DURATION.rawValue
      options.duration_ms = durationMilliseconds
    }
    if let delayMilliseconds {
      options.fields |= MLN_STYLE_TRANSITION_OPTION_DELAY.rawValue
      options.delay_ms = delayMilliseconds
    }
    return try withUnsafePointer(to: &options, body)
  }
}

private final class NativeCustomGeometrySourceCallbackBox: @unchecked Sendable {
  typealias TileCallback = @Sendable (NativeCanonicalTileID) -> Void

  private let fetchTile: TileCallback
  private let cancelTile: TileCallback?
  private let condition = NSCondition()
  private var activeUpcalls = 0
  private var retired = false

  init(fetchTile: @escaping TileCallback, cancelTile: TileCallback? = nil) {
    self.fetchTile = fetchTile
    self.cancelTile = cancelTile
  }

  func fetched(_ tileId: mln_canonical_tile_id) {
    guard beginUpcall() else { return }
    defer { endUpcall() }
    fetchTile(NativeCanonicalTileID(tileId))
  }

  func cancelled(_ tileId: mln_canonical_tile_id) {
    guard beginUpcall() else { return }
    defer { endUpcall() }
    cancelTile?(NativeCanonicalTileID(tileId))
  }

  func retireAndWait() {
    condition.lock()
    retired = true
    while activeUpcalls > 0 {
      condition.wait()
    }
    condition.unlock()
  }

  private func beginUpcall() -> Bool {
    condition.lock()
    defer { condition.unlock() }
    guard !retired else { return false }
    activeUpcalls += 1
    return true
  }

  private func endUpcall() {
    condition.lock()
    activeUpcalls -= 1
    if activeUpcalls == 0 {
      condition.broadcast()
    }
    condition.unlock()
  }
}

/// A retain on one custom geometry source's tile callbacks, handed to the C
/// API as `user_data`.
///
/// The C API owns the retain once it accepts the source, and gives it back by
/// invoking the release callback exactly once when it stops referencing the
/// pointer. A rejected add is the one case that never releases, so the caller
/// releases it with ``release()`` there.
struct NativeCustomGeometrySourceCallbacks: @unchecked Sendable {
  typealias TileCallback = @Sendable (NativeCanonicalTileID) -> Void

  let unmanagedPointer: UnsafeMutableRawPointer

  init(fetchTile: @escaping TileCallback, cancelTile: TileCallback? = nil) {
    unmanagedPointer = Unmanaged.passRetained(
      NativeCustomGeometrySourceCallbackBox(
        fetchTile: fetchTile,
        cancelTile: cancelTile
      )
    ).toOpaque()
  }

  func release() {
    releaseCustomGeometryCallbacks(unmanagedPointer)
  }
}

/// Retires the callbacks behind `userData` and drops the retain the C API held.
///
/// Retiring waits for the tile callbacks still running on native worker
/// threads, so the host's closures are never entered after this returns.
private func releaseCustomGeometryCallbacks(
  _ userData: UnsafeMutableRawPointer
) {
  let box = Unmanaged<NativeCustomGeometrySourceCallbackBox>
    .fromOpaque(userData)
  box.takeUnretainedValue().retireAndWait()
  box.release()
}

private func customGeometryReleaseUserDataCallback(
  _ userData: UnsafeMutableRawPointer?
) {
  guard let userData else { return }
  releaseCustomGeometryCallbacks(userData)
}

private func customGeometryFetchTileCallback(
  _ userData: UnsafeMutableRawPointer?,
  _ tileId: mln_canonical_tile_id
) {
  guard let userData else { return }
  Unmanaged<NativeCustomGeometrySourceCallbackBox>.fromOpaque(userData)
    .takeUnretainedValue().fetched(tileId)
}

private func customGeometryCancelTileCallback(
  _ userData: UnsafeMutableRawPointer?,
  _ tileId: mln_canonical_tile_id
) {
  guard let userData else { return }
  Unmanaged<NativeCustomGeometrySourceCallbackBox>.fromOpaque(userData)
    .takeUnretainedValue().cancelled(tileId)
}

struct NativeCustomGeometrySourceOptions {
  let callbacks: NativeCustomGeometrySourceCallbacks
  var minZoom: Double?
  var maxZoom: Double?
  var tolerance: Double?
  var tileSize: UInt32?
  var buffer: UInt32?
  var clip: Bool?
  var wrap: Bool?

  init(
    callbacks: NativeCustomGeometrySourceCallbacks,
    minZoom: Double? = nil,
    maxZoom: Double? = nil,
    tolerance: Double? = nil,
    tileSize: UInt32? = nil,
    buffer: UInt32? = nil,
    clip: Bool? = nil,
    wrap: Bool? = nil
  ) {
    self.callbacks = callbacks
    self.minZoom = minZoom
    self.maxZoom = maxZoom
    self.tolerance = tolerance
    self.tileSize = tileSize
    self.buffer = buffer
    self.clip = clip
    self.wrap = wrap
  }

  func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_custom_geometry_source_options>) throws -> Result
  ) throws -> Result {
    var options = mln_custom_geometry_source_options_default()
    options.fetch_tile = customGeometryFetchTileCallback
    options.cancel_tile = customGeometryCancelTileCallback
    options.user_data = callbacks.unmanagedPointer
    options.release_user_data = customGeometryReleaseUserDataCallback
    if let minZoom {
      options.fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM.rawValue
      options.min_zoom = minZoom
    }
    if let maxZoom {
      options.fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM.rawValue
      options.max_zoom = maxZoom
    }
    if let tolerance {
      options.fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE.rawValue
      options.tolerance = tolerance
    }
    if let tileSize {
      options.fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE.rawValue
      options.tile_size = tileSize
    }
    if let buffer {
      options.fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER.rawValue
      options.buffer = buffer
    }
    if let clip {
      options.fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP.rawValue
      options.clip = clip
    }
    if let wrap {
      options.fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP.rawValue
      options.wrap = wrap
    }
    return try withUnsafePointer(to: &options, body)
  }
}

struct NativeStyleImageInfo: Equatable {
  let width: UInt32
  let height: UInt32
  let stride: UInt32
  let byteLength: Int
  let pixelRatio: Float
  let sdf: Bool
  let stretchXCount: Int
  let stretchYCount: Int
  let content: ImageContent?
  let textFitWidth: StyleImageTextFit?
  let textFitHeight: StyleImageTextFit?

  init(_ raw: mln_style_image_info) {
    width = raw.width
    height = raw.height
    stride = raw.stride
    byteLength = raw.byte_length
    pixelRatio = raw.pixel_ratio
    sdf = raw.sdf
    stretchXCount = raw.stretch_x_count
    stretchYCount = raw.stretch_y_count
    content = raw.has_content
      ? ImageContent(
        left: raw.content.left,
        top: raw.content.top,
        right: raw.content.right,
        bottom: raw.content.bottom
      )
      : nil
    textFitWidth = raw.has_text_fit_width
      ? StyleImageTextFit(rawValue: raw.text_fit_width)
      : nil
    textFitHeight = raw.has_text_fit_height
      ? StyleImageTextFit(rawValue: raw.text_fit_height)
      : nil
  }
}

struct NativeStyleSourceTileJSON: Equatable {
  let tileURLs: [String]
  let minZoom: Double
  let maxZoom: Double
  let scheme: UInt32
  let bounds: NativeLatLngBounds?
}

struct NativeStyleSourceInfo: Equatable {
  let type: UInt32
  let isVolatile: Bool
  let attribution: String?
  let url: String?
  let tileJSON: NativeStyleSourceTileJSON?
  let tileSize: UInt32?
  let vectorEncoding: UInt32?
  let rasterEncoding: UInt32?
}
