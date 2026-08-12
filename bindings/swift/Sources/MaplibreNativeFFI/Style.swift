internal import CMaplibreNativeC
import Foundation

/// A style source type reported by MapLibre Native.
///
/// This is an open domain. The raw value preserves source types that this
/// binding does not know yet.
public struct StyleSourceType: Equatable, Sendable, Hashable {
  public static let unknown = Self(rawValue: 0)
  public static let vector = Self(rawValue: 1)
  public static let raster = Self(rawValue: 2)
  public static let rasterDEM = Self(rawValue: 3)
  public static let geoJSON = Self(rawValue: 4)
  public static let image = Self(rawValue: 5)
  public static let video = Self(rawValue: 6)
  public static let annotations = Self(rawValue: 7)
  public static let customVector = Self(rawValue: 8)
  public static let customMVTVector = Self(rawValue: 9)

  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }
}

/// The coordinate scheme for tile URLs.
///
/// This is an open domain. The raw value preserves schemes that this binding
/// does not know yet.
public struct StyleTileScheme: Equatable, Sendable, Hashable {
  public static let xyz = Self(rawValue: 0)
  public static let tms = Self(rawValue: 1)

  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }
}

/// A vector tile encoding reported by MapLibre Native.
///
/// This is an open domain. The raw value preserves encodings that this binding
/// does not know yet.
public struct StyleVectorTileEncoding: Equatable, Sendable, Hashable {
  public static let mvt = Self(rawValue: 0)
  public static let mlt = Self(rawValue: 1)

  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }
}

/// A DEM raster encoding reported by MapLibre Native.
///
/// This is an open domain. The raw value preserves encodings that this binding
/// does not know yet.
public struct StyleRasterDEMEncoding: Equatable, Sendable, Hashable {
  public static let mapbox = Self(rawValue: 0)
  public static let terrarium = Self(rawValue: 1)

  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }
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

/// Options for GeoJSON sources. They are fixed when the source is created.
public struct StyleGeoJSONSourceOptions: Equatable, Sendable {
  public var minZoom: Double?
  public var maxZoom: Double?
  public var tolerance: Double?
  public var clusterMaxZoom: Double?
  /// Cluster aggregation expressions keyed by property name, as a JSON object
  /// whose members follow the MapLibre Style Spec `clusterProperties` form.
  public var clusterProperties: Data?
  public var tileSize: UInt32?
  public var buffer: UInt32?
  public var clusterRadius: UInt32?
  public var clusterMinPoints: UInt32?
  public var lineMetrics: Bool?
  public var cluster: Bool?
  /// Applies data set through
  /// ``MapHandle/setGeoJSONSourceData(sourceId:data:)`` synchronously, so it
  /// reaches the next rendered frame.
  public var synchronousUpdate: Bool?

  public init(
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
    synchronousUpdate: Bool? = nil
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
    self.synchronousUpdate = synchronousUpdate
  }

  var nativeOptions: NativeGeoJSONSourceOptions {
    NativeGeoJSONSourceOptions(
      minZoom: minZoom,
      maxZoom: maxZoom,
      tolerance: tolerance,
      clusterMaxZoom: clusterMaxZoom,
      clusterProperties: clusterProperties,
      tileSize: tileSize,
      buffer: buffer,
      clusterRadius: clusterRadius,
      clusterMinPoints: clusterMinPoints,
      lineMetrics: lineMetrics,
      cluster: cluster,
      synchronousUpdate: synchronousUpdate
    )
  }
}

/// Whether a style layer draws.
///
/// This is an open domain. The raw value preserves values that this binding
/// does not know yet.
public struct StyleLayerVisibility: Equatable, Sendable, Hashable {
  public static let visible =
    StyleLayerVisibility(rawValue: MLN_STYLE_LAYER_VISIBILITY_VISIBLE.rawValue)
  public static let none =
    StyleLayerVisibility(rawValue: MLN_STYLE_LAYER_VISIBILITY_NONE.rawValue)

  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
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

/// One stretchable interval along an image axis, in image pixels.
public struct ImageStretch: Equatable, Sendable {
  public var from: Float
  public var to: Float

