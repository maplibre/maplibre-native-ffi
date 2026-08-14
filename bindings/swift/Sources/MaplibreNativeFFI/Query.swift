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
  ) async throws -> Data {
    let operation = try mapNativeFailure {
      try geometry.nativeGeometry.withNativeGeometry { geometry in
        try options.nativeOptions.withNativeOptions { options in
          try startOperation { session, operation in
            mln_render_session_query_rendered_features_start(
              session, geometry, options, operation
            )
          }
        }
      }
    }
    return try await takeRenderQuery(operation)
  }

  func querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions = SourceFeatureQueryOptions()
  ) async throws -> Data {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try options.nativeOptions.withNativeOptions { options in
        try startOperation { session, operation in
          mln_render_session_query_source_features_start(
            session,
            arena.view(sourceId),
            options,
            operation
          )
        }
      }
    }
    return try await takeRenderQuery(operation)
  }

  func queryFeatureExtension(
    sourceId: String,
    feature: Data,
    extensionName: String,
    extensionField: String,
    arguments: Data? = nil
  ) async throws -> Data {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      let start = {
        (arguments: UnsafePointer<mln_buffer_view>?) throws
        -> NativeOperationHandle in
        try self.startOperation { session, operation in
          mln_render_session_query_feature_extensions_start(
            session,
            arena.view(sourceId),
            arena.view(feature),
            arena.view(extensionName),
            arena.view(extensionField),
            arguments,
            operation
          )
        }
      }
      if let arguments {
        var view = arena.view(arguments)
        return try withUnsafePointer(to: &view, start)
      }
      return try start(nil)
    }
    return try await takeRenderQuery(operation)
  }

  func setFeatureState(
    selector: FeatureStateSelector,
    state: Data
  ) async throws {
    let operation = try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try selector.nativeSelector.withNativeSelector { selector in
        try startOperation { session, operation in
          mln_render_session_set_feature_state_start(
            session,
            selector.pointee.source_id,
            selector.pointee.source_layer_id,
            selector.pointee.feature_id,
            arena.view(state),
            operation
          )
        }
      }
    }
    defer { mln_operation_release(operation.raw) }
    try await waitForOperation(operation)
  }

  func featureState(selector: FeatureStateSelector) async throws -> Data {
    let operation = try mapNativeFailure {
      try selector.nativeSelector.withNativeSelector { selector in
        try startOperation { session, operation in
          mln_render_session_get_feature_state_start(
            session,
            selector.pointee.source_id,
            selector.pointee.source_layer_id,
            selector.pointee.feature_id,
            operation
          )
        }
      }
    }
    defer { mln_operation_release(operation.raw) }
    try await waitForOperation(operation)
    return try mapNativeFailure {
      var buffer: mln_buffer = 0
      try checkStatus(mln_render_session_get_feature_state_take_result(
        operation.raw, &buffer
      ))
      return try NativeMemory.copyBuffer(NativeBufferHandle(raw: buffer))
    }
  }

  func removeFeatureState(selector: FeatureStateSelector) async throws {
    let operation = try mapNativeFailure {
      try selector.nativeSelector.withNativeSelector { selector in
        try startOperation { session, operation in
          mln_render_session_remove_feature_state_start(
            session,
            selector.pointee.source_id,
            selector.pointee.source_layer_id,
            selector.pointee.feature_id,
            selector.pointee.state_key,
            operation
          )
        }
      }
    }
    defer { mln_operation_release(operation.raw) }
    try await waitForOperation(operation)
  }

  private func takeRenderQuery(
    _ operation: NativeOperationHandle
  ) async throws -> Data {
    defer { mln_operation_release(operation.raw) }
    try await waitForOperation(operation)
    return try mapNativeFailure {
      var buffer: mln_buffer = 0
      try checkStatus(mln_render_query_take_result(operation.raw, &buffer))
      return try NativeMemory.copyBuffer(NativeBufferHandle(raw: buffer))
    }
  }
}
