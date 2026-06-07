internal import CMaplibreNativeC

enum NativeQuery {
  static func renderedFeatures(
    session: OpaquePointer,
    geometry: UnsafePointer<mln_rendered_query_geometry>,
    options: UnsafePointer<mln_rendered_feature_query_options>?
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "rendered feature query returned a null result") { result in
      try checkStatus(mln_render_session_query_rendered_features(session, geometry, options, result))
    }
  }

  static func sourceFeatures(
    session: OpaquePointer,
    sourceId: mln_string_view,
    options: UnsafePointer<mln_source_feature_query_options>?
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "source feature query returned a null result") { result in
      try checkStatus(mln_render_session_query_source_features(session, sourceId, options, result))
    }
  }

  static func featureQueryResultCount(_ result: OpaquePointer) throws -> Int {
    let output = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_feature_query_result_count(result, count))
    }
    return output.value
  }

  static func featureQueryResultGet(_ result: OpaquePointer, index: Int) throws -> NativeQueriedFeature {
    var feature = mln_queried_feature()
    feature.size = UInt32(MemoryLayout<mln_queried_feature>.size)
    try checkStatus(mln_feature_query_result_get(result, index, &feature))
    return try NativeQueriedFeature(copying: feature)
  }

  static func featureExtensions(
    session: OpaquePointer,
    sourceId: mln_string_view,
    feature: UnsafePointer<mln_feature>,
    extensionName: mln_string_view,
    extensionField: mln_string_view,
    arguments: UnsafePointer<mln_json_value>?
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "feature extension query returned a null result") { result in
      try checkStatus(mln_render_session_query_feature_extensions(session, sourceId, feature, extensionName, extensionField, arguments, result))
    }
  }

  static func featureExtensionResultCopy(_ result: OpaquePointer) throws -> NativeFeatureExtensionResult {
    var info = mln_feature_extension_result_info()
    info.size = UInt32(MemoryLayout<mln_feature_extension_result_info>.size)
    try checkStatus(mln_feature_extension_result_get(result, &info))
    return try NativeFeatureExtensionResult(copying: info)
  }

  static func featureState(_ session: OpaquePointer, selector: UnsafePointer<mln_feature_state_selector>) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_render_session_get_feature_state(session, selector, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try NativeJSONSnapshot.copyValue(snapshot)
  }
}
