internal import CMaplibreNativeC
import Foundation

enum NativeAmbientCacheOperation: UInt32, Hashable {
  case resetDatabase = 1
  case packDatabase = 2
  case invalidate = 3
  case clear = 4
}

enum NativeOfflineRegionDownloadState: UInt32, Hashable {
  case inactive = 0
  case active = 1
}

enum NativeOfflineRegionDefinition: Equatable {
  case tilePyramid(
    styleURL: String,
    bounds: NativeLatLngBounds,
    minZoom: Double,
    maxZoom: Double,
    pixelRatio: Float,
    includeIdeographs: Bool
  )
  case geometry(
    styleURL: String,
    geometry: NativeGeometry,
    minZoom: Double,
    maxZoom: Double,
    pixelRatio: Float,
    includeIdeographs: Bool
  )

  func withNativeDefinition<Result>(_ body: (
    UnsafePointer<mln_offline_region_definition>
  ) throws
    -> Result) throws -> Result
  {
    let arena = NativeInputArena()
    var definition = mln_offline_region_definition()
    definition.size = UInt32(MemoryLayout<mln_offline_region_definition>.size)
    switch self {
    case let .tilePyramid(
      styleURL,
      bounds,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      return try NativeString.withCString(styleURL) { styleURL in
        definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID.rawValue
        definition.data
          .tile_pyramid = mln_offline_tile_pyramid_region_definition(
            size: UInt32(MemoryLayout<
              mln_offline_tile_pyramid_region_definition
            >
            .size),
            style_url: styleURL,
            bounds: bounds.native,
            min_zoom: minZoom,
            max_zoom: maxZoom,
            pixel_ratio: pixelRatio,
            include_ideographs: includeIdeographs
          )
        return try withUnsafePointer(to: &definition, body)
      }
    case let .geometry(
      styleURL,
      geometry,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      return try NativeString.withCString(styleURL) { styleURL in
        definition.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY.rawValue
        definition.data.geometry = mln_offline_geometry_region_definition(
          size: UInt32(MemoryLayout<mln_offline_geometry_region_definition>
            .size),
          style_url: styleURL,
          geometry: arena.allocateGeometry(geometry),
          min_zoom: minZoom,
          max_zoom: maxZoom,
          pixel_ratio: pixelRatio,
          include_ideographs: includeIdeographs
        )
        return try withUnsafePointer(to: &definition, body)
      }
    }
  }

  init(copying raw: mln_offline_region_definition) throws {
    switch raw.type {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID.rawValue:
      let value = raw.data.tile_pyramid
      self = .tilePyramid(
        styleURL: NativeString.copyCString(value.style_url),
        bounds: NativeLatLngBounds(value.bounds),
        minZoom: value.min_zoom,
        maxZoom: value.max_zoom,
        pixelRatio: value.pixel_ratio,
        includeIdeographs: value.include_ideographs
      )
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY.rawValue:
      let value = raw.data.geometry
      guard let geometry = value.geometry else {
        throw NativeStatusFailure
          .swiftNativeError(
            "offline geometry region definition geometry is null"
          )
      }
      self = try .geometry(
        styleURL: NativeString.copyCString(value.style_url),
        geometry: NativeGeometry(copying: geometry.pointee),
        minZoom: value.min_zoom,
        maxZoom: value.max_zoom,
        pixelRatio: value.pixel_ratio,
        includeIdeographs: value.include_ideographs
      )
    default:
      throw NativeStatusFailure
        .swiftNativeError("unknown offline region definition type \(raw.type)")
    }
  }
}

struct NativeOfflineRegionInfo: Equatable {
  let id: Int64
  let definition: NativeOfflineRegionDefinition
  let metadata: Data

  init(copying raw: mln_offline_region_info) throws {
    id = raw.id
    definition = try NativeOfflineRegionDefinition(copying: raw.definition)
    if raw.metadata_size == 0 {
      metadata = Data()
    } else {
      guard let metadataBytes = raw.metadata else {
        throw NativeStatusFailure
          .swiftNativeError("offline region metadata is null")
      }
      metadata = Data(bytes: metadataBytes, count: raw.metadata_size)
    }
  }
}
