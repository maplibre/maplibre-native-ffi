internal import CMaplibreNativeC

enum NativeQuery {
  static func renderedFeatures(
    session: NativeRenderSessionHandle,
    geometry: UnsafePointer<mln_rendered_query_geometry>,
    options: UnsafePointer<mln_rendered_feature_query_options>?
  ) throws -> NativeFeatureQueryResultHandle {
    try NativeHandleFactory
      .create(nullDiagnostic: "rendered feature query returned a null result") { outHandle in
        try checkStatus(mln_render_session_query_rendered_features(
          session.raw,
          geometry,
          options,
          outHandle
        ))
      }
  }

  static func sourceFeatures(
    session: NativeRenderSessionHandle,
    sourceId: mln_string_view,
    options: UnsafePointer<mln_source_feature_query_options>?
  ) throws -> NativeFeatureQueryResultHandle {
    try NativeHandleFactory
      .create(nullDiagnostic: "source feature query returned a null result") { outHandle in
        try checkStatus(mln_render_session_query_source_features(
          session.raw,
          sourceId,
          options,
          outHandle
        ))
      }
  }

  static func featureQueryResultCount(
    _ result: NativeFeatureQueryResultHandle
  ) throws
    -> Int
  {
    let output = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_feature_query_result_count(result.raw, count))
    }
    return output.value
  }

  static func featureQueryResultGet(_ result: NativeFeatureQueryResultHandle,
                                    index: Int) throws -> NativeQueriedFeature
  {
    var feature = mln_queried_feature()
    feature.size = UInt32(MemoryLayout<mln_queried_feature>.size)
    try checkStatus(mln_feature_query_result_get(result.raw, index, &feature))
    return try NativeQueriedFeature(copying: feature)
  }

  static func featureExtensions(
    session: NativeRenderSessionHandle,
    sourceId: mln_string_view,
    feature: UnsafePointer<mln_feature>,
    extensionName: mln_string_view,
    extensionField: mln_string_view,
    arguments: UnsafePointer<mln_json_value>?
  ) throws -> NativeFeatureExtensionResultHandle {
    try NativeHandleFactory
      .create(nullDiagnostic: "feature extension query returned a null result") { outHandle in
        try checkStatus(mln_render_session_query_feature_extensions(
          session.raw,
          sourceId,
          feature,
          extensionName,
          extensionField,
          arguments,
          outHandle
        ))
      }
  }

  static func featureExtensionResultCopy(
    _ result: NativeFeatureExtensionResultHandle
  ) throws
    -> NativeFeatureExtensionResult
  {
    var info = mln_feature_extension_result_info()
    info.size = UInt32(MemoryLayout<mln_feature_extension_result_info>.size)
    try checkStatus(mln_feature_extension_result_get(result.raw, &info))
    return try NativeFeatureExtensionResult(copying: info)
  }

  static func featureState(
    _ session: NativeRenderSessionHandle,
    selector: UnsafePointer<mln_feature_state_selector>
  ) throws -> NativeJSONValue? {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_render_session_get_feature_state(
          session.raw,
          selector,
          outHandle
        ))
      }.value
    let snapshot = NativeJSONSnapshotHandle(raw: snapshotValue)
    guard !snapshot.isNull else { return nil }
    defer { mln_json_snapshot_destroy(snapshot.raw) }
    return try NativeJSONSnapshot.copyValue(snapshot)
  }
}
