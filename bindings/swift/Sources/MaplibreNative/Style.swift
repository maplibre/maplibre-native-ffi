import MaplibreNativeSupport

public enum StyleSourceType: UInt32, Sendable, Hashable {
  case unknown = 0
  case vector = 1
  case raster = 2
  case rasterDEM = 3
  case geoJSON = 4
  case image = 5
  case video = 6
  case annotations = 7
  case customVector = 8
}

public enum StyleTileScheme: UInt32, Sendable, Hashable {
  case xyz = 0
  case tms = 1
}

public enum StyleVectorTileEncoding: UInt32, Sendable, Hashable {
  case mvt = 0
  case mlt = 1
}

public enum StyleRasterDEMEncoding: UInt32, Sendable, Hashable {
  case mapbox = 0
  case terrarium = 1
}

public struct StyleTileSourceOptions: Equatable, Sendable {
  public var minZoom: Double?
  public var maxZoom: Double?
  public var attribution: String?
  public var scheme: StyleTileScheme?
  public var bounds: LatLngBounds?
  public var tileSize: UInt32?
  public var vectorEncoding: StyleVectorTileEncoding?
  public var rasterEncoding: StyleRasterDEMEncoding?

  public init(
    minZoom: Double? = nil,
    maxZoom: Double? = nil,
    attribution: String? = nil,
    scheme: StyleTileScheme? = nil,
    bounds: LatLngBounds? = nil,
    tileSize: UInt32? = nil,
    vectorEncoding: StyleVectorTileEncoding? = nil,
    rasterEncoding: StyleRasterDEMEncoding? = nil
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

  var nativeOptions: NativeStyleTileSourceOptions {
    NativeStyleTileSourceOptions(
      minZoom: minZoom,
      maxZoom: maxZoom,
      attribution: attribution,
      scheme: scheme?.rawValue,
      bounds: bounds?.nativeInput,
      tileSize: tileSize,
      vectorEncoding: vectorEncoding?.rawValue,
      rasterEncoding: rasterEncoding?.rawValue
    )
  }
}

public struct StyleRGBA8Image: Equatable, Sendable {
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

  var nativeImage: NativePremultipliedRGBA8Image {
    NativePremultipliedRGBA8Image(width: width, height: height, stride: stride, pixels: pixels)
  }
}

public struct StyleImageOptions: Equatable, Sendable {
  public var pixelRatio: Float?
  public var sdf: Bool?

  public init(pixelRatio: Float? = nil, sdf: Bool? = nil) {
    self.pixelRatio = pixelRatio
    self.sdf = sdf
  }

  var nativeOptions: NativeStyleImageOptions {
    NativeStyleImageOptions(pixelRatio: pixelRatio, sdf: sdf)
  }
}

public struct StyleImageInfo: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let stride: UInt32
  public let byteLength: Int
  public let pixelRatio: Float
  public let sdf: Bool

  init(native: NativeStyleImageInfo) {
    width = native.width
    height = native.height
    stride = native.stride
    byteLength = native.byteLength
    pixelRatio = native.pixelRatio
    sdf = native.sdf
  }
}

public struct StyleImage: Equatable, Sendable {
  public let info: StyleImageInfo
  public let pixels: [UInt8]
}

public struct StyleSourceInfo: Equatable, Sendable {
  public let type: StyleSourceType
  public let idSize: Int
  public let isVolatile: Bool
  public let attributionSize: Int

  init(native: NativeStyleSourceInfo) {
    type = StyleSourceType(rawValue: native.type) ?? .unknown
    idSize = native.idSize
    isVolatile = native.isVolatile
    attributionSize = native.attributionSize
  }
}

public enum LocationIndicatorImageKind: UInt32, Sendable, Hashable {
  case top = 0
  case bearing = 1
  case shadow = 2
}

public struct CanonicalTileID: Equatable, Sendable {
  public let z: UInt32
  public let x: UInt32
  public let y: UInt32

  public init(z: UInt32, x: UInt32, y: UInt32) {
    self.z = z
    self.x = x
    self.y = y
  }

  init(native: NativeCanonicalTileID) {
    z = native.z
    x = native.x
    y = native.y
  }

  var nativeTileID: NativeCanonicalTileID {
    NativeCanonicalTileID(z: z, x: x, y: y)
  }
}

public struct CustomGeometrySourceOptions: Sendable {
  public typealias TileCallback = @Sendable (CanonicalTileID) -> Void

  public var fetchTile: TileCallback
  public var cancelTile: TileCallback?
  public var minZoom: Double?
  public var maxZoom: Double?
  public var tolerance: Double?
  public var tileSize: UInt32?
  public var buffer: UInt32?
  public var clip: Bool?
  public var wrap: Bool?

