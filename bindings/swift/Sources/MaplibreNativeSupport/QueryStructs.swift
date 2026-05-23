import CMaplibreNativeC

public enum NativeRenderedQueryGeometry: Equatable, Sendable {
  case point(NativeScreenPoint)
  case box(min: NativeScreenPoint, max: NativeScreenPoint)
  case lineString([NativeScreenPoint])

  public func withNativeGeometry<Result>(
    _ body: (UnsafePointer<mln_rendered_query_geometry>) throws -> Result
  ) throws -> Result {
    switch self {
    case .point(let point):
      var geometry = mln_rendered_query_geometry_point(point.native)
      return try withUnsafePointer(to: &geometry, body)
    case .box(let min, let max):
      var geometry = mln_rendered_query_geometry_box(mln_screen_box(min: min.native, max: max.native))
      return try withUnsafePointer(to: &geometry, body)
    case .lineString(let points):
      let nativePoints = points.map(\.native)
      return try nativePoints.withUnsafeBufferPointer { buffer in
        var geometry = mln_rendered_query_geometry_line_string(buffer.baseAddress, buffer.count)
        return try withUnsafePointer(to: &geometry, body)
      }
    }
  }
}

public struct NativeRenderedFeatureQueryOptions: Equatable, Sendable {
  public let layerIds: [String]
  public let filter: NativeJSONValue?

  public init(layerIds: [String] = [], filter: NativeJSONValue? = nil) {
    self.layerIds = layerIds
    self.filter = filter
  }

  public func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_rendered_feature_query_options>?) throws -> Result
  ) throws -> Result {
    if layerIds.isEmpty, filter == nil { return try body(nil) }
    let arena = NativeJSONArena()
    let layerViews = layerIds.map { arena.view($0) }
    return try layerViews.withUnsafeBufferPointer { layerViews in
      var options = mln_rendered_feature_query_options_default()
      if !self.layerIds.isEmpty {
        options.fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS.rawValue
        options.layer_ids = layerViews.baseAddress
        options.layer_id_count = layerViews.count
      }
      if let filter {
        options.filter = arena.allocate(filter)
      }
      return try withUnsafePointer(to: &options, body)
    }
  }
}

public struct NativeSourceFeatureQueryOptions: Equatable, Sendable {
  public let sourceLayerIds: [String]
  public let filter: NativeJSONValue?

  public init(sourceLayerIds: [String] = [], filter: NativeJSONValue? = nil) {
    self.sourceLayerIds = sourceLayerIds
    self.filter = filter
  }

  public func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_source_feature_query_options>?) throws -> Result
  ) throws -> Result {
    if sourceLayerIds.isEmpty, filter == nil { return try body(nil) }
    let arena = NativeJSONArena()
    let layerViews = sourceLayerIds.map { arena.view($0) }
    return try layerViews.withUnsafeBufferPointer { layerViews in
      var options = mln_source_feature_query_options_default()
      if !self.sourceLayerIds.isEmpty {
        options.fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS.rawValue
        options.source_layer_ids = layerViews.baseAddress
        options.source_layer_id_count = layerViews.count
      }
      if let filter {
        options.filter = arena.allocate(filter)
      }
      return try withUnsafePointer(to: &options, body)
    }
  }
}

public struct NativeFeatureQueryResultReader {
  public let handle: OpaquePointer

  public init(handle: OpaquePointer) {
    self.handle = handle
  }

  public func copyFeatures() throws -> [NativeQueriedFeature] {
    let count = try CAPI.featureQueryResultCount(handle)
    return try (0..<count).map { index in
      try CAPI.featureQueryResultGet(handle, index: index)
    }
  }
}
