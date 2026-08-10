internal import CMaplibreNativeC
import Foundation

public enum RenderedQueryGeometry: Equatable, Sendable {
  case point(ScreenPoint)
  /// Screen-space box in logical map pixels. Corners may be given in any order
  /// and may extend past the viewport; queries normalize and clip them.
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
  public var filter: Data?

  public init(layerIds: [String] = [], filter: Data? = nil) {
    self.layerIds = layerIds
    self.filter = filter
  }

  var nativeOptions: NativeRenderedFeatureQueryOptions {
    NativeRenderedFeatureQueryOptions(layerIds: layerIds, filter: filter)
  }
}

public struct SourceFeatureQueryOptions: Equatable, Sendable {
  public var sourceLayerIds: [String]
  public var filter: Data?

  public init(sourceLayerIds: [String] = [], filter: Data? = nil) {
    self.sourceLayerIds = sourceLayerIds
    self.filter = filter
  }

  var nativeOptions: NativeSourceFeatureQueryOptions {
    NativeSourceFeatureQueryOptions(
      sourceLayerIds: sourceLayerIds,
      filter: filter
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

public extension RenderSessionHandle {
  func queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions = RenderedFeatureQueryOptions()
  ) throws -> Data {
    try mapNativeFailure {
      try geometry.nativeGeometry.withNativeGeometry { nativeGeometry in
        try options.nativeOptions.withNativeOptions { nativeOptions in
          try NativeQuery.renderedFeatures(
            session: requireLiveHandle(),
            geometry: nativeGeometry,
            options: nativeOptions
          )
        }
      }
    }
  }

  func querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions = SourceFeatureQueryOptions()
  ) throws -> Data {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try options.nativeOptions.withNativeOptions { nativeOptions in
        try NativeQuery.sourceFeatures(
          session: self.requireLiveHandle(),
          sourceId: arena.view(sourceId),
          options: nativeOptions
        )
      }
    }
  }

  func queryFeatureExtension(
    sourceId: String,
    feature: Data,
    extensionName: String,
    extensionField: String,
    arguments: Data? = nil
  ) throws -> Data {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let call = { (arguments: UnsafePointer<mln_buffer_view>?) throws in
        try NativeQuery.featureExtensions(
          session: self.requireLiveHandle(),
          sourceId: arena.view(sourceId),
          feature: arena.view(feature),
          extensionName: arena.view(extensionName),
          extensionField: arena.view(extensionField),
          arguments: arguments
        )
      }
      guard let arguments else { return try call(nil) }
      var argumentsView = arena.view(arguments)
      return try withUnsafePointer(to: &argumentsView, call)
    }
  }

  func setFeatureState(selector: FeatureStateSelector, state: Data) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try selector.nativeSelector.withNativeSelector { selector in
        try checkStatus(mln_render_session_set_feature_state(
          requireLiveHandle().raw,
          selector,
          arena.view(state)
        ))
      }
    }
  }

  func featureState(selector: FeatureStateSelector) throws -> Data {
    try mapNativeFailure {
      try selector.nativeSelector.withNativeSelector { selector in
        try NativeQuery.featureState(requireLiveHandle(), selector: selector)
      }
    }
  }

  func removeFeatureState(selector: FeatureStateSelector) throws {
    try mapNativeFailure {
      try selector.nativeSelector.withNativeSelector { selector in
        try checkStatus(mln_render_session_remove_feature_state(
          requireLiveHandle().raw,
          selector
        ))
      }
    }
  }
}