  public init(from: Float, to: Float) {
    self.from = from
    self.to = to
  }
}

/// Content-box insets in image pixels, measured from the image's top-left.
public struct ImageContent: Equatable, Sendable {
  public var left: Float
  public var top: Float
  public var right: Float
  public var bottom: Float

  public init(left: Float, top: Float, right: Float, bottom: Float) {
    self.left = left
    self.top = top
    self.right = right
    self.bottom = bottom
  }
}

/// How a stretchable image fits text along one axis.
///
/// This is an open domain. The raw value preserves values that this binding
/// does not know yet.
public struct StyleImageTextFit: Equatable, Sendable, Hashable {
  public static let stretchOrShrink = StyleImageTextFit(
    rawValue: MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK.rawValue
  )
  public static let stretchOnly = StyleImageTextFit(
    rawValue: MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY.rawValue
  )
  public static let proportional = StyleImageTextFit(
    rawValue: MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL.rawValue
  )

  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }
}

public struct StyleImageOptions: Equatable, Sendable {
  public var pixelRatio: Float?
  public var sdf: Bool?
  /// Stretchable intervals along each axis. A present empty array stays
  /// distinguishable from an absent one.
  public var stretchX: [ImageStretch]?
  public var stretchY: [ImageStretch]?
  /// Content box used when `icon-text-fit` applies.
  public var content: ImageContent?
  public var textFitWidth: StyleImageTextFit?
  public var textFitHeight: StyleImageTextFit?

  public init(
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

  var nativeOptions: NativeStyleImageOptions {
    NativeStyleImageOptions(
      pixelRatio: pixelRatio,
      sdf: sdf,
      stretchX: stretchX,
      stretchY: stretchY,
      content: content,
      textFitWidth: textFitWidth,
      textFitHeight: textFitHeight
    )
  }
}

public struct StyleImageInfo: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let stride: UInt32
  public let byteLength: Int
  public let pixelRatio: Float
  public let sdf: Bool
  /// Interval counts for the stretchable axes. Read the intervals themselves
  /// with
  /// ``MapHandle/styleImageStretches(_:)``.
  public let stretchXCount: Int
  public let stretchYCount: Int
  /// Content box, absent when the image carries none.
  public let content: ImageContent?
  public let textFitWidth: StyleImageTextFit?
  public let textFitHeight: StyleImageTextFit?

  init(native: NativeStyleImageInfo) {
    width = native.width
    height = native.height
    stride = native.stride
    byteLength = native.byteLength
    pixelRatio = native.pixelRatio
    sdf = native.sdf
    stretchXCount = native.stretchXCount
    stretchYCount = native.stretchYCount
    content = native.content
    textFitWidth = native.textFitWidth
    textFitHeight = native.textFitHeight
  }
}

public struct StyleImage: Equatable, Sendable {
  public let info: StyleImageInfo
  public let pixels: [UInt8]
}

/// The style's global transition options for paint property changes and symbol
/// placement cross-fades.
public struct StyleTransitionOptions: Equatable, Sendable {
  /// Transition duration in milliseconds. `nil` falls back to the duration the
  /// style declares for each transitioning property.
  public var durationMilliseconds: Double?
  /// Transition delay in milliseconds. `nil` falls back to the delay the style
  /// declares for each transitioning property.
  public var delayMilliseconds: Double?
  /// Whether symbol placement changes cross-fade. `nil` leaves the cross-fade
  /// on. Clearing it makes symbol placement changes apply to the next rendered
  /// frame. Reading the options always reports a value.
  public var enablePlacementTransitions: Bool?

  public init(
    durationMilliseconds: Double? = nil,
    delayMilliseconds: Double? = nil,
    enablePlacementTransitions: Bool? = nil
  ) {
    self.durationMilliseconds = durationMilliseconds
    self.delayMilliseconds = delayMilliseconds
    self.enablePlacementTransitions = enablePlacementTransitions
  }

