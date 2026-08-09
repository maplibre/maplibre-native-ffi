internal import CMaplibreNativeC
import Foundation

enum NativeQuery {
  static func renderedFeatures(
    session: NativeRenderSessionHandle,
    geometry: UnsafePointer<mln_rendered_query_geometry>,
    options: UnsafePointer<mln_rendered_feature_query_options>?
  ) throws -> Data {
    try copyResult { output in
      try checkStatus(mln_render_session_query_rendered_features(
        session.raw, geometry, options, output
      ))
    }
  }

  static func sourceFeatures(
    session: NativeRenderSessionHandle,
    sourceId: mln_buffer_view,
    options: UnsafePointer<mln_source_feature_query_options>?
  ) throws -> Data {
    try copyResult { output in
      try checkStatus(mln_render_session_query_source_features(
        session.raw, sourceId, options, output
      ))
    }
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
}
