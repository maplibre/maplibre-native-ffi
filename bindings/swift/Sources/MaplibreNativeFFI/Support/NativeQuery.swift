internal import CMaplibreNativeC
import Foundation

enum NativeQuery {
  static func renderedFeatures(
    session: NativeRenderSessionHandle,
    geometry: UnsafePointer<mln_rendered_query_geometry>,
    options: UnsafePointer<mln_rendered_feature_query_options>?
  ) throws -> [QueriedFeature] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { output in
      try checkStatus(mln_render_session_query_rendered_features(
        session.raw, geometry, options, output
      ))
    }.value
    let list = NativeQueriedFeatureListHandle(raw: listValue)
    return try copyQueriedFeatureList(list)
  }

  static func sourceFeatures(
    session: NativeRenderSessionHandle,
    sourceId: mln_buffer_view,
    options: UnsafePointer<mln_source_feature_query_options>?
  ) throws -> [QueriedFeature] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { output in
      try checkStatus(mln_render_session_query_source_features(
        session.raw, sourceId, options, output
      ))
    }.value
    let list = NativeQueriedFeatureListHandle(raw: listValue)
    return try copyQueriedFeatureList(list)
  }

  static func featureExtensions(
    session: NativeRenderSessionHandle,
    sourceId: mln_buffer_view,
    feature: mln_buffer_view,
    extensionName: mln_buffer_view,
    extensionField: mln_buffer_view,
    arguments: UnsafePointer<mln_buffer_view>?
  ) throws -> Data {
    try copyResult { output in
      try checkStatus(mln_render_session_query_feature_extensions(
        session.raw, sourceId, feature, extensionName, extensionField,
        arguments, output
      ))
    }
  }

  static func featureState(
    _ session: NativeRenderSessionHandle,
    selector: UnsafePointer<mln_feature_state_selector>
  ) throws -> Data {
    try copyResult { output in
      try checkStatus(mln_render_session_get_feature_state(
        session.raw, selector, output
      ))
    }
  }

  private static func copyResult(
    _ body: (UnsafeMutablePointer<UInt64>) throws -> Void
  ) throws -> Data {
    let raw = try NativeMemory.withTemporary(UInt64(0), body).value
    return try NativeMemory.copyBuffer(NativeBufferHandle(raw: raw))
  }

  private static func copyQueriedFeatureList(
    _ list: NativeQueriedFeatureListHandle
  ) throws -> [QueriedFeature] {
    guard !list.isNull else {
      throw NativeStatusFailure
        .swiftNativeError("queried feature list was null")
    }
    defer { mln_queried_feature_list_destroy(list.raw) }
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_queried_feature_list_count(list.raw, count))
    }.value
    return try (0 ..< count).map { index in
      var hit = mln_queried_feature_default()
      try checkStatus(mln_queried_feature_list_get(list.raw, index, &hit))
      return try copyQueriedFeature(hit)
    }
  }

  private static func copyQueriedFeature(_ hit: mln_queried_feature) throws
    -> QueriedFeature
  {
    let sourceId = hasField(hit, MLN_QUERIED_FEATURE_SOURCE_ID.rawValue)
      ? try NativeString.copyUTF8(
        data: hit.source_id.data,
        size: hit.source_id.size
      )
      : nil
    let sourceLayerId = hasField(
      hit,
      MLN_QUERIED_FEATURE_SOURCE_LAYER_ID.rawValue
    )
      ? try NativeString.copyUTF8(
        data: hit.source_layer_id.data,
        size: hit.source_layer_id.size
      )
      : nil
    let state = hasField(hit, MLN_QUERIED_FEATURE_STATE.rawValue)
      ? try copyView(hit.state)
      : nil
    return try QueriedFeature(
      feature: copyView(hit.feature),
      sourceId: sourceId,
      sourceLayerId: sourceLayerId,
      state: state
    )
  }

  private static func hasField(
    _ hit: mln_queried_feature,
    _ field: UInt32
  ) -> Bool {
    (hit.fields & field) != 0
  }

  private static func copyView(_ view: mln_buffer_view) throws -> Data {
    guard view.size > 0 else { return Data() }
    guard let data = view.data else {
      throw NativeStatusFailure.swiftNativeError(
        "buffer view has nil data with non-zero size"
      )
    }
    return Data(bytes: data, count: view.size)
  }
}
