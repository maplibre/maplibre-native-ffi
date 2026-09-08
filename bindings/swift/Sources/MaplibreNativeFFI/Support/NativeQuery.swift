internal import CMaplibreNativeC
import Foundation

enum NativeQuery {
  static func copyQueriedFeatures(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> [QueriedFeature] {
    try NativeCompletion.values(result, as: mln_queried_feature.self).map {
      try copyQueriedFeature($0)
    }
  }

  private static func copyQueriedFeature(
    _ hit: mln_queried_feature
  ) throws -> QueriedFeature {
    let sourceId = hasField(hit, MLN_QUERIED_FEATURE_SOURCE_ID.rawValue)
      ? try NativeString.copyUTF8(
        data: hit.source_id.data,
        size: hit.source_id.size
      ) : nil
    let sourceLayerId = hasField(
      hit,
      MLN_QUERIED_FEATURE_SOURCE_LAYER_ID.rawValue
    ) ? try NativeString.copyUTF8(
      data: hit.source_layer_id.data,
      size: hit.source_layer_id.size
    ) : nil
    let state = hasField(hit, MLN_QUERIED_FEATURE_STATE.rawValue)
      ? try NativeCompletion.dataView(hit.state) : nil
    return try QueriedFeature(
      feature: NativeCompletion.dataView(hit.feature),
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
}