  init(native: NativeStyleTransitionOptions) {
    durationMilliseconds = native.durationMilliseconds
    delayMilliseconds = native.delayMilliseconds
    enablePlacementTransitions = native.enablePlacementTransitions
  }

  var nativeOptions: NativeStyleTransitionOptions {
    NativeStyleTransitionOptions(
      durationMilliseconds: durationMilliseconds,
      delayMilliseconds: delayMilliseconds,
      enablePlacementTransitions: enablePlacementTransitions
    )
  }
}

/// The retained TileJSON fields of an inline tile source.
public struct StyleSourceTileJSON: Equatable, Sendable {
  public let tileURLs: [String]
  public let minZoom: Double
  public let maxZoom: Double
  public let scheme: StyleTileScheme
  public let bounds: LatLngBounds?

  init(native: NativeStyleSourceTileJSON) {
    tileURLs = native.tileURLs
    minZoom = native.minZoom
    maxZoom = native.maxZoom
    scheme = StyleTileScheme(rawValue: native.scheme)
    bounds = native.bounds.map(LatLngBounds.init(native:))
  }
}

/// A copied snapshot of the retained state for one style source.
public struct StyleSourceInfo: Equatable, Sendable {
  public let type: StyleSourceType
  public let isVolatile: Bool
  public let attribution: String?
  public let url: String?
  public let tileJSON: StyleSourceTileJSON?
  public let tileSize: UInt32?
  public let vectorEncoding: StyleVectorTileEncoding?
  public let rasterEncoding: StyleRasterDEMEncoding?

