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

/// A copied snapshot of the fixed metadata for one style layer.
public struct StyleLayerInfo: Equatable, Sendable {
  /// The layer's style-spec type string, such as `"fill"`.
  public let type: String
  /// Lowest zoom at which the layer draws; `-infinity` with no lower bound.
  public let minZoom: Double
  /// Highest zoom at which the layer draws; `infinity` with no upper bound.
  public let maxZoom: Double
  public let visibility: StyleLayerVisibility
  /// The layer's source ID, absent when the layer has no source.
  public let sourceId: String?
  /// The layer's source-layer ID, absent when the layer carries none.
  public let sourceLayer: String?
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
  @discardableResult
  func addStyleSourceJSON(sourceId: String, sourceJSON: Data) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_add_style_source_json(
          requireLiveHandle().raw,
          arena.view(sourceId),
          arena.view(sourceJSON),
          commandId
        ))
      }.value
    }
  }

  /// Removes one style source by ID. The command commits when a source with
  /// that ID existed and was removed; it fails with `MLN_STATUS_NOT_FOUND`
  /// when none has the ID, and with invalid-state when a layer still uses the
  /// source.
  @discardableResult
  func removeStyleSource(_ sourceId: String) throws -> UInt64 {
    try styleCommand { map, arena, commandId in
      mln_map_remove_style_source(map, arena.view(sourceId), commandId)
    }
  }

  func styleSourceInfo(_ sourceId: String) async throws -> StyleSourceInfo? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceInfoStart(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    guard let info = try mapNativeFailure({
      try NativeStyle.sourceInfoTakeResult(operation)
    }) else { return nil }
    let attribution = info.has_attribution
      ? try await styleSourceAttribution(sourceId) : nil
    let url = (info.fields & MLN_STYLE_SOURCE_INFO_URL.rawValue) != 0
      ? try await styleSourceURL(sourceId) : nil
    let tileURLs = (info.fields & MLN_STYLE_SOURCE_INFO_TILEJSON.rawValue) != 0
      ? try await styleSourceTileURLs(sourceId) : nil
    return StyleSourceInfo(native: NativeStyle.sourceInfo(
      fixed: info,
      attribution: attribution,
      url: url,
      tileURLs: tileURLs
    ))
  }

  func styleSourceAttribution(_ sourceId: String) async throws -> String? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceAttributionStart(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.sourceAttributionTakeResult(operation)
    }
  }

  func styleSourceURL(_ sourceId: String) async throws -> String? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceURLStart(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.sourceURLTakeResult(operation)
    }
  }

  func styleSourceTileURLs(_ sourceId: String) async throws -> [String]? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.sourceTileURLsStart(
        requireLiveHandle(),
        sourceId: arena.view(sourceId)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.sourceTileURLsTakeResult(operation)
    }
  }

  func styleSourceIds() async throws -> [String] {
    let operation = try mapNativeFailure {
      try NativeStyle.sourceIdsStart(requireLiveHandle())
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.sourceIdsTakeResult(operation)
    }
  }

  /// Adds a GeoJSON source that loads data from a URL. `options` is fixed when
  /// the source is created.
  @discardableResult
  func addGeoJSONSourceURL(
    sourceId: String,
    url: String,
    options: StyleGeoJSONSourceOptions = StyleGeoJSONSourceOptions()
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try options.nativeOptions.withNativeOptions { options in
        try NativeMemory.withTemporary(UInt64(0)) { commandId in
          try checkStatus(mln_map_add_geojson_source_url(
            requireLiveHandle().raw,
            arena.view(sourceId),
            arena.view(url),
            options,
            commandId
          ))
        }.value
      }
    }
  }

  /// Adds a GeoJSON source with inline data. `options` is fixed when the source
  /// is created.
  @discardableResult
  func addGeoJSONSourceData(
    sourceId: String,
    data: Data,
    options: StyleGeoJSONSourceOptions = StyleGeoJSONSourceOptions()
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try options.nativeOptions.withNativeOptions { options in
        try NativeMemory.withTemporary(UInt64(0)) { commandId in
          try checkStatus(mln_map_add_geojson_source_data(
            requireLiveHandle().raw,
            arena.view(sourceId),
            arena.view(data),
            options,
            commandId
          ))
        }.value
      }
    }
  }

  @discardableResult
  func setGeoJSONSourceURL(sourceId: String, url: String) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_geojson_source_url(
          requireLiveHandle().raw,
          arena.view(sourceId),
          arena.view(url),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setGeoJSONSourceData(sourceId: String, data: Data) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_geojson_source_data(
          requireLiveHandle().raw,
          arena.view(sourceId),
          arena.view(data),
          commandId
        ))
      }.value
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
      UnsafePointer<mln_style_tile_source_options>?,
      UnsafeMutablePointer<UInt64>?
    ) -> mln_status
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try options.nativeOptions.withNativeOptions { options in
        try NativeMemory.withTemporary(UInt64(0)) { commandId in
          try checkStatus(add(
            requireLiveHandle().raw,
            arena.view(sourceId),
            arena.view(url),
            options,
            commandId
          ))
        }.value
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
      UnsafePointer<mln_style_tile_source_options>?,
      UnsafeMutablePointer<UInt64>?
    ) -> mln_status
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let tileViews = tiles.map { arena.view($0) }
      return try tileViews.withUnsafeBufferPointer { tiles in
        try options.nativeOptions.withNativeOptions { options in
          try NativeMemory.withTemporary(UInt64(0)) { commandId in
            try checkStatus(add(
              requireLiveHandle().raw,
              arena.view(sourceId),
              tiles.baseAddress,
              tiles.count,
              options,
              commandId
            ))
          }.value
        }
      }
    }
  }

  @discardableResult
  func addVectorSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws -> UInt64 {
    try addTiledSourceURL(
      sourceId: sourceId,
      url: url,
      options: options,
      add: mln_map_add_vector_source_url
    )
  }

  @discardableResult
  func addVectorSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws -> UInt64 {
    try addTiledSourceTiles(
      sourceId: sourceId,
      tiles: tiles,
      options: options,
      add: mln_map_add_vector_source_tiles
    )
  }

  @discardableResult
  func addRasterSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws -> UInt64 {
    try addTiledSourceURL(
      sourceId: sourceId,
      url: url,
      options: options,
      add: mln_map_add_raster_source_url
    )
  }

  @discardableResult
  func addRasterSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws -> UInt64 {
    try addTiledSourceTiles(
      sourceId: sourceId,
      tiles: tiles,
      options: options,
      add: mln_map_add_raster_source_tiles
    )
  }

  @discardableResult
  func addRasterDEMSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws -> UInt64 {
    try addTiledSourceURL(
      sourceId: sourceId,
      url: url,
      options: options,
      add: mln_map_add_raster_dem_source_url
    )
  }

  @discardableResult
  func addRasterDEMSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) throws -> UInt64 {
    try addTiledSourceTiles(
      sourceId: sourceId,
      tiles: tiles,
      options: options,
      add: mln_map_add_raster_dem_source_tiles
    )
  }

  /// Adds a custom geometry source, whose tile callbacks run on native worker
  /// threads.
  ///
  /// The source's callbacks stay alive until the C API stops referencing them,
  /// which is when the source is removed, when a style load drops it, or when
  /// the map is destroyed. The C API then releases them, waiting for any tile
  /// callback still running, so a closure is never entered afterwards.
  @discardableResult
  func addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions
  ) throws -> UInt64 {
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
    do {
      return try mapNativeFailure {
        let arena = NativeInputArena()
        defer { withExtendedLifetime(arena) {} }
        return try options.nativeOptions(callbacks: callbacks)
          .withNativeOptions { nativeOptions in
            try NativeMemory.withTemporary(UInt64(0)) { commandId in
              try checkStatus(mln_map_add_custom_geometry_source(
                requireLiveHandle().raw,
                arena.view(sourceId),
                nativeOptions,
                commandId
              ))
            }.value
          }
      }
    } catch {
      // A rejected add is the one case the C API never releases, because it
      // never took the callbacks on.
      callbacks.release()
      throw error
    }
  }

  @discardableResult
  func setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileID,
    data: Data
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_custom_geometry_source_tile_data(
          requireLiveHandle().raw,
          arena.view(sourceId),
          tileId.nativeTileID.native,
          arena.view(data),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileID
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_invalidate_custom_geometry_source_tile(
          requireLiveHandle().raw,
          arena.view(sourceId),
          tileId.nativeTileID.native,
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_invalidate_custom_geometry_source_region(
          requireLiveHandle().raw,
          arena.view(sourceId),
          bounds.nativeInput.native,
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setStyleImage(
    imageId: String,
    image: StyleRGBA8Image,
    options: StyleImageOptions = StyleImageOptions()
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try image.nativeImage.withNativeImage { image in
        try options.nativeOptions.withNativeOptions { options in
          try NativeMemory.withTemporary(UInt64(0)) { commandId in
            try checkStatus(mln_map_set_style_image(
              requireLiveHandle().raw,
              arena.view(imageId),
              image,
              options,
              commandId
            ))
          }.value
        }
      }
    }
  }

  /// Removes one runtime style image by ID. The command commits when an image
  /// with that ID existed and was removed; it fails with
  /// `MLN_STATUS_NOT_FOUND` when none has the ID.
  @discardableResult
  func removeStyleImage(_ imageId: String) throws -> UInt64 {
    try styleCommand { map, arena, commandId in
      mln_map_remove_style_image(map, arena.view(imageId), commandId)
    }
  }

  func styleImageInfo(_ imageId: String) async throws -> StyleImageInfo? {
    let operation = try imageOperation(
      imageId,
      start: mln_map_get_style_image_info_start
    )
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try NativeStyle.imageInfoTakeResult(operation)
      .map(StyleImageInfo.init(native:))
  }

  func styleImage(_ imageId: String) async throws -> StyleImage? {
    guard let info = try await styleImageInfo(imageId) else { return nil }
    let operation = try imageOperation(
      imageId,
      start: mln_map_copy_style_image_premultiplied_rgba8_start
    )
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    guard let pixels = try NativeStyle.imagePixelsTakeResult(operation)
    else { return nil }
    return StyleImage(info: info, pixels: pixels)
  }

  func styleImageStretches(
    _ imageId: String
  ) async throws -> (stretchX: [ImageStretch], stretchY: [ImageStretch])? {
    let operation = try imageOperation(
      imageId,
      start: mln_map_copy_style_image_stretches_start
    )
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try NativeStyle.imageStretchesTakeResult(operation)
  }

  private func imageOperation(
    _ imageId: String,
    start: (mln_map, mln_buffer_view, UnsafeMutablePointer<mln_operation>)
      -> mln_status
  ) throws -> NativeOperationHandle {
    try mapNativeFailure {
      let arena = NativeInputArena(); defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.imageOperationStart(
        requireLiveHandle(),
        imageId: arena.view(imageId),
        start: start
      )
    }
  }

  @discardableResult
  func addImageSourceURL(
    sourceId: String,
    coordinates: [LatLng],
    url: String
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle
        .imageSourceCommand(coordinates
          .map(\.nativeInput))
        { points, count, commandId in
          try checkStatus(mln_map_add_image_source_url(
            requireLiveHandle().raw,
            arena.view(sourceId),
            points,
            count,
            arena.view(url),
            commandId
          ))
        }
    }
  }

  @discardableResult
  func addImageSourceImage(
    sourceId: String,
    coordinates: [LatLng],
    image: StyleRGBA8Image
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try image.nativeImage.withNativeImage { image in
        try NativeStyle
          .imageSourceCommand(coordinates
            .map(\.nativeInput))
          { points, count, commandId in
            try checkStatus(mln_map_add_image_source_image(
              requireLiveHandle().raw,
              arena.view(sourceId),
              points,
              count,
              image,
              commandId
            ))
          }
      }
    }
  }

  @discardableResult
  func setImageSourceURL(sourceId: String, url: String) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_image_source_url(
          requireLiveHandle().raw,
          arena.view(sourceId),
          arena.view(url),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setImageSourceImage(sourceId: String,
                           image: StyleRGBA8Image) throws -> UInt64
  {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try image.nativeImage.withNativeImage { image in
        try NativeMemory.withTemporary(UInt64(0)) { commandId in
          try checkStatus(mln_map_set_image_source_image(
            requireLiveHandle().raw,
            arena.view(sourceId),
            image,
            commandId
          ))
        }.value
      }
    }
  }

  @discardableResult
  func setImageSourceCoordinates(
    sourceId: String,
    coordinates: [LatLng]
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle
        .imageSourceCommand(coordinates
          .map(\.nativeInput))
        { points, count, commandId in
          try checkStatus(mln_map_set_image_source_coordinates(
            requireLiveHandle().raw,
            arena.view(sourceId),
            points,
            count,
            commandId
          ))
        }
    }
  }

  func imageSourceCoordinates(sourceId: String) async throws -> [LatLng]? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena(); defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.imageOperationStart(
        requireLiveHandle(),
        imageId: arena.view(sourceId),
        start: mln_map_get_image_source_coordinates_start
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try NativeStyle.imageSourceCoordinatesTakeResult(operation)?
      .map(LatLng.init(native:))
  }

  @discardableResult
  func addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = nil
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_add_hillshade_layer(
          requireLiveHandle().raw,
          arena.view(layerId),
          arena.view(sourceId),
          arena.view(beforeLayerId ?? ""),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = nil
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_add_color_relief_layer(
          requireLiveHandle().raw,
          arena.view(layerId),
          arena.view(sourceId),
          arena.view(beforeLayerId ?? ""),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String? = nil
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_add_location_indicator_layer(
          requireLiveHandle().raw,
          arena.view(layerId),
          arena.view(beforeLayerId ?? ""),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_location_indicator_location(
          requireLiveHandle().raw,
          arena.view(layerId),
          coordinate.nativeInput.native,
          altitude,
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setLocationIndicatorBearing(layerId: String,
                                   bearing: Double) throws -> UInt64
  {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_location_indicator_bearing(
          requireLiveHandle().raw,
          arena.view(layerId),
          bearing,
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_location_indicator_accuracy_radius(
          requireLiveHandle().raw,
          arena.view(layerId),
          radius,
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setLocationIndicatorImageName(
    layerId: String,
    kind: LocationIndicatorImageKind,
    imageId: String
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_location_indicator_image_name(
          requireLiveHandle().raw,
          arena.view(layerId),
          kind.rawValue,
          arena.view(imageId),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func addStyleLayerJSON(
    _ layerJSON: Data,
    beforeLayerId: String? = nil
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_add_style_layer_json(
          requireLiveHandle().raw,
          arena.view(layerJSON),
          arena.view(beforeLayerId ?? ""),
          commandId
        ))
      }.value
    }
  }

  /// Removes one style layer by ID. The command commits when a layer with
  /// that ID existed and was removed; it fails with `MLN_STATUS_NOT_FOUND`
  /// when none has the ID.
  @discardableResult
  func removeStyleLayer(_ layerId: String) throws -> UInt64 {
    try styleCommand { map, arena, commandId in
      mln_map_remove_style_layer(map, arena.view(layerId), commandId)
    }
  }

  /// Copies fixed metadata for one style layer, or returns `nil` when no
  /// layer has the ID.
  func styleLayerInfo(_ layerId: String) async throws -> StyleLayerInfo? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerInfoStart(
        requireLiveHandle(),
        layerId: arena.view(layerId)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    guard let info = try mapNativeFailure({
      try NativeStyle.layerInfoTakeResult(operation)
    }) else { return nil }
    let sourceId = (info.fields & MLN_STYLE_LAYER_INFO_SOURCE_ID.rawValue) != 0
      ? try await layerSourceId(layerId) : nil
    let sourceLayer =
      (info.fields & MLN_STYLE_LAYER_INFO_SOURCE_LAYER.rawValue) != 0
        ? try await layerSourceLayer(layerId) : nil
    return try mapNativeFailure {
      try StyleLayerInfo(
        type: NativeString.copyUTF8(
          data: info.type.data,
          size: info.type.size
        ),
        minZoom: info.min_zoom,
        maxZoom: info.max_zoom,
        visibility: StyleLayerVisibility(rawValue: info.visibility),
        sourceId: sourceId,
        sourceLayer: sourceLayer
      )
    }
  }

  func styleLayerIds() async throws -> [String] {
    let operation = try mapNativeFailure {
      try NativeStyle.layerIdsStart(requireLiveHandle())
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure { try NativeStyle.layerIdsTakeResult(operation)
    }
  }

  @discardableResult
  func moveStyleLayer(
    _ layerId: String,
    beforeLayerId: String? = nil
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_move_style_layer(
          requireLiveHandle().raw,
          arena.view(layerId),
          arena.view(beforeLayerId ?? ""),
          commandId
        ))
      }.value
    }
  }

  func styleLayerJSON(_ layerId: String) async throws -> Data? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerJSONStart(
        requireLiveHandle(),
        layerId: arena.view(layerId)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.optionalBufferTakeResult(
        operation,
        take: mln_map_get_style_layer_json_take_result
      )
    }
  }

  @discardableResult
  func setStyleLightJSON(_ lightJSON: Data) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_style_light_json(
          requireLiveHandle().raw,
          arena.view(lightJSON),
          commandId
        ))
      }.value
    }
  }

  @discardableResult
  func setStyleLightProperty(
    _ propertyName: String,
    value: Data
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_style_light_property(
          requireLiveHandle().raw,
          arena.view(propertyName),
          arena.view(value),
          commandId
        ))
      }.value
    }
  }

  func styleLightProperty(_ propertyName: String) async throws -> Data? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.lightPropertyStart(
        requireLiveHandle(),
        propertyName: arena.view(propertyName)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.bufferTakeResult(
        operation,
        take: mln_map_get_style_light_property_take_result
      )
    }
  }

  @discardableResult
  func setStyleTransitionOptions(
    _ options: StyleTransitionOptions
  ) throws -> UInt64 {
    try mapNativeFailure {
      try NativeStyle.setTransitionOptions(
        requireLiveHandle(),
        options: options.nativeOptions
      )
    }
  }

  func styleTransitionOptions() async throws -> StyleTransitionOptions {
    let operation = try mapNativeFailure {
      try NativeStyle.transitionOptionsStart(requireLiveHandle())
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try StyleTransitionOptions(
        native: NativeStyle.transitionOptionsTakeResult(operation)
      )
    }
  }

  @discardableResult
  func setLayerProperty(
    layerId: String,
    propertyName: String,
    value: Data
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_layer_property(
          requireLiveHandle().raw,
          arena.view(layerId),
          arena.view(propertyName),
          arena.view(value),
          commandId
        ))
      }.value
    }
  }

  func layerProperty(
    layerId: String,
    propertyName: String
  ) async throws -> Data? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerPropertyStart(
        requireLiveHandle(),
        layerId: arena.view(layerId),
        propertyName: arena.view(propertyName)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.bufferTakeResult(
        operation,
        take: mln_map_get_layer_property_take_result
      )
    }
  }

  @discardableResult
  func setLayerFilter(layerId: String, filter: Data?) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        guard let filter else {
          return try checkStatus(mln_map_set_layer_filter(
            requireLiveHandle().raw, arena.view(layerId), nil, commandId
          ))
        }
        var filterView = arena.view(filter)
        return try withUnsafePointer(to: &filterView) { filter in
          try checkStatus(mln_map_set_layer_filter(
            requireLiveHandle().raw, arena.view(layerId), filter, commandId
          ))
        }
      }.value
    }
  }

  func layerFilter(_ layerId: String) async throws -> Data? {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerFilterStart(
        requireLiveHandle(), layerId: arena.view(layerId)
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.bufferTakeResult(
        operation, take: mln_map_get_layer_filter_take_result
      )
    }
  }

  @discardableResult
  func setLayerSourceLayer(layerId: String,
                           sourceLayer: String) throws -> UInt64
  {
    try styleCommand { map, arena, commandId in
      mln_map_set_layer_source_layer(
        map, arena.view(layerId), arena.view(sourceLayer), commandId
      )
    }
  }

  func layerSourceLayer(_ layerId: String) async throws -> String {
    try await layerText(
      layerId, start: mln_map_copy_layer_source_layer_start,
      take: mln_map_copy_layer_source_layer_take_result
    )
  }

  @discardableResult
  func setLayerSourceId(layerId: String, sourceId: String) throws -> UInt64 {
    try styleCommand { map, arena, commandId in
      mln_map_set_layer_source_id(
        map,
        arena.view(layerId),
        arena.view(sourceId),
        commandId
      )
    }
  }

  func layerSourceId(_ layerId: String) async throws -> String {
    try await layerText(
      layerId, start: mln_map_copy_layer_source_id_start,
      take: mln_map_copy_layer_source_id_take_result
    )
  }

  @discardableResult
  func setLayerMinZoom(layerId: String, minZoom: Double) throws -> UInt64 {
    try styleCommand { map, arena, commandId in
      mln_map_set_layer_min_zoom(map, arena.view(layerId), minZoom, commandId)
    }
  }

  @discardableResult
  func setLayerMaxZoom(layerId: String, maxZoom: Double) throws -> UInt64 {
    try styleCommand { map, arena, commandId in
      mln_map_set_layer_max_zoom(map, arena.view(layerId), maxZoom, commandId)
    }
  }

  @discardableResult
  func setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility
  ) throws -> UInt64 {
    try styleCommand { map, arena, commandId in
      mln_map_set_layer_visibility(
        map, arena.view(layerId), visibility.rawValue, commandId
      )
    }
  }

  private func styleCommand(
    _ body: (mln_map, NativeInputArena, UnsafeMutablePointer<UInt64>)
      -> mln_status
  ) throws -> UInt64 {
    return try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) {
        try checkStatus(body(requireLiveHandle().raw, arena, $0))
      }.value
    }
  }

  private func layerText(
    _ layerId: String,
    start: (mln_map, mln_buffer_view, UnsafeMutablePointer<mln_operation>)
      -> mln_status,
    take: (mln_operation, UnsafeMutablePointer<mln_buffer>) -> mln_status
  ) async throws -> String {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.layerTextStart(
        requireLiveHandle(), layerId: arena.view(layerId), start: start
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtimeForOperations.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.bufferStringTakeResult(operation, take: take)
    }
  }
}
