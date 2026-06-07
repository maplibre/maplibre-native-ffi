internal import CMaplibreNativeC

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
  public let hasAttribution: Bool
  public let attributionSize: Int

  init(native: NativeStyleSourceInfo) {
    type = StyleSourceType(rawValue: native.type) ?? .unknown
    idSize = native.idSize
    isVolatile = native.isVolatile
    hasAttribution = native.hasAttribution
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
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_style_source_json(try requireLivePointer(), arena.view(sourceId), arena.allocate(sourceJSON.nativeValue)))
    }
  }

  @discardableResult public func removeStyleSource(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let removed = try NativeStyle.removeSource(try requireLivePointer(), sourceId: arena.view(sourceId))
      if removed { removeCustomGeometrySourceCallbacks(sourceId: sourceId) }
      return removed
    }
  }

  public func styleSourceExists(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.sourceExists(try requireLivePointer(), sourceId: arena.view(sourceId))
    }
  }

  public func styleSourceType(_ sourceId: String) throws -> StyleSourceType? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.sourceType(try requireLivePointer(), sourceId: arena.view(sourceId)).map { StyleSourceType(rawValue: $0) ?? .unknown }
    }
  }

  public func styleSourceInfo(_ sourceId: String) throws -> StyleSourceInfo? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.sourceInfo(try requireLivePointer(), sourceId: arena.view(sourceId)).map(StyleSourceInfo.init(native:))
    }
  }

  public func styleSourceAttribution(_ sourceId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let sourceIdView = arena.view(sourceId)
      guard let info = try NativeStyle.sourceInfo(try requireLivePointer(), sourceId: sourceIdView) else { return nil }
      guard info.hasAttribution else { return nil }
      return try NativeStyle.copySourceAttribution(try requireLivePointer(), sourceId: sourceIdView, capacity: info.attributionSize).0
    }
  }

  public func styleSourceIds() throws -> [String] {
    try mapNativeFailure { try NativeStyle.sourceIds(try requireLivePointer()) }
  }

  public func addGeoJSONSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_geojson_source_url(try requireLivePointer(), arena.view(sourceId), arena.view(url)))
    }
  }

  public func addGeoJSONSourceData(sourceId: String, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try checkStatus(mln_map_add_geojson_source_data(try requireLivePointer(), arena.view(sourceId), data))
      }
    }
  }

  public func setGeoJSONSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_geojson_source_url(try requireLivePointer(), arena.view(sourceId), arena.view(url)))
    }
  }

  public func setGeoJSONSourceData(sourceId: String, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try checkStatus(mln_map_set_geojson_source_data(try requireLivePointer(), arena.view(sourceId), data))
      }
    }
  }

  private func addTiledSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions,
    add: (OpaquePointer, mln_string_view, mln_string_view, UnsafePointer<mln_style_tile_source_options>?) -> mln_status
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try options.nativeOptions.withNativeOptions { options in
        try checkStatus(add(try requireLivePointer(), arena.view(sourceId), arena.view(url), options))
      }
    }
  }

  private func addTiledSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions,
    add: (OpaquePointer, mln_string_view, UnsafePointer<mln_string_view>?, Int, UnsafePointer<mln_style_tile_source_options>?) -> mln_status
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let tileViews = tiles.map { arena.view($0) }
      try tileViews.withUnsafeBufferPointer { tiles in
        try options.nativeOptions.withNativeOptions { options in
          try checkStatus(add(try requireLivePointer(), arena.view(sourceId), tiles.baseAddress, tiles.count, options))
        }
      }
    }
  }

  public func addVectorSourceURL(sourceId: String, url: String, options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try addTiledSourceURL(sourceId: sourceId, url: url, options: options, add: mln_map_add_vector_source_url)
  }

  public func addVectorSourceTiles(sourceId: String, tiles: [String], options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try addTiledSourceTiles(sourceId: sourceId, tiles: tiles, options: options, add: mln_map_add_vector_source_tiles)
  }

  public func addRasterSourceURL(sourceId: String, url: String, options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try addTiledSourceURL(sourceId: sourceId, url: url, options: options, add: mln_map_add_raster_source_url)
  }

  public func addRasterSourceTiles(sourceId: String, tiles: [String], options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try addTiledSourceTiles(sourceId: sourceId, tiles: tiles, options: options, add: mln_map_add_raster_source_tiles)
  }

  public func addRasterDEMSourceURL(sourceId: String, url: String, options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try addTiledSourceURL(sourceId: sourceId, url: url, options: options, add: mln_map_add_raster_dem_source_url)
  }

  public func addRasterDEMSourceTiles(sourceId: String, tiles: [String], options: StyleTileSourceOptions = StyleTileSourceOptions()) throws {
    try addTiledSourceTiles(sourceId: sourceId, tiles: tiles, options: options, add: mln_map_add_raster_dem_source_tiles)
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
      let arena = NativeInputArena()
      try options.nativeOptions(callbacks: callbacks).withNativeOptions { nativeOptions in
        try checkStatus(mln_map_add_custom_geometry_source(try requireLivePointer(), arena.view(sourceId), nativeOptions))
      }
      storeCustomGeometrySourceCallbacks(callbacks, sourceId: sourceId)
    }
  }

  public func setCustomGeometrySourceTileData(sourceId: String, tileId: CanonicalTileID, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try checkStatus(mln_map_set_custom_geometry_source_tile_data(try requireLivePointer(), arena.view(sourceId), tileId.nativeTileID.native, data))
      }
    }
  }

  public func invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileID) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_invalidate_custom_geometry_source_tile(try requireLivePointer(), arena.view(sourceId), tileId.nativeTileID.native))
    }
  }

  public func invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_invalidate_custom_geometry_source_region(try requireLivePointer(), arena.view(sourceId), bounds.nativeInput.native))
    }
  }

  public func setStyleImage(imageId: String, image: StyleRGBA8Image, options: StyleImageOptions = StyleImageOptions()) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try image.nativeImage.withNativeImage { image in
        try options.nativeOptions.withNativeOptions { options in
          try checkStatus(mln_map_set_style_image(try requireLivePointer(), arena.view(imageId), image, options))
        }
      }
    }
  }

  @discardableResult public func removeStyleImage(_ imageId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.removeImage(try requireLivePointer(), imageId: arena.view(imageId))
    }
  }

  public func styleImageExists(_ imageId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.imageExists(try requireLivePointer(), imageId: arena.view(imageId))
    }
  }

  public func styleImageInfo(_ imageId: String) throws -> StyleImageInfo? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.imageInfo(try requireLivePointer(), imageId: arena.view(imageId)).map(StyleImageInfo.init(native:))
    }
  }

  public func styleImage(_ imageId: String) throws -> StyleImage? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let imageIdView = arena.view(imageId)
      guard let info = try NativeStyle.imageInfo(try requireLivePointer(), imageId: imageIdView) else { return nil }
      guard let pixels = try NativeStyle.copyImagePremultipliedRGBA8(try requireLivePointer(), imageId: imageIdView, capacity: info.byteLength).0 else { return nil }
      return StyleImage(info: StyleImageInfo(native: info), pixels: pixels)
    }
  }

  public func addImageSourceURL(sourceId: String, coordinates: [LatLng], url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try NativeStyle.addImageSourceURL(try requireLivePointer(), sourceId: arena.view(sourceId), coordinates: coordinates.map(\.nativeInput), url: arena.view(url))
    }
  }

  public func addImageSourceImage(sourceId: String, coordinates: [LatLng], image: StyleRGBA8Image) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try image.nativeImage.withNativeImage { image in
        try NativeStyle.addImageSourceImage(try requireLivePointer(), sourceId: arena.view(sourceId), coordinates: coordinates.map(\.nativeInput), image: image)
      }
    }
  }

  public func setImageSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_image_source_url(try requireLivePointer(), arena.view(sourceId), arena.view(url)))
    }
  }

  public func setImageSourceImage(sourceId: String, image: StyleRGBA8Image) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try image.nativeImage.withNativeImage { image in
        try checkStatus(mln_map_set_image_source_image(try requireLivePointer(), arena.view(sourceId), image))
      }
    }
  }

  public func setImageSourceCoordinates(sourceId: String, coordinates: [LatLng]) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try NativeStyle.setImageSourceCoordinates(try requireLivePointer(), sourceId: arena.view(sourceId), coordinates: coordinates.map(\.nativeInput))
    }
  }

  public func imageSourceCoordinates(sourceId: String) throws -> [LatLng]? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.imageSourceCoordinates(try requireLivePointer(), sourceId: arena.view(sourceId))?.map(LatLng.init(native:))
    }
  }

  public func addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_hillshade_layer(try requireLivePointer(), arena.view(layerId), arena.view(sourceId), arena.view(beforeLayerId ?? "")))
    }
  }

  public func addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_color_relief_layer(try requireLivePointer(), arena.view(layerId), arena.view(sourceId), arena.view(beforeLayerId ?? "")))
    }
  }

  public func addLocationIndicatorLayer(layerId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_location_indicator_layer(try requireLivePointer(), arena.view(layerId), arena.view(beforeLayerId ?? "")))
    }
  }

  public func setLocationIndicatorLocation(layerId: String, coordinate: LatLng, altitude: Double) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_location(try requireLivePointer(), arena.view(layerId), coordinate.nativeInput.native, altitude))
    }
  }

  public func setLocationIndicatorBearing(layerId: String, bearing: Double) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_bearing(try requireLivePointer(), arena.view(layerId), bearing))
    }
  }

  public func setLocationIndicatorAccuracyRadius(layerId: String, radius: Double) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_accuracy_radius(try requireLivePointer(), arena.view(layerId), radius))
    }
  }

  public func setLocationIndicatorImageName(layerId: String, kind: LocationIndicatorImageKind, imageId: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_image_name(try requireLivePointer(), arena.view(layerId), kind.rawValue, arena.view(imageId)))
    }
  }

  public func addStyleLayerJSON(_ layerJSON: JSONValue, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_style_layer_json(try requireLivePointer(), arena.allocate(layerJSON.nativeValue), arena.view(beforeLayerId ?? "")))
    }
  }

  @discardableResult public func removeStyleLayer(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.removeLayer(try requireLivePointer(), layerId: arena.view(layerId))
    }
  }

  public func styleLayerExists(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerExists(try requireLivePointer(), layerId: arena.view(layerId))
    }
  }

  public func styleLayerType(_ layerId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerType(try requireLivePointer(), layerId: arena.view(layerId))
    }
  }

  public func styleLayerIds() throws -> [String] {
    try mapNativeFailure { try NativeStyle.layerIds(try requireLivePointer()) }
  }

  public func moveStyleLayer(_ layerId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_move_style_layer(try requireLivePointer(), arena.view(layerId), arena.view(beforeLayerId ?? "")))
    }
  }

  public func styleLayerJSON(_ layerId: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerJSON(try requireLivePointer(), layerId: arena.view(layerId)).map(JSONValue.init(native:))
    }
  }

  public func setStyleLightJSON(_ lightJSON: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_style_light_json(try requireLivePointer(), arena.allocate(lightJSON.nativeValue)))
    }
  }

  public func setStyleLightProperty(_ propertyName: String, value: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_style_light_property(try requireLivePointer(), arena.view(propertyName), arena.allocate(value.nativeValue)))
    }
  }

  public func styleLightProperty(_ propertyName: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.lightProperty(try requireLivePointer(), propertyName: arena.view(propertyName)).map(JSONValue.init(native:))
    }
  }

  public func setLayerProperty(layerId: String, propertyName: String, value: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_layer_property(try requireLivePointer(), arena.view(layerId), arena.view(propertyName), arena.allocate(value.nativeValue)))
    }
  }

  public func layerProperty(layerId: String, propertyName: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerProperty(try requireLivePointer(), layerId: arena.view(layerId), propertyName: arena.view(propertyName)).map(JSONValue.init(native:))
    }
  }

  public func setLayerFilter(layerId: String, filter: JSONValue?) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_layer_filter(try requireLivePointer(), arena.view(layerId), filter.map { arena.allocate($0.nativeValue) }))
    }
  }

  public func layerFilter(_ layerId: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerFilter(try requireLivePointer(), layerId: arena.view(layerId)).map(JSONValue.init(native:))
    }
  }

}
