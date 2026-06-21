
internal import CMaplibreNativeC

public enum RenderedQueryGeometry: Equatable, Sendable {
  case point(ScreenPoint)
  case box(min: ScreenPoint, max: ScreenPoint)
  case lineString([ScreenPoint])

  var nativeGeometry: NativeRenderedQueryGeometry {
    switch self {
    case let .point(point): .point(point.nativeInput)
    case let .box(min, max): .box(min: min.nativeInput, max: max.nativeInput)
    case let .lineString(points): .lineString(points.map(\.nativeInput))
    }
  }
}

public struct RenderedFeatureQueryOptions: Equatable, Sendable {
  public var layerIds: [String]
  public var filter: JSONValue?

  public init(layerIds: [String] = [], filter: JSONValue? = nil) {
    self.layerIds = layerIds
    self.filter = filter
  }

  var nativeOptions: NativeRenderedFeatureQueryOptions {
    NativeRenderedFeatureQueryOptions(
      layerIds: layerIds,
      filter: filter?.nativeValue
    )
  }
}

public struct SourceFeatureQueryOptions: Equatable, Sendable {
  public var sourceLayerIds: [String]
  public var filter: JSONValue?

  public init(sourceLayerIds: [String] = [], filter: JSONValue? = nil) {
    self.sourceLayerIds = sourceLayerIds
    self.filter = filter
  }

  var nativeOptions: NativeSourceFeatureQueryOptions {
    NativeSourceFeatureQueryOptions(
      sourceLayerIds: sourceLayerIds,
      filter: filter?.nativeValue
    )
  }
}

public struct FeatureStateSelector: Equatable, Sendable {
  public var sourceId: String
  public var sourceLayerId: String?
  public var featureId: String?
  public var stateKey: String?

  public init(
    sourceId: String,
    sourceLayerId: String? = nil,
    featureId: String? = nil,
    stateKey: String? = nil
  ) {
    self.sourceId = sourceId
    self.sourceLayerId = sourceLayerId
    self.featureId = featureId
    self.stateKey = stateKey
  }

  var nativeSelector: NativeFeatureStateSelector {
    NativeFeatureStateSelector(
      sourceId: sourceId,
      sourceLayerId: sourceLayerId,
      featureId: featureId,
      stateKey: stateKey
    )
  }
}

public struct QueriedFeature: Equatable, Sendable {
  public let feature: Feature
  public let sourceId: String?
  public let sourceLayerId: String?
  public let state: JSONValue?

  init(native: NativeQueriedFeature) {
    feature = Feature(native: native.feature)
    sourceId = native.sourceId
    sourceLayerId = native.sourceLayerId
    state = native.state.map(JSONValue.init(native:))
  }
}

extension JSONValue {
  init(native: NativeJSONValue) {
    switch native {
    case .null: self = .null
    case let .bool(value): self = .bool(value)
    case let .uint(value): self = .uint(value)
    case let .int(value): self = .int(value)
    case let .double(value): self = .double(value)
    case let .string(value): self = .string(value)
    case let .array(values): self = .array(values.map(JSONValue.init(native:)))
    case let .object(members): self = .object(members.map { JSONMember(
        key: $0.key,
        value: JSONValue(native: $0.value)
      ) })
    }
  }
}

extension Geometry {
  init(native: NativeGeometry) {
    switch native {
    case .empty: self = .empty
    case let .point(point): self = .point(LatLng(native: point))
    case let .lineString(coordinates): self = .lineString(coordinates
        .map(LatLng.init(native:)))
    case let .polygon(rings): self = .polygon(rings
        .map { $0.map(LatLng.init(native:)) })
    case let .multiPoint(coordinates): self = .multiPoint(coordinates
        .map(LatLng.init(native:)))
    case let .multiLineString(lines): self = .multiLineString(lines
        .map { $0.map(LatLng.init(native:)) })
    case let .multiPolygon(polygons): self = .multiPolygon(polygons
        .map { $0.map { $0.map(LatLng.init(native:)) } })
    case let .geometryCollection(geometries): self =
      .geometryCollection(geometries.map(Geometry.init(native:)))
    }
  }
}

