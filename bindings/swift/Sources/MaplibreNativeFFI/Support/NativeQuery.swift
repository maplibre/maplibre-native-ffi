internal import CMaplibreNativeC
import Foundation

// Render-session queries use the common operation and notification adapters
// directly from Query.swift so their typed result transfer remains visible.
// This file copies native queried-feature lists into Swift values.

enum NativeQuery {
  static func copyQueriedFeatureList(
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
