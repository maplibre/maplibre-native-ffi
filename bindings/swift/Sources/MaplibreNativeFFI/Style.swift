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
  /// Slices requested tiles inline during the update pass, so data installed
  /// through ``MapHandle/setGeoJSONSourceData(sourceId:data:)`` reaches the
  /// next rendered frame.
  /// ``MapHandle/setGeoJSONSourceSynchronousTiling(sourceId:enabled:)``
  /// overrides this at runtime.
  public var synchronousTiling: Bool?

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
      synchronousTiling: synchronousTiling
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

/// Prepared GeoJSON source data: one UTF-8 GeoJSON document parsed and tiled
/// into an immutable index that map calls install when adding or updating a
/// GeoJSON source.
///
/// Preparation needs no runtime or map and is callable from any thread, so a
/// host can prepare data concurrently with map work. Install calls borrow the
/// handle, so one prepared value may be installed on any number of sources and
/// closed at any time afterward; closing never invalidates a source the data
/// was installed on.
///
/// The prepared native value is immutable and every operation on this handle
/// is callable from any thread, with the lock-guarded handle state carrying
/// the shared mutable state, so the handle is safe to share across threads.
public final class GeoJSONSourceDataHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeGeoJSONSourceDataHandle>

  /// Parses and tiles one complete GeoJSON document under `options`, which
  /// bake into the prepared data; a source added with it adopts them.
  public init(
    data: Data,
    options: StyleGeoJSONSourceOptions = StyleGeoJSONSourceOptions()
  ) throws {
    let prepared = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeStyle.createGeoJSONSourceData(
        data: arena.view(data),
        options: options.nativeOptions
      )
    }
    handle = try NativeHandleBox(
      typeName: "GeoJSONSourceDataHandle",
      handle: prepared
    )
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() throws {
    try handle.closeOnce { data in
      mln_geojson_source_data_destroy(data.raw)
    }
  }

  /// Runs `use` after checking that this wrapper still owns the handle.
  func withLiveHandle<T>(
    _ use: (NativeGeoJSONSourceDataHandle) throws -> T
  ) throws -> T {
    try handle.withLive(use)
  }
}