extension FeatureIdentifier {
  init(native: NativeFeatureIdentifier) {
    switch native {
    case .none: self = .none
    case let .uint(value): self = .uint(value)
    case let .int(value): self = .int(value)
    case let .double(value): self = .double(value)
    case let .string(value): self = .string(value)
    }
  }
}

extension Feature {
  init(native: NativeFeature) {
    self.init(
      geometry: Geometry(native: native.geometry),
      properties: native.properties.map { JSONMember(
        key: $0.key,
        value: JSONValue(native: $0.value)
      ) },
      identifier: FeatureIdentifier(native: native.identifier)
    )
  }
}

public enum FeatureExtensionResult: Equatable, Sendable {
  case value(JSONValue)
  case featureCollection([Feature])

  init(native: NativeFeatureExtensionResult) {
    switch native {
    case let .value(value): self = .value(JSONValue(native: value))
    case let .featureCollection(features): self = .featureCollection(features
        .map(Feature.init(native:)))
    }
  }
}

public extension RenderSessionHandle {
  func queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions = RenderedFeatureQueryOptions()
  ) throws -> [QueriedFeature] {
    try mapNativeFailure {
      try geometry.nativeGeometry.withNativeGeometry { nativeGeometry in
        try options.nativeOptions.withNativeOptions { nativeOptions in
          let result = try NativeQuery.renderedFeatures(
            session: requireLivePointer(),
            geometry: nativeGeometry,
            options: nativeOptions
          )
          defer { mln_feature_query_result_destroy(result) }
          return try NativeFeatureQueryResultReader(handle: result)
            .copyFeatures().map(QueriedFeature.init(native:))
        }
      }
    }
  }

  func querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions = SourceFeatureQueryOptions()
  ) throws -> [QueriedFeature] {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let sourceId = arena.view(sourceId)
      return try options.nativeOptions.withNativeOptions { nativeOptions in
        let result = try NativeQuery.sourceFeatures(
          session: requireLivePointer(),
          sourceId: sourceId,
          options: nativeOptions
        )
        defer { mln_feature_query_result_destroy(result) }
        return try NativeFeatureQueryResultReader(handle: result).copyFeatures()
          .map(QueriedFeature.init(native:))
      }
    }
  }

  func queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extensionName: String,
    extensionField: String,
    arguments: JSONValue? = nil
  ) throws -> FeatureExtensionResult {
    try mapNativeFailure {
      let arena = NativeInputArena()
      let result = try NativeQuery.featureExtensions(
        session: requireLivePointer(),
        sourceId: arena.view(sourceId),
        feature: arena.allocateFeature(feature.nativeFeature),
        extensionName: arena.view(extensionName),
        extensionField: arena.view(extensionField),
        arguments: arguments.map { arena.allocate($0.nativeValue) }
      )
      defer { mln_feature_extension_result_destroy(result) }
      return try FeatureExtensionResult(native: NativeQuery
        .featureExtensionResultCopy(result))
    }
  }

  func setFeatureState(selector: FeatureStateSelector,
                       state: JSONValue) throws
  {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try selector.nativeSelector.withNativeSelector { selector in
        try checkStatus(mln_render_session_set_feature_state(
          requireLivePointer(),
          selector,
          arena.allocate(state.nativeValue)
        ))
      }
    }
  }

  func featureState(selector: FeatureStateSelector) throws -> JSONValue? {
    try mapNativeFailure {
      try selector.nativeSelector.withNativeSelector { selector in
        try NativeQuery.featureState(requireLivePointer(), selector: selector)
          .map(JSONValue.init(native:))
      }
    }
  }

  func removeFeatureState(selector: FeatureStateSelector) throws {
    try mapNativeFailure {
      try selector.nativeSelector.withNativeSelector { selector in
        try checkStatus(mln_render_session_remove_feature_state(
          requireLivePointer(),
          selector
        ))
      }
    }
  }
}