  public init(
    fetchTile: @escaping TileCallback,
    cancelTile: TileCallback? = nil,
    minZoom: Double? = nil,
    maxZoom: Double? = nil,
    tolerance: Double? = nil,
    tileSize: UInt32? = nil,
    buffer: UInt32? = nil,
    clip: Bool? = nil,
    wrap: Bool? = nil
  ) {
    self.fetchTile = fetchTile
    self.cancelTile = cancelTile
    self.minZoom = minZoom
    self.maxZoom = maxZoom
    self.tolerance = tolerance
    self.tileSize = tileSize
    self.buffer = buffer
    self.clip = clip
    self.wrap = wrap
  }

  func nativeOptions(callbacks: NativeCustomGeometrySourceCallbacks) -> NativeCustomGeometrySourceOptions {
    NativeCustomGeometrySourceOptions(
      callbacks: callbacks,
      minZoom: minZoom,
      maxZoom: maxZoom,
      tolerance: tolerance,
      tileSize: tileSize,
      buffer: buffer,
      clip: clip,
      wrap: wrap
    )
  }
}

extension MapHandle {
  public func addStyleSourceJSON(sourceId: String, sourceJSON: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapAddStyleSourceJSON(
        try requireLivePointer(),
        sourceId: arena.view(sourceId),
        sourceJSON: arena.allocate(sourceJSON.nativeValue)
      )
    }
  }

  @discardableResult public func removeStyleSource(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      let removed = try CAPI.mapRemoveStyleSource(try requireLivePointer(), sourceId: arena.view(sourceId))
      if removed { customGeometrySourceCallbacks.removeValue(forKey: sourceId) }
      return removed
    }
  }

  public func styleSourceExists(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapStyleSourceExists(try requireLivePointer(), sourceId: arena.view(sourceId))
    }
  }

  public func styleSourceType(_ sourceId: String) throws -> StyleSourceType? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetStyleSourceType(try requireLivePointer(), sourceId: arena.view(sourceId)).map { StyleSourceType(rawValue: $0) ?? .unknown }
    }
  }

  public func styleSourceInfo(_ sourceId: String) throws -> StyleSourceInfo? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetStyleSourceInfo(try requireLivePointer(), sourceId: arena.view(sourceId)).map(StyleSourceInfo.init(native:))
    }
  }

  public func styleSourceAttribution(_ sourceId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      let sourceIdView = arena.view(sourceId)
      guard let info = try CAPI.mapGetStyleSourceInfo(try requireLivePointer(), sourceId: sourceIdView) else { return nil }
      return try CAPI.mapCopyStyleSourceAttribution(try requireLivePointer(), sourceId: sourceIdView, capacity: info.attributionSize).0
    }
  }

  public func styleSourceIds() throws -> [String] {
    try mapNativeFailure { try CAPI.mapListStyleSourceIds(try requireLivePointer()) }
  }

  public func addGeoJSONSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapAddGeoJSONSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), url: arena.view(url))
    }
  }

  public func addGeoJSONSourceData(sourceId: String, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try CAPI.mapAddGeoJSONSourceData(try requireLivePointer(), sourceId: arena.view(sourceId), data: data)
      }
    }
  }

  public func setGeoJSONSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetGeoJSONSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), url: arena.view(url))
    }
  }

  public func setGeoJSONSourceData(sourceId: String, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try CAPI.mapSetGeoJSONSourceData(try requireLivePointer(), sourceId: arena.view(sourceId), data: data)
      }
    }
  }

  public func addVectorSourceURL(sourceId: String, url: String, options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try options.nativeOptions.withNativeOptions { options in
        try CAPI.mapAddVectorSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), url: arena.view(url), options: options)
      }
    }
  }

  public func addVectorSourceTiles(sourceId: String, tiles: [String], options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      let tileViews = tiles.map { arena.view($0) }
      try tileViews.withUnsafeBufferPointer { tiles in
        try options.nativeOptions.withNativeOptions { options in
          try CAPI.mapAddVectorSourceTiles(try requireLivePointer(), sourceId: arena.view(sourceId), tiles: tiles.baseAddress, count: tiles.count, options: options)
        }
      }
    }
  }

  public func addRasterSourceURL(sourceId: String, url: String, options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try options.nativeOptions.withNativeOptions { options in
        try CAPI.mapAddRasterSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), url: arena.view(url), options: options)
      }
    }
  }

  public func addRasterSourceTiles(sourceId: String, tiles: [String], options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      let tileViews = tiles.map { arena.view($0) }
      try tileViews.withUnsafeBufferPointer { tiles in
        try options.nativeOptions.withNativeOptions { options in
          try CAPI.mapAddRasterSourceTiles(try requireLivePointer(), sourceId: arena.view(sourceId), tiles: tiles.baseAddress, count: tiles.count, options: options)
        }
      }
    }
  }

  public func addRasterDEMSourceURL(sourceId: String, url: String, options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try options.nativeOptions.withNativeOptions { options in
        try CAPI.mapAddRasterDEMSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), url: arena.view(url), options: options)
      }
    }
  }

  public func addRasterDEMSourceTiles(sourceId: String, tiles: [String], options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      let tileViews = tiles.map { arena.view($0) }
      try tileViews.withUnsafeBufferPointer { tiles in
        try options.nativeOptions.withNativeOptions { options in
          try CAPI.mapAddRasterDEMSourceTiles(try requireLivePointer(), sourceId: arena.view(sourceId), tiles: tiles.baseAddress, count: tiles.count, options: options)
        }
      }
    }
  }

  public func addCustomGeometrySource(sourceId: String, options: CustomGeometrySourceOptions) throws {
    let fetchTile: NativeCustomGeometrySourceCallbacks.TileCallback = { tileId in
      options.fetchTile(CanonicalTileID(native: tileId))
    }
    let cancelTile: NativeCustomGeometrySourceCallbacks.TileCallback?
    if let callback = options.cancelTile {
      cancelTile = { tileId in callback(CanonicalTileID(native: tileId)) }
    } else {
      cancelTile = nil
    }
    let callbacks = NativeCustomGeometrySourceCallbacks(fetchTile: fetchTile, cancelTile: cancelTile)
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try options.nativeOptions(callbacks: callbacks).withNativeOptions { nativeOptions in
        try CAPI.mapAddCustomGeometrySource(try requireLivePointer(), sourceId: arena.view(sourceId), options: nativeOptions)
      }
      customGeometrySourceCallbacks[sourceId] = callbacks
    }
  }

  public func setCustomGeometrySourceTileData(sourceId: String, tileId: CanonicalTileID, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try CAPI.mapSetCustomGeometrySourceTileData(try requireLivePointer(), sourceId: arena.view(sourceId), tileId: tileId.nativeTileID, data: data)
      }
    }
  }

  public func invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileID) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapInvalidateCustomGeometrySourceTile(try requireLivePointer(), sourceId: arena.view(sourceId), tileId: tileId.nativeTileID)
    }
  }

  public func invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapInvalidateCustomGeometrySourceRegion(try requireLivePointer(), sourceId: arena.view(sourceId), bounds: bounds.nativeInput)
    }
  }

  public func setStyleImage(imageId: String, image: StyleRGBA8Image, options: StyleImageOptions = StyleImageOptions()) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try image.nativeImage.withNativeImage { image in
        try options.nativeOptions.withNativeOptions { options in
          try CAPI.mapSetStyleImage(try requireLivePointer(), imageId: arena.view(imageId), image: image, options: options)
        }
      }
    }
  }

  @discardableResult public func removeStyleImage(_ imageId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapRemoveStyleImage(try requireLivePointer(), imageId: arena.view(imageId))
    }
  }

  public func styleImageExists(_ imageId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapStyleImageExists(try requireLivePointer(), imageId: arena.view(imageId))
    }
  }

  public func styleImageInfo(_ imageId: String) throws -> StyleImageInfo? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetStyleImageInfo(try requireLivePointer(), imageId: arena.view(imageId)).map(StyleImageInfo.init(native:))
    }
  }

  public func styleImage(_ imageId: String) throws -> StyleImage? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      let imageIdView = arena.view(imageId)
      guard let info = try CAPI.mapGetStyleImageInfo(try requireLivePointer(), imageId: imageIdView) else { return nil }
      guard let pixels = try CAPI.mapCopyStyleImagePremultipliedRGBA8(try requireLivePointer(), imageId: imageIdView, capacity: info.byteLength).0 else { return nil }
      return StyleImage(info: StyleImageInfo(native: info), pixels: pixels)
    }
  }

  public func addImageSourceURL(sourceId: String, coordinates: [LatLng], url: String) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapAddImageSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), coordinates: coordinates.map(\.nativeInput), url: arena.view(url))
    }
  }

  public func addImageSourceImage(sourceId: String, coordinates: [LatLng], image: StyleRGBA8Image) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try image.nativeImage.withNativeImage { image in
        try CAPI.mapAddImageSourceImage(try requireLivePointer(), sourceId: arena.view(sourceId), coordinates: coordinates.map(\.nativeInput), image: image)
      }
    }
  }

  public func setImageSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetImageSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), url: arena.view(url))
    }
  }

  public func setImageSourceImage(sourceId: String, image: StyleRGBA8Image) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try image.nativeImage.withNativeImage { image in
        try CAPI.mapSetImageSourceImage(try requireLivePointer(), sourceId: arena.view(sourceId), image: image)
      }
    }
  }

  public func setImageSourceCoordinates(sourceId: String, coordinates: [LatLng]) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetImageSourceCoordinates(try requireLivePointer(), sourceId: arena.view(sourceId), coordinates: coordinates.map(\.nativeInput))
    }
  }

  public func imageSourceCoordinates(sourceId: String) throws -> [LatLng]? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetImageSourceCoordinates(try requireLivePointer(), sourceId: arena.view(sourceId))?.map(LatLng.init(native:))
    }
  }

  public func addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapAddHillshadeLayer(try requireLivePointer(), layerId: arena.view(layerId), sourceId: arena.view(sourceId), beforeLayerId: arena.view(beforeLayerId ?? ""))
    }
  }

  public func addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapAddColorReliefLayer(try requireLivePointer(), layerId: arena.view(layerId), sourceId: arena.view(sourceId), beforeLayerId: arena.view(beforeLayerId ?? ""))
    }
  }

  public func addLocationIndicatorLayer(layerId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapAddLocationIndicatorLayer(try requireLivePointer(), layerId: arena.view(layerId), beforeLayerId: arena.view(beforeLayerId ?? ""))
    }
  }

  public func setLocationIndicatorLocation(layerId: String, coordinate: LatLng, altitude: Double) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetLocationIndicatorLocation(try requireLivePointer(), layerId: arena.view(layerId), coordinate: coordinate.nativeInput, altitude: altitude)
    }
  }

  public func setLocationIndicatorBearing(layerId: String, bearing: Double) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetLocationIndicatorBearing(try requireLivePointer(), layerId: arena.view(layerId), bearing: bearing)
    }
  }

  public func setLocationIndicatorAccuracyRadius(layerId: String, radius: Double) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetLocationIndicatorAccuracyRadius(try requireLivePointer(), layerId: arena.view(layerId), radius: radius)
    }
  }

  public func setLocationIndicatorImageName(layerId: String, kind: LocationIndicatorImageKind, imageId: String) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetLocationIndicatorImageName(try requireLivePointer(), layerId: arena.view(layerId), imageKind: kind.rawValue, imageId: arena.view(imageId))
    }
  }

  public func addStyleLayerJSON(_ layerJSON: JSONValue, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapAddStyleLayerJSON(try requireLivePointer(), layerJSON: arena.allocate(layerJSON.nativeValue), beforeLayerId: arena.view(beforeLayerId ?? ""))
    }
  }

  @discardableResult public func removeStyleLayer(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapRemoveStyleLayer(try requireLivePointer(), layerId: arena.view(layerId))
    }
  }

  public func styleLayerExists(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapStyleLayerExists(try requireLivePointer(), layerId: arena.view(layerId))
    }
  }

  public func styleLayerType(_ layerId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetStyleLayerType(try requireLivePointer(), layerId: arena.view(layerId))
    }
  }

  public func styleLayerIds() throws -> [String] {
    try mapNativeFailure { try CAPI.mapListStyleLayerIds(try requireLivePointer()) }
  }

  public func moveStyleLayer(_ layerId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapMoveStyleLayer(try requireLivePointer(), layerId: arena.view(layerId), beforeLayerId: arena.view(beforeLayerId ?? ""))
    }
  }

  public func styleLayerJSON(_ layerId: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetStyleLayerJSON(try requireLivePointer(), layerId: arena.view(layerId)).map(JSONValue.init(native:))
    }
  }

  public func setStyleLightJSON(_ lightJSON: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetStyleLightJSON(try requireLivePointer(), lightJSON: arena.allocate(lightJSON.nativeValue))
    }
  }

  public func setStyleLightProperty(_ propertyName: String, value: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetStyleLightProperty(try requireLivePointer(), propertyName: arena.view(propertyName), value: arena.allocate(value.nativeValue))
    }
  }

  public func styleLightProperty(_ propertyName: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetStyleLightProperty(try requireLivePointer(), propertyName: arena.view(propertyName)).map(JSONValue.init(native:))
    }
  }

  public func setLayerProperty(layerId: String, propertyName: String, value: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetLayerProperty(try requireLivePointer(), layerId: arena.view(layerId), propertyName: arena.view(propertyName), value: arena.allocate(value.nativeValue))
    }
  }

  public func layerProperty(layerId: String, propertyName: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetLayerProperty(try requireLivePointer(), layerId: arena.view(layerId), propertyName: arena.view(propertyName)).map(JSONValue.init(native:))
    }
  }

  public func setLayerFilter(layerId: String, filter: JSONValue?) throws {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      try CAPI.mapSetLayerFilter(try requireLivePointer(), layerId: arena.view(layerId), filter: filter.map { arena.allocate($0.nativeValue) })
    }
  }

  public func layerFilter(_ layerId: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeJSONArena()
      return try CAPI.mapGetLayerFilter(try requireLivePointer(), layerId: arena.view(layerId)).map(JSONValue.init(native:))
    }
  }

}
