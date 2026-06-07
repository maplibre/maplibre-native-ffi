internal import CMaplibreNativeC
import Foundation

struct NativeLatLngBounds: Equatable, Sendable {
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

struct NativeStyleTileSourceOptions: Equatable, Sendable {
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
    if minZoom == nil, maxZoom == nil, attribution == nil, scheme == nil, bounds == nil, tileSize == nil,
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
    return try withUnsafePointer(to: &options, body)
  }
}

struct NativeCanonicalTileID: Equatable, Sendable {
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

struct NativePremultipliedRGBA8Image: Equatable, Sendable {
  let width: UInt32
  let height: UInt32
  let stride: UInt32
  let pixels: [UInt8]

  init(width: UInt32, height: UInt32, stride: UInt32, pixels: [UInt8]) {
    self.width = width
    self.height = height
    self.stride = stride
    self.pixels = pixels
  }

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

struct NativeStyleImageOptions: Equatable, Sendable {
  let pixelRatio: Float?
  let sdf: Bool?

  init(pixelRatio: Float? = nil, sdf: Bool? = nil) {
    self.pixelRatio = pixelRatio
    self.sdf = sdf
  }

  func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_style_image_options>) throws -> Result
  ) throws -> Result {
    var options = mln_style_image_options_default()
    if let pixelRatio {
      options.fields |= MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO.rawValue
      options.pixel_ratio = pixelRatio
    }
    if let sdf {
      options.fields |= MLN_STYLE_IMAGE_OPTION_SDF.rawValue
      options.sdf = sdf
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

final class NativeCustomGeometrySourceCallbacks: @unchecked Sendable {
  typealias TileCallback = @Sendable (NativeCanonicalTileID) -> Void

  private var retainedBox: Unmanaged<NativeCustomGeometrySourceCallbackBox>?

  init(fetchTile: @escaping TileCallback, cancelTile: TileCallback? = nil) {
    retainedBox = Unmanaged.passRetained(
      NativeCustomGeometrySourceCallbackBox(fetchTile: fetchTile, cancelTile: cancelTile)
    )
  }

  deinit {
    guard let retainedBox else { return }
    retainedBox.takeUnretainedValue().retireAndWait()
    retainedBox.release()
  }

  var unmanagedPointer: UnsafeMutableRawPointer {
    retainedBox!.toOpaque()
  }

  func abandonRetainedBox() {
    retainedBox = nil
  }

  static func releaseAbandonedRetainedBoxForTesting(_ pointer: UnsafeMutableRawPointer) {
    let retainedBox = Unmanaged<NativeCustomGeometrySourceCallbackBox>.fromOpaque(pointer)
    retainedBox.takeUnretainedValue().retireAndWait()
    retainedBox.release()
  }
}

private func customGeometryFetchTileCallback(_ userData: UnsafeMutableRawPointer?, _ tileId: mln_canonical_tile_id) {
  guard let userData else { return }
  Unmanaged<NativeCustomGeometrySourceCallbackBox>.fromOpaque(userData).takeUnretainedValue().fetched(tileId)
}

private func customGeometryCancelTileCallback(_ userData: UnsafeMutableRawPointer?, _ tileId: mln_canonical_tile_id) {
  guard let userData else { return }
  Unmanaged<NativeCustomGeometrySourceCallbackBox>.fromOpaque(userData).takeUnretainedValue().cancelled(tileId)
}

struct NativeCustomGeometrySourceOptions: Sendable {
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

struct NativeStyleImageInfo: Equatable, Sendable {
  let width: UInt32
  let height: UInt32
  let stride: UInt32
  let byteLength: Int
  let pixelRatio: Float
  let sdf: Bool

  init(_ raw: mln_style_image_info) {
    width = raw.width
    height = raw.height
    stride = raw.stride
    byteLength = raw.byte_length
    pixelRatio = raw.pixel_ratio
    sdf = raw.sdf
  }
}

struct NativeStyleSourceInfo: Equatable, Sendable {
  let type: UInt32
  let idSize: Int
  let isVolatile: Bool
  let hasAttribution: Bool
  let attributionSize: Int

  init(_ raw: mln_style_source_info) {
    type = raw.type
    idSize = raw.id_size
    isVolatile = raw.is_volatile
    hasAttribution = raw.has_attribution
    attributionSize = raw.attribution_size
  }
}
