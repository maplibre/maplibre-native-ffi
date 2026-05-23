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

public struct NativeFeatureStateSelector: Equatable, Sendable {
  public let sourceId: String
  public let sourceLayerId: String?
  public let featureId: String?
  public let stateKey: String?

  public init(sourceId: String, sourceLayerId: String? = nil, featureId: String? = nil, stateKey: String? = nil) {
    self.sourceId = sourceId
    self.sourceLayerId = sourceLayerId
    self.featureId = featureId
    self.stateKey = stateKey
  }

  public func withNativeSelector<Result>(_ body: (UnsafePointer<mln_feature_state_selector>) throws -> Result) throws -> Result {
    let arena = NativeJSONArena()
    var selector = mln_feature_state_selector()
    selector.size = UInt32(MemoryLayout<mln_feature_state_selector>.size)
    selector.source_id = arena.view(sourceId)
    if let sourceLayerId {
      selector.fields |= MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID.rawValue
      selector.source_layer_id = arena.view(sourceLayerId)
    }
    if let featureId {
      selector.fields |= MLN_FEATURE_STATE_SELECTOR_FEATURE_ID.rawValue
      selector.feature_id = arena.view(featureId)
    }
    if let stateKey {
      selector.fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY.rawValue
      selector.state_key = arena.view(stateKey)
    }
    return try withUnsafePointer(to: &selector, body)
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
