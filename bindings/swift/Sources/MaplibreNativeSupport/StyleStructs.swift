import CMaplibreNativeC

public struct NativeLatLngBounds: Equatable, Sendable {
  public let southwest: NativeLatLng
  public let northeast: NativeLatLng

  public init(southwest: NativeLatLng, northeast: NativeLatLng) {
    self.southwest = southwest
    self.northeast = northeast
  }

  public init(_ raw: mln_lat_lng_bounds) {
    southwest = NativeLatLng(raw.southwest)
    northeast = NativeLatLng(raw.northeast)
  }

  public var native: mln_lat_lng_bounds {
    mln_lat_lng_bounds(southwest: southwest.native, northeast: northeast.native)
  }
}

public struct NativeStyleTileSourceOptions: Equatable, Sendable {
  public var minZoom: Double?
  public var maxZoom: Double?
  public var attribution: String?
  public var scheme: UInt32?
  public var bounds: NativeLatLngBounds?
  public var tileSize: UInt32?
  public var vectorEncoding: UInt32?
  public var rasterEncoding: UInt32?

  public init(
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

  public func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_style_tile_source_options>?) throws -> Result
  ) throws -> Result {
    if minZoom == nil, maxZoom == nil, attribution == nil, scheme == nil, bounds == nil, tileSize == nil,
       vectorEncoding == nil, rasterEncoding == nil
    {
      return try body(nil)
    }
    let arena = NativeJSONArena()
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

public struct NativeCanonicalTileID: Equatable, Sendable {
  public let z: UInt32
  public let x: UInt32
  public let y: UInt32

  public init(z: UInt32, x: UInt32, y: UInt32) {
    self.z = z
    self.x = x
    self.y = y
  }

  public init(_ raw: mln_canonical_tile_id) {
    z = raw.z
    x = raw.x
    y = raw.y
  }

  public var native: mln_canonical_tile_id {
    mln_canonical_tile_id(z: z, x: x, y: y)
  }
}

public struct NativePremultipliedRGBA8Image: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let stride: UInt32
  public let pixels: [UInt8]

  public init(width: UInt32, height: UInt32, stride: UInt32, pixels: [UInt8]) {
    self.width = width
    self.height = height
    self.stride = stride
    self.pixels = pixels
  }

  public func withNativeImage<Result>(
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

public struct NativeStyleImageOptions: Equatable, Sendable {
  public let pixelRatio: Float?
  public let sdf: Bool?

  public init(pixelRatio: Float? = nil, sdf: Bool? = nil) {
    self.pixelRatio = pixelRatio
    self.sdf = sdf
  }

  public func withNativeOptions<Result>(
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

public struct NativeStyleImageInfo: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let stride: UInt32
  public let byteLength: Int
  public let pixelRatio: Float
  public let sdf: Bool

  public init(_ raw: mln_style_image_info) {
    width = raw.width
    height = raw.height
    stride = raw.stride
    byteLength = raw.byte_length
    pixelRatio = raw.pixel_ratio
    sdf = raw.sdf
  }
}

public struct NativeStyleSourceInfo: Equatable, Sendable {
  public let type: UInt32
  public let idSize: Int
  public let isVolatile: Bool
  public let attributionSize: Int

  public init(_ raw: mln_style_source_info) {
    type = raw.type
    idSize = raw.id_size
    isVolatile = raw.is_volatile
    attributionSize = raw.has_attribution ? raw.attribution_size : 0
  }
}