  init(native: NativeStyleSourceInfo) {
    type = StyleSourceType(rawValue: native.type)
    isVolatile = native.isVolatile
    attribution = native.attribution
    url = native.url
    tileJSON = native.tileJSON.map(StyleSourceTileJSON.init(native:))
    tileSize = native.tileSize
    vectorEncoding = native.vectorEncoding.map(StyleVectorTileEncoding.init(
      rawValue:
    ))
    rasterEncoding = native.rasterEncoding.map(StyleRasterDEMEncoding.init(
      rawValue:
    ))
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
  func addStyleSourceJSON(sourceId: String, sourceJSON: Data) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_add_style_source_json(
        requireLiveHandle().raw,
        arena.view(sourceId),
        arena.view(sourceJSON)
      ))
    }
  }

  @discardableResult func removeStyleSource(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let removed = try NativeStyle.removeSource(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      )
      if removed { removeCustomGeometrySourceCallbacks(sourceId: sourceId) }
      return removed
    }
  }

  func styleSourceExists(_ sourceId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceExists(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      )
    }
  }

  func styleSourceType(_ sourceId: String) throws -> StyleSourceType? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceType(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      ).map(StyleSourceType.init(rawValue:))
    }
  }

  func styleSourceInfo(_ sourceId: String) throws -> StyleSourceInfo? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceInfo(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      ).map(StyleSourceInfo.init(native:))
    }
  }

  func styleSourceAttribution(_ sourceId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceAttribution(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      )
    }
  }

  func styleSourceIds() throws -> [String] {
    try mapNativeFailure { try NativeStyle.sourceIds(requireLiveHandle()) }
  }

  /// Adds a GeoJSON source that loads data from a URL. `options` is fixed when
  /// the source is created.
  func addGeoJSONSourceURL(
    sourceId: String,
    url: String,
    options: StyleGeoJSONSourceOptions = StyleGeoJSONSourceOptions()
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try options.nativeOptions.withNativeOptions { options in
        try checkStatus(mln_map_add_geojson_source_url(
          requireLiveHandle().raw,
          arena.view(sourceId),
          arena.view(url),
          options
        ))
      }
    }
  }

  /// Adds a GeoJSON source with inline data. `options` is fixed when the source
  /// is created.
  func addGeoJSONSourceData(
    sourceId: String,
    data: Data,
    options: StyleGeoJSONSourceOptions = StyleGeoJSONSourceOptions()
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try options.nativeOptions.withNativeOptions { options in
        try checkStatus(mln_map_add_geojson_source_data(
          requireLiveHandle().raw,
          arena.view(sourceId),
          arena.view(data),
          options
        ))
      }
    }
  }

  func setGeoJSONSourceURL(sourceId: String, url: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_geojson_source_url(
        requireLiveHandle().raw,
        arena.view(sourceId),
        arena.view(url)
      ))
    }
  }

  func setGeoJSONSourceData(sourceId: String, data: Data) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_geojson_source_data(
        requireLiveHandle().raw,
        arena.view(sourceId),
        arena.view(data)
      ))
    }
  }

  private func addTiledSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions,
    add: (
      mln_map,
      mln_buffer_view,
      mln_buffer_view,
      UnsafePointer<mln_style_tile_source_options>?
    ) -> mln_status
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try options.nativeOptions.withNativeOptions { options in
        try checkStatus(add(
          requireLiveHandle().raw,
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
      mln_map,
      mln_buffer_view,
      UnsafePointer<mln_buffer_view>?,
      Int,
      UnsafePointer<mln_style_tile_source_options>?
    ) -> mln_status
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let tileViews = tiles.map { arena.view($0) }
      try tileViews.withUnsafeBufferPointer { tiles in
        try options.nativeOptions.withNativeOptions { options in
          try checkStatus(add(
            requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try options.nativeOptions(callbacks: callbacks)
        .withNativeOptions { nativeOptions in
          try checkStatus(mln_map_add_custom_geometry_source(
            requireLiveHandle().raw,
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
    data: Data
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_custom_geometry_source_tile_data(
        requireLiveHandle().raw,
        arena.view(sourceId),
        tileId.nativeTileID.native,
        arena.view(data)
      ))
    }
  }

  func invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileID
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_invalidate_custom_geometry_source_tile(
        requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_invalidate_custom_geometry_source_region(
        requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try image.nativeImage.withNativeImage { image in
        try options.nativeOptions.withNativeOptions { options in
          try checkStatus(mln_map_set_style_image(
            requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.removeImage(
        requireLiveHandle(),
        imageId: arena.view(imageId)
      )
    }
  }

  func styleImageExists(_ imageId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.imageExists(
        requireLiveHandle(),
        imageId: arena.view(imageId)
      )
    }
  }

  func styleImageInfo(_ imageId: String) throws -> StyleImageInfo? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.imageInfo(
        requireLiveHandle(),
        imageId: arena.view(imageId)
      ).map(StyleImageInfo.init(native:))
    }
  }

  /// Copies one runtime style image's stretchable intervals, or nil when no
  /// image carries `imageId`.
  func styleImageStretches(
    _ imageId: String
  ) throws -> (stretchX: [ImageStretch], stretchY: [ImageStretch])? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.copyImageStretches(
        requireLiveHandle(),
        imageId: arena.view(imageId)
      )
    }
  }

  func styleImage(_ imageId: String) throws -> StyleImage? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let imageIdView = arena.view(imageId)
      guard let info = try NativeStyle.imageInfo(
        requireLiveHandle(),
        imageId: imageIdView
      ) else { return nil }
      guard let pixels = try NativeStyle.copyImagePremultipliedRGBA8(
        requireLiveHandle(),
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
      defer { withExtendedLifetime(arena) {} }
      try NativeStyle.addImageSourceURL(
        requireLiveHandle(),
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
      defer { withExtendedLifetime(arena) {} }
      try image.nativeImage.withNativeImage { image in
        try NativeStyle.addImageSourceImage(
          requireLiveHandle(),
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_image_source_url(
        requireLiveHandle().raw,
        arena.view(sourceId),
        arena.view(url)
      ))
    }
  }

  func setImageSourceImage(sourceId: String, image: StyleRGBA8Image) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try image.nativeImage.withNativeImage { image in
        try checkStatus(mln_map_set_image_source_image(
          requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try NativeStyle.setImageSourceCoordinates(
        requireLiveHandle(),
        sourceId: arena.view(sourceId),
        coordinates: coordinates.map(\.nativeInput)
      )
    }
  }

  func imageSourceCoordinates(sourceId: String) throws -> [LatLng]? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.imageSourceCoordinates(
        requireLiveHandle(),
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_add_hillshade_layer(
        requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_add_color_relief_layer(
        requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_add_location_indicator_layer(
        requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_location_indicator_location(
        requireLiveHandle().raw,
        arena.view(layerId),
        coordinate.nativeInput.native,
        altitude
      ))
    }
  }

  func setLocationIndicatorBearing(layerId: String, bearing: Double) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_location_indicator_bearing(
        requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_location_indicator_accuracy_radius(
        requireLiveHandle().raw,
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
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_location_indicator_image_name(
        requireLiveHandle().raw,
        arena.view(layerId),
        kind.rawValue,
        arena.view(imageId)
      ))
    }
  }

  func addStyleLayerJSON(
    _ layerJSON: Data,
    beforeLayerId: String? = nil
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_add_style_layer_json(
        requireLiveHandle().raw,
        arena.view(layerJSON),
        arena.view(beforeLayerId ?? "")
      ))
    }
  }

  @discardableResult func removeStyleLayer(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.removeLayer(
        requireLiveHandle(),
        layerId: arena.view(layerId)
      )
    }
  }

  func styleLayerExists(_ layerId: String) throws -> Bool {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerExists(
        requireLiveHandle(),
        layerId: arena.view(layerId)
      )
    }
  }

  func styleLayerType(_ layerId: String) throws -> String? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerType(
        requireLiveHandle(),
        layerId: arena.view(layerId)
      )
    }
  }

  func styleLayerIds() throws -> [String] {
    try mapNativeFailure { try NativeStyle.layerIds(requireLiveHandle()) }
  }

  func moveStyleLayer(_ layerId: String, beforeLayerId: String? = nil) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_move_style_layer(
        requireLiveHandle().raw,
        arena.view(layerId),
        arena.view(beforeLayerId ?? "")
      ))
    }
  }

  func styleLayerJSON(_ layerId: String) throws -> Data? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerJSON(
        requireLiveHandle(),
        layerId: arena.view(layerId)
      )
    }
  }

  func setStyleLightJSON(_ lightJSON: Data) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_style_light_json(
        requireLiveHandle().raw,
        arena.view(lightJSON)
      ))
    }
  }

  func setStyleLightProperty(_ propertyName: String, value: Data) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_style_light_property(
        requireLiveHandle().raw,
        arena.view(propertyName),
        arena.view(value)
      ))
    }
  }

  func styleLightProperty(_ propertyName: String) throws -> Data? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.lightProperty(
        requireLiveHandle(),
        propertyName: arena.view(propertyName)
      )
    }
  }

  /// Replaces the style's global transition options; absent fields clear the
  /// style-wide override rather than merging. Loading a style replaces these
  /// options, so apply an override after the style loads.
  func setStyleTransitionOptions(_ options: StyleTransitionOptions) throws {
    try mapNativeFailure {
      try NativeStyle.setTransitionOptions(
        requireLiveHandle(),
        options: options.nativeOptions
      )
    }
  }

  /// Reads the style's global transition options.
  func styleTransitionOptions() throws -> StyleTransitionOptions {
    try mapNativeFailure {
      try StyleTransitionOptions(
        native: NativeStyle.transitionOptions(requireLiveHandle())
      )
    }
  }

  func setLayerProperty(
    layerId: String,
    propertyName: String,
    value: Data
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_layer_property(
        requireLiveHandle().raw,
        arena.view(layerId),
        arena.view(propertyName),
        arena.view(value)
      ))
    }
  }

  func layerProperty(layerId: String,
                     propertyName: String) throws -> Data?
  {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerProperty(
        requireLiveHandle(),
        layerId: arena.view(layerId),
        propertyName: arena.view(propertyName)
      )
    }
  }

  func setLayerFilter(layerId: String, filter: Data?) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      guard let filter else {
        return try checkStatus(mln_map_set_layer_filter(
          requireLiveHandle().raw, arena.view(layerId), nil
        ))
      }
      var filterView = arena.view(filter)
      try withUnsafePointer(to: &filterView) { filter in
        try checkStatus(mln_map_set_layer_filter(
          requireLiveHandle().raw, arena.view(layerId), filter
        ))
      }
    }
  }

  func layerFilter(_ layerId: String) throws -> Data? {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerFilter(
        requireLiveHandle(),
        layerId: arena.view(layerId)
      )
    }
  }

  /// Sets one layer's source-layer ID. Layer types that take no source, such as
  /// background, are rejected.
  func setLayerSourceLayer(layerId: String, sourceLayer: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_layer_source_layer(
        requireLiveHandle().raw,
        arena.view(layerId),
        arena.view(sourceLayer)
      ))
    }
  }

  /// Copies one layer's source-layer ID, empty when the layer carries none.
  func layerSourceLayer(_ layerId: String) throws -> String {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.copyLayerText(
        requireLiveHandle(),
        layerId: arena.view(layerId),
        copy: mln_map_copy_layer_source_layer
      )
    }
  }

  /// Sets one layer's source ID. Layer types that take no source, such as
  /// background, are rejected. The named source need not exist yet.
  func setLayerSourceId(layerId: String, sourceId: String) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_layer_source_id(
        requireLiveHandle().raw,
        arena.view(layerId),
        arena.view(sourceId)
      ))
    }
  }

  /// Copies one layer's source ID, empty when the layer carries none.
  func layerSourceId(_ layerId: String) throws -> String {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.copyLayerText(
        requireLiveHandle(),
        layerId: arena.view(layerId),
        copy: mln_map_copy_layer_source_id
      )
    }
  }

  /// Sets the lowest zoom at which one layer draws. Pass `-.infinity` for no
  /// lower bound.
  func setLayerMinZoom(layerId: String, minZoom: Double) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_layer_min_zoom(
        requireLiveHandle().raw,
        arena.view(layerId),
        minZoom
      ))
    }
  }

  /// Reads the lowest zoom at which one layer draws. A layer with no lower
  /// bound
  /// reports `-.infinity`.
  func layerMinZoom(_ layerId: String) throws -> Double {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let handle = try requireLiveHandle()
      let layerView = arena.view(layerId)
      return try NativeMemory.withTemporary(0.0) { outZoom in
        try checkStatus(mln_map_get_layer_min_zoom(
          handle.raw,
          layerView,
          outZoom
        ))
      }.value
    }
  }

  /// Sets the highest zoom at which one layer draws. Pass `.infinity` for no
  /// upper bound.
  func setLayerMaxZoom(layerId: String, maxZoom: Double) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_layer_max_zoom(
        requireLiveHandle().raw,
        arena.view(layerId),
        maxZoom
      ))
    }
  }

  /// Reads the highest zoom at which one layer draws. A layer with no upper
  /// bound reports `.infinity`.
  func layerMaxZoom(_ layerId: String) throws -> Double {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let handle = try requireLiveHandle()
      let layerView = arena.view(layerId)
      return try NativeMemory.withTemporary(0.0) { outZoom in
        try checkStatus(mln_map_get_layer_max_zoom(
          handle.raw,
          layerView,
          outZoom
        ))
      }.value
    }
  }

  /// Sets whether one layer draws.
  func setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_layer_visibility(
        requireLiveHandle().raw,
        arena.view(layerId),
        visibility.rawValue
      ))
    }
  }

  /// Reads whether one layer draws.
  func layerVisibility(_ layerId: String) throws -> StyleLayerVisibility {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let handle = try requireLiveHandle()
      let layerView = arena.view(layerId)
      let raw = try NativeMemory.withTemporary(UInt32(0)) { outVisibility in
        try checkStatus(mln_map_get_layer_visibility(
          handle.raw,
          layerView,
          outVisibility
        ))
      }.value
      return StyleLayerVisibility(rawValue: raw)
    }
  }
}