public extension MapHandle {
  @discardableResult
  func addStyleSourceJSON(
    sourceId: String,
    sourceJSON: Data
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_add_style_source_json(
        $0, $1.view(sourceId), $1.view(sourceJSON), $2
      )
    }
  }

  @discardableResult
  func removeStyleSource(
    _ sourceId: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_remove_style_source($0, $1.view(sourceId), $2)
    }
  }

  func styleSourceInfo(_ sourceId: String) async throws -> StyleSourceInfo? {
    try await styleQuery(
      { mln_map_get_style_source_info($0, $1.view(sourceId), $2) },
      convert: Self.copyStyleSourceInfo
    )
  }

  func styleSourceAttribution(_ sourceId: String) async throws -> String? {
    try await styleQuery(
      { mln_map_copy_style_source_attribution($0, $1.view(sourceId), $2) },
      convert: Self.copyOptionalString
    )
  }

  func styleSourceURL(_ sourceId: String) async throws -> String? {
    try await styleQuery(
      { mln_map_copy_style_source_url($0, $1.view(sourceId), $2) },
      convert: Self.copyOptionalString
    )
  }

  func styleSourceTileURLs(_ sourceId: String) async throws -> [String]? {
    guard let info = try await styleSourceInfo(sourceId) else { return nil }
    return info.tileJSON?.tileURLs
  }

  func styleSourceIds() async throws -> [String] {
    try await styleQuery(
      { mln_map_list_style_source_ids($0, $2) },
      convert: Self.copyStrings
    )
  }

  @discardableResult
  func addGeoJSONSourceURL(
    sourceId: String,
    url: String,
    options: StyleGeoJSONSourceOptions = StyleGeoJSONSourceOptions()
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try options.nativeOptions.withNativeOptions { options in
        try startCommand {
          mln_map_add_geojson_source_url(
            $0, arena.view(sourceId), arena.view(url), options, $1
          )
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func addGeoJSONSourceData(
    sourceId: String,
    data: GeoJSONSourceDataHandle
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try data.withLiveHandle { prepared in
        try startCommand {
          mln_map_add_geojson_source_data(
            $0, arena.view(sourceId), prepared.raw, $1
          )
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func setGeoJSONSourceURL(
    sourceId: String,
    url: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_geojson_source_url(
        $0, $1.view(sourceId), $1.view(url), $2
      )
    }
  }

  @discardableResult
  func setGeoJSONSourceData(
    sourceId: String,
    data: GeoJSONSourceDataHandle
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try data.withLiveHandle { prepared in
        try startCommand {
          mln_map_set_geojson_source_data(
            $0, arena.view(sourceId), prepared.raw, $1
          )
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func setGeoJSONSourceSynchronousTiling(
    sourceId: String,
    enabled: Bool
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_geojson_source_synchronous_tiling(
        $0, $1.view(sourceId), enabled, $2
      )
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
      UnsafePointer<mln_completion>
    ) -> mln_status
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try options.nativeOptions.withNativeOptions { options in
        try startCommand {
          add($0, arena.view(sourceId), arena.view(url), options, $1)
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
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
      UnsafePointer<mln_completion>
    ) -> mln_status
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let views = tiles.map { arena.view($0) }
      return try views.withUnsafeBufferPointer { views in
        try options.nativeOptions.withNativeOptions { options in
          try startCommand {
            add(
              $0, arena.view(sourceId), views.baseAddress, views.count,
              options, $1
            )
          }
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func addVectorSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) async throws -> CommandCompletion {
    try await addTiledSourceURL(
      sourceId: sourceId, url: url, options: options,
      add: mln_map_add_vector_source_url
    )
  }

  @discardableResult
  func addVectorSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) async throws -> CommandCompletion {
    try await addTiledSourceTiles(
      sourceId: sourceId, tiles: tiles, options: options,
      add: mln_map_add_vector_source_tiles
    )
  }

  @discardableResult
  func addRasterSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) async throws -> CommandCompletion {
    try await addTiledSourceURL(
      sourceId: sourceId, url: url, options: options,
      add: mln_map_add_raster_source_url
    )
  }

  @discardableResult
  func addRasterSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) async throws -> CommandCompletion {
    try await addTiledSourceTiles(
      sourceId: sourceId, tiles: tiles, options: options,
      add: mln_map_add_raster_source_tiles
    )
  }

  @discardableResult
  func addRasterDEMSourceURL(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) async throws -> CommandCompletion {
    try await addTiledSourceURL(
      sourceId: sourceId, url: url, options: options,
      add: mln_map_add_raster_dem_source_url
    )
  }

  @discardableResult
  func addRasterDEMSourceTiles(
    sourceId: String,
    tiles: [String],
    options: StyleTileSourceOptions = StyleTileSourceOptions()
  ) async throws -> CommandCompletion {
    try await addTiledSourceTiles(
      sourceId: sourceId, tiles: tiles, options: options,
      add: mln_map_add_raster_dem_source_tiles
    )
  }

  @discardableResult
  func addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions
  ) async throws -> CommandCompletion {
    let fetchTile: NativeCustomGeometrySourceCallbacks.TileCallback = {
      options.fetchTile(CanonicalTileID(native: $0))
    }
    let cancelTile: NativeCustomGeometrySourceCallbacks.TileCallback?
    if let callback = options.cancelTile {
      cancelTile = { callback(CanonicalTileID(native: $0)) }
    } else {
      cancelTile = nil
    }
    let callbacks = NativeCustomGeometrySourceCallbacks(
      fetchTile: fetchTile,
      cancelTile: cancelTile
    )
    let future: NativeFuture<CommandCompletion>
    do {
      future = try mapNativeFailure {
        let arena = NativeInputArena()
        defer { withExtendedLifetime(arena) {} }
        return try options.nativeOptions(callbacks: callbacks)
          .withNativeOptions { native in
            try startCommand {
              mln_map_add_custom_geometry_source(
                $0, arena.view(sourceId), native, $1
              )
            }
          }
      }
    } catch {
      callbacks.release()
      throw error
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileID,
    data: Data
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_custom_geometry_source_tile_data(
        $0, $1.view(sourceId), tileId.nativeTileID.native, $1.view(data), $2
      )
    }
  }

  @discardableResult
  func invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileID
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_invalidate_custom_geometry_source_tile(
        $0, $1.view(sourceId), tileId.nativeTileID.native, $2
      )
    }
  }

  @discardableResult
  func invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_invalidate_custom_geometry_source_region(
        $0, $1.view(sourceId), bounds.nativeInput.native, $2
      )
    }
  }

  @discardableResult
  func setStyleImage(
    imageId: String,
    image: StyleRGBA8Image,
    options: StyleImageOptions = StyleImageOptions()
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try image.nativeImage.withNativeImage { image in
        try options.nativeOptions.withNativeOptions { options in
          try startCommand {
            mln_map_set_style_image(
              $0, arena.view(imageId), image, options, $1
            )
          }
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func removeStyleImage(
    _ imageId: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_remove_style_image($0, $1.view(imageId), $2)
    }
  }

  func styleImageInfo(_ imageId: String) async throws -> StyleImageInfo? {
    try await styleImageResult(imageId).map(\.info)
  }

  func styleImage(_ imageId: String) async throws -> StyleImage? {
    try await styleImageResult(imageId)
  }

  func styleImageStretches(
    _ imageId: String
  ) async throws -> (stretchX: [ImageStretch], stretchY: [ImageStretch])? {
    try await styleQuery(
      { mln_map_get_style_image_info($0, $1.view(imageId), $2) }
    ) { result in
      guard result.pointee.value_count > 0 else { return nil }
      let raw: mln_style_image_result = try NativeCompletion.value(result)
      return try (
        Self.copyStretches(raw.stretch_x, count: raw.stretch_x_count),
        Self.copyStretches(raw.stretch_y, count: raw.stretch_y_count)
      )
    }
  }

  private func styleImageResult(_ imageId: String) async throws -> StyleImage? {
    try await styleQuery(
      { mln_map_get_style_image_info($0, $1.view(imageId), $2) },
      convert: Self.copyStyleImage
    )
  }

  @discardableResult
  func addImageSourceURL(
    sourceId: String,
    coordinates: [LatLng],
    url: String
  ) async throws -> CommandCompletion {
    try await imageSourceCommand(coordinates) {
      mln_map_add_image_source_url(
        $0, $1.view(sourceId), $2, $3, $1.view(url), $4
      )
    }
  }

  @discardableResult
  func addImageSourceImage(
    sourceId: String,
    coordinates: [LatLng],
    image: StyleRGBA8Image
  ) async throws -> CommandCompletion {
    let raw = try imageSourceCoordinates(coordinates)
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try raw.withUnsafeBufferPointer { coordinates in
        try image.nativeImage.withNativeImage { image in
          try startCommand {
            mln_map_add_image_source_image(
              $0, arena.view(sourceId), coordinates.baseAddress,
              coordinates.count, image, $1
            )
          }
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func setImageSourceURL(
    sourceId: String,
    url: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_image_source_url(
        $0, $1.view(sourceId), $1.view(url), $2
      )
    }
  }

  @discardableResult
  func setImageSourceImage(
    sourceId: String,
    image: StyleRGBA8Image
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try image.nativeImage.withNativeImage { image in
        try startCommand {
          mln_map_set_image_source_image($0, arena.view(sourceId), image, $1)
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  func setImageSourceCoordinates(
    sourceId: String,
    coordinates: [LatLng]
  ) async throws -> CommandCompletion {
    try await imageSourceCommand(coordinates) {
      mln_map_set_image_source_coordinates(
        $0, $1.view(sourceId), $2, $3, $4
      )
    }
  }

  func imageSourceCoordinates(sourceId: String) async throws -> [LatLng]? {
    try await styleQuery(
      { mln_map_get_image_source_coordinates($0, $1.view(sourceId), $2) }
    ) { result in
      let values = try NativeCompletion.values(result, as: mln_lat_lng.self)
      guard !values.isEmpty else { return nil }
      guard values.count == 4 else {
        throw NativeStatusFailure.swiftNativeError(
          "image source completion did not contain four coordinates"
        )
      }
      return values.map { LatLng(native: NativeLatLng($0)) }
    }
  }

  @discardableResult
  func addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = nil
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_add_hillshade_layer(
        $0, $1.view(layerId), $1.view(sourceId),
        $1.view(beforeLayerId ?? ""), $2
      )
    }
  }

  @discardableResult
  func addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = nil
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_add_color_relief_layer(
        $0, $1.view(layerId), $1.view(sourceId),
        $1.view(beforeLayerId ?? ""), $2
      )
    }
  }

  @discardableResult
  func addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String? = nil
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_add_location_indicator_layer(
        $0, $1.view(layerId), $1.view(beforeLayerId ?? ""), $2
      )
    }
  }

  @discardableResult
  func setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_location_indicator_location(
        $0, $1.view(layerId), coordinate.nativeInput.native, altitude, $2
      )
    }
  }

  @discardableResult
  func setLocationIndicatorBearing(
    layerId: String,
    bearing: Double
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_location_indicator_bearing(
        $0, $1.view(layerId), bearing, $2
      )
    }
  }

  @discardableResult
  func setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_location_indicator_accuracy_radius(
        $0, $1.view(layerId), radius, $2
      )
    }
  }

  @discardableResult
  func setLocationIndicatorImageName(
    layerId: String,
    kind: LocationIndicatorImageKind,
    imageId: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_location_indicator_image_name(
        $0, $1.view(layerId), kind.rawValue, $1.view(imageId), $2
      )
    }
  }

  @discardableResult
  func addStyleLayerJSON(
    _ layerJSON: Data,
    beforeLayerId: String? = nil
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_add_style_layer_json(
        $0, $1.view(layerJSON), $1.view(beforeLayerId ?? ""), $2
      )
    }
  }

  @discardableResult
  func removeStyleLayer(
    _ layerId: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_remove_style_layer($0, $1.view(layerId), $2)
    }
  }

  func styleLayerInfo(_ layerId: String) async throws -> StyleLayerInfo? {
    try await styleQuery(
      { mln_map_get_style_layer_info($0, $1.view(layerId), $2) },
      convert: Self.copyStyleLayerInfo
    )
  }

  func styleLayerIds() async throws -> [String] {
    try await styleQuery(
      { mln_map_list_style_layer_ids($0, $2) },
      convert: Self.copyStrings
    )
  }

  @discardableResult
  func moveStyleLayer(
    _ layerId: String,
    beforeLayerId: String? = nil
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_move_style_layer(
        $0, $1.view(layerId), $1.view(beforeLayerId ?? ""), $2
      )
    }
  }

  func styleLayerJSON(_ layerId: String) async throws -> Data? {
    try await styleQuery(
      { mln_map_get_style_layer_json($0, $1.view(layerId), $2) },
      convert: Self.copyOptionalData
    )
  }

  @discardableResult
  func setStyleLightJSON(
    _ lightJSON: Data
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_style_light_json($0, $1.view(lightJSON), $2)
    }
  }

  @discardableResult
  func setStyleLightProperty(
    _ propertyName: String,
    value: Data
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_style_light_property(
        $0, $1.view(propertyName), $1.view(value), $2
      )
    }
  }

  func styleLightProperty(_ propertyName: String) async throws -> Data? {
    try await styleQuery(
      { mln_map_get_style_light_property($0, $1.view(propertyName), $2) },
      convert: Self.copyOptionalData
    )
  }

  @discardableResult
  func setStyleTransitionOptions(
    _ options: StyleTransitionOptions
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      try options.nativeOptions.withNativeOptions { native in
        try startCommand {
          mln_map_set_style_transition_options($0, native, $1)
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  func styleTransitionOptions() async throws -> StyleTransitionOptions {
    try await styleQuery(
      { mln_map_get_style_transition_options($0, $2) }
    ) { result in
      try StyleTransitionOptions(
        native: NativeStyleTransitionOptions(
          NativeCompletion.value(result)
        )
      )
    }
  }

  @discardableResult
  func setLayerProperty(
    layerId: String,
    propertyName: String,
    value: Data
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_layer_property(
        $0, $1.view(layerId), $1.view(propertyName), $1.view(value), $2
      )
    }
  }

  func layerProperty(
    layerId: String,
    propertyName: String
  ) async throws -> Data? {
    try await styleQuery(
      {
        mln_map_get_layer_property(
          $0, $1.view(layerId), $1.view(propertyName), $2
        )
      },
      convert: Self.copyOptionalData
    )
  }

  @discardableResult
  func setLayerFilter(
    layerId: String,
    filter: Data?
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      guard let filter else {
        return try startCommand {
          mln_map_set_layer_filter($0, arena.view(layerId), nil, $1)
        }
      }
      var view = arena.view(filter)
      return try withUnsafePointer(to: &view) { view in
        try startCommand {
          mln_map_set_layer_filter($0, arena.view(layerId), view, $1)
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  func layerFilter(_ layerId: String) async throws -> Data? {
    try await styleQuery(
      { mln_map_get_layer_filter($0, $1.view(layerId), $2) },
      convert: Self.copyOptionalData
    )
  }

  @discardableResult
  func setLayerSourceLayer(
    layerId: String,
    sourceLayer: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_layer_source_layer(
        $0, $1.view(layerId), $1.view(sourceLayer), $2
      )
    }
  }

  func layerSourceLayer(_ layerId: String) async throws -> String {
    try await styleQuery(
      { mln_map_copy_layer_source_layer($0, $1.view(layerId), $2) },
      convert: NativeCompletion.string
    )
  }

  @discardableResult
  func setLayerSourceId(
    layerId: String,
    sourceId: String
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_layer_source_id(
        $0, $1.view(layerId), $1.view(sourceId), $2
      )
    }
  }

  func layerSourceId(_ layerId: String) async throws -> String {
    try await styleQuery(
      { mln_map_copy_layer_source_id($0, $1.view(layerId), $2) },
      convert: NativeCompletion.string
    )
  }

  @discardableResult
  func setLayerMinZoom(
    layerId: String,
    minZoom: Double
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_layer_min_zoom($0, $1.view(layerId), minZoom, $2)
    }
  }

  @discardableResult
  func setLayerMaxZoom(
    layerId: String,
    maxZoom: Double
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_layer_max_zoom($0, $1.view(layerId), maxZoom, $2)
    }
  }

  @discardableResult
  func setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility
  ) async throws -> CommandCompletion {
    try await styleCommand {
      mln_map_set_layer_visibility(
        $0, $1.view(layerId), visibility.rawValue, $2
      )
    }
  }

  private func styleCommand(
    _ body: (
      mln_map,
      NativeInputArena,
      UnsafePointer<mln_completion>
    ) -> mln_status
  ) async throws -> CommandCompletion {
    let future = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try startCommand { body($0, arena, $1) }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  private func styleQuery<Value: Sendable>(
    _ body: (
      mln_map,
      NativeInputArena,
      UnsafePointer<mln_completion>
    ) -> mln_status,
    convert: @escaping (
      UnsafePointer<mln_completion_result>
    ) throws -> Value
  ) async throws -> Value {
    let future = try mapNativeFailure {
      let map = try requireLiveHandle()
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeCompletion.start(
        { body(map.raw, arena, $0) },
        convert: convert
      )
    }
    return try await mapNativeFailure { try await future.value() }
  }

  private func imageSourceCommand(
    _ coordinates: [LatLng],
    _ body: (
      mln_map,
      NativeInputArena,
      UnsafePointer<mln_lat_lng>?,
      Int,
      UnsafePointer<mln_completion>
    ) -> mln_status
  ) async throws -> CommandCompletion {
    let raw = try imageSourceCoordinates(coordinates)
    let future = try mapNativeFailure {
      let map = try requireLiveHandle()
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try raw.withUnsafeBufferPointer { coordinates in
        try NativeCompletion.startCommand {
          body(
            map.raw, arena, coordinates.baseAddress, coordinates.count, $0
          )
        }
      }
    }
    return try await mapNativeFailure { try await future.value() }
  }

  private func imageSourceCoordinates(
    _ coordinates: [LatLng]
  ) throws -> [mln_lat_lng] {
    guard coordinates.count == 4 else {
      throw MaplibreError.invalidArgument(
        "image source coordinates must contain exactly 4 coordinates"
      )
    }
    return coordinates.map { $0.nativeInput.native }
  }

  private static func copyOptionalData(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> Data? {
    guard result.pointee.value_count > 0 else { return nil }
    return try NativeCompletion.data(result)
  }

  private static func copyOptionalString(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> String? {
    guard result.pointee.value_count > 0 else { return nil }
    return try NativeCompletion.string(result)
  }

  private static func copyStrings(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> [String] {
    try NativeCompletion.values(result, as: mln_buffer_view.self).map {
      try NativeString.copyUTF8(data: $0.data, size: $0.size)
    }
  }

  private static func copyStyleSourceInfo(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> StyleSourceInfo? {
    guard result.pointee.value_count > 0 else { return nil }
    let raw: mln_style_source_result = try NativeCompletion.value(result)
    let has: (UInt32) -> Bool = { (raw.info.fields & $0) != 0 }
    let attribution = raw.info.has_attribution
      ? try NativeString.copyUTF8(
        data: raw.attribution.data, size: raw.attribution.size
      ) : nil
    let url = has(MLN_STYLE_SOURCE_INFO_URL.rawValue)
      ? try NativeString.copyUTF8(data: raw.url.data, size: raw.url.size) : nil
    let tileURLs: [String]
    if raw.tile_url_count == 0 {
      tileURLs = []
    } else {
      guard let values = raw.tile_urls else {
        throw NativeStatusFailure.swiftNativeError(
          "source completion returned a null tile URL array"
        )
      }
      tileURLs = try UnsafeBufferPointer(
        start: values, count: raw.tile_url_count
      ).map { try NativeString.copyUTF8(data: $0.data, size: $0.size) }
    }
    return StyleSourceInfo(native: NativeStyle.sourceInfo(
      fixed: raw.info,
      attribution: attribution,
      url: url,
      tileURLs: tileURLs
    ))
  }

  private static func copyStyleLayerInfo(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> StyleLayerInfo? {
    guard result.pointee.value_count > 0 else { return nil }
    let raw: mln_style_layer_result = try NativeCompletion.value(result)
    let has: (UInt32) -> Bool = { (raw.info.fields & $0) != 0 }
    return try StyleLayerInfo(
      type: NativeString.copyUTF8(
        data: raw.info.type.data, size: raw.info.type.size
      ),
      minZoom: raw.info.min_zoom,
      maxZoom: raw.info.max_zoom,
      visibility: StyleLayerVisibility(rawValue: raw.info.visibility),
      sourceId: has(MLN_STYLE_LAYER_INFO_SOURCE_ID.rawValue)
        ? NativeString.copyUTF8(
          data: raw.source_id.data, size: raw.source_id.size
        ) : nil,
      sourceLayer: has(MLN_STYLE_LAYER_INFO_SOURCE_LAYER.rawValue)
        ? NativeString.copyUTF8(
          data: raw.source_layer.data, size: raw.source_layer.size
        ) : nil
    )
  }

  private static func copyStyleImage(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> StyleImage? {
    guard result.pointee.value_count > 0 else { return nil }
    let raw: mln_style_image_result = try NativeCompletion.value(result)
    let info = StyleImageInfo(native: NativeStyleImageInfo(raw.info))
    let pixels: [UInt8]
    if raw.pixels.size == 0 {
      pixels = []
    } else {
      guard let data = raw.pixels.data else {
        throw NativeStatusFailure.swiftNativeError(
          "style image completion returned null pixels"
        )
      }
      pixels = Array(UnsafeBufferPointer(
        start: data.assumingMemoryBound(to: UInt8.self),
        count: raw.pixels.size
      ))
    }
    return StyleImage(info: info, pixels: pixels)
  }

  private static func copyStretches(
    _ values: UnsafePointer<mln_image_stretch>?,
    count: Int
  ) throws -> [ImageStretch] {
    guard count > 0 else { return [] }
    guard let values else {
      throw NativeStatusFailure.swiftNativeError(
        "style image completion returned a null stretch array"
      )
    }
    return UnsafeBufferPointer(start: values, count: count).map {
      ImageStretch(from: $0.from, to: $0.to)
    }
  }
}
