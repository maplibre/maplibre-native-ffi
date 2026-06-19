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
    NativePremultipliedRGBA8Image(
      width: width,
      height: height,
      stride: stride,
      pixels: pixels
    )
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

  func nativeOptions(callbacks: NativeCustomGeometrySourceCallbacks)
    -> NativeCustomGeometrySourceOptions
  {
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

public extension MapHandle {
  func addStyleSourceJSON(sourceId: String, sourceJSON: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_style_source_json(
        requireLivePointer(),
        arena.view(sourceId),
        arena.allocate(sourceJSON.nativeValue)
      ))
    }
  }

  @discardableResult func removeStyleSource(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let removed = try NativeStyle.removeSource(
        requireLivePointer(),
        sourceId: arena.view(sourceId)
      )
      if removed { removeCustomGeometrySourceCallbacks(sourceId: sourceId) }
      return removed
    }
  }

  func styleSourceExists(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.sourceExists(
        requireLivePointer(),
        sourceId: arena.view(sourceId)
      )
    }
  }

  func styleSourceType(_ sourceId: String) throws -> StyleSourceType? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.sourceType(
        requireLivePointer(),
        sourceId: arena.view(sourceId)
      ).map { StyleSourceType(rawValue: $0) ?? .unknown }
    }
  }

  func styleSourceInfo(_ sourceId: String) throws -> StyleSourceInfo? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.sourceInfo(
        requireLivePointer(),
        sourceId: arena.view(sourceId)
      ).map(StyleSourceInfo.init(native:))
    }
  }

  func styleSourceAttribution(_ sourceId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let sourceIdView = arena.view(sourceId)
      guard let info = try NativeStyle.sourceInfo(
        requireLivePointer(),
        sourceId: sourceIdView
      ) else { return nil }
      guard info.hasAttribution else { return nil }
      return try NativeStyle.copySourceAttribution(
        requireLivePointer(),
        sourceId: sourceIdView,
        capacity: info.attributionSize
      ).0
    }
  }

  func styleSourceIds() throws -> [String] {
    try mapNativeFailure { try NativeStyle.sourceIds(requireLivePointer()) }
  }

  func addGeoJSONSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_geojson_source_url(
        requireLivePointer(),
        arena.view(sourceId),
        arena.view(url)
      ))
    }
  }

  func addGeoJSONSourceData(sourceId: String, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try checkStatus(mln_map_add_geojson_source_data(
          requireLivePointer(),
          arena.view(sourceId),
          data
        ))
      }
    }
  }

  func setGeoJSONSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_geojson_source_url(
        requireLivePointer(),
        arena.view(sourceId),
        arena.view(url)
      ))
    }
  }

  func setGeoJSONSourceData(sourceId: String, data: GeoJSON) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try checkStatus(mln_map_set_geojson_source_data(
          requireLivePointer(),
          arena.view(sourceId),
          data
        ))
      }
    }
  }

  private func addTiledSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions,
    add: (
      OpaquePointer,
      mln_string_view,
      mln_string_view,
      UnsafePointer<mln_style_tile_source_options>?
    ) -> mln_status
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try options.nativeOptions.withNativeOptions { options in
        try checkStatus(add(
          requireLivePointer(),
          arena.view(sourceId),
          arena.view(url),
          options
        ))
      }
    }
  }

  private func addTiledSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions,
    add: (
      OpaquePointer,
      mln_string_view,
      UnsafePointer<mln_string_view>?,
      Int,
      UnsafePointer<mln_style_tile_source_options>?
    ) -> mln_status
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let tileViews = tiles.map { arena.view($0) }
      try tileViews.withUnsafeBufferPointer { tiles in
        try options.nativeOptions.withNativeOptions { options in
          try checkStatus(add(
            requireLivePointer(),
            arena.view(sourceId),
            tiles.baseAddress,
            tiles.count,
            options
          ))
        }
      }
    }
  }

  func addVectorSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws {
    try addTiledSourceURL(
      sourceId: sourceId,
      url: url,
      options: options,
      add: mln_map_add_vector_source_url
    )
  }

  func addVectorSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws {
    try addTiledSourceTiles(
      sourceId: sourceId,
      tiles: tiles,
      options: options,
      add: mln_map_add_vector_source_tiles
    )
  }

  func addRasterSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws {
    try addTiledSourceURL(
      sourceId: sourceId,
      url: url,
      options: options,
      add: mln_map_add_raster_source_url
    )
  }

  func addRasterSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws {
    try addTiledSourceTiles(
      sourceId: sourceId,
      tiles: tiles,
      options: options,
      add: mln_map_add_raster_source_tiles
    )
  }

  func addRasterDEMSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws {
    try addTiledSourceURL(
      sourceId: sourceId,
      url: url,
      options: options,
      add: mln_map_add_raster_dem_source_url
    )
  }

  func addRasterDEMSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws {
    try addTiledSourceTiles(
      sourceId: sourceId,
      tiles: tiles,
      options: options,
      add: mln_map_add_raster_dem_source_tiles
    )
  }

  func addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions
  ) throws {
    let fetchTile: NativeCustomGeometrySourceCallbacks
      .TileCallback = { tileId in
        options.fetchTile(CanonicalTileID(native: tileId))
      }
    let cancelTile: NativeCustomGeometrySourceCallbacks.TileCallback?
    if let callback = options.cancelTile {
      cancelTile = { tileId in callback(CanonicalTileID(native: tileId)) }
    } else {
      cancelTile = nil
    }
    let callbacks = NativeCustomGeometrySourceCallbacks(
      fetchTile: fetchTile,
      cancelTile: cancelTile
    )
    try mapNativeFailure {
      let arena = NativeInputArena()
      try options.nativeOptions(callbacks: callbacks)
        .withNativeOptions { nativeOptions in
          try checkStatus(mln_map_add_custom_geometry_source(
            requireLivePointer(),
            arena.view(sourceId),
            nativeOptions
          ))
        }
      storeCustomGeometrySourceCallbacks(callbacks, sourceId: sourceId)
    }
  }

  func setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileID,
    data: GeoJSON
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try arena.withNativeGeoJSON(data.nativeGeoJSON) { data in
        try checkStatus(mln_map_set_custom_geometry_source_tile_data(
          requireLivePointer(),
          arena.view(sourceId),
          tileId.nativeTileID.native,
          data
        ))
      }
    }
  }

  func invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileID
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_invalidate_custom_geometry_source_tile(
        requireLivePointer(),
        arena.view(sourceId),
        tileId.nativeTileID.native
      ))
    }
  }

  func invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_invalidate_custom_geometry_source_region(
        requireLivePointer(),
        arena.view(sourceId),
        bounds.nativeInput.native
      ))
    }
  }

  func setStyleImage(
    imageId: String,
    image: StyleRGBA8Image,
    options: StyleImageOptions = StyleImageOptions()
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try image.nativeImage.withNativeImage { image in
        try options.nativeOptions.withNativeOptions { options in
          try checkStatus(mln_map_set_style_image(
            requireLivePointer(),
            arena.view(imageId),
            image,
            options
          ))
        }
      }
    }
  }

  @discardableResult func removeStyleImage(_ imageId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.removeImage(
        requireLivePointer(),
        imageId: arena.view(imageId)
      )
    }
  }

  func styleImageExists(_ imageId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.imageExists(
        requireLivePointer(),
        imageId: arena.view(imageId)
      )
    }
  }

  func styleImageInfo(_ imageId: String) throws -> StyleImageInfo? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.imageInfo(
        requireLivePointer(),
        imageId: arena.view(imageId)
      ).map(StyleImageInfo.init(native:))
    }
  }

  func styleImage(_ imageId: String) throws -> StyleImage? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let imageIdView = arena.view(imageId)
      guard let info = try NativeStyle.imageInfo(
        requireLivePointer(),
        imageId: imageIdView
      ) else { return nil }
      guard let pixels = try NativeStyle.copyImagePremultipliedRGBA8(
        requireLivePointer(),
        imageId: imageIdView,
        capacity: info.byteLength
      ).0 else { return nil }
      return StyleImage(info: StyleImageInfo(native: info), pixels: pixels)
    }
  }

  func addImageSourceURL(
    sourceId: String,
    coordinates: [LatLng],
    url: String
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try NativeStyle.addImageSourceURL(
        requireLivePointer(),
        sourceId: arena.view(sourceId),
        coordinates: coordinates.map(\.nativeInput),
        url: arena.view(url)
      )
    }
  }

  func addImageSourceImage(
    sourceId: String,
    coordinates: [LatLng],
    image: StyleRGBA8Image
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try image.nativeImage.withNativeImage { image in
        try NativeStyle.addImageSourceImage(
          requireLivePointer(),
          sourceId: arena.view(sourceId),
          coordinates: coordinates.map(\.nativeInput),
          image: image
        )
      }
    }
  }

  func setImageSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_image_source_url(
        requireLivePointer(),
        arena.view(sourceId),
        arena.view(url)
      ))
    }
  }

  func setImageSourceImage(sourceId: String, image: StyleRGBA8Image) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try image.nativeImage.withNativeImage { image in
        try checkStatus(mln_map_set_image_source_image(
          requireLivePointer(),
          arena.view(sourceId),
          image
        ))
      }
    }
  }

  func setImageSourceCoordinates(
    sourceId: String,
    coordinates: [LatLng]
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try NativeStyle.setImageSourceCoordinates(
        requireLivePointer(),
        sourceId: arena.view(sourceId),
        coordinates: coordinates.map(\.nativeInput)
      )
    }
  }

  func imageSourceCoordinates(sourceId: String) throws -> [LatLng]? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.imageSourceCoordinates(
        requireLivePointer(),
        sourceId: arena.view(sourceId)
      )?.map(LatLng.init(native:))
    }
  }

  func addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = nil
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_hillshade_layer(
        requireLivePointer(),
        arena.view(layerId),
        arena.view(sourceId),
        arena.view(beforeLayerId ?? "")
      ))
    }
  }

  func addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = nil
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_color_relief_layer(
        requireLivePointer(),
        arena.view(layerId),
        arena.view(sourceId),
        arena.view(beforeLayerId ?? "")
      ))
    }
  }

  func addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String? = nil
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_location_indicator_layer(
        requireLivePointer(),
        arena.view(layerId),
        arena.view(beforeLayerId ?? "")
      ))
    }
  }

  func setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_location(
        requireLivePointer(),
        arena.view(layerId),
        coordinate.nativeInput.native,
        altitude
      ))
    }
  }

  func setLocationIndicatorBearing(layerId: String, bearing: Double) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_bearing(
        requireLivePointer(),
        arena.view(layerId),
        bearing
      ))
    }
  }

  func setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_accuracy_radius(
        requireLivePointer(),
        arena.view(layerId),
        radius
      ))
    }
  }

  func setLocationIndicatorImageName(
    layerId: String,
    kind: LocationIndicatorImageKind,
    imageId: String
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_location_indicator_image_name(
        requireLivePointer(),
        arena.view(layerId),
        kind.rawValue,
        arena.view(imageId)
      ))
    }
  }

  func addStyleLayerJSON(
    _ layerJSON: JSONValue,
    beforeLayerId: String? = nil
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_add_style_layer_json(
        requireLivePointer(),
        arena.allocate(layerJSON.nativeValue),
        arena.view(beforeLayerId ?? "")
      ))
    }
  }

  @discardableResult func removeStyleLayer(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.removeLayer(
        requireLivePointer(),
        layerId: arena.view(layerId)
      )
    }
  }

  func styleLayerExists(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerExists(
        requireLivePointer(),
        layerId: arena.view(layerId)
      )
    }
  }

  func styleLayerType(_ layerId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerType(
        requireLivePointer(),
        layerId: arena.view(layerId)
      )
    }
  }

  func styleLayerIds() throws -> [String] {
    try mapNativeFailure { try NativeStyle.layerIds(requireLivePointer()) }
  }

  func moveStyleLayer(_ layerId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_move_style_layer(
        requireLivePointer(),
        arena.view(layerId),
        arena.view(beforeLayerId ?? "")
      ))
    }
  }

  func styleLayerJSON(_ layerId: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerJSON(
        requireLivePointer(),
        layerId: arena.view(layerId)
      ).map(JSONValue.init(native:))
    }
  }

  func setStyleLightJSON(_ lightJSON: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_style_light_json(
        requireLivePointer(),
        arena.allocate(lightJSON.nativeValue)
      ))
    }
  }

  func setStyleLightProperty(_ propertyName: String, value: JSONValue) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_style_light_property(
        requireLivePointer(),
        arena.view(propertyName),
        arena.allocate(value.nativeValue)
      ))
    }
  }

  func styleLightProperty(_ propertyName: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.lightProperty(
        requireLivePointer(),
        propertyName: arena.view(propertyName)
      ).map(JSONValue.init(native:))
    }
  }

  func setLayerProperty(
    layerId: String,
    propertyName: String,
    value: JSONValue
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_layer_property(
        requireLivePointer(),
        arena.view(layerId),
        arena.view(propertyName),
        arena.allocate(value.nativeValue)
      ))
    }
  }

  func layerProperty(layerId: String,
                     propertyName: String) throws -> JSONValue?
  {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerProperty(
        requireLivePointer(),
        layerId: arena.view(layerId),
        propertyName: arena.view(propertyName)
      ).map(JSONValue.init(native:))
    }
  }

  func setLayerFilter(layerId: String, filter: JSONValue?) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_set_layer_filter(
        requireLivePointer(),
        arena.view(layerId),
        filter.map { arena.allocate($0.nativeValue) }
      ))
    }
  }

  func layerFilter(_ layerId: String) throws -> JSONValue? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try NativeStyle.layerFilter(
        requireLivePointer(),
        layerId: arena.view(layerId)
      ).map(JSONValue.init(native:))
    }
  }
}
